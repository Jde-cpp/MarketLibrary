#include "TwsClientCache.h"
//#include <ranges>
#include "../data/HistoricalDataCache.h"
#include "../wrapper/WrapperCache.h"
#include "../types/Contract.h"
#include "../types/proto/results.pb.h"

#include "../../../Framework/source/Cache.h"

#define var const auto
namespace Jde::Markets
{
	TwsClientCache::TwsClientCache( const TwsConnectionSettings& settings, shared_ptr<WrapperCache> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}

	shared_ptr<WrapperCache> TwsClientCache::Wrapper()noexcept
	{
		return std::dynamic_pointer_cast<WrapperCache>(_pWrapper);
	}
	ibapi::Contract TwsClientCache::ToContract( string_view symbol, DayIndex dayIndex, SecurityRight right )noexcept
	{
		ibapi::Contract contract; contract.symbol = symbol; contract.exchange = "SMART"; contract.secType = "OPT";/*only works with symbol*/
		if( dayIndex>0 )
		{
			const DateTime date{ Chrono::FromDays(dayIndex) };
			contract.lastTradeDateOrContractMonth = fmt::format( "{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day() );
		}
		contract.right = ToString( right );

		return contract;
	}
	void TwsClientCache::ReqContractDetails( TickerId reqId, const ibapi::Contract& contract )noexcept
	{
		var suffix = contract.conId ? std::to_string( contract.conId ) : contract.symbol;
		var cacheId = fmt::format( "reqContractDetails.{}.{}.{}.{}.{}", contract.secType, contract.right, contract.lastTradeDateOrContractMonth, contract.strike, suffix );
		var pContracts = Cache::Get<vector<ibapi::ContractDetails>>( cacheId );
		if( pContracts )
		{
			for( var& contract : *pContracts )
				Wrapper()->contractDetails( reqId, contract );
			Wrapper()->contractDetailsEnd( reqId );
		}
		else
		{
			Wrapper()->AddCacheId( reqId, cacheId );
			TwsClient::reqContractDetails( reqId, contract );
		}
	}
	void TwsClientCache::ReqSecDefOptParams( TickerId reqId, ContractPK underlyingConId, string_view symbol )noexcept
	{
		auto cacheId = fmt::format( "OptParams.{}", underlyingConId );
		var pData = Cache::Get<vector<Proto::Results::OptionParams>>( cacheId );
		if( pData )
		{
			for( var& param : *pData )
			{
				std::set<std::string> expirations;
				for( auto i=0; i<param.expirations_size(); ++i )
				{
					const DateTime date{ Chrono::FromDays(param.expirations(i)) };
					expirations.emplace( fmt::format("{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day()) );
				}
				std::set<double> strikes;
				for( auto i=0; i<param.strikes_size(); ++i )
					strikes.emplace( param.strikes(i) );
				Wrapper()->securityDefinitionOptionalParameter( reqId, param.exchange(), param.underlying_contract_id(), param.trading_class(), param.multiplier(), expirations, strikes );
			}
			Wrapper()->securityDefinitionOptionalParameterEnd( reqId );
		}
		else
		{
			Wrapper()->AddCacheId( reqId, cacheId );
			TwsClient::reqSecDefOptParams( reqId, underlyingConId, symbol );
		}
	}
	void TwsClientCache::ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex endDay, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept
	{
		var current = CurrentTradingDay( contract.Exchange );
		auto addBars = [&]( DayIndex end, DayIndex subDayCount, map<DayIndex,VectorPtr<sp<ibapi::Bar>>>& existing, time_t lastTime=0 )
		{
			VectorPtr<ibapi::Bar> pBars;
			if( !lastTime )
				pBars = ReqHistoricalDataSync( contract, end, subDayCount, barSize, display, useRth, false ).get();
			else if( time(nullptr)-lastTime>barSize )
				pBars = ReqHistoricalDataSync( contract, lastTime, barSize, display, useRth ).get();
			if( pBars )
			{
				HistoricalDataCache::Push( contract, display, barSize, useRth, *pBars );
				for( var& bar : *pBars )
				{
					var day = Chrono::DaysSinceEpoch( Clock::from_time_t(ConvertIBDate(bar.time)) );
					existing.emplace( day, VectorPtr<sp<ibapi::Bar>>{new vector<sp<ibapi::Bar>>{}} ).first->second->push_back( make_shared<ibapi::Bar>(bar) );
				}
			}
		};
		auto pData = HistoricalDataCache::ReqHistoricalData( contract, endDay, dayCount, barSize, (Proto::Requests::Display)display, useRth );// : MapPtr<DayIndex,VectorPtr<sp<const ibapi::Bar>>>{};
		if( pData )
		{
			ASSERT( pData->size() );
			if( !pData->begin()->second )
			{
				uint startDayCount = 0; DayIndex endDay = 0;
				for( var& [day,pBars] : *pData )
				{
					if( pBars )
						break;
					++startDayCount;
					endDay = day;
				}
				addBars( endDay, startDayCount, *pData );
			}
			var isOpen = IsOpen( contract );
			if( !pData->rbegin()->second || (current==pData->rbegin()->first && isOpen) )
			{
				DayIndex endDayCount = 0, currentDay = endDay;
				time_t lastTime = 0;
				for( auto p = pData->rbegin(); p!=pData->rend(); ++p )
				{
					if( p->second )
					{
						if( p->first==current && isOpen && p->second->size() )
							lastTime = ConvertIBDate( (*p->second->rbegin())->time );
						break;
					}
					++endDayCount;
					currentDay = p->first;
				}
				addBars( currentDay, endDayCount, *pData, lastTime );
			}
		}
		else
		{
			pData = make_shared<map<DayIndex,VectorPtr<sp<ibapi::Bar>>>>();
			addBars( endDay, dayCount, *pData );
		}

		var now = time( nullptr ); time_t minTime=now, maxTime=0;
		for( var& dayBars : *pData )
		{
			for( var& pBar : *dayBars.second )
			{
				var timet = ConvertIBDate( pBar->time );
				minTime = std::min( timet, minTime );
				maxTime = std::max( timet, maxTime );
				Wrapper()->historicalData( reqId, *pBar );
			}
		}
		Wrapper()->historicalDataEnd( reqId, minTime==now ? "" : ToIBDate(DateTime{minTime}), maxTime==0 ? "" : ToIBDate(DateTime{maxTime}) );
	}
}