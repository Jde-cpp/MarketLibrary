﻿#include "TwsAwaitable.h"
#include "../../../../Framework/source/Cache.h"
#include "../TwsClientCo.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Proto::Results;
	ITwsAwaitable::ITwsAwaitable()noexcept:_pTws{ Tws::InstancePtr() }{}
	α ITwsAwaitable::WrapperPtr()noexcept->sp<WrapperCo>{ return _pTws->WrapperPtr(); }

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
			auto pContract = Future<Contract>( Tws::ContractDetails(_underlyingConId) ).get();
			_pTws->reqSecDefOptParams( id, _underlyingConId, pContract->LocalSymbol );
		}
		catch( IException& e )
		{
			_pPromise->get_return_object().SetResult( e.Clone() );
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

	vector<HCoroutine> AllOpenOrdersAwait::_handles;
	std::mutex AllOpenOrdersAwait::_mutex;
	sp<Proto::Results::Orders> AllOpenOrdersAwait::_pData;

	α AllOpenOrdersAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitableImpl::await_suspend( h );
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
	α AllOpenOrdersAwait::await_resume()noexcept->TaskResult
	{
		base::AwaitResume();
		return TaskResult{ _pPromise->get_return_object().GetResult() };
	}
}