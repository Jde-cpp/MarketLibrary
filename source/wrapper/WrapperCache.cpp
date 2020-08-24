#include "WrapperCache.h"
#include "../types/Contract.h"
#include "../../../Framework/source/Cache.h"

#define var const auto

namespace Jde::Markets
{
	void WrapperCache::contractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept
	{
		if( _cacheIds.Has(reqId) )
		{
			WrapperLog::contractDetails( reqId, contractDetails );
			unique_lock l{_detailsMutex};
			_details.try_emplace( reqId, sp<vector<ibapi::ContractDetails>>{ new vector<ibapi::ContractDetails>{} } ).first->second->push_back( contractDetails );
		}
	}
	void WrapperCache::contractDetailsEnd( int reqId )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::contractDetailsEnd( reqId );
			unique_lock l{_detailsMutex};
			var pDetails = _details.find( reqId );
			var pResults = pDetails==_details.end() ? sp<vector<ibapi::ContractDetails>>{} : pDetails->second;
			Cache::Set( cacheId, pResults );
			_details.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}

	Proto::Results::OptionParams WrapperCache::ToOptionParam( const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		Proto::Results::OptionParams a; a.set_exchange( exchange ); a.set_multiplier( multiplier ); a.set_trading_class( tradingClass ); a.set_underlying_contract_id( underlyingConId );
		for( var strike : strikes )
			a.add_strikes( strike );

		for( var& expiration : expirations )
			a.add_expirations( Contract::ToDay(expiration) );
		return a;
	}

	void WrapperCache::securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
			var a = ToOptionParam(  exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
			unique_lock l{_optionParamsMutex};
			_optionParams.try_emplace( reqId, sp<vector<Proto::Results::OptionParams>>{ new vector<Proto::Results::OptionParams>{} } ).first->second->push_back( a );
		}
	}

	void WrapperCache::securityDefinitionOptionalParameterEnd( int reqId )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::securityDefinitionOptionalParameterEnd( reqId );
			unique_lock l{_optionParamsMutex};
			var pParams = _optionParams.find( reqId );
			var pResults = pParams==_optionParams.end() ? sp<vector<Proto::Results::OptionParams>>{} : pParams->second;
			Cache::Set( cacheId, pResults );
			_optionParams.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}

	void WrapperCache::ToBar( const ibapi::Bar& bar, Proto::Results::Bar& proto )noexcept
	{
		var time = bar.time.size()==8 ? DateTime{ (uint16)stoi(bar.time.substr(0,4)), (uint8)stoi(bar.time.substr(4,2)), (uint8)stoi(bar.time.substr(6,2)) } : DateTime( stoi(bar.time) );
		proto.set_time( (int)time.TimeT() );

		proto.set_high( bar.high );
		proto.set_low( bar.low );
		proto.set_open( bar.open );
		proto.set_close( bar.close );
		proto.set_wap( bar.wap );
		proto.set_volume( bar.volume );
		proto.set_count( bar.count );
	}
	void WrapperCache::historicalData( TickerId reqId, const ibapi::Bar& bar )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::historicalData( reqId, bar );
			Proto::Results::Bar proto;
			ToBar( bar, proto );
			unique_lock l{_historicalDataMutex};
			_historicalData.try_emplace( reqId, sp<vector<Proto::Results::Bar>>{ new vector<Proto::Results::Bar>{} } ).first->second->push_back( proto );
		}
	}
	void WrapperCache::historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		WrapperLog::historicalDataEnd( reqId, startDateStr, endDateStr );
		if( cacheId.size() )
		{
			unique_lock l{_historicalDataMutex};
			var pParams = _historicalData.find( reqId );
			var pResults = pParams==_historicalData.end() ? sp<vector<Proto::Results::Bar>>{} : pParams->second;
			Cache::Set( cacheId, pResults );
			_optionParams.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}

	void WrapperCache::newsProviders( const std::vector<NewsProvider>& newsProviders )noexcept
	{
		WrapperLog::newsProviders( newsProviders );

		auto pResults = make_shared<Proto::Results::StringMap>();
		for( var& provider : newsProviders )
			(*pResults->mutable_values())[provider.providerCode] = provider.providerName;
		Cache::Set( "RequestProviders", pResults );
	}
}