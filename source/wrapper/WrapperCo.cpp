#include "WrapperCo.h"
#include <jde/markets/types/Contract.h>
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../client/awaitables/HistoricalDataAwaitable.h"
#include "../data/Accounts.h"
#include "../types/Exchanges.h"
#include "../types/IBException.h"
#define var const auto

namespace Jde::Markets
{
	α Resume( UnorderedMapValue<int,HCoroutine>& handles, int reqId, sp<void> pResult )->void
	{
		auto p = handles.MoveOut( reqId ); RETURN_IF( !p, "({})Could not get co-handle", reqId );
		p->promise().get_return_object().SetResult( pResult );
		Coroutine::CoroutinePool::Resume( move(*p) );
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
				pHandle->promise().get_return_object().SetResult( IBException::SP(errorMsg, errorCode, id) );
				Coroutine::CoroutinePool::Resume( move(*pHandle) );
			}
			return pHandle.has_value();
		};
		bool handled = r(_contractSingleHandles ) || r(_newsArticleHandles) || r(_newsHandles) || r(_newsArticleHandles);
		if( auto p = !handled ? _historical.Find(id) : std::nullopt; p )
		{
			auto h = (*_historical.Find(id))->_hCoroutine;
			h.promise().get_return_object().SetResult( IBException::SP(errorMsg, errorCode, id) );
			_historical.erase( id );
			_historicalData.erase( id );
			Coroutine::CoroutinePool::Resume( move(h) );
			handled = true;
		}
		return handled;
	}
	α WrapperCo::error( int id, int errorCode, str errorMsg )noexcept->void
	{
		error2( id, errorCode, errorMsg );
	}

	bool error2( int id, int errorCode, str errorMsg )noexcept;

	α WrapperCo::historicalNews( int reqId, str time, str providerCode, str articleId, str headline )noexcept->void
	{
		WrapperLog::historicalNews( reqId, time, providerCode, articleId, headline );
		auto& existing = Collections::InsertShared( _news, reqId );
		auto pNew = existing.add_historical();
		if( time.size()==21 )
			pNew->set_time( (uint32)DateTime((uint16)stoi(time.substr(0,4)), (uint8)stoi(time.substr(5,2)), (uint8)stoi(time.substr(8,2)), (uint8)stoi(time.substr(11,2)), (uint8)stoi(time.substr(14,2)), (uint8)stoi(time.substr(17,2)) ).TimeT() );//missing tenth seconds.

		pNew->set_provider_code( providerCode );
		pNew->set_article_id( articleId );
		pNew->set_headline( headline );
	}
	α WrapperCo::historicalNewsEnd( int reqId, bool hasMore )noexcept->void
	{
		WrapperLog::historicalNewsEnd( reqId, hasMore );
		sp<Proto::Results::NewsCollection> pCollection;
		if( auto pExisting = _news.find(reqId); pExisting!=_news.end() )
		{
			pCollection = pExisting->second;
			_news.erase( pExisting );
		}
		else
			pCollection = make_shared<Proto::Results::NewsCollection>();
		pCollection->set_has_more( hasMore );
		auto pHandle = _newsHandles.MoveOut( reqId );
		if( pHandle )
		{
			auto& returnObject = pHandle->promise().get_return_object();
			returnObject.SetResult( pCollection );
			Coroutine::CoroutinePool::Resume( move(*pHandle) );
		}
	}

	α WrapperCo::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept->void
	{
		WrapperLog::contractDetails( reqId, contractDetails );
		_contracts.try_emplace( reqId ).first->second.push_back( ms<Contract>(contractDetails) );
	}
	α WrapperCo::contractDetailsEnd( int reqId )noexcept->void
	{
		WrapperLog::contractDetailsEnd( reqId );

		auto pExisting = _contracts.find( reqId ); var have = pExisting!=_contracts.end();
		vector<sp<Contract>> contracts = have ? move( pExisting->second ) : vector<sp<Contract>>{};
		if( have )
			_contracts.erase( pExisting );

		for( auto& p : contracts )
			Cache::Set( format(Contract::CacheFormat, p->Id), p );

		auto pHandle = _contractSingleHandles.MoveOut( reqId );
		if( pHandle )
		{
			auto& returnObject = pHandle->promise().get_return_object(); WARN_IF( contracts.size()>1, "({}) returned {} contracts, expected 1", reqId, contracts.size() );
			if( contracts.size()==0 )
				returnObject.SetResult( IBException::SP("no contracts returned", -1, reqId) );
			else
				returnObject.SetResult( contracts.front() );
			Coroutine::CoroutinePool::Resume( move(*pHandle) );
		}
	}
	α WrapperCo::managedAccounts( str x )noexcept->void
	{
		WrapperLog::managedAccounts( x );
		Accounts::Set( Str::Split(x) );
		if( _accountHandle )
			Coroutine::CoroutinePool::Resume( move(_accountHandle) );
	}
	α WrapperCo::newsProviders( const std::vector<NewsProvider>& providers )noexcept->void
	{
		auto p = make_shared<map<string,string>>();
		for_each( providers.begin(), providers.end(), [p](auto x){p->emplace(x.providerCode, x.providerName);} );
		_newsProviderHandles.ForEach( [p](auto h)
		{
			h.promise().get_return_object().SetResult( make_shared<map<string,string>>(*p) );
			Coroutine::CoroutinePool::Resume( move(h) );
		} );
	}

	α WrapperCo::newsArticle( int reqId, int articleType, str articleText )noexcept->void
	{
		auto p = make_shared<Proto::Results::NewsArticle>();
		p->set_is_text( articleType==0 );
		p->set_value( articleText );
		Resume( _newsArticleHandles, reqId, p );
	}

	bool WrapperCo::HistoricalData( TickerId reqId, const ::Bar& bar )noexcept
	{
		WrapperLog::historicalData( reqId, bar );
		bool has = _historical.Has( reqId );
		if( has )
			_historicalData.try_emplace( reqId ).first->second.push_back( bar );
		return has;
	}
	bool WrapperCo::HistoricalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept
	{
		WrapperLog::historicalDataEnd( reqId, startDateStr, endDateStr );
		auto ppAwaitable = _historical.Find( reqId );
		if( !ppAwaitable.has_value() )
			return false;
		auto pData = _historicalData.find( reqId );
		(*ppAwaitable)->SetTwsResults( reqId, pData==_historicalData.end() ? vector<::Bar>{} : move(pData->second) );//

		_historical.erase( reqId );
		if( pData!=_historicalData.end() )
			_historicalData.erase( pData );
		return true;
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

	α WrapperCo::securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<string>& expirations, const std::set<double>& strikes )noexcept->void
	{
		WrapperLog::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		if( CIString{exchange}=="SMART"sv )
			*Collections::InsertUnique( _optionParams, reqId )->add_exchanges() = ToOptionParam(  exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
	}

	α WrapperCo::securityDefinitionOptionalParameterEnd( int reqId )noexcept->void
	{
		WrapperLog::securityDefinitionOptionalParameterEnd( reqId );
		var pParams = _optionParams.find( reqId );
		auto pResults = pParams!=_optionParams.end() ? move( (*pParams).second ) : make_unique<Proto::Results::OptionExchanges>();
		_optionParams.erase( reqId );
		Resume( _secDefOptParamHandles, reqId, move(pResults) );
	}
}