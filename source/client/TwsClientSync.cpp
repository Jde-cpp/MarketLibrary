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
		Tws{ settings, wrapper, pReaderSignal, clientId }
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