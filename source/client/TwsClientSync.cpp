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
	sp<TwsClientSync> TwsClientSync::_pSyncInstance;
	TwsClientSync& TwsClientSync::Instance()noexcept{ ASSERT(_pSyncInstance); return *_pSyncInstance; }
	bool TwsClientSync::IsConnected()noexcept{ auto p = _pSyncInstance; return p && p->isConnected(); }
	sp<TwsClientSync> TwsClientSync::CreateInstance( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		_pInstance = sp<TwsClientSync>{ new TwsClientSync(settings, wrapper, pReaderSignal, clientId) };
		_pSyncInstance = static_pointer_cast<TwsClientSync>( _pInstance );
		TwsProcessor::CreateInstance( _pSyncInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();

		INFO( "Connected to Tws Host='{}', Port'{}', Client='{}'"sv, settings.Host, _pSyncInstance->_port, clientId );
		return _pSyncInstance;
	}
	TwsClientSync::TwsClientSync( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		TwsClientCache( settings, wrapper, pReaderSignal, clientId )
	{}

	sp<WrapperSync> TwsClientSync::Wrapper()noexcept
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
			WARN( "Timed out looking for current time"sv );
		return time;
	}
	void TwsClientSync::OnError( TickerId id, int errorCode, const std::string& errorMsg )
	{
		auto pError = sp<IBException>( new IBException{errorMsg, errorCode, id} );
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
		var reqId = RequestId();
		auto pCV = make_shared<std::condition_variable>();
		_conditionVariables.emplace( reqId, pCV );
		_wrapper.AddHeadTimestamp( reqId, [&, reqId](auto t){OnHeadTimestamp(reqId,t);}, [&](auto id, auto code, const auto& msg){OnError(id,code,msg);} );
		mutex mutex;
		unique_lock l{mutex};
		TwsClient::reqHeadTimestamp( reqId, contract, whatToShow, 1, 2 );
		if( pCV->wait_for(l, 30s)==std::cv_status::timeout )
			WARN( "Timed out looking for HeadTimestamp."sv );
		if( _errors.Find(reqId) )
		{
			auto pError = _errors.Find(reqId);
			_errors.erase( reqId );
			throw move( *pError );
		}
		var time = _headTimestamps.Find( reqId );
		_headTimestamps.erase( reqId );
		_conditionVariables.erase( reqId );
		return time.value_or( TimePoint{} );
	}

/*	TwsClientSync::Future<::Bar> TwsClientSync::ReqHistoricalDataSync(const Contract& contract, Day endDay, Day dayCount, EBarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache)noexcept(false)
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
	*/

	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( sv symbol )noexcept
	{
		::Contract contract;
		contract.symbol = symbol;
		contract.secType = "STK";
		contract.currency="USD";
		contract.exchange = contract.primaryExchange = "SMART";

		return ReqContractDetails( contract );
	}
/*	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetails( sv symbol, Day dayIndex, SecurityRight right )noexcept
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
	TwsClientSync::Future<::ContractDetails> TwsClientSync::ReqContractDetailsInst( ContractPK id )noexcept//TODO find out why return multiple?
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

	std::future<sp<string>> TwsClientSync::ReqFundamentalData( const ::Contract &contract, sv reportType )noexcept
	{
		var reqId = RequestId();
		auto future = _wrapper.FundamentalDataPromise( reqId, 5s );
		TwsClient::reqFundamentalData( reqId, contract, reportType );
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
			WARN( "Timed out looking for reqIds."sv );
		else
			SetRequestId( future.get() );
	}
}