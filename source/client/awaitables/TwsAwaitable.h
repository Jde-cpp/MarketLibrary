#pragma once
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
	struct ΓM ITwsAwait : IAwait
	{
		ITwsAwait()noexcept;
		using base=IAwait;
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
	};

	struct ΓM SecDefOptParamAwait : ITwsAwaitShared
	{
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

	struct ΓM AllOpenOrdersAwait : ITwsAwaitUnique
	{
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
		Ω Finish()noexcept->void;
		Ω Push( up<Proto::Results::OpenOrder> order )noexcept->void;
		Ω Push( up<Proto::Results::OrderStatus> status )noexcept->void;
	private:
		HCoroutine _h;
		static vector<HCoroutine> _handles; static std::mutex _mutex;
		static sp<Proto::Results::Orders> _pData;
	};
}