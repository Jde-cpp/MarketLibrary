#include "HistoricalDataAwaitable.h"
#include "../../../../Framework/source/Cache.h"
#include "../../data/HistoricalDataCache.h"
#include "../../data/BarData.h"
#include "../../wrapper/WrapperCo.h"

#define var const auto

namespace Jde::Markets
{
	static var& _logLevel{ Logging::TagLevel("hist") };

	α HistoricalDataAwaitable::AddTws( ibapi::OrderId reqId, const vector<::Bar>& bars )->void
	{
		HistoricalDataCache::Push( *_contractPtr, _display, _barSize, _useRth, bars, _end, _dayCount );
		for( var& bar : bars )
		{
			var day = Chrono::ToDay( ConvertIBDate(bar.time) );
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
		if( find_if( _twsRequests.begin(), _twsRequests.end(), [](auto x){return x!=0;})==_twsRequests.end() )
		{
			LOG( "({})HistoricalDataAwaitable::AddTws - resume"sv, _hCoroutine.address() );
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
	α HistoricalDataAwaitable::Missing()noexcept->vector<tuple<DayIndex,DayIndex>>
	{
		DayIndex start=0;
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
	}
	α HistoricalDataAwaitable::AsyncFetch( HCoroutine h )noexcept->Task2
	{
		if( _contractPtr->SecType==SecurityType::Stock )
		{
			try
			{
				for( var& [start,end] : Missing() )
				{
					auto pBars = ( co_await BarData::CoLoad( _contractPtr, start, end) ).Get<map<DayIndex,VectorPtr<CandleStick>>>();
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
			catch( const IException& )//just reload.
			{}
		}
		if( SetData() )
		{
			DBG( "({})HistoricalDataAwaitable::AsyncFetch have files"sv, h.address() );
			CoroutinePool::Resume( move(h) );
			co_return;
		}
		_hCoroutine = move(h);

		if( var current=CurrentTradingDay(*_contractPtr); _cache.rbegin()->first==current && IsOpen(*_contractPtr) )//missing will return nothing
			_cache[current] = nullptr;
		var requests = Missing(); _twsRequests.reserve( requests.size() );
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
			_pTws->reqHistoricalData( id, *_contractPtr->ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::ToString(_barSize)}, string{TwsDisplay::ToString(_display)}, _useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
		}
	}
	void HistoricalDataAwaitable::await_suspend( HCoroutine h )noexcept
	{
		base::await_suspend( h );
		AsyncFetch( move(h) );
	}
	α HistoricalDataAwaitable::await_resume()noexcept->TaskResult
	{
		return _dataPtr ? TaskResult{ _dataPtr } : base::await_resume();
	}
}