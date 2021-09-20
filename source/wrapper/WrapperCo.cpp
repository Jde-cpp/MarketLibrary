#include "WrapperCo.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../types/IBException.h"

#define var const auto

namespace Jde::Markets
{
	bool WrapperCo::error2( int id, int errorCode, str errorMsg )noexcept
	{
		if( WrapperLog::error2(id, errorCode, errorMsg) )
			return true;
		WrapperLog::error( id, errorCode, errorMsg );
		auto r = [&]( auto& handles )->bool
		{
			auto pHandle = handles.MoveOut( id );
			if( pHandle )
			{
				pHandle->promise().get_return_object().SetResult( IB_Exception(errorMsg, errorCode, id) );
				Coroutine::CoroutinePool::Resume( move(*pHandle) );
			}
			return pHandle.has_value();
		};
		bool handled = r(_contractSingleHandles ) || r(_newsArticleHandles) || r(_newsHandles) || r(_newsArticleHandles);
		if( !handled && _historical.Has(id) )
		{
			auto h = (*_historical.Find(id))->_hCoroutine;
			h.promise().get_return_object().SetResult( IB_Exception(errorMsg, errorCode, id) );
			_historical.erase( id );
			_historicalData.erase( id );
			Coroutine::CoroutinePool::Resume( move(h) );
			handled = true;
		}
		return handled;
	}
	void WrapperCo::error( int id, int errorCode, str errorMsg )noexcept
	{
		error2( id, errorCode, errorMsg );
	}

	bool error2( int id, int errorCode, str errorMsg )noexcept;

	void WrapperCo::historicalNews( int reqId, str time, str providerCode, str articleId, str headline )noexcept
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
	void WrapperCo::historicalNewsEnd( int reqId, bool hasMore )noexcept
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

	void WrapperCo::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		WrapperLog::contractDetails( reqId, contractDetails );
		_contracts.try_emplace( reqId ).first->second.push_back( make_shared<Contract>(contractDetails) );
	}
	void WrapperCo::contractDetailsEnd( int reqId )noexcept
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
				returnObject.SetResult( IB_Exception("no contracts returned", -1, reqId) );
			else
				returnObject.SetResult( contracts.front() );
			Coroutine::CoroutinePool::Resume( move(*pHandle) );
		}
	}

	void WrapperCo::newsProviders( const std::vector<NewsProvider>& providers )noexcept
	{
		auto p = make_shared<map<string,string>>();
		for_each( providers.begin(), providers.end(), [p](auto x){p->emplace(x.providerCode, x.providerName);} );
		_newsProviderHandles.ForEach( [p](auto h)
		{
			h.promise().get_return_object().SetResult( make_shared<map<string,string>>(*p) );
			Coroutine::CoroutinePool::Resume( move(h) );
		} );
	}

	void WrapperCo::newsArticle( int reqId, int articleType, str articleText )noexcept
	{
		auto pHandle = _newsArticleHandles.MoveOut( reqId ); RETURN_IF( !pHandle, "({})Could not get co-handle", reqId );
		auto p = make_shared<Proto::Results::NewsArticle>();
		p->set_is_text( articleType==0 );
		p->set_value( articleText );
		pHandle->promise().get_return_object().SetResult( p );
		Coroutine::CoroutinePool::Resume( move(*pHandle) );
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
		(*ppAwaitable)->AddTws( reqId, pData==_historicalData.end() ? vector<::Bar>{} : move(pData->second) );//

		_historical.erase( reqId );
		if( pData!=_historicalData.end() )
			_historicalData.erase( pData );
		return true;
	}

}