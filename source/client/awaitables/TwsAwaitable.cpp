#include "TwsAwaitable.h"
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
		WrapperPtr()->_orderHandles.MoveIn( _order.orderId, move(h) );
//		LOG( "({})receiveOrder( contract='{}' {}x{} )"sv, _order.orderId, pIbContract->symbol, order.lmtPrice, ToDouble(order.totalQuantity) );
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
				//WebSendGateway::PushS( "Place order failed", e.Move(), {{session.SessionId}, r.id()} );
			}
			//todo allow cancel
/*			std::thread{ [ pBlockly=*p ]()
			{
				pBlockly->Run();
				while( pBlockly->Running() )
					std::this_thread::sleep_for( 5s );
			}}.detach();
*/
		}
		if( place )
			_pTws->placeOrder( *_pContract, _order );//can't have h destruct this when _blockId.size() is using it.

/*		if( !order.whatIf && r.stop()>0 )
		{
			var parentId = _tws.RequestId();
			_requestSession.emplace( parentId, make_tuple(sessionId, r.id()) );
			Jde::Markets::MyOrder parent{ parentId, r.order() };
			parent.IsBuy( !order.IsBuy() );
			parent.OrderType( r::EOrderType::StopLimit );
			parent.totalQuantity = order.totalQuantity;
			parent.auxPrice = r.stop();
			parent.lmtPrice = r.stop_limit();
			parent.parentId = reqId;
			_tws.placeOrder( *pIbContract, parent );
		}*/
	}
	α PlaceOrderAwait::await_resume()noexcept->AwaitResult
	{
		base::AwaitResume();
		return move( _pPromise->get_return_object().Result() );
	}
}