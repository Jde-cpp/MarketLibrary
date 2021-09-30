#include "WrapperSync.h"
#include <EReaderOSSignal.h>
#include "../client/TwsClientSync.h"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/collections/Collections.h"
#include <jde/markets/types/Contract.h>
#include "../types/IBException.h"

#define var const auto
namespace Jde::Markets
{
	WrapperSync::WrapperSync()noexcept:
		_pReaderSignal{ make_shared<EReaderOSSignal>(1000) }
	{
	}
	sp<TwsClientSync> WrapperSync::CreateClient( uint twsClientId )noexcept(false)
	{
		TwsConnectionSettings twsSettings;
		from_json( Jde::Settings::Global().SubContainer("tws")->Json(), twsSettings );
		_pClient = TwsClientSync::CreateInstance( twsSettings, shared_from_this(), _pReaderSignal, twsClientId );
		_pTickWorker = TickManager::TickWorker::CreateInstance( _pClient );
		return _pClient;
	}
	WrapperSync::~WrapperSync()
	{
		Shutdown();
	}
	void WrapperSync::Shutdown()noexcept
	{
		_pClient = nullptr;
		_pTickWorker = nullptr;
	}
	void WrapperSync::SendCurrentTime( const TimePoint& time )noexcept
	{
		unique_lock l{ _currentTimeCallbacksMutex };
		for( var& fnctn : _currentTimeCallbacks )
			fnctn( time );
		_currentTimeCallbacks.clear();
	}
	bool WrapperSync::error2( int id, int errorCode, str errorString )noexcept
	{
		bool handled = WrapperCo::error2( id, errorCode, errorString );
		if( handled )
			return true;
		if( errorCode==502 )
			SendCurrentTime( TimePoint{} );
		else if( id>0 )
		{
			if( (handled = _historicalData.Contains(id)) )
			{
				if( errorCode==162 )
				{
					WrapperCache::historicalDataEnd( id, "", "" );
					_historicalData.End( id );
				}
				else
					_historicalData.Error( id, IBException{errorString, errorCode, id, __func__,__FILE__, __LINE__} );
			}
			else if( (handled = _fundamentalData.Contains(id)) )
				_fundamentalData.Error( id, IBException{errorString, errorCode, id, __func__,__FILE__, __LINE__} );
			else if( (handled = _detailsData.Contains(id)) )
				_detailsData.Error( id, IBException{errorString, errorCode, id, __func__,__FILE__, __LINE__} );
			else if( (handled = _ratioData.Contains(id)) )
				_ratioData.Error( id, IBException{errorString, errorCode, id, __func__,__FILE__, __LINE__} );
			else
			{
				lock_guard l{ _errorCallbacksMutex };
				auto pCallback = _errorCallbacks.find( id );
				handled = pCallback != _errorCallbacks.end();
				if( handled )
				{
					pCallback->second( id, errorCode, errorString );
					_errorCallbacks.erase( pCallback );
				}
				{ lock_guard l2{ _headTimestampMutex }; _headTimestamp.erase( id ); }
			}
		}
		else if( errorCode==1100 )
		{
			shared_lock l{ _disconnectCallbacksMutex };
			for( var& callback : _disconnectCallbacks )
				callback( false );
		}
		else if( errorCode==2106 )//HMDS data farm connection is OK:ushmds
		{
			shared_lock l{ _disconnectCallbacksMutex };
			for( var& callback : _disconnectCallbacks )
				callback( true );
		}
		return handled;
	}
	void WrapperSync::AddDisconnectCallback( const DisconnectCallback& callback )noexcept
	{
		unique_lock l{ _disconnectCallbacksMutex };
		_disconnectCallbacks.push_front( callback );
	}

	void WrapperSync::currentTime( long time )noexcept
	{
		WrapperLog::currentTime( time );
		shared_lock l{_currentTimeCallbacksMutex};

		for( var& callback : _currentTimeCallbacks )
			callback( Clock::from_time_t(time) );
	}

	void WrapperSync::AddCurrentTime( CurrentTimeCallback& fnctn )noexcept
	{
		unique_lock l{_currentTimeCallbacksMutex};
		_currentTimeCallbacks.push_front( fnctn );
	}
	void WrapperSync::fundamentalData( TickerId reqId, str data )noexcept
	{
		_fundamentalData.Push( reqId, make_shared<string>(data) );
	}
////////////////////////////////////////////////////////////////
	void WrapperSync::AddHeadTimestamp( TickerId reqId, const HeadTimestampCallback& fnctn, const ErrorCallback& errorFnctn )noexcept
	{
		{
			lock_guard l{ _headTimestampMutex };
			_headTimestamp.emplace( reqId, fnctn );
		}
		{
			lock_guard l{ _errorCallbacksMutex };
			_errorCallbacks.emplace( reqId, errorFnctn );
		}
	}
	void WrapperSync::CheckTimeouts()noexcept
	{
		_historicalData.CheckTimeouts();
	}
	void WrapperSync::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		if( WrapperCo::_contractSingleHandles.Has(reqId) ) return WrapperCo::contractDetails( reqId, contractDetails );
		WrapperCache::contractDetails( reqId, contractDetails );
		_detailsData.Push( reqId, contractDetails );
	}
	void WrapperSync::contractDetailsEnd( int reqId )noexcept
	{
		if( WrapperCo::_contractSingleHandles.Has(reqId) ) return WrapperCo::contractDetailsEnd( reqId );
		WrapperCache::contractDetailsEnd( reqId );
		_detailsData.End( reqId );
	}

	std::future<VectorPtr<Proto::Results::Position>> WrapperSync::PositionPromise()noexcept
	{
		_positionPromiseMutex.lock();
		ASSERT( !_positionPromisePtr );  ASSERT( !_positionsPtr );

		_positionPromisePtr = make_shared<std::promise<VectorPtr<Proto::Results::Position>>>();
		_positionsPtr = make_shared<vector<Proto::Results::Position>>();
		return _positionPromisePtr->get_future();
	}
	void WrapperSync::position( str account, const ::Contract& contract, double position, double avgCost )noexcept
	{
		if( _positionsPtr )
		{
			ASSERT( _positionPromisePtr );
			WrapperLog::position( account, contract, position, avgCost );
			Proto::Results::Position y; y.set_allocated_contract( Contract{contract}.ToProto(true).get() ); y.set_account_number( account ); y.set_size( position ); y.set_avg_cost( avgCost );
			_positionsPtr->emplace_back( y );
		}
	}
	void WrapperSync::positionEnd()noexcept
	{
		WrapperLog::positionEnd();
		if( _positionsPtr )
		{
			ASSERT( _positionPromisePtr );
			_positionPromisePtr->set_value( _positionsPtr );
			_positionsPtr = nullptr;
			_positionPromisePtr = nullptr;
			_positionPromiseMutex.unlock();
		}
	}

	std::shared_future<TickerId> WrapperSync::ReqIdsPromise()noexcept
	{
		unique_lock l{ _requestIdsPromiseMutex };
		if( !_requestIdsPromisePtr )
		{
			_requestIdsPromisePtr = make_shared<std::promise<TickerId>>();
			_requestIdsFuturePtr = make_shared<std::shared_future<TickerId>>( _requestIdsPromisePtr->get_future() );
		}
		return *_requestIdsFuturePtr;
	}
	WrapperData<::ContractDetails>::Future WrapperSync::ContractDetailsPromise( ReqId reqId )noexcept
	{
		return _detailsData.Promise( reqId, 5s );
	}

/*	void WrapperSync::securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		securityDefinitionOptionalParameterSync( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
	}

	bool WrapperSync::securityDefinitionOptionalParameterSync( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		WrapperCache::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		var captured = _optionFutures.Contains( reqId );
		if( captured )
			*Collections::InsertShared( _optionData, reqId ).add_exchanges() = WrapperCache::ToOptionParam( exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		return captured;
	}

	void WrapperSync::securityDefinitionOptionalParameterEnd( int reqId )noexcept
	{
		securityDefinitionOptionalParameterEndSync( reqId );
	}

	bool WrapperSync::securityDefinitionOptionalParameterEndSync( int reqId )noexcept
	{
		WrapperCache::securityDefinitionOptionalParameterEnd( reqId );
		var captured = _optionFutures.Contains(reqId);
		if( captured )
		{
			auto pData = _optionData.find( reqId );
			_optionFutures.Push( reqId, pData==_optionData.end() ? make_shared<Proto::Results::OptionExchanges>() : pData->second );
			if( pData!=_optionData.end() )
				_optionData.erase( pData );
		}
		return captured;
	}
*/
	WrapperItem<Proto::Results::OptionExchanges>::Future WrapperSync::SecDefOptParamsPromise( ReqId reqId )noexcept
	{
		return _optionFutures.Promise( reqId, 15s );
	}
	WrapperItem<string>::Future WrapperSync::FundamentalDataPromise( ReqId reqId, Duration duration )noexcept
	{
		return _fundamentalData.Promise( reqId, duration );
	}

	WrapperItem<map<string,double>>::Future WrapperSync::RatioPromise( ReqId reqId, Duration duration )noexcept
	{
		lock_guard l{_ratioMutex};
		_ratioValues.emplace( reqId, map<string,double>{} );
		return _ratioData.Promise( reqId, duration );
	}

	WrapperData<::Bar>::Future WrapperSync::ReqHistoricalDataPromise( ReqId reqId, Duration timeout )noexcept
	{
		return _historicalData.Promise( reqId, timeout );
	}
	void WrapperSync::historicalData( TickerId reqId, const ::Bar& bar )noexcept
	{
		if( !WrapperCo::HistoricalData(reqId, bar) )
			historicalDataSync( reqId, bar );
	}
	bool WrapperSync::historicalDataSync( TickerId reqId, const ::Bar& bar )noexcept
	{
		WrapperCache::historicalData( reqId, bar );
		var captured = _historicalData.Contains( reqId );
		if( captured )
			_historicalData.Push( reqId, bar );
		return captured;
	}
	void WrapperSync::historicalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept
	{
		if( !WrapperCo::HistoricalDataEnd(reqId, startDateStr, endDateStr) )
			historicalDataEndSync( reqId, startDateStr, endDateStr );
	}
	bool WrapperSync::historicalDataEndSync( int reqId, str startDateStr, str endDateStr )noexcept
	{
		WrapperCache::historicalDataEnd( reqId, startDateStr, endDateStr );
		var captured = _historicalData.Contains( reqId );
		if( captured )
			_historicalData.End( reqId );
		return captured;
	}
	void WrapperSync::nextValidId( ibapi::OrderId orderId )noexcept
	{
		WrapperLog::nextValidId( orderId );
		unique_lock l{ _requestIdsPromiseMutex };
		if( _requestIdsPromisePtr )
		{
			_requestIdsPromisePtr->set_value( orderId );
			_requestIdsPromisePtr = nullptr;
			_requestIdsFuturePtr = nullptr;
		}
		else if( _pClient )
			_pClient->SetRequestId( orderId );
	}

	void WrapperSync::openOrderEnd()noexcept
	{
		WrapperLog::openOrderEnd();
	}

	void WrapperSync::headTimestamp( int reqId, str headTimestamp )noexcept
	{
		WrapperLog::headTimestamp( reqId, headTimestamp );
		lock_guard l{ _headTimestampMutex };
		auto pIdFunction = _headTimestamp.find( reqId );
		if( pIdFunction!=_headTimestamp.end() )
		{
			var callback = pIdFunction->second;
			TimePoint date{ Clock::from_time_t(std::stoul(headTimestamp)) };
			callback( date );
			_headTimestamp.erase( pIdFunction );
		}
		else
			DBG( "Could not find headTimestamp request '{}'"sv, reqId );
	}


}