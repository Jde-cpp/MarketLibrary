#include "TwsClientCo.h"
#include "../wrapper/WrapperCo.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#define var const auto

namespace Jde::Markets
{
	Tws::Tws( const TwsConnectionSettings& settings, sp<WrapperCo> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}
	α Tws::WrapperPtr()noexcept->sp<WrapperCo>{ return dynamic_pointer_cast<WrapperCo>( _pWrapper ); }
	/*****************************************************************************************************/
	α HistoricalNewsAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitUnique::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_newsHandles.MoveIn( id, move(h) );
		_fnctn( id, _pTws);
	}
	α Tws::HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept->HistoricalNewsAwait{ return HistoricalNewsAwait{ [=]( ibapi::OrderId id, sp<TwsClient> p )noexcept{p->reqHistoricalNews( id, conId, providerCodes, totalResults, start, end );} }; }
	/*****************************************************************************************************/
	α ContractAwait::await_ready()noexcept->bool{ return base::await_ready() || (_id && (bool)(_pCache = Cache::Get<::ContractDetails>(CacheId())) ); }
	α ContractAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitShared::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_contractHandles.MoveIn( id, move(h) );
		_fnctn( id, _pTws );
	}
	α ContractAwait::await_resume()noexcept->AwaitResult
	{
		AwaitResume();
		if( !_pPromise )
			return AwaitResult{ _pCache };

		auto& task = _pPromise->get_return_object();
		auto& result = task.Result();
		if( result.HasValue() )
		{
			auto pDetails = result.UP<vector<sp<::ContractDetails>>>();
			for( var& p : *pDetails )
				Cache::Set<::ContractDetails>( format(Contract::CacheDetailsFormat, p->contract.conId), p );
			if( _single )
			{
				if( pDetails->size()==0 )
					task.SetResult( Exception(_sl, "{} returned 0 records expected 1.", ToString()) );
				else
				{
					task.SetResult( pDetails->front() );
					WARN_IF( pDetails->size()>1, "{} returned {} records expected 1.", ToString(), pDetails->size() );
				}
			}
		}

		return result;
	}
	α Tws::ContractDetail( ContractPK conId )noexcept->ContractAwait{ return ContractAwait{ conId, [=]( ReqId id, sp<TwsClient> p )noexcept
	{
		::Contract c; c.conId=conId;
		p->reqContractDetails( id, c );
	}};}

	α Tws::ContractDetails( const ::Contract& c )noexcept->ContractAwait{ return ContractAwait{ c.conId, [=]( TickerId id, sp<TwsClient> p )noexcept
	{
		p->reqContractDetails( id, c );
	}, c.conId!=0}; }
	α Tws::ContractDetail( const ::Contract& c )noexcept->ContractAwait{ return ContractAwait{ c.conId, [&c]( ReqId id, sp<TwsClient> p )noexcept
	{
		p->reqContractDetails( id, c );
	}};}
	/*****************************************************************************************************/
	α NewsProviderAwait::await_ready()noexcept->bool{ return base::await_ready() || (bool)(_pCache = Cache::Get<map<string,string>>(CacheId()) ); }
	α NewsProviderAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ITwsAwaitShared::await_suspend( h );
		WrapperPtr()->_newsProviderHandles.MoveIn( move(h) );
		_fnctn( _pTws );
	}
	α NewsProviderAwait::await_resume()noexcept->AwaitResult
	{
		AwaitResult result = _pPromise ? base::await_resume() : AwaitResult{ _pCache };
		if( _pPromise && result.HasValue() )
			Cache::Set<map<string,string>>( CacheId(), _pCache = result.SP<map<string,string>>() );
		return result;
	}
	α Tws::NewsProviders()noexcept->NewsProviderAwait{ return NewsProviderAwait{ [&]( sp<TwsClient> p )noexcept
	{
		p->reqNewsProviders();
	} 	};}
	/*****************************************************************************************************/
	#define ADD_REQUEST(x) ITwsAwaitUnique::await_suspend( h );	var id = _pTws->RequestId(); WrapperPtr()->x.MoveIn( id, move(h) ); _fnctn( id, _pTws );
	α NewsArticleAwait::await_suspend( HCoroutine h )noexcept->void
	{
		ADD_REQUEST( _newsArticleHandles );
	}
	α Tws::NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwait{ return NewsArticleAwait{ [=]( ibapi::OrderId id, sp<TwsClient> p )noexcept
	{
		p->reqNewsArticle( id, providerCode, articleId );
	} 	};}
}