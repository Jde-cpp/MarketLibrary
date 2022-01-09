#include "OrderManager.h"
#include "types/OrderEnums.h"
#include "client/TwsClientSync.h"

#define OMInstancePtr if( sp<OrderWorker> p=OrderWorker::Instance(); p ) p
#define var const auto
namespace Jde::Markets
{
	static const LogTag& _logLevel = Logging::TagLevel( "tws-orders" );
	flat_map<::OrderId,OrderManager::Cache> _cache;

	α OrderManager::Cancel( Coroutine::Handle h )noexcept->void
	{
		OMInstancePtr->Cancel( h );
	}
	α OrderManager::Latest( ::OrderId orderId )noexcept->LockWrapperAwait
	{
		return LockWrapperAwait{ "OrderManager._cache", [orderId]( Coroutine::AwaitResult& y )
		{
			auto p = _cache.find( orderId );
			y.Set( p==_cache.end() ? nullptr : new OrderManager::Cache(p->second) );
		} };
	}

	α OrderManager::Push( ::OrderId orderId, str status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int /*clientId*/, str whyHeld, double mktCapPrice )noexcept->void
	{
		var eOrderStatus = ToOrderStatus( status );
		OrderStatus x{ orderId, eOrderStatus, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, whyHeld, mktCapPrice };
		OMInstancePtr->Push( ms<const OrderStatus>(x) );
	}
	α OrderManager::Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept->void
	{
		OMInstancePtr->Push( ms<const MyOrder>(order), contract, ms<const OrderState>(orderState) );
	}

	α OrderManager::Push( const ::Order& order, const ::Contract& contract )noexcept->void
	{
		OMInstancePtr->Push( ms<const MyOrder>(order), contract );
	}

	vector<sp<OrderManager::IListener>> _listeners; //shared_mutex _listenerMutex;
	α OrderManager::Listen( sp<IListener> p )noexcept->Task
	{
		DBG("OrderManager::Listen( sp<IListener> p )noexcept->Task");
		var _ = ( co_await CoLockKey( "OrderManager._listeners", false) ).UP<CoLockGuard>();
		_listeners.emplace_back( p );
	}

	α OrderManager::Push( ::OrderId id, int errorCode, string errorMsg )noexcept->Task
	{
		sp<const IBException> pException{ new IBException{move(errorMsg), errorCode, id} };//make_shared doesn't work.
		string account;
		{
			var _ = ( co_await CoLockKey( "OrderManager._cache", true) ).UP<CoLockGuard>();
			if( auto p=_cache.find(id); p!=_cache.end() )
			{
				account = p->second.OrderPtr->account;
				p->second.ExceptionPtr = pException;
			}
		}
		if( account.size() )
		{
			var _ = ( co_await CoLockKey( "OrderManager._listeners", true) ).UP<CoLockGuard>();//TODO use a mutex class vs string
			for( var l : _listeners )
				l->OnOrderException( account, pException );
		}
		else
			LOG( "({}) - could not find order", id );
	}

namespace OrderManager
{
	flat_multimap<::OrderId,OrderWorker::SubscriptionInfo> _orderSubscriptions; mutex _orderSubscriptionMutex;

	Awaitable::Awaitable( const CombinedParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		CombinedParams{ params }
	{}

	α Awaitable::await_suspend( coroutine_handle<Task::promise_type> h )noexcept->void
	{
		_pPromise = &h.promise();
		OMInstancePtr->Subscribe( OrderWorker::SubscriptionInfo{{h, _hClient}, *this} );
	}

	α OrderWorker::Cancel( Coroutine::Handle h )noexcept->void
	{
		unique_lock l{ _orderSubscriptionMutex };
		if( auto p = std::find_if(_orderSubscriptions.begin(), _orderSubscriptions.end(), [h](var x){ return x.second.HClient==h;}); p!=_orderSubscriptions.end() )
		{
			LOG( "({})OrderWorker::Cancel({})"sv, p->first, h );

			p->second.HCo.destroy();
			_orderSubscriptions.erase( p );
		}
		else
			LOG( "OrderWorker - Could not find handle {}."sv, h );
	}

	α OrderWorker::Subscribe( const SubscriptionInfo& params )noexcept->void
	{
		ASSERT( params.Params.OrderPtr );
		unique_lock l{ _orderSubscriptionMutex };
		//--------------------------------------------------
		LOG( "({})OrderManager add subscription."sv, params.Params.OrderPtr->orderId );
		_orderSubscriptions.emplace( params.Params.OrderPtr->orderId, params );
	}
	α OrderWorker::Push( sp<const OrderStatus> pStatus )noexcept->void
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
	α OrderWorker::Push( sp<const MyOrder> pOrder, const ::Contract& contract, sp<const OrderState> pState )noexcept->void
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
				cache.ContractPtr = ms<const Markets::Contract>( contract );
			pContract = cache.ContractPtr;
		}
		else
			_incoming.try_emplace( pOrder->orderId, Cache{pOrder, pContract = ms<Contract>(contract), {}, pState} );
		LOG( "({})added order -{} limit={}"sv, pOrder->orderId, pContract->Display(), pOrder->lmtPrice );
		std::unique_lock<std::mutex> lk( _mtx );
		l.unlock();
		_cv.notify_one();
	}

	α Update( ::OrderId id, Cache&& update )->Task
	{
		Cache latest;
		{
			var _ = ( co_await CoLockKey( "OrderManager._cache", true) ).UP<CoLockGuard>();//TODO make unique.
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
				LOGS( move(log) );
			}
			else
			{
				LOG( "OrderManager add {} to cache."sv, id );
				_cache.emplace( id, update );
				latest = update;
			}
		}
		{
			unique_lock l2{ _orderSubscriptionMutex };
			auto range = _orderSubscriptions.equal_range( id );
			for( auto p = range.first; p!=_orderSubscriptions.end() && p->first==id; )
			{
				LOG( "OrderManager have subscription."sv );
				auto& original = p->second.Params;
				var orderChange = (!original.OrderPtr && latest.OrderPtr) || (original.OrderPtr && latest.OrderPtr && latest.OrderPtr->Changes( *original.OrderPtr, original.OrderFields)!=MyOrder::Fields::None );
				var statusChange = orderChange || (!original.StatusPtr && latest.StatusPtr) || (original.StatusPtr && latest.StatusPtr && latest.StatusPtr->Changes( *original.StatusPtr /*, original.StatusFields*/)!=OrderStatus::Fields::None );
				var stateChange = statusChange || (!original.StatePtr && latest.StatePtr) || (original.StatePtr && latest.StatePtr && OrderStateChanges( *latest.StatePtr, *original.StatePtr/*, original.StateFields*/)!=OrderStateFields::None );
				if( stateChange )
				{
					LOG( "OrderManager changes. {} {}"sv, _orderSubscriptions.size(), Threading::GetThreadId() );
					auto& h = p->second.HCo;
					h.promise().get_return_object().SetResult( ms<Cache>(latest) );
					Coroutine::CoroutinePool::Resume( move(h) );
					p = _orderSubscriptions.erase( p );
				}
				else
				{
					LOG( "OrderManager no changes."sv );
					++p;
				}
			}
		}
		if( latest.StatusPtr || latest.StatePtr )
		{
			var _ = ( co_await CoLockKey( "OrderManager._listeners", true) ).UP<CoLockGuard>();//TODO make shared, use a mutex class vs string
			for( var l : _listeners )
				l->OnOrderChange( latest.OrderPtr, latest.ContractPtr, latest.StatusPtr, latest.StatePtr );
		}
	}
	α OrderWorker::Process()noexcept->void//TODO take out thread, just use coroutines
	{
		unique_lock l{_incomingMutex};
		if( auto p=_incoming.begin(); p!=_incoming.end() )
		{
			Update( p->first, move(p->second) );
			_incoming.erase( p );
		}
		else
		{
			std::unique_lock<std::mutex> lk( _mtx );
			l.unlock();
			_cv.wait_for( lk, WakeDuration );
		}
	}

	sp<OrderWorker> OrderWorker::_pInstance;
	sp<TwsClientSync> OrderWorker::_pTws;
	std::once_flag _singleThread;
	α OrderWorker::Instance()noexcept->sp<OrderWorker>
	{
		std::call_once( _singleThread, []()
		{
			LOG( "Creating OrderWroker"sv );
			OrderWorker::_pInstance = ms<OrderWorker>();
			OrderWorker::_pInstance->Start();
			_pTws = dynamic_pointer_cast<TwsClientSync>( TwsClient::InstancePtr() );
			_pTws->reqAllOpenOrders();
		});
		return _pInstance;
	}
}}