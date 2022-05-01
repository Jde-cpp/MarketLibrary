#pragma once
#pragma warning( disable : 4244 )
#include <boost/container/flat_map.hpp>
#pragma warning( default : 4244 )

#include "../../Framework/source/collections/Queue.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/coroutine/CoWorker.h"
#include "../../Framework/source/coroutine/Lock.h"
#include "../../Framework/source/threading/Worker.h"
#include <jde/coroutine/Task.h>
#include <jde/markets/types/MyOrder.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/Exports.h>
#include "types/IBException.h"


namespace Jde::Markets{ struct TwsClientSync; }

namespace Jde::Markets::OrderManager
{
	struct IListener
	{
		β OnOrderChange( sp<const MyOrder> OrderPtr, sp<const Markets::Contract> ContractPtr, sp<const OrderStatus> StatusPtr, sp<const OrderState> StatePtr )noexcept->Task=0;
		β OnOrderException( string account, sp<const IBException> e )->Task=0;
	};
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
		sp<const ::OrderState> StatePtr;
		OrderStateFields StateFields{OrderStateFields::None};
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
		sp<const ::OrderState> StatePtr;
		sp<const IBException> ExceptionPtr;
		α ToString()Ι->string{ return OrderPtr ? format("({}){} - {}", OrderPtr->orderId, ContractPtr ? ContractPtr->Display() : "NOCONTRACT", OrderPtr->lmtPrice) : "NOORDER";  }
	};

	using boost::container::flat_multimap;
	using boost::container::flat_map;

	struct ΓM Awaitable final : CancelAwait, CombinedParams
	{
		typedef CancelAwait base;
		Awaitable( const CombinedParams& params, Handle& h )ι;
		~Awaitable()=default;
		α await_ready()ι->bool{ return OrderParams::OrderFields==MyOrder::Fields::None && StatusParams::StatusFields==OrderStatus::Fields::None && StateParams::StateFields==OrderStateFields::None; }
		α await_suspend( HCoroutine h )ι->void;
		α await_resume()ι->AwaitResult{ /*DBG("({})OrderManager::Awaitable::await_resume"sv, std::this_thread::get_id());*/ return _pPromise ? move(_pPromise->get_return_object().Result()) : Task::TResult{}; }
	private:
		Task::promise_type* _pPromise{nullptr};
		α End( Handle h, const Cache* pCache )noexcept->void; 	std::once_flag _singleEnd;
	};
#define Φ ΓM α
	Φ Cancel( Handle h )noexcept->AsyncReadyAwait;
	Ξ Subscribe( const CombinedParams& params, Handle& h )ι{ return Awaitable{params, h}; }
	Φ Listen( sp<IListener> p )ι->Task;
	Φ Latest( ::OrderId orderId )ι->LockWrapperAwait;
	α Push( ::OrderId orderId, str status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, str whyHeld, double mktCapPrice )ι->void;
	Φ Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )ι->void;
	α Push( const ::Order& order, const ::Contract& contract )ι->void;
	α Push( ::OrderId id, int errorCode, string errorMsg )ι->Task;

	struct OrderWorker final: TCoWorker<OrderWorker,Awaitable>
	{
		typedef TCoWorker<OrderWorker,Awaitable> base;
		struct SubscriptionInfo : base::Handles<>{ CombinedParams Params; };
		OrderWorker():base{"OrderWorker"}{};
	private:
		static sp<OrderWorker> Instance()ι;
		Φ Process()noexcept->void override;
		optional<Cache> Latest( ::OrderId orderId )noexcept;
		α Cancel( Handle h )noexcept->AsyncReadyAwait;
		α Subscribe( const SubscriptionInfo& params )noexcept->void;
		α Push( sp<const OrderStatus> status )noexcept->void;
		α Push( sp<const MyOrder> order, const ::Contract& contract, sp<const OrderState> state={} )noexcept->void;

		static sp<OrderWorker> _pInstance;
		static sp<TwsClientSync> _pTws;
		flat_map<::OrderId,Cache> _incoming; mutex _incomingMutex;
		friend Awaitable;
		friend Φ Cancel( Handle h )noexcept->AsyncReadyAwait;
		friend ΓM optional<Cache> GetLatest( ::OrderId orderId )noexcept;
		friend α Push( ::OrderId orderId, str status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, str whyHeld, double mktCapPrice )noexcept->void;
		friend Φ Push( const ::Order& order, const ::Contract& contract, const ::OrderState& orderState )noexcept->void;
		friend α Push( const ::Order& order, const ::Contract& contract )noexcept->void;
	};
}
#undef Φ