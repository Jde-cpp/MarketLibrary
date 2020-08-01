#include "./HistoricalDataCache.h"
#include "../types/Bar.h"
#include "../types/Contract.h"
#include "../data/BarData.h"
#include "../../../Framework/source/Cache.h"

#define var const auto
namespace Jde::Markets::HistoricalDataCache
{
	using namespace Chrono;
	using EBarSize=Proto::Requests::BarSize;
	using EDisplay=Proto::Requests::Display;
	typedef sp<ibapi::Bar> BarPtr;
	struct DataCache
	{
		optional<tuple<DayIndex,DayIndex>> Contains( EBarSize barSize, bool useRth, const set<DayIndex>& days )noexcept;
		VectorPtr<BarPtr> Get( const Contract& contract, DayIndex day, EBarSize barSize, bool useRth )noexcept;
		static void Push( const Contract& contract, EDisplay display, bool useRth, const vector<ibapi::Bar>& bars )noexcept;
		void Push( map<DayIndex,vector<BarPtr>> dayBars, bool extended )noexcept;
		static constexpr EBarSize Size{EBarSize::Minute};
		static string CacheId( ContractPK contractId, EDisplay display )noexcept{ return format("HistoricalDataCache.{}.{}", contractId, display); }
	private:
		map<DayIndex,vector<BarPtr>> _rth;	shared_mutex _rthMutex;
		map<DayIndex,vector<BarPtr>> _extended; shared_mutex _extendedMutex;
	};
	void Push( const Contract& contract, EDisplay display, EBarSize barSize, bool useRth, const vector<ibapi::Bar>& bars )noexcept
	{
		if( barSize==EBarSize::Minute )
			DataCache::Push( contract, display, useRth, bars );
		else
			TRACE( "Pushing barsize '{}' not supported."sv, BarSize::TryToString(barSize) );
	}
	//files
	MapPtr<DayIndex,VectorPtr<sp<ibapi::Bar>>> ReqHistoricalData( const Contract& contract, DayIndex endDay, uint dayCount, EBarSize barSize, EDisplay display, bool useRth )noexcept
	{
		MapPtr<DayIndex,VectorPtr<BarPtr>> pBars;
		if( !dayCount ){ ERR0("0 daycount sent in."sv); return pBars; }
		set<DayIndex> days;
		for( DayIndex i = endDay; days.size()<dayCount; --i )
		{
			if( !IsHoliday(i, contract.Exchange) )//change to Exchanges...080
				days.emplace( i );
		}

		auto pValues = Cache::TryGet<DataCache>( DataCache::CacheId(contract.Id, display) );
		uint missingCount;
		auto load = [&]()
		{
			missingCount = days.size();
			auto startEnd = pValues->Contains( barSize, useRth, days );
			if( startEnd.has_value() )
			{
				pBars = make_shared<map<DayIndex,VectorPtr<BarPtr>>>();
				var [start,end] = startEnd.value();
				for( var day : days )
				{
					auto p = day>=start && day<=end ? pValues->Get(contract, day, barSize, useRth) : VectorPtr<BarPtr>{};
					pBars->emplace( day, p );
					if( p )
						--missingCount;
				}
			}
			return startEnd;
		};
		var startEnd = load();
		if( useRth && display==EDisplay::Trades && missingCount && barSize%EBarSize::Minute==0 )//look in files
		{
			auto add = [pBars, pValues, contract]( const map<DayIndex,VectorPtr<CandleStick>>& additional )
			{
				map<DayIndex,vector<BarPtr>> bars;
				for( auto [day,pSticks] : additional )
				{
					if( pBars->find(day)->second )
						ERR("Day {} has value, but loaded again."sv, day );
					auto& dayBars = bars.emplace( day, vector<BarPtr>{} ).first->second;
					auto baseTime = RthBegin( contract, day );
					for( var& stick : *pSticks )
					{
						baseTime+=1min;
						dayBars.push_back( make_shared<ibapi::Bar>(stick.ToIB(baseTime)) );
					}
				}
				pValues->Push( bars, false );
			};
			if( !startEnd.has_value() )
				add( *BarData::ReqHistoricalData(contract, *days.begin(), *days.end()) );
			else
			{
				var [start,end] = startEnd.value();
				var pEnd = days.find( start );
				if( pEnd!=days.begin() )
					add( *BarData::ReqHistoricalData( contract, *days.begin(), *std::prev(pEnd)) );
				var pStart = days.find( end );
				if( pStart!=days.end() && std::next(pStart)!=days.end() )
					add( *BarData::ReqHistoricalData(contract, *std::next(pStart), *days.rbegin()) );
			}
			load();
		}
		return pBars;
	}

	optional<tuple<DayIndex,DayIndex>> DataCache::Contains( EBarSize barSize, bool useRth, const set<DayIndex>& days )noexcept
	{
		optional<tuple<DayIndex,DayIndex>> result;
		auto contains = Size>EBarSize::None && ( Size==EBarSize::Day || Size<=EBarSize::Hour ) && barSize%Size==0;
		if( contains )
		{
			shared_lock l1{_rthMutex};
			auto pBegin = _rth.lower_bound( *days.begin() ), pEnd = _rth.lower_bound( *days.rbegin() );
			contains = pBegin!=_rth.end() || pEnd!=_rth.end();
			if( contains && !useRth )
			{
				shared_lock l1{_extendedMutex};
				for( auto p=pBegin; contains && p!=pEnd; ++p )
					contains = _extended.find(p->first)!=_extended.end();
			}
			if( contains )
				result = make_tuple( pBegin->first, pEnd->first );//
		}
		return result;
	}
	VectorPtr<BarPtr> DataCache::Get( const Contract& contract, DayIndex day, EBarSize barSize, bool useRth )noexcept
	{
		auto pResult = make_shared<vector<BarPtr>>();
		shared_lock l1{_rthMutex};
		var pRth = _rth.find(day);  if( pRth==_rth.end() ){ ERR("trying to add bars but does not contain {}"sv, DateDisplay(FromDays(day)) ); return pResult; }
		var& rth = pRth->second;
		vector<BarPtr>* pExtended; vector<BarPtr>::iterator ppExtendedBegin;
		if( !useRth )
		{
			var start = rth.size() ? ConvertIBDate( rth.front()->time ) : std::numeric_limits<time_t>::max();
			shared_lock l1{_extendedMutex};
			var pExtendedIter = _extended.find(day);  if( pExtendedIter==_extended.end() ){ ERR("trying to add bars extended but does not contain {}"sv, DateDisplay(FromDays(day)) ); return pResult; }
			pExtended = &pExtendedIter->second;
			for( ppExtendedBegin = pExtended->begin(); ppExtendedBegin!=pExtended->end() && ConvertIBDate((*ppExtendedBegin)->time)<start; ++ppExtendedBegin )
				pResult->push_back( *ppExtendedBegin );
		}
		for( var& pBar : rth )
			pResult->push_back( pBar );
		_rthMutex.unlock();
		if( !useRth )
		{
			shared_lock l1{_extendedMutex};
			for( ; ppExtendedBegin!=pExtended->end(); ++ppExtendedBegin )
				pResult->push_back( *ppExtendedBegin );
		}
		if( barSize!=DataCache::Size )
		{
			ASSERT( barSize%DataCache::Size==0 );
			auto pNewResult = make_shared<vector<BarPtr>>();
			var start = useRth ? RthBegin(contract, day) : ExtendedBegin( contract, day );
			var end = Chrono::Min( Clock::now(), useRth ? RthEnd(contract, day) : ExtendedEnd(contract, day) );
			auto ppBar = pResult->begin();
			var duration = BarSize::BarDuration( barSize );
			for( auto barEnd = start+duration; barEnd<=end; barEnd+=duration )
			{
				ibapi::Bar combined{ ToIBDate(barEnd), 0, std::numeric_limits<double>::max(), 0, 0, 0, 0, 0 };
				double sum = 0;
				for( ;ppBar!=pResult->end() && Clock::from_time_t(ConvertIBDate((*ppBar)->time))<=barEnd; ++ppBar )
				{
					auto bar = **ppBar;
					combined.high = std::max( combined.high, bar.high );
					combined.low = std::min( combined.low, bar.low );
					if( combined.open==0.0 )
						combined.open = bar.open;
					combined.close = bar.close;
					sum = bar.wap*bar.volume;
					combined.volume += bar.volume;
					combined.count += bar.count;
				}
				if( combined.low!=std::numeric_limits<double>::max() )
				{
					combined.wap = sum/combined.volume;
					pNewResult->push_back( make_shared<ibapi::Bar>(combined) );
				}
			}
			pResult = pNewResult;
		}
		return pResult;
	}

	void DataCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<ibapi::Bar>& bars )noexcept
	{
		map<DayIndex,vector<BarPtr>> rthBars;
		map<DayIndex,vector<BarPtr>> extendedBars;
		for( var& bar : bars )
		{
			var time = ConvertIBDate( bar.time );
			auto& saveBars = rthBars;
			if( !useRth )
			{
				if( !IsRth(contract, Clock::from_time_t(time)) )
					saveBars = extendedBars;
			}
			saveBars.emplace( Chrono::ToDay(time), vector<BarPtr>{} ).first->second.push_back( make_shared<ibapi::Bar>(bar) );
		}
		auto pValues = Cache::TryGet<DataCache>( DataCache::CacheId(contract.Id, display) );
		if( rthBars.size() )
		{
			pValues->Push( rthBars, false );
			if( display==EDisplay::Trades )
				BarData::Save( contract, rthBars );
		}
		if( extendedBars.size() )
			pValues->Push( extendedBars, true );
	}
	void DataCache::Push( map<DayIndex,vector<BarPtr>> dayBars, bool extended )noexcept
	{
		auto cache = extended ? _extended : _rth;
		unique_lock l{ extended ? _extendedMutex : _rthMutex };
		for( var& [day,bars] : dayBars )
		{
			auto result = cache.emplace( day, bars );
			if( result.second )
				continue;
			map<time_t,BarPtr> combined;
			for( var& pBar : bars )
				combined.emplace( ConvertIBDate(pBar->time), pBar );
			for( var& pBar : result.first->second )
				combined.emplace( ConvertIBDate(pBar->time), pBar );
			vector<BarPtr> newValues;
			for( var& timeBar : combined )
				newValues.push_back( timeBar.second );
			result.first->second = newValues;
		}
	}
}
