#include "OrderManager.h"
#include "types/OrderEnums.h"

#define InstancePtr if( sp<OrderWorker> p=OrderWorker::Instance(); p ) p
#define var const auto
namespace Jde::Markets
{
	inline void OrderManager::Cancel( Coroutine::Handle h )noexcept
	{
		//InstancePtr->Cancel( h );
	}
	void OrderManager::Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int /*clientId*/, const std::string& whyHeld, double mktCapPrice )noexcept
	{
		var eOrderStatus = ToOrderStatus( status );
		OrderStatus x{orderId, eOrderStatus, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, whyHeld, mktCapPrice};
		InstancePtr->Push( make_shared<const OrderStatus>(x) );
	}
	void OrderManager::Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept
	{
		InstancePtr->Push( make_shared<const MyOrder>(order), contract, make_shared<const OrderState>(orderState) );
	}

	void OrderManager::Push( const ::Order& order, const ::Contract& contract )noexcept
	{
		InstancePtr->Push( make_shared<const MyOrder>(order), contract );
	}


namespace OrderManager
{
	Awaitable::Awaitable( const CombinedParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		CombinedParams{ params }
	{}

	// Task<OrderParams> Awaitable::SubscribeOrders( Awaitable::Handle h )noexcept
	// {
	// 	auto t = co_await OrderManager::Subscribe( (OrderParams&)*this, _orderHandle );
	// 	_orderHandle = 0;
	// 	if( _statusHandle )
	// 		OrderManager::CancelStatus( _statusHandle );
	// 	CombinedParams x{ move(t), move((StatusParams)*this) };
	// 	End( h, &x );
	// }
	// Task<StatusParams> Awaitable::SubscribeStatus( Awaitable::Handle h )noexcept
	// {
	// 	auto t = co_await OrderManager::Subscribe( (StatusParams&)*this, _statusHandle );
	// 	_statusHandle = 0;
	// 	if( _orderHandle )
	// 		OrderManager::CancelOrder( _orderHandle );
	// 	CombinedParams x{ move((OrderParams)*this), move(t) };
	// 	End( h, &x );
	// }

/*	void Awaitable::End( Awaitable::Handle h, const Cache* pCache )noexcept
	{
		std::call_once( _singleEnd, [h, pCache]()mutable
		{
			if( pCache )
			{
				h.promise().get_return_object().result = *pCache;
				h.resume();
			}
			else
				h.destroy();
		});
	}
*/
	void Awaitable::await_suspend( Awaitable::Handle h )noexcept
	{
		InstancePtr->Subscribe( OrderWorker::SubscriptionInfo{ {h, _hClient}, *this} );
	}

/*	void OrderManager::Add( const MyOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields, Handles handles )noexcept
	{
		if( orderFields!=MyOrder::Fields::None )
		{
			unique_lock l{ _orderSubscriptionMutex };
			_subscriptions.emplace( order.orderId, {handles,order,orderFields} );
		}
		if( statusFields!=OrderStatus::Fields::None )
		{
			unique_lock l{ _statusSubscriptionMutex };
			_statusSubscriptions.try_emplace( order.orderId, {handles,status,statusFields} );
		}
	}
*/
	void OrderWorker::Cancel( Coroutine::Handle h )noexcept
	{
		unique_lock l{ _subscriptionMutex };
		if( auto p = std::find_if(_subscriptions.begin(), _subscriptions.end(), [h](var x){ return x.second.HClient==h;}); p!=_subscriptions.end() )
		{
			DBG( "Cancel({})"sv, h );

			p->second.HCoroutine.destroy();
			_subscriptions.erase( p );
		}
		else
			DBG( "Could not find handle {}."sv, h );
	}

	void OrderWorker::Subscribe( const SubscriptionInfo& params )noexcept
	{
		ASSERT( params.Params.OrderPtr );
		unique_lock l{ _subscriptionMutex };
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
		if( auto p = _incoming.find(pOrder->orderId); p!=_incoming.end() )
		{
			auto& cache = p->second;
			cache.OrderPtr = pOrder;
			if( pState )
				cache.StatePtr = pState;
			if( !cache.ContractPtr )
				cache.ContractPtr = make_shared<Contract>( contract );
		}
		else
			_incoming.try_emplace( pOrder->orderId, Cache{pOrder, make_shared<Contract>(contract), {}, pState} );

		std::unique_lock<std::mutex> lk( _mtx );
		l.unlock();
		_cv.notify_one();
	}

	void OrderWorker::Process()noexcept
	{
		OrderId id;
		optional<Cache> newCache;
		{
			unique_lock l{_incomingMutex};
			if( auto p=_incoming.begin(); p!=_incoming.end() )
			{
				id = p->first;
				newCache = p->second;
				_incoming.erase( p );
			}
			else
			{
				std::unique_lock<std::mutex> lk( _mtx );
				l.unlock();
				_cv.wait_for( lk, WakeDuration );
			}
		}
		if( newCache )
		{
			{
				unique_lock l{ _cacheMutex };
				if( auto p=_cache.find(id); p!=_cache.end() )
				{
					auto& new_ = newCache.value();
					auto& v = p->second;
					if( new_.OrderPtr )
						v.OrderPtr = new_.OrderPtr;
					if( new_.StatusPtr )
						v.StatusPtr = new_.StatusPtr;
					if( new_.StatePtr )
						v.StatePtr = new_.StatePtr;
				}
				else
					_cache.emplace( id, newCache.value() );
			}
			{
				unique_lock l{ _subscriptionMutex };
				auto range = _subscriptions.equal_range( id );
				for( auto p = range.first; p!=range.second; ++p )
				{
					var& new_ = newCache.value();
					auto& original = p->second.Params;
					var orderChange = (!original.OrderPtr && new_.OrderPtr) || (original.OrderPtr && new_.OrderPtr && new_.OrderPtr->Changes( *original.OrderPtr, original.OrderFields)!=MyOrder::Fields::None );
					var statusChange = orderChange || (!original.StatusPtr && new_.StatusPtr) || (original.StatusPtr && new_.StatusPtr && new_.StatusPtr->Changes( *original.StatusPtr, original.StatusFields)!=OrderStatus::Fields::None );
					var stateChange = statusChange || (!original.StatePtr && new_.StatePtr) || (original.StatePtr && new_.StatePtr && new_.StatePtr->Changes( *original.StatePtr, original.StateFields)!=OrderState::Fields::None );
					if( stateChange )
					{
						/*original.OrderPtr = new_.OrderPtr;
						original.StatusPtr = new_.StatusPtr;
						original.StatePtr = new_.StatePtr;*/
						auto& h = p->second.HCoroutine;
						h.promise().get_return_object().Result = new_;
						h.resume();
						_subscriptions.erase( p );
					}
				}
			}
		}
	}

	sp<OrderWorker> OrderWorker::_pInstance{nullptr};
	std::once_flag _singleThread;
	sp<OrderWorker> OrderWorker::Instance()noexcept
	{
		auto pInstance = _pInstance;
		if( !pInstance && !IApplication::ShuttingDown() )
		{
			std::call_once( _singleThread, [&pInstance]()mutable
			{
				pInstance = make_shared<OrderWorker>();
				pInstance->Start();
			});
		}
		return pInstance;
	}
/*	void StatusWorker::Process()noexcept
	{
		while( !Threading::GetThreadInterruptFlag().IsSet() || !_queue.Empty() )
		{
			auto pStatus = _queue.WaitAndPop( WakeDuration );
			unique_lock l{ _subscriptionMutex };
			auto range = _subscriptions.equal_range( pStatus->Id );
			for( auto p = range.first; p!=range.second; ++p )
			{
				auto& value = p->second;
				auto changes = pStatus->Changes( value.Status, value.Fields );
				if( changes!=MyOrder::Fields::None )
				{
					auto& h = value.HCoroutine;
					h.promise().get_return_object().result = move( *pStatus );
					h.resume();
					_subscriptions.erase( p );
				}
			}
		}
	}*/
}}