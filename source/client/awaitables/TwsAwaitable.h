#pragma once
#include <Order.h>
#include <jde/coroutine/Task.h>
#include <jde/markets/Exports.h>
#include <jde/markets/TypeDefs.h>
//#include <jde/markets/types/proto/results.pb.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::Markets
{
	namespace Proto::Results{ class OptionExchanges; class OpenOrder; class OrderStatus; class Orders; }
	using namespace Jde::Coroutine;
	struct WrapperCo; struct Tws;

	using namespace Jde::Coroutine;
	class ΓM ITwsAwait : public IAwait
	{
		using base=IAwait;
	public:
		ITwsAwait( SRCE )noexcept;
		
		α await_ready()noexcept->bool override{ return !_pTws; }
		α await_suspend( HCoroutine h )noexcept->void override{ base::await_suspend( h ); };
		α await_resume()noexcept->AwaitResult override{ base::AwaitResume(); return move(_pPromise->get_return_object().Result()); }
	protected:
		sp<WrapperCo> WrapperPtr()noexcept;
		sp<Tws> _pTws;
	};
	
	struct ITwsAwaitUnique : ITwsAwait
	{
	};

	struct ITwsAwaitShared : ITwsAwait
	{
		ITwsAwaitShared( SRCE ):ITwsAwait{sl}{}
	};
#define Base ITwsAwaitShared
	struct ΓM SecDefOptParamAwait final : Base
	{
		using base=Base;
		SecDefOptParamAwait( ContractPK conId, bool smart )noexcept:_underlyingConId{conId}, _smart{smart}{};
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
	private:
		α CacheId()const noexcept{ return format( "OptParams.{}", _underlyingConId ); }
		const ContractPK _underlyingConId;
		const bool _smart;
		sp<Proto::Results::OptionExchanges> _dataPtr;
	};
#undef Base
#define Base ITwsAwaitUnique
	struct ΓM AllOpenOrdersAwait final : Base
	{
		using base=Base;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
		Ω Finish()noexcept->void;
		Ω Push( Proto::Results::OpenOrder& order )noexcept->void;
		Ω Push( up<Proto::Results::OrderStatus> status )noexcept->void;
	private:
		HCoroutine _h;
		static vector<HCoroutine> _handles; static std::mutex _mutex;
		static up<Proto::Results::Orders> _pData;
	};

	struct ΓM PlaceOrderAwait final : Base
	{
		using base=Base;
		PlaceOrderAwait( sp<::Contract> c, ::Order o, string blockId ):_pContract{c}, _order{move(o)}, _blockId{move(blockId)}{}
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
	private:
		sp<::Contract> _pContract;
		::Order _order;
		string _blockId;
	};
}
#undef Base