#include "WrapperSync.h"
#include <EReaderOSSignal.h>
#include "../client/TwsClientSync.h"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../types/Contract.h"
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
	bool WrapperSync::error2( int id, int errorCode, const std::string& errorString )noexcept
	{
		bool handled = WrapperLog::error2( id, errorCode, errorString );
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
	void WrapperSync::CheckTimeouts()noexcept
	{
		_historicalData.CheckTimeouts();
	}
	void WrapperSync::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		WrapperCache::contractDetails( reqId, contractDetails );
		_detailsData.Push( reqId, contractDetails );
	}
	void WrapperSync::contractDetailsEnd( int reqId )noexcept
	{
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
	void WrapperSync::position( const std::string& account, const ::Contract& contract, double position, double avgCost )noexcept
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

	void WrapperSync::securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		securityDefinitionOptionalParameterSync( reqId, exchange, underlyingConId, tradingClass, multiplier, expirations, strikes );
	}

	bool WrapperSync::securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
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
	void WrapperSync::newsProviders( const vector<NewsProvider>& newsProviders )noexcept
	{
		WrapperCache::newsProviders( newsProviders );
		_newsProviderData.End( make_shared<vector<NewsProvider>>(newsProviders) );
	}
/*	void WrapperSync::newsProviders( const std::vector<NewsProvider>& providers, bool isCache )noexcept
	{
	}*/

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
		//std::function<void(EndCallback&)> fnctn = []( function<void()>& f2 ){ f2(); };
		//_openOrderEnds.ForEach( fnctn );
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

/*	bool WrapperSync::TickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib )noexcept
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
			for( var& subValue : values )
			{
				if( subValue.size()==0 )
					continue;
				if( tickType==TickType::IB_DIVIDENDS )
				{
					var dividendValues = StringUtilities::Split( subValue );
					if( subValue==",,," )
					{
						AddRatioTick( tickerId, "DIV_PAST_YEAR", 0.0 );
						AddRatioTick( tickerId, "DIV_NEXT_YEAR", 0.0 );
						AddRatioTick( tickerId, "DIV_NEXT_DAY", 0.0 );
						AddRatioTick( tickerId, "DIV_NEXT", 0.0 );
					}
					else if( dividendValues.size()!=4 )
						DBG( "({})Could not convert '{}' to dividends."sv, tickerId, subValue );
					else
					{
						AddRatioTick( tickerId, "DIV_PAST_YEAR", stod(dividendValues[0]) );
						AddRatioTick( tickerId, "DIV_NEXT_YEAR", stod(dividendValues[1]) );
						var& dateString = dividendValues[2];
						if( dateString.size()==8 )
						{
							const DateTime date( stoi(dateString.substr(0,4)), (uint8)stoi(dateString.substr(4,2)), (uint8)stoi(dateString.substr(6,2)) );
							AddRatioTick( tickerId, "DIV_NEXT_DAY", Chrono::DaysSinceEpoch(date.GetTimePoint()) );
						}
						else
							DBG( "({})Could not read next dividend day '{}'."sv, tickerId, dateString );
						AddRatioTick( tickerId, "DIV_NEXT", stod(dividendValues[3]) );
					}
				}
				else
				{
					var pair = StringUtilities::Split( subValue, '=' );
					if( pair.size()!=2 || pair[0]=="CURRENCY" )
						continue;
					try
					{
						AddRatioTick( tickerId, pair[0], stod(pair[1]) );
					}
					catch( std::invalid_argument& )
					{
						DBG( "Could not convert [{}]='{}' to double."sv, pair[0], pair[1] );
					}
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
			&& values.find("NPRICE")!=values.end()/ *
			&& values.find("AvgVolume")!=values.end()* / )
		{
			static set<TickerId> setTickers;
			if( setTickers.emplace(tickerId).second )//.contains(tickerId)
			{
				std::thread( [tickerId, this]()
				{
					std::this_thread::sleep_for( 1s );//TODO Remove this.
					TwsClientSync::Instance().cancelMktData( tickerId );
					lock_guard l{_ratioMutex};
					auto& values = _ratioValues[tickerId];
					_ratioData.Push( tickerId, make_shared<map<string,double>>(values) );
					_ratioValues.erase( tickerId );
				}).detach();
			}
		}
	}*/
}