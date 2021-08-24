#include "TwsClientCo.h"
#include "../wrapper/WrapperCo.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../data/HistoricalDataCache.h"
#include "../data/BarData.h"
#define var const auto

namespace Jde::Markets
{
	ITwsAwaitable::ITwsAwaitable()noexcept:_pTws{ TwsClientCo::InstancePtr() }{}
	sp<WrapperCo> ITwsAwaitable::WrapperPtr()noexcept{ return _pTws->WrapperPtr(); }

	TwsClientCo::TwsClientCo( const TwsConnectionSettings& settings, shared_ptr<WrapperCo> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}
	sp<WrapperCo> TwsClientCo::WrapperPtr()noexcept{ return dynamic_pointer_cast<WrapperCo>( _pWrapper ); }

	/*****************************************************************************************************/
	α HistoricalDataAwaitable::AddTws( ibapi::OrderId reqId, const vector<::Bar>& bars )->void
	{
		HistoricalDataCache::Push( *_contractPtr, _display, _barSize, _useRth, bars, _end, _dayCount );
		for( auto bar : bars )
		{
			var day = Chrono::ToDay( ConvertIBDate(bar.time) );
			auto p = _cache.find( day );
			if( p==_cache.end() )
				p = _cache.try_emplace( day, make_shared<vector<sp<::Bar>>>() ).first;
			else if( p->second==nullptr )
				p->second = make_shared<vector<sp<::Bar>>>();
			p->second->push_back( make_shared<::Bar>(move(bar)) );
		}
		SetData( true );
		if( auto p = find( _twsRequests.begin(), _twsRequests.end(), reqId ); p != _twsRequests.end() )
			*p=0;
		if( find_if( _twsRequests.begin(), _twsRequests.end(), [](auto x){return x!=0;})==_twsRequests.end() )
		{
			LOG( CoroutinePool::LogLevel(), "({})HistoricalDataAwaitable::AddTws - resume"sv, _hCoroutine.address() );
			CoroutinePool::Resume( move(_hCoroutine) );
		}
	}
	bool HistoricalDataAwaitable::SetData( bool force )noexcept
	{
		var set = _cache.size() && _cache.begin()->second && _cache.rbegin()->second && ( _cache.rbegin()->first!=CurrentTradingDay(*_contractPtr) || !IsOpen(*_contractPtr) );
		if( force || set )//have inclusive and not asking for live data.
		{
			_dataPtr = make_shared<vector<::Bar>>();
			for( var& dayBars : _cache )
			{
				CONTINUE_IF( !dayBars.second, "Missing day {}", dayBars.first );
				for( var& pBar : *dayBars.second )
					_dataPtr->push_back( *pBar );
			}
		}
		return set;
	}
	bool HistoricalDataAwaitable::await_ready()noexcept
	{
		_cache = HistoricalDataCache::ReqHistoricalData2( *_contractPtr, _end, _dayCount, _barSize, (Proto::Requests::Display)_display, _useRth );// : MapPtr<DayIndex,VectorPtr<sp<const ::Bar>>>{};
		return SetData();
	}
	α HistoricalDataAwaitable::AsyncFetch( HCoroutine h )noexcept->Task2
	{
		auto missing = [this]()
		{
			uint start=0;
			vector<tuple<DayIndex,DayIndex>> values;
			for( var& [day,pBars] : _cache )
			{
				if( pBars )
				{
					if( start )
					{
						values.push_back( make_tuple(start,day) );
						start = 0;
					}
				}
				else if( !start )
					start = day;
			}
			if( start )
				values.push_back( make_tuple(start, _cache.rbegin()->first) );
			return values;
		};
		try
		{
			for( var& [start,end] : missing() )
			{
				DBG( "BarData::CoLoad"sv );
				auto pBars = ( co_await BarData::CoLoad( *_contractPtr, start, end) ).Get<map<DayIndex,VectorPtr<CandleStick>>>();
				for( auto [day,pSticks] : *pBars )
				{
					ERR_IF( _cache.find(end)->second, "Day {} has value, but loaded again.", end );
					auto pDayBars = _cache[end] = make_shared<vector<sp<Bar>>>();
					auto baseTime = RthBegin( *_contractPtr, end );
					for( var& stick : *pSticks )
					{
						pDayBars->push_back( make_shared<::Bar>(stick.ToIB(baseTime)) );
						baseTime+=1min;
					}
				}
			}
		}
		catch( const Exception& e )//just reload.
		{}
		if( SetData() )
		{
			DBG( "({})HistoricalDataAwaitable::AsyncFetch have files"sv, h.address() );
			CoroutinePool::Resume( move(h) );
			co_return;
		}
		_hCoroutine = move(h);
//		DBG( "({})HistoricalDataAwaitable::_hCoroutine"sv, _hCoroutine.address() );
		var requests = missing(); _twsRequests.reserve( requests.size() );
		for( uint i=0; i<requests.size(); ++i )
			_twsRequests.push_back( _pTws->RequestId() );
		for( uint i=0; i<requests.size(); ++i )
		{
			var start = std::get<0>( requests[i] ); var end = std::get<0>( requests[i] ); var id = _twsRequests[i];
			WrapperPtr()->_historical.emplace( id, this );
			const DateTime endTime{ Chrono::FromDays(end) };
			const string endTimeString{ format("{}{:0>2}{:0>2} 23:59:59 GMT", endTime.Year(), endTime.Month(), endTime.Day()) };
			uint dayCount = 0;
			for( DayIndex iDay=end; iDay>=start; --iDay )
			{
				if( !IsHoliday(start) )
					++dayCount;
			}
			_pTws->reqHistoricalData( id, *_contractPtr->ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::TryToString((BarSize::Enum)_barSize)}, string{TwsDisplay::ToString(_display)}, _useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
		}
	}
	void HistoricalDataAwaitable::await_suspend( HCoroutine h )noexcept
	{
		base::await_suspend( h );
		AsyncFetch( move(h) );
	}
	α HistoricalDataAwaitable::await_resume()noexcept->TaskResult
	{
		//DBG( "({})~~~~~~~~~~~~HistoricalDataAwaitable::await_resume~~~~~~~~~~~~~"sv, _hCoroutine.address() );
		return _dataPtr ? TaskResult{ _dataPtr } : base::await_resume();
	}
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
	bool ContractAwaitable::await_ready()noexcept{ return base::await_ready() || (bool)(_pCache = Cache::Get<Contract>(CacheId()) ); }
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
			_pCache = result.Get<Jde::Markets::Contract>();
			Cache::Set<Contract>( CacheId(), _pCache );
		}
		return result;
	}
	α TwsClientCo::ContractDetails( ContractPK conId )noexcept->ContractAwaitable{ return ContractAwaitable{ conId, [=]( ibapi::OrderId id, sp<TwsClient> p )noexcept
	{
		::Contract c; c.conId=conId;
		p->reqContractDetails( id, c );
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