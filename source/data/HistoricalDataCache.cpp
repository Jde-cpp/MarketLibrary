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
	typedef sp<::Bar> BarPtr;
	struct DataCache
	{
		optional<tuple<DayIndex,DayIndex>> Contains( EBarSize barSize, bool useRth, const set<DayIndex>& days )noexcept;
		VectorPtr<BarPtr> Get( const Contract& contract, DayIndex day, EBarSize barSize, bool useRth )noexcept;
		static void Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept;
		static void PushDay( const Contract& contract, EDisplay display, const vector<::Bar>& bars )noexcept;
		static void PushOption( const Contract& contract, EDisplay display, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept;
		void Push( const map<DayIndex,vector<BarPtr>>& dayBars, bool extended )noexcept;
		void PushDay( const flat_map<DayIndex,::Bar>& dayBars )noexcept;

		static constexpr EBarSize Size{EBarSize::Minute};
		static string CacheId( ContractPK contractId, EDisplay display )noexcept{ return format("HistoricalDataCache.{}.{}", contractId, display); }
		static string OptionCacheId( ContractPK contractId, EDisplay display )noexcept{ return format("HistoricalDataCacheOption.{}.{}", contractId, display); }
		static string DayCacheId( ContractPK contractId, EDisplay display )noexcept{ return format("HistoricalDataCacheDay.{}.{}", contractId, display); }
	private:
		map<DayIndex,vector<BarPtr>> _rth;	shared_mutex _rthMutex;
		map<DayIndex,vector<BarPtr>> _extended; shared_mutex _extendedMutex;
	};
	void Push( const Contract& contract, EDisplay display, EBarSize barSize, bool useRth, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept
	{
		if( barSize==EBarSize::Minute )
			DataCache::Push( contract, display, useRth, bars );
		else if( barSize==EBarSize::Day && useRth )
			DataCache::PushDay( contract, display, bars );
		else if( barSize==EBarSize::Hour && contract.SecType==SecurityType::Option )
			DataCache::PushOption( contract, display, bars, end, subDayCount );
		else
			TRACE( "Pushing barsize '{}' not supported."sv, BarSize::TryToString(barSize) );
	}
	//files
	MapPtr<DayIndex,VectorPtr<sp<::Bar>>> ReqHistoricalData( const Contract& contract, DayIndex endDay, uint dayCount, EBarSize barSize, EDisplay display, bool useRth )noexcept
	{
		MapPtr<DayIndex,VectorPtr<BarPtr>> pBars;
		if( !dayCount ){ ERR0("0 daycount sent in."sv); return pBars; }
		set<DayIndex> days;
		for( DayIndex i = endDay; days.size()<dayCount; --i )
		{
			if( !IsHoliday(i, contract.Exchange) )//change to Exchanges...080
				days.emplace( i );
		}
		var cacheId = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? DataCache::OptionCacheId(contract.Id, display)
			: barSize==EBarSize::Day ? DataCache::DayCacheId(contract.Id, display) : DataCache::CacheId(contract.Id, display);
		auto pValues = Cache::TryGet<DataCache>( cacheId );
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
		if( useRth && display==EDisplay::Trades && missingCount && barSize%EBarSize::Minute==0 && BarData::HavePath() && contract.SecType==SecurityType::Stock )//look in files
		{
			auto add = [pBars, &days, pValues, contract]( const map<DayIndex,VectorPtr<CandleStick>>& additional )mutable
			{
				if( !pBars )
				{
					pBars = make_shared<map<DayIndex,VectorPtr<BarPtr>>>();
					for_each( days.begin(), days.end(), [pBars](var day){pBars->emplace(day, VectorPtr<BarPtr>{});} );
				}
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
						dayBars.push_back( make_shared<::Bar>(stick.ToIB(baseTime)) );
					}
				}
				pValues->Push( bars, false );
			};
			if( !startEnd.has_value() )
				add( *BarData::Load(contract, *days.begin(), *days.rbegin()) );
			else
			{
				var [start,end] = startEnd.value();
				var pEnd = days.find( start );
				if( pEnd!=days.begin() )
					add( *BarData::Load( contract, *days.begin(), *std::prev(pEnd)) );
				var pStart = days.find( end );
				if( pStart!=days.end() && std::next(pStart)!=days.end() )
					add( *BarData::Load(contract, *std::next(pStart), *days.rbegin()) );
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
			shared_lock l1{ _rthMutex }; //MyLock l1{ _rthMutex, "_rthMutex", "Push", __LINE__ };
			var start = *days.begin();
			var end = *days.rbegin();
			DBG( "_rth.size={}, key={}"sv, _rth.size(), _rth.size() ? _rth.begin()->first : 0 );
			auto pBegin = _rth.lower_bound( start ), pEnd = _rth.lower_bound( end );
			contains = pBegin!=_rth.end() || pEnd!=_rth.end();
			if( contains && !useRth )
			{
				shared_lock l1{_extendedMutex};
				for( auto p=pBegin; contains && p!=pEnd; ++p )
					contains = _extended.find(p->first)!=_extended.end();
			}
			if( contains )
				result = make_tuple( pBegin->first, pEnd==_rth.end() ? _rth.rbegin()->first : pEnd->first );//
		}
		return result;
	}
	VectorPtr<BarPtr> DataCache::Get( const Contract& contract, DayIndex day, EBarSize barSize, bool useRth )noexcept
	{
		auto pResult = make_shared<vector<BarPtr>>();
		shared_lock l1{_rthMutex}; //MyLock l1{ _rthMutex, "_rthMutex", "Push", __LINE__ };//
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
		l1.unlock(); //_rthMutex.unlock();
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
			var end = Min( Clock::now(), useRth ? RthEnd(contract, day) : ExtendedEnd(contract, day) );
			auto ppBar = pResult->begin();
			var duration = barSize==EBarSize::Day ? end-start : BarSize::BarDuration( barSize );  ASSERT( barSize<=EBarSize::Day );
			DBG( "start={}, end={}, start+duration={}"sv, ToIsoString(start), ToIsoString(end), ToIsoString(start+duration) );
			for( auto barEnd = start+duration; barEnd<=end; barEnd+=duration )
			{
				::Bar combined{ ToIBDate(barEnd), 0, std::numeric_limits<double>::max(), 0, 0, 0, 0, 0 };
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
					pNewResult->push_back( make_shared<::Bar>(combined) );
				}
			}
			pResult = pNewResult;
		}
		return pResult;
	}

	void DataCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept
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
			saveBars.emplace( Chrono::ToDay(time), vector<BarPtr>{} ).first->second.push_back( make_shared<::Bar>(bar) );
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
	void DataCache::PushDay( const Contract& contract, EDisplay display, const vector<::Bar>& bars )noexcept
	{
		flat_map<DayIndex,::Bar> rthBars;
		for( var& bar : bars )
			rthBars.emplace( ToDay(ConvertIBDate(bar.time)), bar );

		auto pValues = Cache::TryGet<DataCache>( DataCache::DayCacheId(contract.Id, display) );
		if( rthBars.size() )
			pValues->PushDay( rthBars );
	}
	void DataCache::PushOption( const Contract& contract, EDisplay display, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept
	{
		map<DayIndex,vector<BarPtr>> rthBars;
		for( var& bar : bars )
		{
			var day = Chrono::ToDay( ConvertIBDate(bar.time) );
			auto pDay = rthBars.find( day );
			if( pDay==rthBars.end() )
				pDay = rthBars.emplace( day, vector<BarPtr>{} ).first;
			pDay->second.push_back( make_shared<::Bar>(bar) );
		}
		for( uint i=0, iDay=end ; i<subDayCount; ++i, iDay=PreviousTradingDay(iDay) )
			rthBars.emplace( iDay, vector<BarPtr>{} );

		auto pValues = Cache::TryGet<DataCache>( DataCache::OptionCacheId(contract.Id, display) );
		//if( rthBars.size() )
			pValues->Push( rthBars, false );
	}
	void DataCache::Push( const map<DayIndex,vector<BarPtr>>& dayBars, bool extended )noexcept
	{
		auto& cache = extended ? _extended : _rth;
		auto& mutex = extended ? _extendedMutex : _rthMutex;
		unique_lock l{ mutex }; //MyLock l{mutex, extended ? "_extendedMutex" : "_rthMutex", "Push", __LINE__};
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

	void DataCache::PushDay( const flat_map<DayIndex,::Bar>& dayBars )noexcept
	{
		unique_lock l{ _rthMutex };
		for( var& [day,bar] : dayBars )
		{
			auto result = _rth.emplace( day, vector<BarPtr>{make_shared<::Bar>(bar)} );
			if( !result.second )
				result.first->second = vector<BarPtr>{ make_shared<::Bar>(bar) };
		}
	}
}
