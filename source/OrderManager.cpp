#include "OrderManager.h"
#include "types/OrderEnums.h"
#include "client/TwsClientSync.h"

#define OMInstancePtr if( sp<OrderWorker> p=OrderWorker::Instance(); p ) p
#define var const auto
namespace Jde::Markets
{
	static const LogTag& _logLevel = Logging::TagLevel( "tws-orders" );
	flat_map<::OrderId,OrderManager::Cache> _cache;
	using namespace Coroutine;

	α OrderManager::Cancel( Coroutine::Handle h )noexcept->AsyncReadyAwait
	{
		sp<OrderWorker> p=OrderWorker::Instance();
		return p ? p->Cancel( h ) : AsyncReadyAwait{};
	}
	namespace Internal
	{
		α Latest( ::OrderId orderId, up<CoLockGuard> _ )noexcept->up<OrderManager::Cache>
		{
			auto p = _cache.find( orderId );
			auto y = p==_cache.end() ? nullptr : mu<OrderManager::Cache>(p->second);
			return y;
		}
	}
	α OrderManager::Latest( ::OrderId orderId )noexcept->LockWrapperAwait
	{
		return LockWrapperAwait{ "OrderManager._cache", [orderId]( AwaitResult& y, up<CoLockGuard> l )
		{
			up<OrderManager::Cache> p = Internal::Latest( orderId, move(l) );
			y.Set( p.release() );
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
		string account;
		auto pException{ ms<IBException>(IBException{errorMsg, errorCode, id, ELogLevel::None}) };
		{
			var _ = ( co_await CoLockKey( "OrderManager._cache", true) ).UP<CoLockGuard>();
			if( auto p=_cache.find(id); p!=_cache.end() )
			{
				account = p->second.OrderPtr->account;
				p->second.ExceptionPtr = pException;//make_shared doesn't work.
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

	α CombinedParams::Changes( const Cache& x )Ι->bool
	{
		var orderChange = (!OrderPtr && x.OrderPtr) || (OrderPtr && x.OrderPtr && x.OrderPtr->Changes( *OrderPtr, OrderFields)!=MyOrder::Fields::None );
		var statusChange = orderChange || (!StatusPtr && x.StatusPtr) || (StatusPtr && x.StatusPtr && x.StatusPtr->Changes( *StatusPtr )!=OrderStatus::Fields::None );
		var stateChange = statusChange 
			|| (!StatePtr && x.StatePtr && x.StatusPtr && x.StatePtr->status!=ToString(x.StatusPtr->Status) ) 
			|| (StatePtr && x.StatePtr && OrderStateChanges( *x.StatePtr, *StatePtr)!=OrderStateFields::None );
		ASSERT( x.StatePtr  ||  (!x.StatePtr && !StatePtr) );
		return stateChange;
	}

	Awaitable::Awaitable( const CombinedParams& params, Handle& h )noexcept:
		base{ h },
		CombinedParams{ params }
	{}

	α Awaitable::await_ready()ι->bool
	{ 
		bool ready = OrderParams::OrderFields==MyOrder::Fields::None && StatusParams::StatusFields==OrderStatus::Fields::None && StateParams::StateFields==OrderStateFields::None; 
		if( auto pLock = ready ? nullptr : LockWrapperAwait::TryLock("OrderManager._cache", true); pLock )
		{
			if( auto p = _cache.find(OrderPtr->orderId); p!=_cache.end() )
			{
				ready = Changes( p->second );
				if( ready )
					_pReady = mu<Cache>( p->second );
			}
		}
		return ready;
	}
	α Awaitable::await_suspend( coroutine_handle<Task::promise_type> h )noexcept->void
	{
		_pPromise = &h.promise();
		OMInstancePtr->Subscribe( OrderWorker::SubscriptionInfo{{h, _hClient}, *this} );
	}
	α Awaitable::await_resume()ι->AwaitResult
	{ 
		/*DBG("({})OrderManager::Awaitable::await_resume"sv, std::this_thread::get_id());*/ 
		return _pPromise ? move(_pPromise->get_return_object().Result()) : AwaitResult{ move(_pReady) }; 
	}
	α OrderWorker::Cancel( Coroutine::Handle h )noexcept->AsyncReadyAwait
	{
		::OrderId orderId;
		auto  ready = [h, &orderId]()
		{
			optional<AwaitResult> y;
			unique_lock _{ _orderSubscriptionMutex };
			if( auto p = std::find_if(_orderSubscriptions.begin(), _orderSubscriptions.end(), [h](var x)
				{ 
					return x.second.HClient==h; 
				}); p!=_orderSubscriptions.end() )
			{
				var orderId = p->first;
				LOG( "({})OrderWorker::Cancel({})"sv, orderId, h );
				p->second.HCo.destroy();
				_orderSubscriptions.erase( p );
				if( auto pLock = ForceSuspend() ? nullptr : LockWrapperAwait::TryLock("OrderManager._cache", true); pLock )
					y = AwaitResult{ Internal::Latest(orderId, move(pLock)) };
			}
			else
			{
				LOG( "OrderWorker::Cancel({}) Could not find handle."sv, h );
				y = AwaitResult{ nullptr };
			}
			return y;
		};
		auto suspend = [orderId]( HCoroutine h )->Task
		{
			auto pCache = ( co_await OrderManager::Latest(orderId) ).UP<Cache>();
			h.promise().get_return_object().SetResult( move(pCache) );
			h.resume();
		};
		return AsyncReadyAwait{ ready, suspend };
	}

	α OrderWorker::Subscribe( const SubscriptionInfo& params )noexcept->void
	{
		ASSERT( params.Params.OrderPtr );
		TRACE( "({})OrderManager add subscription.", params.Params.OrderPtr->orderId );
		unique_lock _{ _orderSubscriptionMutex };
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
	α OrderWorker::Push( sp<const MyOrder> pOrder, const ::Contract& contract, sp<const OrderState> pState )ι->void
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
			LOG( "({})OrderWorker::Push update {} limit={}", pOrder->orderId, pContract->Display(), pOrder->lmtPrice );
		}
		else
		{
			_incoming.emplace( pOrder->orderId, Cache{pOrder, pContract = ms<Contract>(contract), {}, pState} );
			//LOG( "({})OrderWorker::Push {} limit={}", pOrder->orderId, pContract->Display(), pOrder->lmtPrice );
		}
		std::unique_lock<std::mutex> lk( _mtx );
		l.unlock();
		_cv.notify_one();
	}

	α Update( ::OrderId id, Cache&& update_ )ι->Task
	{
		Cache latest;
		{
			Cache update{ move(update_) };//need local
			var _ = ( co_await CoLockKey("OrderManager._cache", true) ).UP<CoLockGuard>();
			if( auto p=_cache.find(id); p!=_cache.end() )
			{
				string log = format( "({})OrderManager update - ", id ); var initialSize = log.size();
				auto& v = p->second;
				if( update.OrderPtr )
				{
					if( update.OrderPtr->lmtPrice!=v.OrderPtr->lmtPrice )
						log.append( format("limit:{} vs {}", update.OrderPtr->lmtPrice, v.OrderPtr->lmtPrice) );
					ASSERT( v.OrderPtr->LastUpdate!=TP{} );
					var lastUpdate = v.OrderPtr->LastUpdate;
					v.OrderPtr = update.OrderPtr;
					v.OrderPtr->LastUpdate = lastUpdate;
				}
				if( OrderStatus::Fields fields{ update.StatusPtr ? v.StatusPtr ? v.StatusPtr->Changes(*update.StatusPtr) : OrderStatus::Fields::All : OrderStatus::Fields::None }; fields!=OrderStatus::Fields::None )
				{
					v.StatusPtr = update.StatusPtr;
					vector<string> statusLog;
					if( fields && OrderStatus::Fields::Status )
						statusLog.push_back( format("Value:{}", ToString(update.StatusPtr->Status)) );
					if( fields && OrderStatus::Fields::Filled )
						statusLog.push_back( format("Filled:{}", update.StatusPtr->Filled) );
					if( fields && OrderStatus::Fields::Remaining )
						statusLog.push_back( format("Remaining:{}", update.StatusPtr->Remaining) );
					log.append( format("Status{{ {} }}", Str::AddCommas(statusLog)) );
				}
				if( update.StatePtr )
				{
					if( !v.StatePtr || v.StatePtr->status!=update.StatePtr->status )
						log.append( format(" State:{} ", update.StatePtr->status) );
					v.StatePtr = update.StatePtr;
				}
				latest = v;
				if( log.size()>initialSize )
					LOGS( move(log) );
			}
			else
			{
				LOG( "{} - OrderManager add to cache."sv, update.ToString() );
				if( update.OrderPtr->LastUpdate==TP{} )
					update.OrderPtr->LastUpdate = Clock::now();

				_cache.emplace( id, update );
				latest = update;
			}
		}
		{
			unique_lock l2{ _orderSubscriptionMutex };
			for( auto p = _orderSubscriptions.find(id); p!=_orderSubscriptions.end() && p->first==id; )
			{
				TRACE( "({})OrderManager have subscription.", id );
				auto& original = p->second.Params;
				var orderChange = (!original.OrderPtr && latest.OrderPtr) || (original.OrderPtr && latest.OrderPtr && latest.OrderPtr->Changes( *original.OrderPtr, original.OrderFields)!=MyOrder::Fields::None );
				var statusChange = orderChange || (!original.StatusPtr && latest.StatusPtr) || (original.StatusPtr && latest.StatusPtr && latest.StatusPtr->Changes( *original.StatusPtr /*, original.StatusFields*/)!=OrderStatus::Fields::None );
				var stateChange = statusChange || (!original.StatePtr && latest.StatePtr) || (original.StatePtr && latest.StatePtr && OrderStateChanges( *latest.StatePtr, *original.StatePtr/*, original.StateFields*/)!=OrderStateFields::None );
				if( stateChange )
				{
					TRACE( "OrderManager trigger subscription. total subscriptions size={}", _orderSubscriptions.size() );
					p->second.HCo.promise().get_return_object().SetResult( mu<Cache>(latest) );
					CoroutinePool::Resume( move(p->second.HCo) );
				}
				else
					LOG( "OrderManager no changes."sv );
				p = stateChange ? _orderSubscriptions.erase( p ) : std::next( p );
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
			LOG( "Creating OrderWorker" );
			OrderWorker::_pInstance = ms<OrderWorker>();
			OrderWorker::_pInstance->Start();
			_pTws = dynamic_pointer_cast<TwsClientSync>( TwsClient::InstancePtr() );
			_pTws->reqAllOpenOrders();
		});
		return _pInstance;
	}
}}