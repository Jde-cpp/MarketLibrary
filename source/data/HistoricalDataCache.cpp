#include "./HistoricalDataCache.h"
#include <bar.h>
#include "../client/TwsClientSync.h"
#include "../types/Bar.h"
#include <jde/markets/types/Contract.h>
#include "../data/BarData.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/math/MathUtilities.h"
#pragma warning( disable : 4244 )
#include "../types/proto/bar.pb.h"
#pragma warning( default : 4244 )


#define var const auto
#define _client TwsClientSync::Instance()
namespace Jde::Markets
{
	using namespace Chrono;
	using EBarSize=Proto::Requests::BarSize;
	using EDisplay=Proto::Requests::Display;
	typedef sp<::Bar> BarPtr;
	struct DataCache
	{
		virtual optional<tuple<DayIndex,DayIndex>> Contains( const flat_set<DayIndex>& days, bool useRth, EBarSize barSize )noexcept=0;
		virtual VectorPtr<BarPtr> Get( const Contract& contract, DayIndex day, bool useRth, EBarSize barSize )noexcept=0;

		virtual EBarSize Size()const noexcept=0;
		virtual sv CacheIdPrefix()const noexcept=0;
		string CacheId( ContractPK contractId, EDisplay display )const noexcept{ return format("{}.{}.{}", CacheIdPrefix(), contractId, (uint)display); }
		static string CacheId( sv prefix, ContractPK contractId, EDisplay display )noexcept{ return format("{}.{}.{}", prefix, contractId, (uint)display); }
	};
	struct SubDataCache : DataCache
	{
		VectorPtr<BarPtr> Get( const Contract& contract, DayIndex day, bool useRth, EBarSize barSize )noexcept override;
		optional<tuple<DayIndex,DayIndex>> Contains( const flat_set<DayIndex>& days, bool useRth, EBarSize barSize )noexcept override;
		void Push( const flat_map<DayIndex,vector<BarPtr>>& dayBars, bool extended )noexcept;
	private:
		flat_map<DayIndex,vector<BarPtr>> _rth;	shared_mutex _rthMutex;
		flat_map<DayIndex,vector<BarPtr>> _extended; shared_mutex _extendedMutex;
	};
	struct OptionCache final : SubDataCache
	{
		static void Push( const Contract& contract, EDisplay display, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept;
		static string CacheId( ContractPK contractId, EDisplay display )noexcept{ return DataCache::CacheId( Prefix, contractId, display); }
	protected:
		sv CacheIdPrefix()const noexcept override{ return Prefix; }
		constexpr static sv Prefix = "HistoricalDataCacheOption";
		EBarSize Size()const noexcept override{ return EBarSize::Hour; }
	};

	struct MinuteCache final: SubDataCache
	{
		static void Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept;

		static string CacheId( ContractPK contractId, EDisplay display )noexcept{ return DataCache::CacheId( Prefix, contractId, display); }
		sv CacheIdPrefix()const noexcept override{ return Prefix; }
		constexpr static sv Prefix = "HistoricalDataCache";

		EBarSize Size()const noexcept override{ return EBarSize::Minute; }
	};

	struct DayCache final : DataCache
	{
		static void Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept;
		optional<tuple<DayIndex,DayIndex>> Contains( const flat_set<DayIndex>& days, bool useRth, EBarSize barSize )noexcept override;
		VectorPtr<BarPtr> Get( const Contract& contract, DayIndex day, bool useRth, EBarSize barSize )noexcept override;

		static string CacheId( ContractPK contractId, EDisplay display )noexcept{ return DataCache::CacheId( Prefix, contractId, display); }
		sv CacheIdPrefix()const noexcept override{ return Prefix; }
		constexpr static sv Prefix = "HistoricalDataCacheDay";

		EBarSize Size()const noexcept override{ return EBarSize::Day; }
		void Push( const flat_map<DayIndex,::Bar>& dayBars, bool rth )noexcept;
	private:
		map<DayIndex,::Bar> _rth;	shared_mutex _rthMutex;
		map<DayIndex,::Bar> _extended; shared_mutex _extendedMutex;
	};

	void HistoricalDataCache::Push( const Contract& contract, EDisplay display, EBarSize barSize, bool useRth, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept
	{
		if( barSize==EBarSize::Minute )
			MinuteCache::Push( contract, display, useRth, bars );
		else if( barSize==EBarSize::Day )
			DayCache::Push( contract, display, useRth, bars );
		else if( barSize==EBarSize::Hour && contract.SecType==SecurityType::Option )
			OptionCache::Push( contract, display, bars, end, subDayCount );
		else
			TRACE( "Pushing barsize '{}' not supported."sv, BarSize::TryToString(barSize) );
	}

	flat_map<DayIndex,VectorPtr<BarPtr>> HistoricalDataCache::ReqHistoricalData2( const Contract& contract, DayIndex end, uint dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept
	{
		flat_map<DayIndex,VectorPtr<BarPtr>> bars;
		if( !dayCount ){ ERR("0 daycount sent in."sv); return bars; }
		flat_set<DayIndex> days;
		for( DayIndex i = end; days.size()<dayCount; --i )
		{
			if( !IsHoliday(i, contract.Exchange) )//change to Exchanges...080
				days.emplace_hint( days.begin(), i );
		}
		var cacheId = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? OptionCache::CacheId( contract.Id, display )
			: barSize==EBarSize::Day ? DayCache::CacheId( contract.Id, display ) : MinuteCache::CacheId( contract.Id, display );
		sp<DataCache> pCache = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? dynamic_pointer_cast<DataCache>( Cache::Emplace<OptionCache>(cacheId) )
			: barSize==EBarSize::Day ? dynamic_pointer_cast<DataCache>(Cache::Emplace<DayCache>(cacheId) ) : dynamic_pointer_cast<DataCache>(Cache::Emplace<MinuteCache>(cacheId) );
		auto startEnd = pCache->Contains( days, useRth, barSize );
		if( startEnd.has_value() )
		{
			var [start,end] = startEnd.value();
			for( var day : days )
				bars.emplace( day, day>=start && day<=end ? pCache->Get(contract, day, useRth, barSize) : VectorPtr<BarPtr>{} );
		}
		else
		{
			for( var day : days )
				bars.emplace( day, VectorPtr<BarPtr>{} );
		}
		return bars;
	}
	MapPtr<DayIndex,VectorPtr<BarPtr>> HistoricalDataCache::ReqHistoricalData( const Contract& contract, DayIndex endDay, uint dayCount, EBarSize barSize, EDisplay display, bool useRth )noexcept
	{
		MapPtr<DayIndex,VectorPtr<BarPtr>> pBars;
		if( !dayCount ){ ERR("0 daycount sent in."sv); return pBars; }
		flat_set<DayIndex> days;
		for( DayIndex i = endDay; days.size()<dayCount; --i )
		{
			if( !IsHoliday(i, contract.Exchange) )//change to Exchanges...080
				days.emplace( i );
		}
		var cacheId = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? OptionCache::CacheId( contract.Id, display )
			: barSize==EBarSize::Day ? DayCache::CacheId( contract.Id, display ) : MinuteCache::CacheId( contract.Id, display );
		sp<DataCache> pCache = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? dynamic_pointer_cast<DataCache>( Cache::Emplace<OptionCache>(cacheId) )
			: barSize==EBarSize::Day ? dynamic_pointer_cast<DataCache>(Cache::Emplace<DayCache>(cacheId) ) : dynamic_pointer_cast<DataCache>(Cache::Emplace<MinuteCache>(cacheId) );
		uint missingCount;
		auto load = [&]()
		{
			missingCount = days.size();
			auto startEnd = pCache->Contains( days, useRth, barSize );
			if( startEnd.has_value() )
			{
				pBars = make_shared<map<DayIndex,VectorPtr<BarPtr>>>();
				var [start,end] = startEnd.value();
				for( var day : days )
				{
					auto p = day>=start && day<=end ? pCache->Get( contract, day, useRth, barSize ) : VectorPtr<BarPtr>{};
					pBars->emplace( day, p );
					if( p )
						--missingCount;
				}
			}
			return startEnd;
		};
		var startEnd = load();
		if( useRth && display==EDisplay::Trades && missingCount && barSize%EBarSize::Minute==0 && barSize!=EBarSize::Day /*&& BarData::HavePath()*/ && contract.SecType==SecurityType::Stock )//look in files
		{
			auto add = [pBars, &days, pMinutes=dynamic_pointer_cast<SubDataCache>(pCache), contract]( const map<DayIndex,VectorPtr<CandleStick>>& additional )mutable
			{//add to cache so can subsequently load pBars from cache.
				if( !pBars )
				{
					pBars = make_shared<map<DayIndex,VectorPtr<BarPtr>>>();
					std::for_each( days.begin(), days.end(), [pBars](var day){pBars->emplace(day, VectorPtr<BarPtr>{});} );
				}
				flat_map<DayIndex,vector<BarPtr>> bars;
				for( auto [day,pSticks] : additional )
				{
					if( pBars->find(day)->second )
						ERR("Day {} has value, but loaded again."sv, day );
					auto& dayBars = bars.emplace( day, vector<BarPtr>{} ).first->second;
					auto baseTime = RthBegin( contract, day );
					for( var& stick : *pSticks )
					{
						dayBars.push_back( make_shared<::Bar>(stick.ToIB(baseTime)) );
						baseTime+=1min;
					}
				}
				pMinutes->Push( bars, true );
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
	sp<HistoricalDataCache::StatCount> HistoricalDataCache::ReqStats( const Contract& contract, double days, DayIndex start, DayIndex end )noexcept(false)
	{
		double fullDaysDouble;
		var minutesInDay = DayLengthMinutes( contract.Exchange );
		double partialDays = std::modf( days, &fullDaysDouble );
		auto fullDays = Math::URound<DayIndex>( fullDaysDouble );
		auto minutes = Math::URound<uint16>( partialDays*minutesInDay );
		if( minutes==minutesInDay )
		{
			minutes=0;
			++fullDays;
		}
#pragma region Tests
		THROW_IF( fullDays==0 && minutes<1, "days '{}' should be at least 1 minute.", days );
		THROW_IF( days>3*365, "days {} should be less than 3 years {}", days, 3*365 );
		THROW_IF( end>DaysSinceEpoch(Clock::now()), "end should be < now" );
		const DayIndex dayCount = DayCount( TradingDay{start, contract.Exchange}, end );
		THROW_IF( dayCount<days, "days should be less than end-start." );
		THROW_IF( end<start, "end '{}' should be greater than start '{}'.", end, start );
#pragma endregion
		var cacheId = minutes==0 ? "" : format( "ReqStdDev.{}.{}.{}.{}", contract.Symbol, fullDays, start, end );
		if( var pValue = Cache::Get<HistoricalDataCache::StatCount>(cacheId); !cacheId.empty() && pValue )
			return pValue;

		var pAllBars = _client.ReqHistoricalDataSync( contract, end, dayCount, minutes==0 ? EBarSize::Day : EBarSize::Minute, Proto::Requests::Display::Trades, true, true ).get();
		THROW_IF( pAllBars->size()==0, "No history" );
		map<DayIndex,vector<::Bar>> bars;
		for_each( pAllBars->begin(), pAllBars->end(), [&bars](var bar)
		{
			bars.try_emplace( ToDay(ConvertIBDate(bar.time)) ).first->second.push_back(bar);
		} );

		vector<double> returns;
		auto pEnd = bars.rbegin();
		typedef decltype(pEnd) RIterator;
		const DayIndex diff = fullDays<6 ? TradingDay{pEnd->first, contract.Exchange}-(fullDays==0 ? 0 : fullDays-1) : pEnd->first-( fullDays-1 );
		auto pForward = bars.lower_bound( diff );
		RIterator pStart{ pForward==bars.end() ? pForward : std::next(pForward) };
		for( ; pStart!=bars.rend() && pEnd!=bars.rend(); ++pStart, ++pEnd )
		{
			var index = minutes==0 ? pStart->second.size()-1 : minutes<=pStart->second.size() ? pStart->second.size()-minutes : std::numeric_limits<uint>::max();
			var pStartBar = pStart->second.size() && index!=std::numeric_limits<uint>::max() ? &pStart->second[index] : nullptr;
			var pEndBar = pEnd->second.size() ? &pEnd->second.back() : nullptr;
			if( pStartBar && pEndBar )
				returns.push_back( 1+(pEndBar->close-pStartBar->open)/pStartBar->open );
		}
		auto pValue = make_shared<HistoricalDataCache::StatCount>( Math::Statistics(returns), returns.size() );
		if( !cacheId.empty() )
			Cache::Set<HistoricalDataCache::StatCount>( cacheId, pValue );
		return pValue;
	}

	optional<tuple<DayIndex,DayIndex>> SubDataCache::Contains( const flat_set<DayIndex>& days, bool useRth, EBarSize barSize )noexcept
	{
		optional<tuple<DayIndex,DayIndex>> result;
		auto contains = Size()>EBarSize::None && ( Size()==EBarSize::Day || Size()<=EBarSize::Hour ) && barSize%Size()==0;
		if( contains )
		{
			shared_lock l1{ _rthMutex }; //MyLock l1{ _rthMutex, "_rthMutex", "Push", __LINE__ };
			var start = *days.begin();
			var end = *days.rbegin();
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
	optional<tuple<DayIndex,DayIndex>> DayCache::Contains( const flat_set<DayIndex>& days, bool useRth, EBarSize )noexcept
	{
		auto& cache = useRth ? _rth : _extended;
		auto& mutex = useRth ? _rthMutex : _extendedMutex;


		shared_lock l1{ mutex };
		var start = *days.begin();
		var end = *days.rbegin();
		var pBegin = cache.lower_bound( start ), pEnd = cache.lower_bound( end );
		var contains = pBegin!=cache.end() || pEnd!=cache.end();

		return contains ? make_tuple( pBegin->first, pEnd==cache.end() ? cache.rbegin()->first : pEnd->first ) : optional<tuple<DayIndex,DayIndex>>{};
	}
	VectorPtr<BarPtr> SubDataCache::Get( const Contract& contract, DayIndex day, bool useRth, EBarSize barSize )noexcept
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
			var pExtendedIter = _extended.find(day);
			if( pExtendedIter==_extended.end() )
			{
				ERR("trying to add bars extended but does not contain {}"sv, DateDisplay(FromDays(day)) ); return pResult;
			}
			pExtended = &pExtendedIter->second;
			for( ppExtendedBegin = pExtended->begin(); ppExtendedBegin!=pExtended->end() && ConvertIBDate((*ppExtendedBegin)->time)<start; ++ppExtendedBegin )
				pResult->push_back( *ppExtendedBegin );
		}
		for( var& pBar : rth )
			pResult->push_back( pBar );
		l1.unlock();
		if( !useRth )
		{
			shared_lock l1{_extendedMutex};
			for( ; ppExtendedBegin!=pExtended->end(); ++ppExtendedBegin )
				pResult->push_back( *ppExtendedBegin );
		}
		if( barSize!=Size() )
		{
			ASSERT( barSize%Size()==0 );
			auto pNewResult = make_shared<vector<BarPtr>>();
			var start = useRth ? RthBegin(contract, day) : ExtendedBegin( contract, day );
			var end = Min( Clock::now(), useRth ? RthEnd(contract, day) : ExtendedEnd(contract, day) );
			auto ppBar = pResult->begin();
			var duration = barSize==EBarSize::Day ? end-start : BarSize::BarDuration( barSize );  ASSERT( barSize<=EBarSize::Day );
			var minuteCount = std::chrono::duration_cast<std::chrono::minutes>( duration ).count();
			for( auto barStart = start, barEnd=Clock::from_time_t( (Clock::to_time_t(start)/60+minuteCount)/minuteCount*minuteCount*60 ); barEnd<=end; barStart=barEnd, barEnd+=duration )//1st barEnd for hour is 10am not 10:30am
			{
				::Bar combined{ ToIBDate(barStart), 0, std::numeric_limits<double>::max(), 0, 0, 0, 0, 0 };
				double sum = 0;
				for( ;ppBar!=pResult->end() && Clock::from_time_t(ConvertIBDate((*ppBar)->time))<barEnd; ++ppBar )
				{
					var& bar = **ppBar;
					combined.high = std::max( combined.high, bar.high );
					combined.low = std::min( combined.low, bar.low );
					if( combined.open==0.0 )
						combined.open = bar.open;
					combined.close = bar.close;
					sum = bar.wap*bar.volume;
					DBG( "bar time={}, volume={}"sv, bar.time, bar.volume );
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
	void MinuteCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept
	{
		flat_map<DayIndex,vector<BarPtr>> rthBars;
		flat_map<DayIndex,vector<BarPtr>> extendedBars;
		for( var& bar : bars )
		{
			var time = ConvertIBDate( bar.time );
			auto& saveBars = useRth || IsRth(contract, Clock::from_time_t(time)) ? rthBars : extendedBars;
			saveBars.try_emplace( Chrono::ToDay(time) ).first->second.push_back( make_shared<::Bar>(bar) );
		}
		auto pValues = dynamic_pointer_cast<SubDataCache>( Cache::Emplace<MinuteCache>(CacheId(contract.Id, display)) );
		if( rthBars.size() )
		{
			pValues->Push( rthBars, true );
			if( display==EDisplay::Trades )
				BarData::Save( contract, rthBars );
		}
		if( extendedBars.size() )
			pValues->Push( extendedBars, false );
	}
	void DayCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept
	{
		flat_map<DayIndex,::Bar> cacheBars;
		for( var& bar : bars )
			cacheBars.emplace( ToDay(ConvertIBDate(bar.time)), bar );

		auto pValues = Cache::Emplace<DayCache>( CacheId(contract.Id, display) );
		if( cacheBars.size() )
			pValues->Push( cacheBars, useRth );
	}
	void OptionCache::Push( const Contract& contract, EDisplay display, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept
	{
		flat_map<DayIndex,vector<BarPtr>> rthBars;
		for( var& bar : bars )
		{
			var day = Chrono::ToDay( ConvertIBDate(bar.time) );
			auto pDay = rthBars.find( day );
			if( pDay==rthBars.end() )
				pDay = rthBars.emplace( day, vector<BarPtr>{} ).first;
			pDay->second.push_back( make_shared<::Bar>(bar) );
		}
		for( DayIndex i=0, iDay=end ; i<subDayCount; ++i, iDay=PreviousTradingDay(iDay) )
			rthBars.emplace( iDay, vector<BarPtr>{} );

		auto pValues = static_pointer_cast<SubDataCache>( Cache::Emplace<OptionCache>(CacheId(contract.Id, display)) );
		pValues->Push( rthBars, true );
	}

	void SubDataCache::Push( const flat_map<DayIndex,vector<BarPtr>>& dayBars, bool rth )noexcept
	{
		auto& cache = rth ? _rth : _extended ;
		auto& mutex = rth ? _rthMutex : _extendedMutex;
		unique_lock l{ mutex };
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

	void DayCache::Push( const flat_map<DayIndex,::Bar>& dayBars, bool rth )noexcept
	{
		auto& cache = rth ? _rth : _extended;
		auto& mutex = rth ? _rthMutex : _extendedMutex;
		unique_lock l{ mutex };
		for( var& [day,bar] : dayBars )
		{
			auto result = cache.emplace( day, bar );
			if( !result.second )
				result.first->second = bar;
		}
	}
	VectorPtr<BarPtr> DayCache::Get( const Contract& contract, DayIndex day, bool rth, EBarSize barSize )noexcept
	{
		auto& cache = rth ? _rth : _extended;
		auto& mutex = rth ? _rthMutex : _extendedMutex;
		shared_lock l1{ mutex };
		auto pResult = make_shared<vector<BarPtr>>();
		var p = cache.find( day );
		if( p==cache.end() )
			ERR("trying to add bars but does not contain {}"sv, DateDisplay(FromDays(day)) );
		else
			pResult->push_back( make_shared<::Bar>(p->second) );

		return pResult;
	}
}
