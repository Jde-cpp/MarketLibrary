#include "OrderManager.h"
#include "types/OrderEnums.h"
#include "client/TwsClientSync.h"

#define OMInstancePtr if( sp<OrderWorker> p=OrderWorker::Instance(); p ) p
#define var const auto
namespace Jde::Markets
{
	void OrderManager::Cancel( Coroutine::Handle h )noexcept
	{
		OMInstancePtr->Cancel( h );
	}
	optional<OrderManager::Cache> OrderManager::GetLatest( ::OrderId orderId )noexcept
	{
		var p = OrderWorker::Instance();
		return p ? p->Latest( orderId ) : optional<OrderManager::Cache>{};
	}
	void OrderManager::Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int /*clientId*/, const std::string& whyHeld, double mktCapPrice )noexcept
	{
		var eOrderStatus = ToOrderStatus( status );
		OrderStatus x{orderId, eOrderStatus, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, whyHeld, mktCapPrice};
		OMInstancePtr->Push( make_shared<const OrderStatus>(x) );
	}
	void OrderManager::Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept
	{
		OMInstancePtr->Push( make_shared<const MyOrder>(order), contract, make_shared<const OrderState>(orderState) );
	}

	void OrderManager::Push( const ::Order& order, const ::Contract& contract )noexcept
	{
		OMInstancePtr->Push( make_shared<const MyOrder>(order), contract );
	}


namespace OrderManager
{
	Awaitable::Awaitable( const CombinedParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		CombinedParams{ params }
	{}

	void Awaitable::await_suspend( coroutine_handle<Task2::promise_type> h )noexcept
	{
		_pPromise = &h.promise();
		OMInstancePtr->Subscribe( OrderWorker::SubscriptionInfo{{h, _hClient}, *this} );
	}

	void OrderWorker::Cancel( Coroutine::Handle h )noexcept
	{
		unique_lock l{ _subscriptionMutex };
		if( auto p = std::find_if(_subscriptions.begin(), _subscriptions.end(), [h](var x){ return x.second.HClient==h;}); p!=_subscriptions.end() )
		{
			DBG( "({})OrderWorker::Cancel({})"sv, p->first, h );

			p->second.HCoroutine.destroy();
			_subscriptions.erase( p );
		}
		else
			DBG( "OrderWorker - Could not find handle {}."sv, h );
	}

	void OrderWorker::Subscribe( const SubscriptionInfo& params )noexcept
	{
		ASSERT( params.Params.OrderPtr );
		unique_lock l{ _subscriptionMutex };
		//--------------------------------------------------
		DBG( "({})OrderManager add subscription."sv, params.Params.OrderPtr->orderId );
		_subscriptions.emplace( params.Params.OrderPtr->orderId, params );
	}
	void OrderWorker::Push( sp<const OrderStatus> pStatus )noexcept
	{
		unique_lock l{_incomingMutex};
		if( auto p = _incoming.find(pStatus->Id); p!=_incoming.end() )
			p->second.StatusPtr = pStatus;
		else
			_incoming.try_emplace( pStatus->Id, Cache{{}, {}, pStatus, {}} );
		std::unique_lock<std::mutex> lk( _mtx );
		l.unlock();
		_cv.notify_one();
	}
	void OrderWorker::Push( sp<const MyOrder> pOrder, const ::Contract& contract, sp<const OrderState> pState )noexcept
	{
		ASSERT( pOrder );
		unique_lock l{_incomingMutex};
		sp<const Contract> pContract;
		if( auto p = _incoming.find(pOrder->orderId); p!=_incoming.end() )
		{
			auto& cache = p->second;
			cache.OrderPtr = pOrder;
			if( pState )
				cache.StatePtr = pState;
			if( !cache.ContractPtr )
				cache.ContractPtr = make_shared<const Markets::Contract>( contract );
			pContract = cache.ContractPtr;
		}
		else
			_incoming.try_emplace( pOrder->orderId, Cache{pOrder, pContract = make_shared<Contract>(contract), {}, pState} );
		DBG( "({})added order -{} limit={}"sv, pOrder->orderId, pContract->Display(), pOrder->lmtPrice );
		std::unique_lock<std::mutex> lk( _mtx );
		l.unlock();
		_cv.notify_one();
	}

	void OrderWorker::Process()noexcept
	{
		::OrderId id;
		optional<Cache> pUpdate;
		{
			unique_lock l{_incomingMutex};
			if( auto p=_incoming.begin(); p!=_incoming.end() )
			{
				id = p->first;
				pUpdate = p->second;
				_incoming.erase( p );
			}
			else
			{
				std::unique_lock<std::mutex> lk( _mtx );
				l.unlock();
				_cv.wait_for( lk, WakeDuration );
			}
		}
		if( !pUpdate )
			return;
		Cache latest;
		{
			var& update = *pUpdate;
			unique_lock l{ _cacheMutex };
			if( auto p=_cache.find(id); p!=_cache.end() )
			{
				string log = format( "({})OrderManager update."sv, id );
				auto& v = p->second;
				if( update.OrderPtr )
				{
					log.append( format(" limit={}", update.OrderPtr->lmtPrice) );
					var lastUpdate = v.OrderPtr->LastUpdate;
					v.OrderPtr = update.OrderPtr;
					v.OrderPtr->LastUpdate = lastUpdate;
				}
				if( update.StatusPtr )
					v.StatusPtr = update.StatusPtr;
				if( update.StatePtr )
					v.StatePtr = update.StatePtr;
				latest = v;
				LOGS( ELogLevel::Debug, move(log) );
			}
			else
			{
				DBG( "OrderManager add {} to cache."sv, id );
				_cache.emplace( id, update );
				latest = update;
			}
		}
		{
			unique_lock l222{ _subscriptionMutex };
			auto range = _subscriptions.equal_range( id );
			for( auto p = range.first; p!=_subscriptions.end() && p->first==id; )
			{
				DBG( "OrderManager have subscription."sv );
				auto& original = p->second.Params;
				var orderChange = (!original.OrderPtr && latest.OrderPtr) || (original.OrderPtr && latest.OrderPtr && latest.OrderPtr->Changes( *original.OrderPtr, original.OrderFields)!=MyOrder::Fields::None );
				var statusChange = orderChange || (!original.StatusPtr && latest.StatusPtr) || (original.StatusPtr && latest.StatusPtr && latest.StatusPtr->Changes( *original.StatusPtr, original.StatusFields)!=OrderStatus::Fields::None );
				var stateChange = statusChange || (!original.StatePtr && latest.StatePtr) || (original.StatePtr && latest.StatePtr && latest.StatePtr->Changes( *original.StatePtr, original.StateFields)!=OrderState::Fields::None );
				if( stateChange )
				{
					DBG( "OrderManager changes. {} {}"sv, _subscriptions.size(), Threading::GetThreadId() );
					auto& h = p->second.HCoroutine;
					h.promise().get_return_object().SetResult( make_shared<Cache>(latest) );
					Coroutine::CoroutinePool::Resume( move(h) );
					p = _subscriptions.erase( p );
				}
				else
				{
					DBG( "OrderManager no changes."sv );
					++p;
				}
			}
		}
	}
	optional<Cache> OrderWorker::Latest( ::OrderId orderId )noexcept
	{
		shared_lock l{_cacheMutex};
		auto p = _cache.find( orderId );
		return p==_cache.end() ? optional<Cache>{} : p->second;
	}
	sp<OrderWorker> OrderWorker::_pInstance;
	sp<TwsClientSync> OrderWorker::_pTws;
	std::once_flag _singleThread;
	sp<OrderWorker> OrderWorker::Instance()noexcept
	{
		std::call_once( _singleThread, []()
		{
			DBG( "Creating OrderWroker"sv );
			OrderWorker::_pInstance = make_shared<OrderWorker>();
			OrderWorker::_pInstance->Start();
			_pTws = dynamic_pointer_cast<TwsClientSync>( TwsClient::InstancePtr() );
			_pTws->reqAllOpenOrders();
		});
		return _pInstance;
	}
}}