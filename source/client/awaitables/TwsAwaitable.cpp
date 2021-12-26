#include "TwsAwaitable.h"
#include "../../../../Framework/source/Cache.h"
#include "../TwsClientCo.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Proto::Results;
	ITwsAwait::ITwsAwait()noexcept:_pTws{ Tws::InstancePtr() }{}
	α ITwsAwait::WrapperPtr()noexcept->sp<WrapperCo>{ return _pTws->WrapperPtr(); }

	α SecDefOptParamAwait::await_ready()noexcept->bool
	{
		auto cacheId = format( "OptParams.{}", _underlyingConId );
		_dataPtr = Cache::Get<OptionExchanges>( CacheId() );
		return _dataPtr || base::await_ready();
	}
	α SecDefOptParamAwait::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_secDefOptParamHandles.MoveIn( id, move(h) );
		try
		{
			auto pContract = SFuture<::ContractDetails>( Tws::ContractDetail(_underlyingConId) ).get();
			_pTws->reqSecDefOptParams( id, _underlyingConId, pContract->contract.localSymbol );
		}
		catch( IException& e )
		{
			_pPromise->get_return_object().SetResult( e.Clone() );
			h.resume();
		}
	}
	α SecDefOptParamAwait::await_resume()noexcept->AwaitResult
	{
		base::AwaitResume();
		if( !_dataPtr )
			_dataPtr = Cache::Set<OptionExchanges>( CacheId(), _pPromise->get_return_object().Result().SP<OptionExchanges>() );

		sp<void> p = _smart
			? _dataPtr->exchanges().size() ? make_shared<ExchangeContracts>( _dataPtr->exchanges()[0] ) : sp<void>{}
			: _dataPtr;
		return AwaitResult{ p };
	}

	vector<HCoroutine> AllOpenOrdersAwait::_handles;
	std::mutex AllOpenOrdersAwait::_mutex;
	sp<Proto::Results::Orders> AllOpenOrdersAwait::_pData;

	α AllOpenOrdersAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitUnique::await_suspend( h );
		lock_guard l{ _mutex };
		_handles.push_back( h );
		if( _handles.size()==1 )
		{
			_pData = make_shared<Proto::Results::Orders>();
			_pTws->reqAllOpenOrders();
		}
	}
	α AllOpenOrdersAwait::Push( up<OpenOrder> p )noexcept->void
	{
		lock_guard l{ _mutex };
		if( _pData )
			*_pData->add_orders() = move(*p.release());
	}
	α AllOpenOrdersAwait::Push( up<OrderStatus> p )noexcept->void
	{
		lock_guard l{ _mutex };
		if( _pData )
			*_pData->add_statuses() = move(*p.release());
	}

	α AllOpenOrdersAwait::Finish()noexcept->void
	{
		lock_guard l{ _mutex };
		for( auto&& h : _handles )
		{
			h.promise().get_return_object().SetResult( _pData );
			CoroutinePool::Resume( move(h) );
		}
		_pData = nullptr;
		_handles.clear();
	}
	α AllOpenOrdersAwait::await_resume()noexcept->AwaitResult
	{
		base::AwaitResume();
		return AwaitResult{ _pPromise->get_return_object().Result() };
	}
}