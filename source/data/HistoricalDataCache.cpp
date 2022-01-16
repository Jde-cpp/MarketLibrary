#include "./HistoricalDataCache.h"
#include <bar.h>
#include "../client/TwsClientSync.h"
#include "../types/Bar.h"
#include <jde/markets/types/Contract.h>
#include "../data/BarData.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/math/MathUtilities.h"
#include "../types/Exchanges.h"
#pragma warning( disable : 4244 )
#include "../types/proto/bar.pb.h"
#pragma warning( default : 4244 )

#define var const auto
namespace Jde::Markets
{
	using namespace Chrono;
	using BarPtr=sp<::Bar>;
	static const LogTag& _logLevel{ Logging::TagLevel("mrk-hist") };

	struct IDataCache
	{
		β Contains( const flat_set<Day>& days, bool useRth, EBarSize barSize )noexcept->optional<tuple<Day,Day>> =0;
		β Get( const Contract& contract, Day day, bool useRth, EBarSize barSize )noexcept->VectorPtr<BarPtr> =0;

		β Size()const noexcept->EBarSize=0;
		β CacheIdPrefix()const noexcept->sv=0;
		α CacheId( ContractPK contractId, EDisplay display )const noexcept->string{ return format("{}.{}.{}", CacheIdPrefix(), contractId, (uint)display); }
		Ω CacheId( sv prefix, ContractPK contractId, EDisplay display )noexcept->string{ return format("{}.{}.{}", prefix, contractId, (uint)display); }
	};

	struct ISubDayCache : IDataCache
	{
		virtual ~ISubDayCache()=0;
		α Get( const Contract& contract, Day day, bool useRth, EBarSize barSize )noexcept->VectorPtr<BarPtr> override;
		α Contains( const flat_set<Day>& days, bool useRth, EBarSize barSize )noexcept->optional<tuple<Day,Day>> override;
		α Push( const flat_map<Day,vector<BarPtr>>& dayBars, bool extended )noexcept->void;
	private:
		flat_map<Day,vector<BarPtr>> _rth;	shared_mutex _rthMutex;
		flat_map<Day,vector<BarPtr>> _extended; shared_mutex _extendedMutex;
	};
	ISubDayCache::~ISubDayCache(){}
	struct OptionCache final : ISubDayCache
	{
		Ω Push( const Contract& contract, EDisplay display, const vector<::Bar>& bars, Day end, Day subDayCount )noexcept->void;
		Ω CacheId( ContractPK contractId, EDisplay display )noexcept->string{ return IDataCache::CacheId( Prefix, contractId, display); }
	protected:
		α CacheIdPrefix()const noexcept->sv override{ return Prefix; }
		α Size()const noexcept->EBarSize override{ return EBarSize::Hour; }

		constexpr static sv Prefix = "HistoricalDataCacheOption";
	};

	struct MinuteCache final: ISubDayCache
	{
		Ω Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept->void;

		Ω CacheId( ContractPK contractId, EDisplay display )noexcept->string{ return IDataCache::CacheId( Prefix, contractId, display); }
		α CacheIdPrefix()const noexcept->sv override{ return Prefix; }
		α Size()const noexcept->EBarSize override{ return EBarSize::Minute; }

		constexpr static sv Prefix = "HistoricalDataCache";
	};

	struct DayCache final : IDataCache
	{
		Ω Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept->void;
		α Contains( const flat_set<Day>& days, bool useRth, EBarSize barSize )noexcept->optional<tuple<Day,Day>> override;
		α Get( const Contract& contract, Day day, bool useRth, EBarSize barSize )noexcept->VectorPtr<BarPtr> override;
		Ω CacheId( ContractPK contractId, EDisplay display )noexcept->string{ return IDataCache::CacheId( Prefix, contractId, display); }
		α CacheIdPrefix()const noexcept->sv override{ return Prefix; }
		α Size()const noexcept->EBarSize override{ return EBarSize::Day; }
		α Push( const flat_map<Day,::Bar>& dayBars, bool rth )noexcept->void;

		constexpr static sv Prefix = "HistoricalDataCacheDay";
	private:
		map<Day,::Bar> _rth;	shared_mutex _rthMutex;
		map<Day,::Bar> _extended; shared_mutex _extendedMutex;
	};

	α HistoryCache::Clear( ContractPK contractId, EDisplay display, EBarSize barSize )noexcept->void
	{
		ASSERT_DESC( barSize==EBarSize::Minute, "Not implemented" );
		Cache::Clear( MinuteCache::CacheId(contractId, display) );
	}
	α HistoryCache::Set( const Contract& contract, EDisplay display, EBarSize barSize, bool useRth, const vector<::Bar>& bars, Day end, Day subDayCount )noexcept->void
	{
		if( barSize==EBarSize::Minute )
			MinuteCache::Push( contract, display, useRth, bars );
		else if( barSize==EBarSize::Day )
			DayCache::Push( contract, display, useRth, bars );
		else if( barSize==EBarSize::Hour && contract.SecType==SecurityType::Option )
			OptionCache::Push( contract, display, bars, end, subDayCount );
		else
			TRACE( "Pushing barsize '{}' not supported."sv, BarSize::ToString(barSize) );
	}

	α HistoryCache::Get( const Contract& contract, Day end, Day tradingDays, EBarSize barSize, Proto::Requests::Display display, bool useRth )noexcept->flat_map<Day,VectorPtr<BarPtr>>
	{
		if( barSize==EBarSize::Month ) BREAK;
		flat_map<Day,VectorPtr<BarPtr>> bars;
		if( !tradingDays ){ ERR("0 tradingDays sent in."sv); return bars; }
		flat_set<Day> days;
		for( Day i = end; days.size()<tradingDays; --i )
		{
			if( !IsHoliday(i, contract.Exchange) )//change to Exchanges...080
				days.emplace_hint( days.begin(), i );
		}
		var cacheId = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? OptionCache::CacheId( contract.Id, display )
			: barSize==EBarSize::Day ? DayCache::CacheId( contract.Id, display ) : MinuteCache::CacheId( contract.Id, display );
		sp<IDataCache> pCache = contract.SecType==SecurityType::Option && barSize==EBarSize::Hour
			? dynamic_pointer_cast<IDataCache>( Jde::Cache::Emplace<OptionCache>(cacheId) )
			: barSize==EBarSize::Day ? dynamic_pointer_cast<IDataCache>(Jde::Cache::Emplace<DayCache>(cacheId) ) : dynamic_pointer_cast<IDataCache>(Cache::Emplace<MinuteCache>(cacheId) );
		auto startEnd = pCache->Contains( days, useRth, barSize );
		if( startEnd.has_value() )
		{
			var [start,end] = startEnd.value();
			LOG( "{} cache {}-{} {} {} rth={}", contract.Symbol, DateDisplay(start), DateDisplay(end), BarSize::ToString(barSize), TwsDisplay::ToString(display), useRth );
			for( var day : days )
				bars.emplace( day, day>=start && day<=end ? pCache->Get(contract, day, useRth, barSize) : VectorPtr<BarPtr>{} );
		}
		else
		{
			LOG( "({})No cache - {} days={} barSize='{}' {} rth={}", contract.Symbol, DateDisplay(end), days.size(), BarSize::ToString(barSize), TwsDisplay::ToString(display), useRth );
			for( var day : days )
				bars.emplace( day, VectorPtr<BarPtr>{} );
		}
		return bars;
	}

	α ISubDayCache::Contains( const flat_set<Day>& days, bool useRth, EBarSize barSize )noexcept->optional<tuple<Day,Day>>
	{
		optional<tuple<Day,Day>> result;
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
	α DayCache::Contains( const flat_set<Day>& days, bool useRth, EBarSize )noexcept->optional<tuple<Day,Day>>
	{
		auto& cache = useRth ? _rth : _extended;
		auto& mutex = useRth ? _rthMutex : _extendedMutex;

		shared_lock l1{ mutex };
		var start = *days.begin();
		var end = *days.rbegin();
		var pBegin = cache.lower_bound( start ), pEnd = cache.lower_bound( end );
		var contains = pBegin!=cache.end() || pEnd!=cache.end();

		return contains ? make_tuple( pBegin->first, pEnd==cache.end() ? cache.rbegin()->first : pEnd->first ) : optional<tuple<Day,Day>>{};
	}
	α ISubDayCache::Get( const Contract& contract, Day day, bool useRth, EBarSize barSize )noexcept->VectorPtr<BarPtr>
	{
		auto pResult = ms<vector<BarPtr>>();
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
		if( barSize<=EBarSize::Day && barSize!=Size() )
			pResult = BarData::Combine( contract, day, *pResult, barSize, Size(), useRth );

		return pResult;
	}
	α MinuteCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept->void
	{
		flat_map<Day,vector<BarPtr>> rthBars;
		flat_map<Day,vector<BarPtr>> extendedBars;
		for( var& bar : bars )
		{
			var time = ConvertIBDate( bar.time );
			auto& saveBars = useRth || IsRth(contract, Clock::from_time_t(time)) ? rthBars : extendedBars;
			saveBars.try_emplace( Chrono::ToDays(time) ).first->second.push_back( ms<::Bar>(bar) );
		}
		auto pValues = dynamic_pointer_cast<ISubDayCache>( Cache::Emplace<MinuteCache>(CacheId(contract.Id, display)) );
		if( rthBars.size() )
		{
			pValues->Push( rthBars, true );
			if( display==EDisplay::Trades )
				BarData::Save( contract, rthBars );
		}
		if( extendedBars.size() )
			pValues->Push( extendedBars, false );
	}
	α DayCache::Push( const Contract& contract, EDisplay display, bool useRth, const vector<::Bar>& bars )noexcept->void
	{
		flat_map<Day,::Bar> cacheBars;
		for( var& bar : bars )
			cacheBars.emplace( ToDays(ConvertIBDate(bar.time)), bar );

		auto pValues = Cache::Emplace<DayCache>( CacheId(contract.Id, display) );
		if( cacheBars.size() )
			pValues->Push( cacheBars, useRth );
	}
	α OptionCache::Push( const Contract& contract, EDisplay display, const vector<::Bar>& bars, Day end, Day subDayCount )noexcept->void
	{
		flat_map<Day,vector<BarPtr>> rthBars;
		for( var& bar : bars )
		{
			var day = Chrono::ToDays( ConvertIBDate(bar.time) );
			auto pDay = rthBars.find( day );
			if( pDay==rthBars.end() )
				pDay = rthBars.emplace( day, vector<BarPtr>{} ).first;
			pDay->second.push_back( ms<::Bar>(bar) );
		}
		for( Day i=0, iDay=end ; i<subDayCount; ++i, iDay=PreviousTradingDay(iDay) )
			rthBars.emplace( iDay, vector<BarPtr>{} );

		auto pValues = static_pointer_cast<ISubDayCache>( Cache::Emplace<OptionCache>(CacheId(contract.Id, display)) );
		pValues->Push( rthBars, true );
	}

	α ISubDayCache::Push( const flat_map<Day,vector<BarPtr>>& dayBars, bool rth )noexcept->void
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

	α DayCache::Push( const flat_map<Day,::Bar>& dayBars, bool rth )noexcept->void
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
	α DayCache::Get( const Contract& contract, Day day, bool rth, EBarSize barSize )noexcept->VectorPtr<BarPtr>
	{
		auto& cache = rth ? _rth : _extended;
		auto& mutex = rth ? _rthMutex : _extendedMutex;
		shared_lock l1{ mutex };
		auto pResult = ms<vector<BarPtr>>();
		var p = cache.find( day );
		if( p==cache.end() )
			ERR("({}) Trying to add '{}' bars but does not contain '{}'"sv, contract.Symbol, ToString(barSize), DateDisplay(FromDays(day)) );
		else
			pResult->push_back( ms<::Bar>(p->second) );

		return pResult;
	}
}