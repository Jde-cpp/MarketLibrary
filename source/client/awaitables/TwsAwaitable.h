#pragma once
#include <jde/coroutine/Task.h>
#include <jde/markets/TypeDefs.h>
#include <jde/markets/types/proto/results.pb.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"

namespace Jde::Markets
{
	using namespace Jde::Coroutine;
	struct WrapperCo; struct TwsClientCo;

	using namespace Jde::Coroutine;
	struct JDE_MARKETS_EXPORT ITwsAwaitable
	{
		ITwsAwaitable()noexcept;
	protected:
		sp<WrapperCo> WrapperPtr()noexcept;
		sp<TwsClientCo> _pTws;
	};
	struct ITwsAwaitableImpl : ITwsAwaitable, IAwaitable
	{
		using base=IAwaitable;
		α await_ready()noexcept->bool override{ return !_pTws; }
		α await_suspend( typename base::THandle h )noexcept->void override{ base::await_suspend( h ); _pPromise = &h.promise(); };
		α await_resume()noexcept->TaskResult override{ base::AwaitResume(); return move(_pPromise->get_return_object().GetResult()); }
	};

	struct SecDefOptParamAwaitable :ITwsAwaitableImpl
	{
		SecDefOptParamAwaitable( ContractPK conId, bool smart )noexcept:_underlyingConId{conId}, _smart{smart}{};
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->TaskResult override;
	private:
		α CacheId()const noexcept{ return format( "OptParams.{}", _underlyingConId ); }
		const ContractPK _underlyingConId;
		const bool _smart;
		sp<Proto::Results::OptionExchanges> _dataPtr;
	};
}