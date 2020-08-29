#include "WrapperSync.h"
#include <EReaderOSSignal.h>
#include "../client/TwsClientSync.h"
#include "../../../Framework/source/Settings.h"
#include "../types/Contract.h"
#include "../types/IBException.h"

#define var const auto
namespace Jde::Markets
{
	WrapperSync::WrapperSync()noexcept:
		_pReaderSignal{ make_shared<EReaderOSSignal>(1000) }
	{
	}
	void WrapperSync::CreateClient( uint twsClientId )noexcept
	{
		TwsConnectionSettings twsSettings;
		from_json( Jde::Settings::Global().SubContainer("tws")->Json(), twsSettings );
		TwsClientSync::CreateInstance( twsSettings, shared_from_this(), _pReaderSignal, twsClientId );
	}

	void WrapperSync::SendCurrentTime( const TimePoint& time )noexcept
	{
		unique_lock l{ _currentTimeCallbacksMutex };
		for( var& fnctn : _currentTimeCallbacks )
			fnctn( time );
		_currentTimeCallbacks.clear();
	}
	bool WrapperSync::error2( int id, int errorCode, const std::string& errorString )noexcept
	{
		WrapperLog::error( id, errorCode, errorString );
		bool handled = false;
		if( errorCode==502 )
			SendCurrentTime( TimePoint{} );
		if( id>0 )
		{
			if( (handled = _historicalData.Contains(id)) )
			{
				if( errorCode==162 )
					WrapperCache::historicalDataEnd( id, "", "" );
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
			//{ lock_guard l2{ _requestCallbacksMutex }; _requestCallbacks.erase( id ); }
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
	void WrapperSync::fundamentalData( TickerId reqId, const std::string& data )noexcept
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

	/*void WrapperSync::AddRequestHistoricalData( TickerId reqId, ReqHistoricalDataCallback fnctn, ErrorCallback errorCallback )noexcept
	{
		{
			unique_lock l{ _requestCallbacksMutex };
			_requestCallbacks.emplace( reqId, fnctn );
		}
		{
			lock_guard<mutex> l{ _errorCallbacksMutex };
			_errorCallbacks.emplace( reqId, errorCallback );
		}
	}*/
/*	void WrapperSync::AddRequestIds( ReqIdCallback& fnctn )noexcept
	{
		_requestIds.Push( fnctn );
	}*/

	void WrapperSync::contractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept
	{
		WrapperCache::contractDetails( reqId, contractDetails );
		_detailsData.Push( reqId, contractDetails );
	}
	void WrapperSync::contractDetailsEnd( int reqId )noexcept
	{
		WrapperCache::contractDetailsEnd( reqId );
		_detailsData.End( reqId );
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

	WrapperData<ibapi::ContractDetails>::Future WrapperSync::ContractDetailsPromise( ReqId reqId )noexcept
	{
		return _detailsData.Promise( reqId );
	}

	void WrapperSync::securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		securityDefinitionOptionalParameterSync( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
	}

	bool WrapperSync::securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		WrapperCache::securityDefinitionOptionalParameter( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
		var captured = _optionsData.Contains(reqId);
		if( captured )
		{
			var params = WrapperCache::ToOptionParam( exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
			Proto::Results::OptionParams a; a.set_exchange( exchange ); a.set_multiplier( multiplier ); a.set_trading_class( tradingClass ); a.set_underlying_contract_id( underlyingConId );
			for( var strike : strikes )
				a.add_strikes( strike );

			for( var& expiration : expirations )
				a.add_expirations( Contract::ToDay(expiration) );
			_optionsData.Push( reqId, a );
		}
		return captured;
	}

	void WrapperSync::securityDefinitionOptionalParameterEnd( int reqId )noexcept
	{
		securityDefinitionOptionalParameterEndSync( reqId );
	}

	bool WrapperSync::securityDefinitionOptionalParameterEndSync( int reqId )noexcept
	{
		WrapperCache::securityDefinitionOptionalParameterEnd( reqId );
		var captured = _optionsData.Contains(reqId);
		if( captured )
			_optionsData.End( reqId );
		return captured;
	}

	WrapperData<Proto::Results::OptionParams>::Future WrapperSync::SecDefOptParamsPromise( ReqId reqId )noexcept
	{
		return _optionsData.Promise( reqId );
	}
	WrapperItem<string>::Future WrapperSync::FundamentalDataPromise( ReqId reqId )noexcept
	{
		return _fundamentalData.Promise( reqId );
	}

	WrapperItem<map<string,double>>::Future WrapperSync::RatioPromise( ReqId reqId )noexcept
	{
		lock_guard l{_ratioMutex};
		_ratioValues.emplace( reqId, map<string,double>{} );
		return _ratioData.Promise( reqId );
	}

	WrapperData<ibapi::Bar>::Future WrapperSync::ReqHistoricalDataPromise( ReqId reqId )noexcept
	{
		//DBG( "({}) - Promise"sv, reqId );
		return _historicalData.Promise( reqId );
	}
	void WrapperSync::historicalData( TickerId reqId, const ibapi::Bar& bar )noexcept
	{
		historicalDataSync( reqId, bar );
	}
	bool WrapperSync::historicalDataSync( TickerId reqId, const ibapi::Bar& bar )noexcept
	{
		WrapperCache::historicalData( reqId, bar );
		var captured = _historicalData.Contains( reqId );
		if( captured )
			_historicalData.Push( reqId, bar );
		return captured;
	}
	void WrapperSync::historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept
	{
		historicalDataEndSync( reqId, startDateStr, endDateStr );
	}
	bool WrapperSync::historicalDataEndSync( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept
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
	}

	void WrapperSync::openOrderEnd()noexcept
	{
		WrapperLog::openOrderEnd();
		std::function<void(EndCallback&)> fnctn = []( function<void()>& f2 ){ f2(); };
		_openOrderEnds.ForEach( fnctn );
	}

	void WrapperSync::headTimestamp( int reqId, const std::string& headTimestamp )noexcept
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

	bool WrapperSync::TickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib )noexcept
	{
		WrapperLog::tickPrice( tickerId, field, price, attrib );
		var handled = _ratioData.Contains( tickerId );
		if( handled )
			AddRatioTick( tickerId, std::to_string(field), price );
		return handled;
	}
	bool WrapperSync::TickSize( TickerId tickerId, TickType field, int size )noexcept
	{
		WrapperLog::tickSize( tickerId, field, size );
		var handled = _ratioData.Contains( tickerId );
		if( handled )
			AddRatioTick( tickerId, std::to_string(field), size );
		return handled;
	}
	bool WrapperSync::TickString( TickerId tickerId, TickType tickType, const std::string& value )noexcept
	{
		WrapperLog::tickString( tickerId, tickType, value );
		var handled = _ratioData.Contains( tickerId );
		if( handled )
		{
			var values = StringUtilities::Split( value, ';' );
			for( var& value : values )
			{
				if( value.size()==0 )
					continue;
				var pair = StringUtilities::Split( value, '=' );
				if( pair.size()!=2 || pair[0]=="CURRENCY" )
					continue;
				try
				{
					AddRatioTick( tickerId, pair[0], stod(pair[1]) );
				}
				catch( std::invalid_argument& e )
				{
					DBG( "Could not convert [{}]='{}' to double."sv, pair[0], pair[1] );
				}
			}
		}
		return handled;
	}
	bool WrapperSync::TickGeneric( TickerId tickerId, TickType field, double value )noexcept
	{
		WrapperLog::tickGeneric( tickerId, field, value );
		var handled = _ratioData.Contains( tickerId );
		if( handled )
			AddRatioTick( tickerId, std::to_string(field), value );
		return handled;
	}
	void WrapperSync::AddRatioTick( TickerId tickerId, string_view key, double value )noexcept
	{
		lock_guard l{_ratioMutex};
		auto& values = _ratioValues[tickerId];
		values.emplace( key, value );
		if(	values.find("3")!=values.end()
			&& values.find("2")!=values.end()
			&& values.find("0")!=values.end()
			&& values.find("1")!=values.end()
			&& values.find("9")!=values.end()
			&& values.find("MKTCAP")!=values.end()
			&& values.find("NPRICE")!=values.end() )
		{
			TwsClientSync::Instance().cancelMktData( tickerId );
			_ratioData.Push( tickerId, make_shared<map<string,double>>(values) );
			_ratioValues.erase( tickerId );
		}
	}
}