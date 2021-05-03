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
	using namespace Chrono;
	TwsClientCache::TwsClientCache( const TwsConnectionSettings& settings, shared_ptr<WrapperCache> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}

	shared_ptr<WrapperCache> TwsClientCache::Wrapper()noexcept
	{
		return std::dynamic_pointer_cast<WrapperCache>(_pWrapper);
	}
	::Contract TwsClientCache::ToContract( sv symbol, DayIndex dayIndex, SecurityRight right, double strike )noexcept
	{
		::Contract contract; contract.symbol = symbol; contract.exchange = "SMART"; contract.secType = "OPT";/*only works with symbol*/
		if( dayIndex>0 )
		{
			const DateTime date{ FromDays(dayIndex) };
			contract.lastTradeDateOrContractMonth = format( "{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day() );
		}
		if( strike )
			contract.strike = strike;
		contract.right = ToString( right );

		return contract;
	}
	void TwsClientCache::ReqContractDetails( TickerId reqId, const ::Contract& contract )noexcept
	{
		var suffix = contract.conId ? std::to_string( contract.conId ) : contract.symbol;
		var cacheId = format( "reqContractDetails.{}.{}.{}.{}.{}", contract.secType, contract.right, contract.lastTradeDateOrContractMonth, contract.strike, suffix );
		var pContracts = Cache::Get<vector<::ContractDetails>>( cacheId );
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
	void TwsClientCache::ReqSecDefOptParams( TickerId reqId, ContractPK underlyingConId, sv symbol )noexcept
	{
		auto cacheId = format( "OptParams.{}", underlyingConId );
		if( var pData = Cache::Get<Proto::Results::OptionExchanges>(cacheId); pData )
		{
			for( var& exchange : pData->exchanges() )
			{
				std::set<std::string> expirations;
				for( var& expiration : exchange.expirations() )
				{
					const DateTime date{ FromDays(expiration) };
					expirations.emplace( format("{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day()) );
				}
				std::set<double> strikes;
				for( var& strike : exchange.strikes() )
					strikes.emplace( strike );
				Wrapper()->securityDefinitionOptionalParameter( reqId, string{ToString(exchange.exchange())}, exchange.underlying_contract_id(), exchange.trading_class(), exchange.multiplier(), expirations, strikes );
			}
			Wrapper()->securityDefinitionOptionalParameterEnd( reqId );
		}
		else
		{
			Wrapper()->AddCacheId( reqId, cacheId );
			TwsClient::reqSecDefOptParams( reqId, underlyingConId, symbol );
		}
	}
	void TwsClientCache::RequestNewsProviders()noexcept
	{
		auto cacheId = format( "RequestProviders" );
		var pData = Cache::Get<Proto::Results::StringMap>( cacheId );
		if( pData )
		{
			vector<NewsProvider> newsProviders;
			for( var& param : pData->values() )
				newsProviders.push_back( NewsProvider{param.first, param.second} );
			Wrapper()->newsProviders( newsProviders );
		}
		else
			TwsClient::reqNewsProviders();
	}
	void TwsClientCache::ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex endDay, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept(false)
	{
		auto addBars = [&]( DayIndex end, DayIndex subDayCount, map<DayIndex,VectorPtr<sp<::Bar>>>& existing, time_t lastTime=0 )
		{
			VectorPtr<::Bar> pBars;
			if( !lastTime )
				pBars = ReqHistoricalDataSync( contract, end, subDayCount, barSize, display, useRth, false ).get();
			else if( time(nullptr)-lastTime>barSize )
				pBars = ReqHistoricalDataSync( contract, lastTime, barSize, display, useRth ).get();
			if( pBars )
			{
				HistoricalDataCache::Push( contract, display, barSize, useRth, *pBars, end, subDayCount );
				auto pExisting = existing.end(); time_t currentStart = 0; time_t currentEnd = 0;
				for( var& bar : *pBars )
				{
					var time = ConvertIBDate( bar.time );
					if( time<currentStart || time>currentEnd )
					{
						var tp = Clock::from_time_t( time );
						var day = DaysSinceEpoch( tp );
						currentStart = Clock::to_time_t(BeginningOfDay(tp) ); currentEnd = currentStart+24*60*60;
						pExisting = existing.find( day );
						if( pExisting==existing.end() )
							pExisting = existing.emplace( day, make_shared<vector<sp<::Bar>>>() ).first;
						if( !pExisting->second )
							pExisting->second = make_shared<vector<sp<::Bar>>>();
					}
					pExisting->second->push_back( make_shared<::Bar>(bar) );
				}
			}
		};
		auto pData = HistoricalDataCache::ReqHistoricalData( contract, endDay, dayCount, barSize, (Proto::Requests::Display)display, useRth );// : MapPtr<DayIndex,VectorPtr<sp<const ::Bar>>>{};
		if( pData )
		{
			ASSERT( pData->size() );
			if( !pData->begin()->second )
			{
				DayIndex startDayCount = 0; DayIndex endDay = 0;
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
			if( var current = CurrentTradingDay( contract.Exchange );
				!pData->rbegin()->second || (current==pData->rbegin()->first && isOpen) )//for current trading day bars
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
				if( endDayCount )
					addBars( currentDay, endDayCount, *pData, lastTime );
			}
		}
		else
		{
			pData = make_shared<map<DayIndex,VectorPtr<sp<::Bar>>>>();
			addBars( endDay, dayCount, *pData );
		}

		var now = time( nullptr ); time_t minTime=now, maxTime=0;
		for( var& dayBars : *pData )
		{
			if( !dayBars.second )//not sure why this is happening.
				continue;
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
	//tws is very slow
	void TwsClientCache::reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept
	{
		var cacheId = format( "{}.{}.{}.{}.{}", conId, StringUtilities::AddCommas(providerCodes), totalResults, Clock::to_time_t(start), Clock::to_time_t(end) );
		var pData = Cache::Get<Proto::Results::HistoricalNewsCollection>( cacheId );
		if( pData )
		{
//TODO
		}
		else
			TwsClient::reqHistoricalNews( requestId, conId, providerCodes, totalResults, start, end );
	}
}