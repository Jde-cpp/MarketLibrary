#include "TwsAwaitable.h"
#include <jde/markets/types/MyOrder.h>
#include <jde/blockly/BlocklyLibrary.h>
#include <jde/blockly/IBlockly.h>

#include "../../../../Framework/source/Cache.h"
#include "../TwsClientCo.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Proto::Results;
	ITwsAwait::ITwsAwait( SL sl )noexcept:IAwait{sl}, _pTws{ Tws::InstancePtr() }{}
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
			? _dataPtr->exchanges().size() ? ms<ExchangeContracts>( _dataPtr->exchanges()[0] ) : sp<void>{}
			: _dataPtr;
		return AwaitResult{ p };
	}

	vector<HCoroutine> AllOpenOrdersAwait::_handles;
	std::mutex AllOpenOrdersAwait::_mutex;
	up<Proto::Results::Orders> AllOpenOrdersAwait::_pData;

	α AllOpenOrdersAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitUnique::await_suspend( h );
		lock_guard l{ _mutex };
		_handles.push_back( h );
		if( _handles.size()==1 )
		{
			_pData = mu<Proto::Results::Orders>();
			_pTws->reqAllOpenOrders();
		}
	}
	α AllOpenOrdersAwait::Push( OpenOrder& o )noexcept->void
	{
		lock_guard l{ _mutex };
		if( _pData )
			*_pData->add_orders() = o;
	}
	α AllOpenOrdersAwait::Push( up<Proto::Results::OrderStatus> p )noexcept->void
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
			h.promise().get_return_object().SetResult( move(_pData) );
			CoroutinePool::Resume( move(h) );
		}
		_pData = nullptr;
		_handles.clear();
	}
	α AllOpenOrdersAwait::await_resume()noexcept->AwaitResult
	{
		base::AwaitResume();
		return move( _pPromise->get_return_object().Result() );
	}

	α PlaceOrderAwait::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		if( !_order.orderId )
			_order.orderId = _pTws->RequestId();
		bool place = true;
		if( _blockId.size() )
		{
			try
			{
				auto pp = up<sp<Markets::MBlockly::IBlockly>>( Blockly::CreateAllocatedExecutor(_blockId, _order.orderId, _pContract->conId) );
				auto p = *pp;
				p->Run();
			}
			catch( IException& e )
			{
				place = false;
				_pPromise->get_return_object().SetResult( e.Move() );
			}
		}
		if( place )
		{
			var parentId = !_order.whatIf && _stop>0 ? _order.orderId : 0;//whatif=don't actually execute.
			if( !parentId )
				WrapperPtr()->_orderHandles.MoveIn( _order.orderId, move(h) );
			_pTws->placeOrder( *_pContract, _order, _sl );//can't have h destruct this when _blockId.size() is using it.
			if( parentId )
			{
				var childId = _pTws->RequestId();
				WrapperPtr()->_orderHandles.MoveIn( childId, move(h) );
				MyOrder parent{ _order };
				parent.orderId = childId;
				parent.IsBuy( _order.action!="BUY" );
				parent.OrderType( Proto::EOrderType::StopLimit );
				ASSERT( parent.totalQuantity==_order.totalQuantity );  //parent.totalQuantity = _order.totalQuantity;//is this needed?
				parent.auxPrice = _stop;
				parent.lmtPrice = _stopLimit;
				parent.parentId = parentId;
				_pTws->placeOrder( *_pContract, parent, _sl );
			}
		}
	}
	α PlaceOrderAwait::await_resume()noexcept->AwaitResult
	{
		base::AwaitResume();
		return move( _pPromise->get_return_object().Result() );
	}
}