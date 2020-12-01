#include "TwsClientSync.h"
#include "../wrapper/WrapperSync.h"
#include "../TwsProcessor.h"
#include "../types/IBException.h"

#define var const auto
#define _wrapper (*Wrapper())
namespace Jde::Markets
{
	using namespace Chrono;
	using EBarSize=Proto::Requests::BarSize;
	sp<TwsClientSync> pInstance;
	TwsClientSync& TwsClientSync::Instance()noexcept{ ASSERT(pInstance); return *pInstance; }
	bool TwsClientSync::IsConnected()noexcept{ return pInstance && pInstance->isConnected(); }
	sp<TwsClientSync> TwsClientSync::CreateInstance( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{//TODO call to TwsClient::CreateInstance
		DBG0( "TwsClientSync::CreateInstance"sv );
		_pInstance = sp<TwsClientSync>{ new TwsClientSync(settings, wrapper, pReaderSignal, clientId) };
		pInstance = static_pointer_cast<TwsClientSync>( _pInstance );
		TwsProcessor::CreateInstance( pInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();

		DBG( "Connected to Tws Host='{}', Port'{}', Client='{}'"sv, settings.Host, pInstance->_port, clientId );
		return pInstance;
	//	pInstance->ReqIds();
	}
	TwsClientSync::TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClientCache( settings, wrapper, pReaderSignal, clientId )
	{}

	shared_ptr<WrapperSync> TwsClientSync::Wrapper()noexcept
	{
		return std::dynamic_pointer_cast<WrapperSync>(_pWrapper);
	}
	void TwsClientSync::CheckTimeouts()noexcept
	{
		_wrapper.CheckTimeouts();
	}

	TimePoint TwsClientSync::CurrentTime()noexcept
	{
		std::condition_variable cv; mutex mutex;
		TimePoint time;
		WrapperSync::CurrentTimeCallback fnctn = [&time, &cv]( TimePoint t ){ time = t; cv.notify_one(); };
		_wrapper.AddCurrentTime( fnctn );
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
	TimePoint TwsClientSync::HeadTimestamp( const ::Contract &contract, const std::string& whatToShow )noexcept(false)
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
		_wrapper.AddHeadTimestamp( reqId, [&, reqId](auto t){OnHeadTimestamp(reqId,t);}, [&](auto id, auto code, const auto& msg){OnError(id,code,msg);} );
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

	TwsClientSync::Future<::Bar> TwsClientSync::ReqHistoricalDataSync( const Contract& contract, DayIndex endDay, DayIndex dayCount, EBarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false)
	{
		var reqId = RequestId();
		auto future = _wrapper.ReqHistoricalDataPromise( reqId, barSize==EBarSize::Day && dayCount<3 ? 10s : 5min );
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

	TwsClientSync::Future<::Bar> TwsClientSync::ReqHistoricalDataSync( const Contract& contract, time_t start, EBarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept
	{
		var now = std::time(nullptr);
		time_t end=start;
		for( ; end<now; end+=barSize );
		var seconds = end-start;
		if( seconds )
		{
			var reqId = RequestId();
			auto future = _wrapper.ReqHistoricalDataPromise( reqId, 10s );
			const DateTime endTime{ end };
			const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
			reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} S", seconds), string{BarSize::TryToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
			return future;
		}
		std::promise<sp<vector<::Bar>>> promise;
		promise.set_value( make_shared<vector<::Bar>>() );
		return promise.get_future();
	}

/*	TwsClientSync::Future<::Bar> TwsClientSync::ReqHistoricalData( const ::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate )noexcept(false)
	{
		var reqId = RequestId();
		auto future = _wrapper->ReqHistoricalDataPromise( reqId );
		TwsClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false/ *keepUpToDate* /, TagValueListSPtr() );
		return future;
	}*/
/*		var reqId = RequestId();
		auto pCV = make_shared<std::condition_variable>();
		_conditionVariables.emplace( reqId, pCV );
		Jde::Markets::WrapperSync::ReqHistoricalDataCallback callback = [&, reqId](auto t){OnReqHistoricalData(reqId,t);};
		_wrapper->AddRequestHistoricalData( reqId, callback, [&](auto id, auto code, const auto& msg){OnError(id,code,msg);} );
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
		return pResults ? pResults : make_shared<list<::Bar>>();*/
	//}
	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( string_view symbol )noexcept
	{
		::Contract contract;
		contract.symbol = symbol;
		contract.secType = "STK";
		contract.currency="USD";
		contract.exchange = contract.primaryExchange = "SMART";

		return ReqContractDetails( contract );
	}
/*	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( string_view symbol, DayIndex dayIndex, SecurityRight right )noexcept
	{
		::Contract contract; contract.symbol = symbol; contract.exchange = "SMART"; contract.secType = "OPT";/ *only works with symbol
		if( dayIndex>0 )
		{
			const DateTime date{ Chrono::FromDays(dayIndex) };
			contract.lastTradeDateOrContractMonth = format( "{}{:0>2}{:0>2}", date.Year(), date.Month(), date.Day() );
		}
		contract.right = ToString( right );

		return ReqContractDetails( contract );
	}*/
	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( ContractPK id )noexcept//TODO find out why return multiple?
	{
		ASSERT( id!=0 );
		::Contract contract; contract.conId = id; contract.exchange = "SMART"; contract.secType = "STK";
		return ReqContractDetails( contract );
	}
	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( const ::Contract& contract )noexcept
	{
		var reqId = RequestId();
		auto future = _wrapper.ContractDetailsPromise( reqId );
		TwsClientCache::ReqContractDetails( reqId, contract );
		// if( set && cacheReqId )
		// {
		// 	var pContracts = future.get();
		// 	for( var& contract : *pContracts )
		// 		_wrapper.contractDetails( reqId, contract );
		// 	_wrapper.contractDetailsEnd( reqId );
		// }
		// else if( !set )
		// 	TwsClient::reqContractDetails( reqId, contract );
		return future;
	}
	sp<Proto::Results::ExchangeContracts> TwsClientSync::ReqSecDefOptParamsSmart( ContractPK underlyingConId, string_view symbol )noexcept(false)
	{
		auto pParams = ReqSecDefOptParams( underlyingConId, symbol ).get();
		for( auto i = 0; i<pParams->exchanges_size(); ++i )
		{
			if( pParams->exchanges(i).exchange()==Exchanges::Smart )
				return make_shared<Proto::Results::ExchangeContracts>( pParams->exchanges(i) );
		}
		THROW( Exception("Could not find Smart options for '{}'", symbol) );
	}
	std::future<sp<Proto::Results::OptionExchanges>> TwsClientSync::ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept
	{
		var reqId = RequestId();
		auto future = _wrapper.SecDefOptParamsPromise( reqId );
		TwsClientCache::ReqSecDefOptParams( reqId, underlyingConId, symbol );
		return future;
	}
/*	void TwsClientSync::reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol, string_view futFopExchange, string_view underlyingSecType )noexcept
	{
		auto future = _wrapper.SecDefOptParamsPromise( tickerId );
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
				_wrapper.securityDefinitionOptionalParameter( tickerId, param.exchange(), param.underlying_contract_id(), param.trading_class(), param.multiplier(), expirations, strikes );
			}
			_wrapper.securityDefinitionOptionalParameterEnd( tickerId );
		}
	}
*/
	std::future<sp<string>> TwsClientSync::ReqFundamentalData( const ::Contract &contract, string_view reportType )noexcept
	{
		var reqId = RequestId();
		auto future = _wrapper.FundamentalDataPromise( reqId, 5s );
		TwsClient::reqFundamentalData( reqId, contract, reportType );
		return future;
	}
	/*
	std::future<sp<map<string,double>>> TwsClientSync::ReqRatios( const ::Contract &contract )noexcept
	{
		var reqId = RequestId();
		auto future = _wrapper.RatioPromise( reqId, 5s );//~~~
		TwsClient::reqMktData( reqId, contract, "165,258,456", false, false, {} );//456=dividends - https://interactivebrokers.github.io/tws-api/tick_types.html
		return future;
	}
*/
	TwsClientSync::Future<NewsProvider> TwsClientSync::RequestNewsProviders()noexcept
	{
		auto future = _wrapper.NewsProviderPromise();
		TwsClientCache::RequestNewsProviders();
		return future;
	}

	std::future<VectorPtr<Proto::Results::Position>> TwsClientSync::RequestPositions()noexcept(false)//should throw if currently perpetual reqPositions.
	{
		auto future = _wrapper.PositionPromise();
		TwsClient::reqPositions();
		return future;
	}

	void TwsClientSync::ReqIds()noexcept
	{
		var future = _wrapper.ReqIdsPromise();
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
		_wrapper.AddOpenOrderEnd( callback );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqAllOpenOrders();
		if( !done && cv.wait_for(l, 60s)==std::cv_status::timeout && !done )
			THROW( Exception("Timed out looking for open orders.") );
	}
	*/
}