#include "TwsClientCo.h"
#include "../wrapper/WrapperCo.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#define var const auto

namespace Jde::Markets
{
	TwsClientCo::TwsClientCo( const TwsConnectionSettings& settings, shared_ptr<WrapperCo> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}
	sp<WrapperCo> TwsClientCo::WrapperPtr()noexcept{ return dynamic_pointer_cast<WrapperCo>( _pWrapper ); }
	/*****************************************************************************************************/
	void HistoricalNewsAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		ITwsAwaitableImpl::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_newsHandles.MoveIn( id, move(h) );
		_fnctn( id, _pTws);
	}
	α TwsClientCo::HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept->HistoricalNewsAwaitable{ return HistoricalNewsAwaitable{ [=]( ibapi::OrderId id, sp<TwsClient> p )noexcept{p->reqHistoricalNews( id, conId, providerCodes, totalResults, start, end );} }; }
	/*****************************************************************************************************/
	bool ContractAwaitable::await_ready()noexcept{ return base::await_ready() || (_id && (bool)(_pCache = Cache::Get<Contract>(CacheId())) ); }
	void ContractAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		ITwsAwaitableImpl::await_suspend( h );
		var id = _pTws->RequestId();
		WrapperPtr()->_contractSingleHandles.MoveIn( id, move(h) );
		_fnctn( id, _pTws );
	}
	α ContractAwaitable::await_resume()noexcept->typename ContractAwaitable::TResult
	{
		ContractAwaitable::TResult result = _pPromise ? base::await_resume() : TaskResult{ _pCache };
		if( _pPromise && result.HasValue() )
		{
			_pCache = result.Get<Contract>();
			Cache::Set<Contract>( CacheId(), _pCache );
		}
		return result;
	}
	α TwsClientCo::ContractDetails( ContractPK conId )noexcept->ContractAwaitable{ return ContractAwaitable{ conId, [=]( TickerId id, sp<TwsClient> p )noexcept
	{
		::Contract c; c.conId=conId;
		p->reqContractDetails( id, c );
	}};}

	α TwsClientCo::ContractDetails( sp<::Contract> c )noexcept->ContractAwaitable{ return ContractAwaitable{ 0, [=]( TickerId id, sp<TwsClient> p )noexcept
	{
		p->reqContractDetails( id, *c );
	}};}
	/*****************************************************************************************************/
	bool NewsProviderAwaitable::await_ready()noexcept{ return base::await_ready() || (bool)(_pCache = Cache::Get<map<string,string>>(CacheId()) ); }
	void NewsProviderAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		ITwsAwaitableImpl::await_suspend( h );
		WrapperPtr()->_newsProviderHandles.MoveIn( move(h) );
		_fnctn( _pTws );
	}
	α NewsProviderAwaitable::await_resume()noexcept->typename NewsProviderAwaitable::TResult
	{
		NewsProviderAwaitable::TResult result = _pPromise ? base::await_resume() : TaskResult{ _pCache };
		if( _pPromise && result.HasValue() )
			Cache::Set<map<string,string>>( CacheId(), _pCache = result.Get<map<string,string>>() );
		return result;
	}
	α TwsClientCo::NewsProviders()noexcept->NewsProviderAwaitable{ return NewsProviderAwaitable{ [&]( sp<TwsClient> p )noexcept
	{
		p->reqNewsProviders();
	} 	};}
	/*****************************************************************************************************/
	#define ADD_REQUEST(x) ITwsAwaitableImpl::await_suspend( h );	var id = _pTws->RequestId(); WrapperPtr()->x.MoveIn( id, move(h) ); _fnctn( id, _pTws );
	void NewsArticleAwaitable::await_suspend( typename base::THandle h )noexcept
	{
		ADD_REQUEST( _newsArticleHandles );
	}
	α TwsClientCo::NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwaitable{ return NewsArticleAwaitable{ [=]( ibapi::OrderId id, sp<TwsClient> p )noexcept
	{
		p->reqNewsArticle( id, providerCode, articleId );
	} 	};}
}