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
	{}

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
	α WrapperSync::Shutdown()noexcept->void
	{
		_pClient = nullptr;
		_pTickWorker = nullptr;
	}
	α WrapperSync::SendCurrentTime( const TimePoint& time )noexcept->void
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
			if( (handled = _fundamentalData.Contains(id)) )
				_fundamentalData.Error( id, IBException{errorString, errorCode, id} );
			else if( (handled = _ratioData.Contains(id)) )
				_ratioData.Error( id, IBException{errorString, errorCode, id} );
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
	α WrapperSync::AddDisconnectCallback( const DisconnectCallback& callback )noexcept->void
	{
		unique_lock l{ _disconnectCallbacksMutex };
		_disconnectCallbacks.push_front( callback );
	}

	α WrapperSync::currentTime( long time )noexcept->void
	{
		WrapperLog::currentTime( time );
		shared_lock l{_currentTimeCallbacksMutex};

		for( var& callback : _currentTimeCallbacks )
			callback( Clock::from_time_t(time) );
	}

	α WrapperSync::AddCurrentTime( CurrentTimeCallback& fnctn )noexcept->void
	{
		unique_lock l{_currentTimeCallbacksMutex};
		_currentTimeCallbacks.push_front( fnctn );
	}
	α WrapperSync::fundamentalData( TickerId reqId, str data )noexcept->void
	{
		_fundamentalData.Push( reqId, make_shared<string>(data) );
	}

	α WrapperSync::CheckTimeouts()noexcept->void
	{
		//_historicalData.CheckTimeouts();
	}
	std::future<VectorPtr<Proto::Results::Position>> WrapperSync::PositionPromise()noexcept
	{
		_positionPromiseMutex.lock();
		ASSERT( !_positionPromisePtr );  ASSERT( !_positionsPtr );

		_positionPromisePtr = make_shared<std::promise<VectorPtr<Proto::Results::Position>>>();
		_positionsPtr = make_shared<vector<Proto::Results::Position>>();
		return _positionPromisePtr->get_future();
	}
	α WrapperSync::position( str account, const ::Contract& contract, ::Decimal position, double avgCost )noexcept->void
	{
		if( _positionsPtr )
		{
			ASSERT( _positionPromisePtr );
			WrapperLog::position( account, contract, position, avgCost );
			Proto::Results::Position y; y.set_allocated_contract( Contract{contract}.ToProto().release() ); y.set_account_number( account ); y.set_size( ToDouble(position) ); y.set_avg_cost( avgCost );
			_positionsPtr->emplace_back( y );
		}
	}
	α WrapperSync::positionEnd()noexcept->void
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

	α WrapperSync::nextValidId( ibapi::OrderId orderId )noexcept->void
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

	α WrapperSync::openOrderEnd()noexcept->void
	{
		WrapperLog::openOrderEnd();
	}
}