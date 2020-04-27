#include "TwsClient.h"
#include "TwsProcessor.h"
#define var const auto

namespace Jde::Markets
{
	sp<TwsClient> TwsClient::_pInstance;
	void TwsClient::CreateInstance( const TwsConnectionSettings& settings, shared_ptr<EWrapper> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		if( _pInstance )
			DBG0( "Creating new Instance of TwsClient, removing old."sv );
		_pInstance = sp<TwsClient>( new TwsClient(settings, wrapper, pReaderSignal, clientId) );
		TwsProcessor::CreateInstance( _pInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //Make sure thread is still alive, ie not shutting down.  while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();
		DBG( "Connected to Tws Host='{}', Port'{}', Client='{}'"sv, settings.Host, _pInstance->_port, clientId );
	}

	TwsClient::TwsClient( const TwsConnectionSettings& settings, shared_ptr<EWrapper> pWrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		EClientSocket( pWrapper.get(), pReaderSignal.get() ),
		_pWrapper{ pWrapper },
		_settings{ settings }
	{
		for( var port : _settings.Ports )
		{
			DBG( "Attempt to connect to Tws:  {}"sv, port );
			if( eConnect(_settings.Host.c_str(), port, (int)clientId) )
			{
				DBG( "connected to Tws:  {}."sv, port );
				_port = port;
				break;
			}
			else
				DBG( "connect to Tws:  {} failed"sv, port );
		}
		if( !_port )
			THROW( Exception("Could not connect to IB {}"sv, _settings) );
	}

	void TwsClient::SetRequestId( TickerId id )noexcept
	{
		if( _requestId<id )
			_requestId = id;
	}

	void TwsClient::reqAccountUpdatesMulti( TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV )noexcept
	{
		LOG( _logLevel, "reqAccountUpdatesMulti( {}, {}, {}, {} )"sv, reqId, account, modelCode, ledgerAndNLV );
		EClient::reqAccountUpdatesMulti( reqId, account, modelCode, ledgerAndNLV );
	}
	void TwsClient::reqHistoricalData( TickerId reqId, const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string&  barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept
	{
		var contractDisplay = contract.localSymbol.size() ? contract.localSymbol : std::to_string( contract.conId );
		LOG( _logLevel, "reqHistoricalData( '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}' )"sv, reqId, contractDisplay, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );

		EClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );
	}
	void TwsClient::reqMktData( TickerId reqId, const ibapi::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions )noexcept
	{
		LOG( _logLevel, "reqMktData( '{}', '{}', '{}', snapshot='{}', regulatorySnaphsot='{}' )"sv, reqId, contract.conId, genericTicks, snapshot, regulatorySnaphsot );
		EClientSocket::reqMktData( reqId, contract, genericTicks, snapshot, regulatorySnaphsot, mktDataOptions );
	}

	void TwsClient::reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol, string_view futFopExchange, string_view underlyingSecType )noexcept
	{
		LOG( _logLevel, "reqSecDefOptParams( {}, '{}', '{}', '{}', {} )"sv, tickerId, underlyingSymbol, futFopExchange, underlyingSecType, underlyingConId );
		EClientSocket::reqSecDefOptParams( tickerId, string(underlyingSymbol), string(futFopExchange), string(underlyingSecType), underlyingConId );
	}
	void TwsClient::reqContractDetails( int reqId, const ibapi::Contract& contract )noexcept
	{
		LOG( _logLevel, "reqContractDetails( {}, '{}', '{}', '{}' )"sv, reqId, (contract.conId==0 ? contract.symbol : std::to_string(contract.conId)), contract.secType, contract.lastTradeDateOrContractMonth );
		EClientSocket::reqContractDetails( reqId, contract );
	}
	void TwsClient::reqHeadTimestamp( int tickerId, const ibapi::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept
	{
		LOG( _logLevel, "reqHeadTimestamp( '{}', '{}', '{}', useRTH:  '{}', formatDate:  '{}' )"sv, tickerId, contract.conId, whatToShow, useRTH, formatDate );
		EClientSocket::reqHeadTimestamp( tickerId, contract, whatToShow, useRTH, formatDate );
	}
	void TwsClient::reqFundamentalData( TickerId tickerId, const ibapi::Contract &contract, string_view reportType )noexcept
	{
		LOG( _logLevel, "reqFundamentalData( '{}', '{}', '{}' )"sv, tickerId, contract.conId, reportType );
		EClientSocket::reqFundamentalData( tickerId, contract, string{reportType}, TagValueListSPtr{} );
	}


	void TwsClient::reqCurrentTime()noexcept
	{
		LOG0( _logLevel, "reqCurrentTime"sv );
		EClientSocket::reqCurrentTime();
	}

	void TwsClient::reqOpenOrders()noexcept
	{
		LOG0( _logLevel, "reqOpenOrders"sv );
		EClientSocket::reqOpenOrders();
	}
	void TwsClient::reqAllOpenOrders()noexcept
	{
		LOG0( _logLevel, "reqAllOpenOrders"sv );
		EClientSocket::reqAllOpenOrders();
	}

	void TwsClient::placeOrder( const ibapi::Contract& contract, const ibapi::Order& order )noexcept
	{
		var contractDisplay = fmt::format( "({}){}",  contract.symbol, contract.conId );
		LOG( _logLevel, "({})placeOrder( {}, {}, {}@{} )"sv, order.orderId, contractDisplay, order.orderType, (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice );
		ibapi::Order order2;
		order2.orderId = order.orderId;
		order2.orderType = order.orderType;
		order2.lmtPrice = order.lmtPrice;
		order2.totalQuantity = order.totalQuantity;
		order2.action = order.action;
		order2.tif = order.tif;

		EClientSocket::placeOrder( order.orderId, contract, order );
	}
}
