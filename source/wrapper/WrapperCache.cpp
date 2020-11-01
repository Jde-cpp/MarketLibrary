#include "WrapperCache.h"
#include "../types/Contract.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/collections/Collections.h"

#define var const auto

namespace Jde::Markets
{
	void WrapperCache::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		if( _cacheIds.Has(reqId) )
		{
			WrapperLog::contractDetails( reqId, contractDetails );
			unique_lock l{_detailsMutex};
			_details.try_emplace( reqId, sp<vector<::ContractDetails>>{ new vector<::ContractDetails>{} } ).first->second->push_back( contractDetails );
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
			var pResults = pDetails==_details.end() ? sp<vector<::ContractDetails>>{} : pDetails->second;
			Cache::Set( cacheId, pResults );
			_details.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}

	Proto::Results::ExchangeContracts WrapperCache::ToOptionParam( string_view exchangeString, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		auto exchange = ToExchange( exchangeString );
		if( exchange==Exchanges::Smart && CIString{ "SMART"sv }!=exchangeString )
			exchange = Exchanges::UnknownExchange;
		Proto::Results::ExchangeContracts a; a.set_exchange( exchange ); a.set_multiplier( multiplier ); a.set_trading_class( tradingClass ); a.set_underlying_contract_id( underlyingConId );
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
			DBG( "Caching {}"sv, exchange );
			*Collections::InsertUnique( _optionParams, reqId )->add_exchanges() = ToOptionParam(  exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		}
	}

	void WrapperCache::securityDefinitionOptionalParameterEnd( int reqId )noexcept
	{
		var cacheId = _cacheIds.Find( reqId, string{} );
		if( cacheId.size() )
		{
			WrapperLog::securityDefinitionOptionalParameterEnd( reqId );
			var pParams = _optionParams.find( reqId );
			auto pResults = pParams==_optionParams.end() ? make_unique<Proto::Results::OptionExchanges>() : move( pParams->second );
			Cache::Set( cacheId, sp<Proto::Results::OptionExchanges>(pResults.release()) );
			_optionParams.erase( reqId );
			_cacheIds.erase( reqId );
		}
	}

	void WrapperCache::ToBar( const ::Bar& bar, Proto::Results::Bar& proto )noexcept
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
	void WrapperCache::historicalData( TickerId reqId, const ::Bar& bar )noexcept
	{
		WrapperLog::historicalData( reqId, bar );
	}
	void WrapperCache::historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept
	{
		WrapperLog::historicalDataEnd( reqId, startDateStr, endDateStr );
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