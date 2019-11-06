#include "stdafx.h"
#include "TwsClient.h"
#include "TwsProcessor.h"
#define var const auto

namespace Jde::Markets
{
	sp<TwsClient> TwsClient::_pInstance;
	void TwsClient::CreateInstance( const TwsConnectionSettings& settings, shared_ptr<EWrapper> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		if( _pInstance )
			DBG0( "Creating new Instance of TwsClient, removing old." );
		_pInstance = sp<TwsClient>( new TwsClient(settings, wrapper, pReaderSignal, clientId) );
		TwsProcessor::CreateInstance( _pInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //Make sure thread is still alive, ie not shutting down.  while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();
		DBG( "Connected to Tws Host='{}', Port'{}', Client='{}'", settings.Host, _pInstance->_port, clientId );
	}

	TwsClient::TwsClient( const TwsConnectionSettings& settings, shared_ptr<EWrapper> pWrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		EClientSocket( pWrapper.get(), pReaderSignal.get() ),
		_pWrapper{ pWrapper },
		_settings{ settings }
	{
		for( var port : _settings.Ports )
		{
			DBG( "Attempt to connect to Tws:  {}", port );
			if( eConnect(_settings.Host.c_str(), port, (int)clientId) )
			{
				DBG( "connected to Tws:  {}.", port );
				_port = port;
				break;
			}
			else
				DBG( "connect to Tws:  {} failed", port );
		}
		if( !_port )
			THROW( Exception("Could not connect to IB {}", _settings) );
	}

	void TwsClient::SetRequestId( TickerId id )noexcept
	{
		if( _requestId<id )
			_requestId = id;
	}

	void TwsClient::reqAccountUpdatesMulti( TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV )noexcept
	{
		LOG( _logLevel, "reqAccountUpdatesMulti( {}, {}, {}, {} )", reqId, account, modelCode, ledgerAndNLV );
		EClient::reqAccountUpdatesMulti( reqId, account, modelCode, ledgerAndNLV );
	}
	void TwsClient::reqHistoricalData( TickerId reqId, const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string&  barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept
	{
		LOG( _logLevel, "reqHistoricalData( '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}' )", reqId, contract.conId, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );

		EClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );
	}
	void TwsClient::reqMktData( TickerId tickerId, const ibapi::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions )noexcept
	{ 
		LOG( _logLevel, "reqMktData( '{}', '{}', '{}', snapshot='{}', regulatorySnaphsot='{}' )", tickerId, contract.conId, genericTicks, snapshot, regulatorySnaphsot );
		EClientSocket::reqMktData( tickerId, contract, genericTicks, snapshot, regulatorySnaphsot, mktDataOptions ); 
	}
	
	void TwsClient::reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol, string_view futFopExchange, string_view underlyingSecType )noexcept
	{
		LOG( _logLevel, "reqSecDefOptParams( '{}', '{}', '{}', '{}', '{}' )", tickerId, underlyingSymbol, futFopExchange, underlyingSecType, underlyingConId );
		EClientSocket::reqSecDefOptParams( tickerId, string(underlyingSymbol), string(futFopExchange), string(underlyingSecType), underlyingConId ); 
	}
	void TwsClient::reqContractDetails( int reqId, const ibapi::Contract& contract )noexcept
	{
		LOG( _logLevel, "reqContractDetails( '{}', '{}', '{}', '{}' )", reqId, contract.conId, contract.secType, contract.localSymbol );
		EClientSocket::reqContractDetails( reqId, contract );
	}
	void TwsClient::reqHeadTimestamp( int tickerId, const ibapi::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept
	{
		LOG( _logLevel, "reqHeadTimestamp( '{}', '{}', '{}', useRTH:  '{}', formatDate:  '{}' )", tickerId, contract.conId, whatToShow, useRTH, formatDate );
		EClientSocket::reqHeadTimestamp( tickerId, contract, whatToShow, useRTH, formatDate );
	}

	void TwsClient::reqCurrentTime()noexcept
	{
		LOG0( _logLevel, "reqCurrentTime" );
		EClientSocket::reqCurrentTime();
	}
}
