#include "TwsClient.h"
#include "../TwsProcessor.h"
#include "../wrapper/WrapperLog.h"
#include "../types/Bar.h"
#include "../types/Contract.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Chrono;
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
	shared_ptr<WrapperLog> TwsClient::WrapperLogPtr()noexcept
	{
		return std::dynamic_pointer_cast<WrapperLog>(_pWrapper);
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
	void TwsClient::reqExecutions( int reqId, const ExecutionFilter& filter )noexcept
	{
		LOG( _logLevel, "reqExecutions( {}, {}, {}, {} )"sv, reqId, filter.m_acctCode, filter.m_time, filter.m_symbol );
		EClient::reqExecutions( reqId, filter );
	}
	void TwsClient::ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex endDay, DayIndex dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept
	{
		const DateTime endTime{ EndOfDay(FromDays(endDay)) };
		const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
		reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::TryToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
	}
	void TwsClient::reqHistoricalData( TickerId reqId, const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string&  barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept
	{
		var contractDisplay = contract.localSymbol.size() ? contract.localSymbol : std::to_string( contract.conId );
		var size = WrapperLogPtr()->HistoricalDataRequestSize();
		var send = size<_settings.MaxHistoricalDataRequest;
		LOGN( _logLevel, "({})reqHistoricalData( '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}' ){}"sv, ReqHistoricalDataLogId, reqId, contractDisplay, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions, send ? "*" : "" );
		if( send )
		{
			WrapperLogPtr()->AddHistoricalDataRequest( reqId );
			EClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );
		}
		else
			_pWrapper->error( reqId, 322, format("Only '{}' historical data requests allowed at one time - {}.", _settings.MaxHistoricalDataRequest, size) );
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
	void TwsClient::reqNewsProviders()noexcept
	{
		LOGN0( _logLevel, "reqNewsProviders"sv, ReqNewsProvidersLogId );
		EClientSocket::reqNewsProviders();
	}

	void TwsClient::reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept
	{
		var providers = StringUtilities::AddSeparators( providerCodes, "+"sv );
		auto toIBTime = []( TimePoint t )
		{
			string result;
			if( t!=TimePoint{} )
			{
				const DateTime x{ t };
				result = format( "{}-{:0>2}-{:0>2} {:0>2}:{:0>2}:{:0>2}", x.Year(), x.Month(), x.Day(), x.Hour(), x.Minute(), x.Second() );
			}
			return result;
		};

		var startString = toIBTime( start ); var endString = toIBTime( end );

		LOG( _logLevel, "({})reqHistoricalNews( '{}', '{}', '{}', '{}', '{}' )"sv, requestId, conId, providers, startString, endString, totalResults );

		EClientSocket::reqHistoricalNews( requestId, conId, providers, startString, endString, totalResults, nullptr );
	}
	void TwsClient::reqNewsArticle( TickerId requestId, const string& providerCode, const string& articleId )noexcept
	{
		LOG( _logLevel, "({})reqNewsArticle( '{}', '{}' )"sv, requestId, providerCode, articleId );
		EClientSocket::reqNewsArticle( requestId, providerCode, articleId, nullptr );
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
		var contractDisplay = format( "({}){}",  contract.symbol, contract.conId );
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
