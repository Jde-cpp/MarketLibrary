#pragma once
#pragma warning( disable : 4244 )
#include <boost/container/flat_map.hpp>
#pragma warning( default : 4244 )

#include "../../Framework/source/collections/Queue.h"
#include "../../Framework/source/threading/Worker.h"
#include "../../Framework/source/coroutine/CoWorker.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/coroutine/Task.h>
#include <jde/markets/types/MyOrder.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/Exports.h>


namespace Jde::Markets{ struct TwsClientSync; }

namespace Jde::Markets::OrderManager
{
	using namespace Coroutine;
	struct OrderParams /*~final*/
	{
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

	//using std::experimental::coroutine_handle;
	//using std::experimental::suspend_never;
	using boost::container::flat_multimap;
	using boost::container::flat_map;

	struct JDE_MARKETS_EXPORT Awaitable final : CancelAwaitable<Task2>, CombinedParams
	{
		typedef CancelAwaitable<Task2> base;
		typedef Task2::promise_type PromiseType;
		//typedef coroutine_handle<PromiseType> Handle;
		Awaitable( const CombinedParams& params, Handle& h )noexcept;
		~Awaitable()=default;
		bool await_ready()noexcept{ return OrderParams::OrderFields==MyOrder::Fields::None && StatusParams::StatusFields==OrderStatus::Fields::None && StateParams::StateFields==OrderState::Fields::None; }
		void await_suspend( coroutine_handle<Task2::promise_type> h )noexcept;
		Task2::TResult await_resume()noexcept{ DBG("({})OrderManager::Awaitable::await_resume"sv, std::this_thread::get_id()); return _pPromise ? _pPromise->get_return_object().GetResult() : Task2::TResult{}; }
	private:
		PromiseType* _pPromise{nullptr};
		void End( Awaitable::Handle h, const Cache* pCache )noexcept; 	std::once_flag _singleEnd;
	};

	JDE_MARKETS_EXPORT void Cancel( Handle h )noexcept;
	inline auto Subscribe( const CombinedParams& params, Handle& h )noexcept{ return Awaitable{params, h}; }
	JDE_MARKETS_EXPORT optional<Cache> GetLatest( ::OrderId orderId )noexcept;
	void Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept;
	JDE_MARKETS_EXPORT void Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept;
	void Push( const ::Order& order, const ::Contract& contract )noexcept;


	struct OrderWorker final: TCoWorker<OrderWorker,Awaitable>
	{
		typedef TCoWorker<OrderWorker,Awaitable> base;
		struct SubscriptionInfo : base::Handles<>{ CombinedParams Params; };
		OrderWorker():base{"OrderWorker"}{};
	private:
		static sp<OrderWorker> Instance()noexcept;
		void Process()noexcept override;
		optional<Cache> Latest( ::OrderId orderId )noexcept;
		void Cancel( Handle h )noexcept;
		void Subscribe( const SubscriptionInfo& params )noexcept;
		void Push( sp<const OrderStatus> status )noexcept;
		void Push( sp<const MyOrder> order, const ::Contract& contract, sp<const OrderState> state={} )noexcept;

		static sp<OrderWorker> _pInstance;
		static sp<TwsClientSync> _pTws;
		flat_map<::OrderId,Cache> _incoming; mutex _incomingMutex;
		flat_map<::OrderId,Cache> _cache; shared_mutex _cacheMutex;
		flat_multimap<::OrderId,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;
		friend Awaitable;
		friend JDE_MARKETS_EXPORT void Cancel( Handle h )noexcept;
		friend JDE_MARKETS_EXPORT optional<Cache> GetLatest( ::OrderId orderId )noexcept;
		friend void Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept;
		friend JDE_MARKETS_EXPORT void Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept;
		friend void Push( const ::Order& order, const ::Contract& contract )noexcept;
	};
}