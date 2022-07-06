#pragma once
#include <Order.h>
#include <jde/coroutine/Task.h>
#include <jde/markets/Exports.h>
#include <jde/markets/TypeDefs.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::Markets
{
	namespace Proto{ class OrderStatus; }
	namespace Proto::Results{ class OptionExchanges; class OpenOrder; class Orders; }
	using namespace Jde::Coroutine;
	struct WrapperCo; struct Tws;

	using namespace Jde::Coroutine;
	class ΓM ITwsAwait : public IAwait
	{
		using base=IAwait;
	public:
		ITwsAwait( SRCE )ι;
		
		α await_ready()ι->bool override{ return !_pTws; }
		α await_suspend( HCoroutine h )ι->void override{ base::await_suspend( h ); };
		α await_resume()ι->AwaitResult override{ base::AwaitResume(); return move(_pPromise->get_return_object().Result()); }
	protected:
		sp<WrapperCo> WrapperPtr()ι;
		sp<Tws> _pTws;
	};
	
	struct ITwsAwaitUnique : ITwsAwait
	{
		ITwsAwaitUnique( SL sl ):ITwsAwait{ sl }{}
	};

	struct ITwsAwaitShared : ITwsAwait
	{
		ITwsAwaitShared( SRCE ):ITwsAwait{sl}{}
	};
#define Base ITwsAwaitShared
	struct ΓM SecDefOptParamAwait final : Base
	{
		using base=Base;
		SecDefOptParamAwait( ContractPK conId, bool smart )ι:_underlyingConId{conId}, _smart{smart}{};
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		α CacheId()Ι{ return format( "OptParams.{}", _underlyingConId ); }
		const ContractPK _underlyingConId;
		const bool _smart;
		sp<Proto::Results::OptionExchanges> _dataPtr;
	};
#undef Base
#define Base ITwsAwaitUnique
	struct ΓM AllOpenOrdersAwait final : Base
	{
		using base=Base;
		AllOpenOrdersAwait( SRCE )ι:base{sl}{}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
		Ω Finish()ι->void;
		Ω Push( Proto::Results::OpenOrder& order )ι->void;
		Ω Push( up<Proto::OrderStatus> status )ι->void;
	private:
		HCoroutine _h;
		static vector<HCoroutine> _handles; static std::mutex _mutex;
		static up<Proto::Results::Orders> _pData;
	};

	struct ΓM PlaceOrderAwait final : Base
	{
		using base=Base;
		PlaceOrderAwait( sp<::Contract> c, ::Order o, string blockId, double stop, double stopLimit, SRCE ):base{sl},_pContract{c}, _order{move(o)}, _blockId{move(blockId)}, _stop{stop}, _stopLimit{stopLimit}
		{}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		sp<::Contract> _pContract;
		::Order _order;
		string _blockId;
		double _stop;
		double _stopLimit;
	};

	struct ΓM HeadTimestampAwait final : Base
	{
		using base=Base;
		HeadTimestampAwait( sp<::Contract> c, sv whatToShow, SRCE )ι:base{sl},_pContract{c}, _whatToShow{whatToShow}
		{}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		sp<::Contract> _pContract;
		string _whatToShow;
	};
}
#undef Base