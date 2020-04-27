#include "TwsClientSync.h"
#include "wrapper/WrapperSync.h"
#include "TwsProcessor.h"
#include "types/IBException.h"

#define var const auto
namespace Jde::Markets
{
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
		pInstance->ReqIds();
	}
	TwsClientSync::TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClient( settings, wrapper, pReaderSignal, clientId )
	{}

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

/*	void TwsClientSync::OnReqHistoricalData( TickerId reqId, sp<list<ibapi::Bar>> pBars )
	{
		VERIFY( _historicalData.emplace(reqId, pBars).second );
		auto pCv = _conditionVariables.Find( reqId );
		if( pCv )
			pCv->notify_one();
	};
*/
	std::future<sp<list<ibapi::Bar>>> TwsClientSync::ReqHistoricalData( const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate )noexcept(false)
	{
		var reqId = RequestId();
		auto future = Wrapper()->ReqHistoricalDataPromise( reqId );
		TwsClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false/*keepUpToDate*/, TagValueListSPtr() );
		return future;
	}
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
	std::future<sp<list<ibapi::ContractDetails>>> TwsClientSync::ReqContractDetails( ContractPK id )noexcept
	{
		ibapi::Contract contract; contract.conId = id; contract.exchange = "SMART";
		return ReqContractDetails( contract );
	}
	std::future<sp<list<ibapi::ContractDetails>>> TwsClientSync::ReqContractDetails( const ibapi::Contract& contract )noexcept
	{
		var reqId = RequestId();
		auto future = Wrapper()->ContractDetailsPromise( reqId );
		TwsClient::reqContractDetails( reqId, contract );
		return future;
	}
	std::future<sp<list<OptionsData>>> TwsClientSync::ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept(false)
	{
		var reqId = RequestId();
		auto future = Wrapper()->SecDefOptParamsPromise( reqId );
		TwsClient::reqSecDefOptParams( reqId, underlyingConId, symbol );
		return future;
	}
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
		if( future.wait_for(10s)==std::future_status::timeout )
			WARN0( "Timed out looking for reqIds."sv );
		else
			SetRequestId( future.get() );




		//TwsClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, false/*keepUpToDate*/, TagValueListSPtr() );
		//return future;
/*		std::condition_variable cv;
		Jde::Markets::WrapperSync::ReqIdCallback callback = [&](auto id){ SetRequestId(id); cv.notify_one(); };
		Wrapper()->AddRequestIds( callback );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqIds();
		if( cv.wait_for(l, 10s)==std::cv_status::timeout )
*/
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