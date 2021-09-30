#include "TwsAwaitable.h"
#include "../../../../Framework/source/Cache.h"
#include "../TwsClientCo.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Proto::Results;
	ITwsAwaitable::ITwsAwaitable()noexcept:_pTws{ TwsClientCo::InstancePtr() }{}
	sp<WrapperCo> ITwsAwaitable::WrapperPtr()noexcept{ return _pTws->WrapperPtr(); }


	α SecDefOptParamAwaitable::await_ready()noexcept->bool
	{
		auto cacheId = format( "OptParams.{}", _underlyingConId );
		_dataPtr = Cache::Get<OptionExchanges>( CacheId() );
		return _dataPtr || base::await_ready();
	}
	α SecDefOptParamAwaitable::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_secDefOptParamHandles.MoveIn( id, move(h) );
		try
		{
			auto pContract = Future<Contract>( TwsClientCo::ContractDetails(_underlyingConId) ).get();
			_pTws->reqSecDefOptParams( id, _underlyingConId, pContract->LocalSymbol );
		}
		catch( Exception& e )
		{
			_pPromise->get_return_object().SetResult( move(e) );
			h.resume();
		}
	}
	α SecDefOptParamAwaitable::await_resume()noexcept->TaskResult
	{
		base::AwaitResume();
		if( !_dataPtr )
			_dataPtr = Cache::Set<OptionExchanges>( CacheId(), _pPromise->get_return_object().GetResult().Get<OptionExchanges>() );

		sp<void> p = _smart
			? _dataPtr->exchanges().size() ? make_shared<ExchangeContracts>( _dataPtr->exchanges()[0] ) : sp<void>{}
			: _dataPtr;
		return TaskResult{ p };
	}
}