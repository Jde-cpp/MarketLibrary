#include "TwsClientSync.h"
#include "../wrapper/WrapperSync.h"
#include "../TwsProcessor.h"
#include "../types/IBException.h"

#define var const auto
namespace Jde::Markets
{
	using namespace Chrono;
	sp<TwsClientSync> pInstance;
	TwsClientSync& TwsClientSync::Instance()noexcept{ ASSERT(pInstance); return *pInstance; }
	void TwsClientSync::CreateInstance( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		DBG0( "TwsClientSync::CreateInstance"sv );
		pInstance = sp<TwsClientSync>{ new TwsClientSync(settings, wrapper, pReaderSignal, clientId) };
		TwsProcessor::CreateInstance( pInstance, pReaderSignal );
		while( !pInstance->isConnected() ) //while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();
		DBG( "Connected to Tws Host='{}', Port'{}', Client='{}'"sv, settings.Host, pInstance->_port, clientId );
	//	pInstance->ReqIds();
	}
	TwsClientSync::TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClientCache( settings, wrapper, pReaderSignal, clientId )
	{}

	shared_ptr<WrapperSync> TwsClientSync::Wrapper()noexcept
	{
		return std::dynamic_pointer_cast<WrapperSync>(_pWrapper);
	}
	TimePoint TwsClientSync::CurrentTime()noexcept
	{
		std::condition_variable cv; mutex mutex;
		TimePoint time;
		WrapperSync::CurrentTimeCallback fnctn = [&time, &cv]( TimePoint t ){ time = t; cv.notify_one(); };
		Wrapper()->AddCurrentTime( fnctn );
		TwsClient::reqCurrentTime();
		unique_lock l{mutex};
		if( cv.wait_for( l, 5s )==std::cv_status::timeout )
			WARN0N( "Timed out looking for current time" );
		return time;
	}
	void TwsClientSync::OnError( TickerId id, int errorCode, const std::string& errorMsg )
	{
		auto pError = make_shared<IBException>( errorMsg, errorCode, id );
		_errors.emplace( id, pError );
		auto pCv = _conditionVariables.Find( id );
		if( pCv )
			pCv->notify_one();
	}
	void TwsClientSync::OnHeadTimestamp( TickerId reqId, TimePoint t )
	{
		_headTimestamps.emplace( reqId, t );
		auto pCv = _conditionVariables.Find( reqId );
		if( pCv )
			pCv->notify_one();
	}
	TimePoint TwsClientSync::HeadTimestamp( const ibapi::Contract &contract, const std::string& whatToShow )noexcept(false)
	{
/*
		TimePoint time;
		sp<IBException> pError;
		atomic<bool> returned = false;
		WrapperSync::ErrorCallback errorFnctn = [&pError, &cv, &returned, contract]( TickerId id, int errorCode, const std::string& errorMsg )
		{
			returned = true;

			cv.notify_one();
		};
		WrapperSync::CurrentTimeCallback fnctn = [&time, &cv, &returned]( TimePoint t )
		{
			returned = true;
			time = t;
			cv.notify_one();
		};*/
		var reqId = RequestId();
		auto pCV = make_shared<std::condition_variable>();
		_conditionVariables.emplace( reqId, pCV );
		Wrapper()->AddHeadTimestamp( reqId, [&, reqId](auto t){OnHeadTimestamp(reqId,t);}, [&](auto id, auto code, const auto& msg){OnError(id,code,msg);} );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqHeadTimestamp( reqId, contract, whatToShow, 1, 2 );
		if( pCV->wait_for(l, 30s)==std::cv_status::timeout )
			WARN0( "Timed out looking for HeadTimestamp."sv );
		if( _errors.Find(reqId) )
		{
			auto pError = _errors.Find(reqId);
			_errors.erase( reqId );
			throw *pError;
		}
		var time = _headTimestamps.Find( reqId, TimePoint{} );
		_headTimestamps.erase( reqId );
		_conditionVariables.erase( reqId );
		return time;
	}

	TwsClientSync::Future<ibapi::Bar> TwsClientSync::ReqHistoricalDataSync( const Contract& contract, DayIndex endDay, uint dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false)
	{
		var reqId = RequestId();
		auto future = Wrapper()->ReqHistoricalDataPromise( reqId );
		if( useCache )
			TwsClientCache::ReqHistoricalData( reqId, contract, endDay, dayCount, barSize, display, useRth );
		else
		{
			const DateTime endTime{ Chrono::FromDays(endDay) };
			const string endTimeString{ format("{}{:0>2}{:0>2} 23:59:59 GMT", endTime.Year(), endTime.Month(), endTime.Day()) };
			reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::TryToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
		}
		return future;
	}

	TwsClientSync::Future<ibapi::Bar> TwsClientSync::ReqHistoricalDataSync( const Contract& contract, time_t start, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept
	{
		var now = std::time(nullptr);
		time_t end=start;
		for( ; end<now; end+=barSize );
		var seconds = end-start;
		if( seconds )
		{
			var reqId = RequestId();
			auto future = Wrapper()->ReqHistoricalDataPromise( reqId );
			const DateTime endTime{ end };
			const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
			reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} S", seconds), string{BarSize::TryToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
			return future;
		}
		std::promise<sp<vector<ibapi::Bar>>> promise;
		promise.set_value( make_shared<vector<ibapi::Bar>>() );
		return promise.get_future();
	}

/*	TwsClientSync::Future<ibapi::Bar> TwsClientSync::ReqHistoricalData( const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate )noexcept(false)
	{
		var reqId = RequestId();
		auto future = Wrapper()->ReqHistoricalDataPromise( reqId );
		TwsClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false/ *keepUpToDate* /, TagValueListSPtr() );
		return future;
	}*/
/*		var reqId = RequestId();
		auto pCV = make_shared<std::condition_variable>();
		_conditionVariables.emplace( reqId, pCV );
		Jde::Markets::WrapperSync::ReqHistoricalDataCallback callback = [&, reqId](auto t){OnReqHistoricalData(reqId,t);};
		Wrapper()->AddRequestHistoricalData( reqId, callback, [&](auto id, auto code, const auto& msg){OnError(id,code,msg);} );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false/ *keepUpToDate* /, TagValueListSPtr() );
		if( pCV->wait_for(l, timeout)==std::cv_status::timeout )
			WARN0( "Timed out looking for reqHistoricalData."sv );
		if( _errors.Find(reqId) )
		{
			auto pError = _errors.Find(reqId);
			_errors.erase( reqId );
			throw *pError;
		}
		var pResults = _historicalData.Find( reqId );
		_historicalData.erase( reqId );
		_conditionVariables.erase( reqId );
		return pResults ? pResults : make_shared<list<ibapi::Bar>>();*/
	//}
	TwsClientSync::Future<ibapi::ContractDetails> TwsClientSync::ReqContractDetails( string_view symbol )noexcept
	{
		ibapi::Contract contract;
		contract.symbol = symbol;
		contract.secType = "STK";
		contract.currency="USD";
		contract.exchange = contract.primaryExchange = "SMART";

		return ReqContractDetails( contract );
	}
/*	TwsClientSync::Future<ibapi::ContractDetails> TwsClientSync::ReqContractDetails( string_view symbol, DayIndex dayIndex, SecurityRight right )noexcept
	{
		ibapi::Contract contract; contract.symbol = symbol; contract.exchange = "SMART"; contract.secType = "OPT";/ *only works with symbol
		if( dayIndex>0 )
		{
			const DateTime date{ Chrono::FromDays(dayIndex) };
			contract.lastTradeDateOrContractMonth = format( "{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day() );
		}
		contract.right = ToString( right );

		return ReqContractDetails( contract );
	}*/
	TwsClientSync::Future<ibapi::ContractDetails> TwsClientSync::ReqContractDetails( ContractPK id )noexcept
	{
		ASSERT( id!=0 );
		ibapi::Contract contract; contract.conId = id; contract.exchange = "SMART"; contract.secType = "STK";
		return ReqContractDetails( contract );
	}
	TwsClientSync::Future<ibapi::ContractDetails> TwsClientSync::ReqContractDetails( const ibapi::Contract& contract )noexcept
	{
		var reqId = RequestId();
		auto future = Wrapper()->ContractDetailsPromise( reqId );
		TwsClientCache::ReqContractDetails( reqId, contract );
		// if( set && cacheReqId )
		// {
		// 	var pContracts = future.get();
		// 	for( var& contract : *pContracts )
		// 		Wrapper()->contractDetails( reqId, contract );
		// 	Wrapper()->contractDetailsEnd( reqId );
		// }
		// else if( !set )
		// 	TwsClient::reqContractDetails( reqId, contract );
		return future;
	}
	Proto::Results::OptionParams TwsClientSync::ReqSecDefOptParamsSmart( ContractPK underlyingConId, string_view symbol )noexcept(false)
	{
		auto params = ReqSecDefOptParams( underlyingConId, symbol ).get();
		auto pSmart = std::find_if( params->begin(), params->end(), [](auto& param){return param.exchange()=="SMART";} );
		if( pSmart==params->end() )
			THROW( Exception("Could not find Smart options for '{}'", symbol) );
		return *pSmart;
	}
	TwsClientSync::Future<Proto::Results::OptionParams> TwsClientSync::ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept
	{
		var reqId = RequestId();
		auto future = Wrapper()->SecDefOptParamsPromise( reqId );
		TwsClientCache::ReqSecDefOptParams( reqId, underlyingConId, symbol );
		return future;
	}
/*	void TwsClientSync::reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol, string_view futFopExchange, string_view underlyingSecType )noexcept
	{
		auto future = Wrapper()->SecDefOptParamsPromise( tickerId );
		//if( !set )
			TwsClientCache::reqSecDefOptParams( tickerId, underlyingConId, underlyingSymbol, futFopExchange, underlyingSecType );
		else
		{
			var pData = future.get();
			for( var& param : *pData )
			{
				std::set<std::string> expirations;
				for( auto i=0; i<param.expirations_size(); ++i )
				{
					const DateTime date{ Chrono::FromDays(param.expirations(i)) };
					expirations.emplace( format("{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day()) );
				}
				std::set<double> strikes;
				for( auto i=0; i<param.strikes_size(); ++i )
					strikes.emplace( param.strikes(i) );
				Wrapper()->securityDefinitionOptionalParameter( tickerId, param.exchange(), param.underlying_contract_id(), param.trading_class(), param.multiplier(), expirations, strikes );
			}
			Wrapper()->securityDefinitionOptionalParameterEnd( tickerId );
		}
	}
*/
	std::future<sp<string>> TwsClientSync::ReqFundamentalData( const ibapi::Contract &contract, string_view reportType )noexcept
	{
		var reqId = RequestId();
		auto future = Wrapper()->FundamentalDataPromise( reqId );
		TwsClient::reqFundamentalData( reqId, contract, reportType );
		return future;
	}
	std::future<sp<map<string,double>>> TwsClientSync::ReqRatios( const ibapi::Contract &contract )noexcept
	{
		var reqId = RequestId();
		auto future = Wrapper()->RatioPromise( reqId );
		TwsClient::reqMktData( reqId, contract, "258", false, false, TagValueListSPtr{} );
		return future;
	}

	void TwsClientSync::ReqIds()noexcept
	{
		var future = Wrapper()->ReqIdsPromise();
		TwsClient::reqIds();
		if( future.wait_for(10s)==std::future_status::timeout )
			WARN0( "Timed out looking for reqIds."sv );
		else
			SetRequestId( future.get() );
	}
/*
	vector<ActiveOrderPtr> TwsClientSync::ReqAllOpenOrders()noexcept(false)
	{
		DBG0( "ReqAllOpenOrders" );
		vector<ActiveOrderPtr> results;
		std::condition_variable cv;
		atomic<bool> done = false;
		Jde::Markets::WrapperSync::EndCallback callback = [&](){ done=true; cv.notify_one(); };
		Wrapper()->AddOpenOrderEnd( callback );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqAllOpenOrders();
		if( !done && cv.wait_for(l, 60s)==std::cv_status::timeout && !done )
			THROW( Exception("Timed out looking for open orders.") );
	}
	*/
}