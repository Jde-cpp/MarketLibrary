#include "WrapperCo.h"
#include <jde/markets/types/Contract.h>
#include <jde/markets/types/MyOrder.h>
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../client/awaitables/HistoricalDataAwaitable.h"
#include "../data/Accounts.h"
#include "../types/Exchanges.h"
#include "../types/IBException.h"

#define var const auto
#define $ noexcept->void
namespace Jde::Markets
{
	flat_map<TickerId,vector<::Bar>> _historicalData;

	ⓣ Resume( UnorderedMapValue<int,HCoroutine>& handles, int reqId, up<T> pResult )$
	{
		if( auto p = handles.MoveOut( reqId ); p )
		{
			p->promise().get_return_object().SetResult<T>( move(pResult) );
			Coroutine::CoroutinePool::Resume( move(*p) );
		}
		else
			WARN( "({})Could not get co-handle", reqId );
	}

	ⓣ Resume( UnorderedMapValue<ReqId,HCoroutine>& handles, int reqId, sp<T>&& pResult, bool warn=true )$
	{
		if( auto h = handles.MoveOut( reqId ); h )
		{
			h->promise().get_return_object().SetSP<T>( move(pResult) );
			Coroutine::CoroutinePool::Resume( move(*h) );
		}
		else if( warn )
			WARN( "({})Could not get co-handle", reqId );
	}

	bool WrapperCo::error2( int id, int errorCode, str errorMsg )noexcept
	{
		if( WrapperLog::error2(id, errorCode, errorMsg) )
			return true;
		auto r = [&]( auto& handles )->bool
		{
			auto pHandle = handles.MoveOut( id );
			if( pHandle )
			{
				pHandle->promise().get_return_object().SetResult( IBException{errorMsg, errorCode, id} );
				Coroutine::CoroutinePool::Resume( move(*pHandle) );
			}
			return pHandle.has_value();
		};
		if( auto h = errorCode!=399 ? nullopt : _orderHandles.Find(id); h )
			h->promise().get_return_object().SetResult( IBException{errorMsg, errorCode, id, ELogLevel::None} );

		bool handled = errorCode==399 || r( _contractHandles ) || r(_orderHandles) || r(_newsArticleHandles) || r(_newsHandles) || r(_newsArticleHandles);
		if( auto p = !handled ? _historical.Find(id) : std::nullopt; p )
		{
			auto h = (*_historical.Find(id))->_hCoroutine;
			h.promise().get_return_object().SetResult( IBException{errorMsg, errorCode, id} );
			_historical.erase( id );
			_historicalData.erase( id );
			Coroutine::CoroutinePool::Resume( move(h) );
			handled = true;
		}
		return handled;
	}
	α WrapperCo::error( int id, int errorCode, str errorMsg, str advancedOrderRejectJson )$
	{
		error2( id, errorCode, errorMsg );
	}

	bool error2( int id, int errorCode, str errorMsg )noexcept;

	α WrapperCo::historicalNews( int reqId, str time, str providerCode, str articleId, str headline )$
	{
		WrapperLog::historicalNews( reqId, time, providerCode, articleId, headline );
		auto pNew = EmplaceUnique( _news, reqId )->add_historical();
		if( time.size()==21 )
			pNew->set_time( (uint32)DateTime((uint16)stoi(time.substr(0,4)), (uint8)stoi(time.substr(5,2)), (uint8)stoi(time.substr(8,2)), (uint8)stoi(time.substr(11,2)), (uint8)stoi(time.substr(14,2)), (uint8)stoi(time.substr(17,2)) ).TimeT() );//missing tenth seconds.

		pNew->set_provider_code( providerCode );
		pNew->set_article_id( articleId );
		pNew->set_headline( headline );
	}
	α WrapperCo::historicalNewsEnd( int reqId, bool hasMore )$
	{
		WrapperLog::historicalNewsEnd( reqId, hasMore );
		up<Proto::Results::NewsCollection> pCollection;
		if( auto pExisting = _news.find(reqId); pExisting!=_news.end() )
		{
			pCollection = move( pExisting->second );
			_news.erase( pExisting );
		}
		else
			pCollection = mu<Proto::Results::NewsCollection>();
		pCollection->set_has_more( hasMore );
		auto pHandle = _newsHandles.MoveOut( reqId );
		if( pHandle )
		{
			pHandle->promise().get_return_object().SetResult( pCollection.release() );
			Coroutine::CoroutinePool::Resume( move(*pHandle) );
		}
	}

	α WrapperCo::contractDetails( int reqId, const ::ContractDetails& contractDetails )$
	{
		WrapperLog::contractDetails( reqId, contractDetails );
		EmplaceUnique( _requestContracts, reqId )->push_back( ms<::ContractDetails>(contractDetails) );
	}
	α WrapperCo::contractDetailsEnd( int reqId )$
	{
		WrapperLog::contractDetailsEnd( reqId );

		AwaitResult::Value v;
		if( auto p = _requestContracts.find(reqId); p!=_requestContracts.end() && p->second )
		{
			v = p->second.release();
			_requestContracts.erase( p );
		}
		else
			v = new IBException{ "no contracts returned", -1, reqId };

		if( auto pHandle = _contractHandles.MoveOut( reqId ); pHandle )
		{
			pHandle->promise().get_return_object().SetResult( move(v) );
			Coroutine::CoroutinePool::Resume( move(*pHandle) );
		}
	}
	α WrapperCo::managedAccounts( str x )$
	{
		WrapperLog::managedAccounts( x );
		Accounts::Set( Str::Split(x) );
		if( _accountHandle )
			Coroutine::CoroutinePool::Resume( move(_accountHandle) );
	}
	α WrapperCo::newsProviders( const std::vector<NewsProvider>& providers )$
	{
		auto p = ms<map<string,string>>();
		for_each( providers.begin(), providers.end(), [p](auto x){p->emplace(x.providerCode, x.providerName);} );
		_newsProviderHandles.ForEach( [p](auto h)
		{
			h.promise().get_return_object().SetResult( ms<map<string,string>>(*p) );
			Coroutine::CoroutinePool::Resume( move(h) );
		} );
	}

	α WrapperCo::newsArticle( int reqId, int articleType, str articleText )$
	{
		auto p = mu<Proto::Results::NewsArticle>();
		p->set_is_text( articleType==0 );
		p->set_value( articleText );
		Resume( _newsArticleHandles, reqId, move(p) );
	}

	α WrapperCo::historicalData( TickerId reqId, const ::Bar& bar )$
	{
		WrapperLog::historicalData( reqId, bar );
		bool has = _historical.Has( reqId );
		if( has )
			_historicalData.try_emplace( reqId ).first->second.push_back( bar );
//		return has;
	}

	α WrapperCo::historicalDataEnd( int reqId, str startDateStr, str endDateStr )$
	{
		WrapperLog::historicalDataEnd( reqId, startDateStr, endDateStr );
		auto ppAwaitable = _historical.Find( reqId );
		if( !ppAwaitable.has_value() )
			return /*false*/;
		auto pData = _historicalData.find( reqId );
		(*ppAwaitable)->SetTwsResults( reqId, pData==_historicalData.end() ? vector<::Bar>{} : move(pData->second) );//

		_historical.erase( reqId );
		if( pData!=_historicalData.end() )
			_historicalData.erase( pData );
		//return true;
	}

	Proto::Results::ExchangeContracts ToOptionParam( sv exchangeString, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		auto exchange = ToExchange( exchangeString );
		if( exchange==Exchanges::Smart && CIString{ "SMART"sv }!=exchangeString )
			exchange = Exchanges::UnknownExchange;
		Proto::Results::ExchangeContracts y; y.set_exchange( exchange ); y.set_multiplier( multiplier ); y.set_trading_class( tradingClass ); y.set_underlying_contract_id( underlyingConId );
		for( var strike : strikes )
			y.add_strikes( strike );

		for( var& expiration : expirations )
			y.add_expirations( Contract::ToDay(expiration) );
		return y;
	}

	α WrapperCo::securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<string>& expirations, const std::set<double>& strikes )$
	{
		WrapperLog::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		if( CIString{exchange}=="SMART"sv )
			*EmplaceShared( _optionParams, reqId )->add_exchanges() = ToOptionParam(  exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
	}

	α WrapperCo::securityDefinitionOptionalParameterEnd( int reqId )$
	{
		WrapperLog::securityDefinitionOptionalParameterEnd( reqId );
		var pParams = _optionParams.find( reqId );
		auto pResults = pParams!=_optionParams.end() ? move( (*pParams).second ) : ms<Proto::Results::OptionExchanges>();
		_optionParams.erase( reqId );
		Resume( _secDefOptParamHandles, reqId, move(pResults) );
	}

	α WrapperCo::OpenOrder( ::OrderId orderId, const ::Contract& contract, const ::Order& order, const ::OrderState& state )noexcept->sp<Proto::Results::OpenOrder>
	{
		WrapperLog::openOrder( orderId, contract, order, state );
		auto p = ms<Proto::Results::OpenOrder>();
		p->set_allocated_contract( Contract{contract}.ToProto().release() );
		p->set_allocated_order( MyOrder{order}.ToProto().release() );
		p->set_allocated_state( ToProto(state).release() );
		if( var h = _orderHandles.Find(orderId); h && h->promise().get_return_object().Result().HasError() )
		{
			auto pExp = h->promise().get_return_object().Result().Error();
			p->mutable_state()->set_warning_text( format("({}){}", pExp->Code, pExp->What()) );
		}
		Resume( _orderHandles, orderId, move(p), false );

		return p;
	}
}