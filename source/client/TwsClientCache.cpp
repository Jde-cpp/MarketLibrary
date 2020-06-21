#include "TwsClientCache.h"
#include "../../../Framework/source/Cache.h"
#include "../wrapper/WrapperCache.h"
#include "../types/Contract.h"
#include "../types/proto/results.pb.h"

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
	void TwsClientCache::ReqHistoricalData( TickerId reqId, ContractPK contractId, DayIndex endDay, uint dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useCache, bool useRth )noexcept
	{
		var cacheId = useCache ? fmt::format( "ReqHistoricalData.{}.{}.{}.{}.{}.{}", contractId, endDay, dayCount, barSize, display, useRth ) : string{};
		var pData = cacheId.size() ? Cache::Get<vector<Proto::Results::Bar>>( cacheId ) : nullptr;
		if( pData )
		{
			//auto toString = []( const DateTime& time ){ return fmt::format("{}{:0>2}{:0>2}:{:0>2}{:0>2}{:0>2}", time.Year(), time.Month(), time.Day(), time.Hour(), time.Minute(), time.Second()); };
			var now = time( nullptr );
			int minTime=now, maxTime=0;
			for( var& bar : *pData )
			{
				var timet = bar.time();
				minTime = std::min( timet, minTime );
				maxTime = std::max( timet, maxTime );

				ibapi::Bar ibBar{ ToIBDate(DateTime{timet}), bar.high(), bar.low(), bar.open(), bar.close(), bar.wap(), (long long)bar.volume(), bar.count() };
				Wrapper()->historicalData( reqId, ibBar );
			}
			Wrapper()->historicalDataEnd( reqId, minTime==now ? "" : ToIBDate(DateTime{minTime}), maxTime==0 ? "" : ToIBDate(DateTime{maxTime}) );
		}
		else
		{
			Wrapper()->AddCacheId( reqId, cacheId );
			ibapi::Contract contract; contract.conId = contractId;
			//contract.symbol = "SPX"; contract.secType = "IND"; contract.exchange="CBOE"; contract.currency="USD";
			const TimePoint tp = Chrono::EndOfDay( Chrono::FromDays(endDay) );
			const DateTime endTime{ tp };
			const string endTimeString{ fmt::format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
			TwsClient::reqHistoricalData( reqId, contract, endTimeString, fmt::format("{} D", dayCount), BarSize::ToString((BarSize::Enum)barSize), TwsDisplay::ToString(display), useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
		}
	}
}