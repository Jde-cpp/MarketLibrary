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

	using boost::container::flat_multimap;
	using boost::container::flat_map;

	struct ΓM Awaitable final : CancelAwaitable<Task2>, CombinedParams
	{
		typedef CancelAwaitable<Task2> base;
		typedef Task2::promise_type PromiseType;
		Awaitable( const CombinedParams& params, Handle& h )noexcept;
		~Awaitable()=default;
		α await_ready()noexcept->bool{ return OrderParams::OrderFields==MyOrder::Fields::None && StatusParams::StatusFields==OrderStatus::Fields::None && StateParams::StateFields==OrderState::Fields::None; }
		α await_suspend( coroutine_handle<Task2::promise_type> h )noexcept->void;
		Task2::TResult await_resume()noexcept{ DBG("({})OrderManager::Awaitable::await_resume"sv, std::this_thread::get_id()); return _pPromise ? _pPromise->get_return_object().GetResult() : Task2::TResult{}; }
	private:
		PromiseType* _pPromise{nullptr};
		α End( Handle h, const Cache* pCache )noexcept->void; 	std::once_flag _singleEnd;
	};

	ΓM α Cancel( Handle h )noexcept->void;
	inline auto Subscribe( const CombinedParams& params, Handle& h )noexcept{ return Awaitable{params, h}; }
	ΓM optional<Cache> GetLatest( ::OrderId orderId )noexcept;
	α Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept->void;
	ΓM α Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept->void;
	α Push( const ::Order& order, const ::Contract& contract )noexcept->void;


	struct OrderWorker final: TCoWorker<OrderWorker,Awaitable>
	{
		typedef TCoWorker<OrderWorker,Awaitable> base;
		struct SubscriptionInfo : base::Handles<>{ CombinedParams Params; };
		OrderWorker():base{"OrderWorker"}{};
	private:
		static sp<OrderWorker> Instance()noexcept;
		α Process()noexcept->void override;
		optional<Cache> Latest( ::OrderId orderId )noexcept;
		α Cancel( Handle h )noexcept->void;
		α Subscribe( const SubscriptionInfo& params )noexcept->void;
		α Push( sp<const OrderStatus> status )noexcept->void;
		α Push( sp<const MyOrder> order, const ::Contract& contract, sp<const OrderState> state={} )noexcept->void;

		static sp<OrderWorker> _pInstance;
		static sp<TwsClientSync> _pTws;
		flat_map<::OrderId,Cache> _incoming; mutex _incomingMutex;
		flat_map<::OrderId,Cache> _cache; shared_mutex _cacheMutex;
		flat_multimap<::OrderId,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;
		friend Awaitable;
		friend ΓM α Cancel( Handle h )noexcept->void;
		friend ΓM optional<Cache> GetLatest( ::OrderId orderId )noexcept;
		friend α Push( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept->void;
		friend ΓM α Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept->void;
		friend α Push( const ::Order& order, const ::Contract& contract )noexcept->void;
	};
}