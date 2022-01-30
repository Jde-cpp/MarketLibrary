﻿#include "HistoricalDataAwaitable.h"
#include <jde/markets/types/Contract.h>
#include "../../../../Framework/source/Cache.h"
#include "../TwsClientCo.h"
#include "../../data/HistoricalDataCache.h"
#include "../../data/BarData.h"
#include "../../types/Exchanges.h"
#include "../../types/IBException.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	static var& _logLevel{ Logging::TagLevel("mrk-hist") };

	HistoryAwait::HistoryAwait( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, time_t start, SL sl )noexcept:
		ITwsAwaitShared{ sl },
		_pContract{ pContract }, _end{ end }, _dayCount{ dayCount }, _start{ start }, _barSize{ barSize }, _display{ display }, _useRth{ useRth }
	{
		LOG( "HistoryAwait={:x}", (uint)this );
	}

	HistoryAwait::~HistoryAwait(){ LOG( "HistoryAwait={:x}", (uint)this ); }
	α HistoryAwait::SetData( bool force )noexcept->bool
	{
		bool set = false;
		if( _cache.size() )
		{
			var haveStartAndEnd = _cache.begin()->second && _cache.rbegin()->second;
			var endsToday = _cache.rbegin()->first==CurrentTradingDay( *_pContract );
			var isOpen = IsOpen( *_pContract, _useRth );
			set = haveStartAndEnd && ( !endsToday || !isOpen );
		}
		if( force || set )//have inclusive and not asking for live data.
		{
			_dataPtr = make_shared<vector<::Bar>>();
			for( var& dayBars : _cache )
			{
				if( !dayBars.second )
					continue;
				for( var& pBar : *dayBars.second )
					_dataPtr->push_back( *pBar );
			}
		}
		return set;
	}
	α HistoryAwait::await_ready()noexcept->bool
	{
		_cache = HistoryCache::Get( *_pContract, _end, _dayCount, _barSize, (Proto::Requests::Display)_display, _useRth );// : MapPtr<Day,VectorPtr<sp<const ::Bar>>>{};
		return SetData();
	}
	α HistoryAwait::Missing()noexcept->vector<tuple<Day,Day>>
	{
		Day start=0, previous=0;
		vector<tuple<Day,Day>> values;
		for( var& [day,pBars] : _cache )
		{
			if( pBars )
			{
				if( start )
				{
					values.push_back( make_tuple(start,previous) );
					start = 0;
				}
			}
			else if( !start )
				start = day;
			previous = day;
		}
		if( start )
			values.push_back( make_tuple(start, _cache.rbegin()->first) );
		return values;
	}

	α HistoryAwait::await_suspend( HCoroutine h )noexcept->void
	{
		base::await_suspend( h );
		AsyncFetch( move(h) );
	}
	α HistoryAwait::AsyncFetch( HCoroutine h )noexcept->Task
	{
		if( _pContract->SecType==SecurityType::Stock && _display==EDisplay::Trades && _useRth && _barSize<EBarSize::Week )
		{
			DBG( "this={:x} _cache={:x}", (uint)this, (uint)&_cache );
			try
			{
				vector<::Bar> bars;
				for( var& [start,end] : Missing() )
				{
					auto pCache = &_cache;//for some reason gets lost in msvc.
					auto pBars = ( co_await BarData::CoLoad( _pContract, start, end) ).SP<map<Day,VectorPtr<CandleStick>>>();
					if( !pBars->size() )
						continue;
					LOG( "({})HistoryAwait::AsyncFetch have files {}-{}", _pContract->Symbol, DateDisplay(start), DateDisplay(end) );
					for( var& [day,pSticks] : *pBars )
					{
						CONTINUE_IF( pCache->find(day)->second, "Day {} has value, but loaded again.", day );
						auto pDayBars = ms<vector<sp<Bar>>>();
						for( auto baseTime = RthBegin( *_pContract, day ); var& stick : *pSticks )
						{
							var ib = ms<::Bar>( stick.ToIB(baseTime) );
							pDayBars->push_back( ib );
							bars.push_back( *ib );
							baseTime+=1min;
						}
						VectorPtr<sp<::Bar>> pEmplaced = _barSize==EBarSize::Minute ? pDayBars : BarData::Combine(*_pContract, day, *pDayBars, _barSize );
						(*pCache)[day] = pEmplaced;
						if( _barSize==EBarSize::Day )
							HistoryCache::SetDay( *_pContract, true, *pEmplaced );
						else
							HistoryCache::Set( *_pContract, EDisplay::Trades, EBarSize::Minute, true, bars );
					}
				}
				if( bars.size() && SetData() )
				{
					LOG( "HistoryAwait::AsyncFetch have files" );
					CoroutinePool::Resume( move(h) );
					co_return;
				}
			}
			catch( const IException& )//just reload.
			{}
		}
		_hCoroutine = move(h);

		if( var current=CurrentTradingDay(*_pContract); _cache.rbegin()->first==current && IsOpen(*_pContract) )//missing will return nothing
			_cache[current] = nullptr;
		var requests = Missing(); _twsRequests.reserve( requests.size() );
		LOG( "AsyncFetch - requests={}", requests.size() );
		for( uint i=0; i<requests.size(); ++i )
			_twsRequests.push_back( _pTws->RequestId() );
		for( uint i=0; i<requests.size(); ++i )
		{
			var start = std::get<0>( requests[i] ); var end = std::get<1>( requests[i] ); var id = _twsRequests[i];
			WrapperPtr()->_historical.emplace( id, this );
			const DateTime endTime{ Chrono::FromDays(end) };
			var endTimeString{ format("{}{:0>2}{:0>2} 23:59:59 GMT", endTime.Year(), endTime.Month(), endTime.Day()) };
			uint dayCount = 0;
			for( auto iDay=end; iDay>=start; --iDay )
			{
				if( !IsHoliday(iDay) )
					++dayCount;
			}
			if( var size=WrapperLog::HistoricalDataRequestSize(); size>TwsClient::MaxHistoricalDataRequest() )
			{
				h.promise().get_return_object().SetResult( IBException{format("Only '{}' historical data requests allowed at one time - {}.", TwsClient::MaxHistoricalDataRequest(), size), 322, id} );
				h.resume();
			}
			else
				_pTws->reqHistoricalData( id, *_pContract->ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::ToString(_barSize)}, string{TwsDisplay::ToString(_display)}, _useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
		}
	}

	α HistoryAwait::SetTwsResults( ibapi::OrderId reqId, const vector<::Bar>& bars )->void
	{
		HistoryCache::Set( *_pContract, _display, _barSize, _useRth, bars, _end, _dayCount );
		for( var& bar : bars )
		{
			var day = Chrono::ToDays( ConvertIBDate(bar.time) );
			auto p = _cache.find( day );
			if( p==_cache.end() )
				p = _cache.try_emplace( day, make_shared<vector<sp<::Bar>>>() ).first;
			else if( p->second==nullptr )
				p->second = make_shared<vector<sp<::Bar>>>();
			p->second->push_back( make_shared<::Bar>(bar) );
		}
		SetData( true );
		if( auto p = find( _twsRequests.begin(), _twsRequests.end(), reqId ); p != _twsRequests.end() )
			*p=0;
		if( find_if(_twsRequests.begin(), _twsRequests.end(), [](auto x){return x!=0;})==_twsRequests.end() )
		{
			LOG( "({})HistoryAwait::AddTws - resume", reqId );
			CoroutinePool::Resume( move(_hCoroutine) );
		}
	}
}