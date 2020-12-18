#pragma once
#include <boost/container/flat_map.hpp>
#include "../../Framework/source/collections/Queue.h"
#include "../../Framework/source/threading/Worker.h"
#include "../../Framework/source/coroutine/CoWorker.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/coroutine/Task.h"
#include "types/MyOrder.h"
#include "types/Contract.h"


namespace Jde::Markets{ struct TwsClientSync; }

namespace Jde::Markets::OrderManager
{
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;
	using boost::container::flat_multimap;
	using boost::container::flat_map;

	struct OrderParams /*~final*/
	{
		//OrderParams( OrderParams&& x )noexcept{}//TODO
		sp<const MyOrder> OrderPtr;
		MyOrder::Fields OrderFields{MyOrder::Fields::None};
	};
	struct StatusParams /*~final*/
	{
		sp<const OrderStatus> StatusPtr;
		OrderStatus::Fields StatusFields{OrderStatus::Fields::None};
	};
	struct StateParams /*~final*/
	{
		sp<const OrderState> StatePtr;
		OrderState::Fields StateFields{OrderState::Fields::None};
	};
	struct CombinedParams /*~final*/ : OrderParams, StatusParams, StateParams
	{
		CombinedParams( const OrderParams& o, const StatusParams& s, const StateParams& s2 )noexcept:OrderParams{o}, StatusParams{s}, StateParams{s2}{}
	};
	struct Cache /*~final*/
	{
		sp<const MyOrder> OrderPtr;
		sp<const Markets::Contract> ContractPtr;
		sp<const OrderStatus> StatusPtr;
		sp<const OrderState> StatePtr;
	};
/*
	template<typename T>
	struct Task final
	{
		struct promise_type
		{
			Task<T>& get_return_object()noexcept{ return _returnObject; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
			Task<T> _returnObject;
		};
		T result;
	};
*/
	struct JDE_MARKETS_EXPORT Awaitable final : Coroutine::CancelAwaitable<Coroutine::Task<Cache>>, CombinedParams
	{
		typedef Coroutine::CancelAwaitable<Coroutine::Task<Cache>> base;
		typedef Coroutine::Task<Cache>::promise_type PromiseType;
		typedef coroutine_handle<PromiseType> Handle;
		Awaitable( const CombinedParams& params, Coroutine::Handle& h )noexcept;
		~Awaitable()=default;
		bool await_ready()noexcept{ return OrderParams::OrderFields==MyOrder::Fields::None && StatusParams::StatusFields==OrderStatus::Fields::None && StateParams::StateFields==OrderState::Fields::None; }
		void await_suspend( Awaitable::Handle h )noexcept;
		Cache await_resume()noexcept{ DBG("({})OrderManager::Awaitable::await_resume"sv, std::this_thread::get_id()); return _pPromise->get_return_object().Result; }
	private:
		PromiseType* _pPromise;
		void End( Awaitable::Handle h, const Cache* pCache )noexcept; 	std::once_flag _singleEnd;
	};

	// struct JDE_MARKETS_EXPORT OrderAwaitable final : Coroutine::CancelAwaitable, OrderParams
	// {
	// 	typedef Task<OrderParams>::promise_type PromiseType;
	// 	typedef coroutine_handle<PromiseType> Handle;
	// 	OrderAwaitable( const OrderParams& orderParams, Coroutine::Handle& handle )noexcept;
	// 	~OrderAwaitable()=default;
	// 	bool await_ready()noexcept{ return OrderParams::Fields==MyOrder::Fields::None; }
	// 	void await_suspend( Handle h )noexcept;
	// 	OrderParams await_resume()noexcept{ DBG("({})OrderAwaitable::await_resume"sv, std::this_thread::get_id()); return std::move(_pPromise->get_return_object().result.value()); }
	// private:
	// 	PromiseType* _pPromise;
	// };

	// struct JDE_MARKETS_EXPORT StatusAwaitable final : Coroutine::CancelAwaitable, StatusParams
	// {
	// 	typedef Task<StatusParams>::promise_type PromiseType;
	// 	typedef coroutine_handle<PromiseType> Handle;
	// 	StatusAwaitable( const StatusParams& params, Coroutine::Handle& handle )noexcept;
	// 	~StatusAwaitable()=default;
	// 	bool await_ready()noexcept{ return StatusParams::Fields==OrderStatus::Fields::None; }
	// 	void await_suspend( Handle h )noexcept;
	// 	StatusParams await_resume()noexcept{ DBG("({})StatusAwaitable::await_resume"sv, std::this_thread::get_id()); return std::move(_pPromise->get_return_object().result.value()); }
	// private:
	// 	PromiseType* _pPromise;
	// };

	void Cancel( Coroutine::Handle h )noexcept;
	inline auto Subscribe( const CombinedParams& params, Coroutine::Handle& h )noexcept{ return Awaitable{params, h}; }
	optional<Cache> GetLatest( ::OrderId orderId )noexcept;
	void Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept;
	void Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept;
	void Push( const ::Order& order, const ::Contract& contract )noexcept;

	struct OrderWorker final: Coroutine::TCoWorker<OrderWorker,Awaitable>
	{
		typedef Coroutine::TCoWorker<OrderWorker,Awaitable> base;
		struct SubscriptionInfo : base::Handles<>{ CombinedParams Params; };
		OrderWorker():base{"OrderWorker"}{};
	private:
		static sp<OrderWorker> Instance()noexcept;
		//void Shutdown()noexcept override;
		void Process()noexcept override;
		optional<Cache> Latest( ::OrderId orderId )noexcept;
		void Cancel( Coroutine::Handle h )noexcept;
		void Subscribe( const SubscriptionInfo& params )noexcept;
		void Push( sp<const OrderStatus> status )noexcept;
		void Push( sp<const MyOrder> order, const ::Contract& contract, sp<const OrderState> state={} )noexcept;

		static sp<OrderWorker> _pInstance;
		static sp<TwsClientSync> _pTws;
		flat_map<::OrderId,Cache> _incoming; mutex _incomingMutex;
		flat_map<::OrderId,Cache> _cache; shared_mutex _cacheMutex;
		flat_multimap<::OrderId,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;
		friend Awaitable;
		friend void Cancel( Coroutine::Handle h )noexcept;
		friend optional<Cache> GetLatest( ::OrderId orderId )noexcept;
		friend void Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept;
		friend void Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept;
		friend void Push( const ::Order& order, const ::Contract& contract )noexcept;
	};
/*	struct StatusWorker final: Threading::CoWorker<StatusWorker,StatusAwaitable>
	{
		typedef Threading::CoWorker<StatusWorker,StatusAwaitable> base;
		struct StatusHandles : base::Handles<>{ OrderStatus Status; OrderStatus::Fields Fields; };
		StatusWorker():base{"StatusWorker"}{};
		void Shutdown()noexcept override;
		void Process()noexcept override;
	private:
		Queue<OrderStatus> _queue;
		flat_multimap<::OrderId,OrderHandles> _subscriptions; mutex _subscriptionMutex;
	};*/


// 	struct JDE_MARKETS_EXPORT OrderManager final: Threading::Worker
// 	{
// /*		struct Handles{ Awaitable::Handle HCoroutine; Coroutine::Handle HClient; };
// 		struct OrderHandles : Handles{ MyOrder Order; MyOrder::Fields Fields;};
// 		struct StatusHandles : Handles{ OrderStatus::Fields Fields;};
// 		OrderManager():Threading::Worker{"OrderManager"}{};
// 		~OrderManager(){ DBG0("OrderManager::~OrderManager"sv); }
// */
// 		static auto Subscribe( const CombinedParams& params, Coroutine::Handle& h )noexcept{ return Awaitable{params, h}; }
// //		static auto Subscribe( const OrderParams& orderParams, Coroutine::Handle& h )noexcept{ return OrderAwaitable{orderParams, h}; }
// //		static auto Subscribe( const StatusParams& statusParams, Coroutine::Handle& h )noexcept{ return StatusAwaitable{statusParams, h}; }
// 		static void Cancel( Coroutine::Handle handle )noexcept;
// 		static void CancelOrder( Coroutine::Handle handle )noexcept;
// 		static void CancelStatus( Coroutine::Handle handle )noexcept;
// 	private:
// 		void Process()noexcept override;
// 		static sp<OrderManager> Instance()noexcept;
// 		//static void Add( const MyOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields, Awaitable::Handle h, Coroutine::Handle myHandle )noexcept;
// 		void Add( const MyOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields, Handles handles )noexcept;
// 		void Cancel2( Coroutine::Handle handle )noexcept;

// 		std::condition_variable _cv; mutable std::mutex _mtx;

// 		flat_multimap<::OrderId,StatusHandles> _statusSubscriptions; mutex _statusSubscriptionMutex;
// 		static constexpr Duration WakeDuration{5s};
// 		friend Awaitable;
// 	};
}