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
			shared_lock l{_detailsMutex};
			_details.try_emplace( reqId, sp<vector<ibapi::ContractDetails>>{ new vector<ibapi::ContractDetails>{} } ).first->second->push_back( contractDetails );
		}
	}
	void WrapperCache::contractDetailsEnd( int reqId )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::contractDetailsEnd( reqId );
			shared_lock l{_detailsMutex};
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

	//Log -> Cache -> Sync -> [Web|Historian]
	void WrapperCache::securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
			var a = ToOptionParam(  exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
			shared_lock l{_optionParamsMutex};
			_optionParams.try_emplace( reqId, sp<vector<Proto::Results::OptionParams>>{ new vector<Proto::Results::OptionParams>{} } ).first->second->push_back( a );
		}
	}

	//Proto::Results::OptionParams securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
	void WrapperCache::securityDefinitionOptionalParameterEnd( int reqId )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::securityDefinitionOptionalParameterEnd( reqId );
			shared_lock l{_optionParamsMutex};
			var pParams = _optionParams.find( reqId );
			var pResults = pParams==_optionParams.end() ? sp<vector<Proto::Results::OptionParams>>{} : pParams->second;
			Cache::Set( cacheId, pResults );
			_optionParams.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}
}