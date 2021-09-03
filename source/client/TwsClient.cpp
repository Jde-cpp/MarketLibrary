#include "TwsClient.h"
#include "../TwsProcessor.h"
#include "../wrapper/WrapperLog.h"
#include "../types/Bar.h"
#include <jde/markets/types/Contract.h>
#include "../OrderManager.h"
#include "../../../Framework/source/Cache.h"

#define var const auto

namespace Jde::Markets
{
	using namespace Chrono;
	sp<TwsClient> TwsClient::_pInstance;
	ELogLevel TwsClient::_logLevel{ ELogLevel::Debug };
	void TwsClient::CreateInstance( const TwsConnectionSettings& settings, shared_ptr<EWrapper> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		if( _pInstance )
			DBG( "Creating new Instance of TwsClient, removing old."sv );
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
			if( eConnect(_settings.Host.c_str(), port, (int)clientId, false) )
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

	uint TwsClient::RequestAccountUpdates( sv acctCode, sp<IAccountUpdateHandler> callback )noexcept
	{
		LOG(_logLevel, "reqAccountUpdates( '{}', '{}' )", true, acctCode );
		auto [handle,subscribe] = WrapperLogPtr()->AddAccountUpdate( acctCode, callback );
		if( subscribe )
		 	EClient::reqAccountUpdates( true, string{acctCode} );
		return handle;
	}
	void TwsClient::CancelAccountUpdates( sv acctCode, Handle handle )noexcept
	{
		auto p = _pInstance; if( !p ) return;
		LOG(_logLevel, "({})CancelAccountUpdates( '{}' )", handle, acctCode );
		if( p->WrapperLogPtr()->RemoveAccountUpdate( acctCode, handle) )
		{
			LOG(_logLevel, "reqAccountUpdates( '{}', '{}' )", false, acctCode );
			p->reqAccountUpdates( false, string{acctCode} );
		}
	}
	void TwsClient::reqAccountUpdatesMulti( TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV )noexcept
	{
		LOG( _logLevel, "({})reqAccountUpdatesMulti( {}, {}, {} )", reqId, account, modelCode, ledgerAndNLV );
		EClient::reqAccountUpdatesMulti( reqId, account, modelCode, ledgerAndNLV );
	}
	void TwsClient::reqExecutions( int reqId, const ExecutionFilter& filter )noexcept
	{
		LOG( _logLevel, "({})reqExecutions( {}, {}, {} )", reqId, filter.m_acctCode, filter.m_time, filter.m_symbol );
		EClient::reqExecutions( reqId, filter );
	}
	void TwsClient::ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex endDay, DayIndex dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept
	{
		const DateTime endTime{ EndOfDay(FromDays(endDay)) };
		const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
		reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::TryToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
	}
	void TwsClient::reqHistoricalData( TickerId reqId, const ::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string&  barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept
	{
		var contractDisplay = contract.localSymbol.size() ? contract.localSymbol : std::to_string( contract.conId );
		var size = WrapperLogPtr()->HistoricalDataRequestSize();
		var send = size<_settings.MaxHistoricalDataRequest;
		LOG( _logLevel, "({})reqHistoricalData( '{}', '{}', '{}', '{}', '{}', useRth='{}', keepUpToDate='{}' ){}", reqId, contractDisplay, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH!=0, keepUpToDate, size/*send ? "" : "*"*/ );
		if( send )
		{
			ASSERT( durationStr!="0 D" );
			WrapperLogPtr()->AddHistoricalDataRequest2( reqId );
			if( contract.symbol=="CAT" )
				DBG( "{}"sv, endDateTime );
			EClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );
		}
		else
			_pWrapper->error( reqId, 322, format("Only '{}' historical data requests allowed at one time - {}.", _settings.MaxHistoricalDataRequest, size) );
	}
	void TwsClient::reqMktData( TickerId reqId, const ::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions )noexcept
	{
		LOG( _logLevel, "({})reqMktData( '{}', '{}', snapshot='{}', regulatorySnaphsot='{}' )", reqId, contract.conId, genericTicks, snapshot, regulatorySnaphsot );
		EClientSocket::reqMktData( reqId, contract, genericTicks, snapshot, regulatorySnaphsot, mktDataOptions );
	}

	void TwsClient::reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol, sv futFopExchange, sv underlyingSecType )noexcept
	{
		LOG( _logLevel, "({})reqSecDefOptParams( '{}', '{}', '{}', {} )", tickerId, underlyingSymbol, futFopExchange, underlyingSecType, underlyingConId );
		EClientSocket::reqSecDefOptParams( tickerId, string(underlyingSymbol), string(futFopExchange), string(underlyingSecType), underlyingConId );
	}
	void TwsClient::reqContractDetails( int reqId, const ::Contract& contract )noexcept
	{
		LOG( _logLevel, "({})reqContractDetails( '{}', '{}', '{}' )", reqId, (contract.conId==0 ? contract.symbol : std::to_string(contract.conId)), contract.secType, contract.lastTradeDateOrContractMonth );
		EClientSocket::reqContractDetails( reqId, contract );
	}
	void TwsClient::reqHeadTimestamp( int tickerId, const ::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept
	{
		LOG( _logLevel, "({})reqHeadTimestamp( '{}', '{}', useRTH:  '{}', formatDate:  '{}' )", tickerId, contract.conId, whatToShow, useRTH, formatDate );
		EClientSocket::reqHeadTimestamp( tickerId, contract, whatToShow, useRTH, formatDate );
	}
	void TwsClient::reqFundamentalData( TickerId tickerId, const ::Contract &contract, sv reportType )noexcept
	{
		LOG( _logLevel, "({})reqFundamentalData( '{}', '{}' )", tickerId, contract.conId, reportType );
		EClientSocket::reqFundamentalData( tickerId, contract, string{reportType}, TagValueListSPtr{} );
	}
	void TwsClient::reqNewsProviders()noexcept
	{
		LOG( _logLevel, "reqNewsProviders", ReqNewsProvidersLogId );
		EClientSocket::reqNewsProviders();
	}

	void TwsClient::reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept
	{
		var providers = Str::AddSeparators( providerCodes, "+"sv );
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

		var startString = toIBTime( start );
		var endString = toIBTime( end );
		LOG( _logLevel, "({})reqHistoricalNews( '{}', '{}', '{}', '{}', '{}' )", requestId, conId, providers, startString, endString, totalResults );

		EClientSocket::reqHistoricalNews( requestId, conId, providers, startString, endString, (int)totalResults, nullptr );
	}
	void TwsClient::reqNewsArticle( TickerId requestId, str providerCode, str articleId )noexcept
	{
		LOG( _logLevel, "({})reqNewsArticle( '{}', '{}' )", requestId, providerCode, articleId );
		EClientSocket::reqNewsArticle( requestId, providerCode, articleId, nullptr );
	}

	void TwsClient::reqCurrentTime()noexcept
	{
		LOG( _logLevel, "reqCurrentTime" );
		EClientSocket::reqCurrentTime();
	}

	void TwsClient::reqOpenOrders()noexcept
	{
		LOG( _logLevel, "reqOpenOrders" );
		EClientSocket::reqOpenOrders();
	}
	void TwsClient::reqAllOpenOrders()noexcept
	{
		LOG( _logLevel, "reqAllOpenOrders" );
		EClientSocket::reqAllOpenOrders();
	}
	void TwsClient::reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept
	{
		LOG( _logLevel, "({})reqRealTimeBars( {}, {}, {} )", id, contract.conId, barSize, whatToShow, useRTH );
		EClientSocket::reqRealTimeBars( id, contract, barSize, whatToShow, useRTH, realTimeBarsOptions );
	}
	void TwsClient::placeOrder( const ::Contract& contract, const ::Order& order )noexcept
	{
		var contractDisplay = format( "({}){}",  contract.symbol, contract.conId );
		LOG( _logLevel, "({})placeOrder( {}, {}, {}@{} )", order.orderId, contractDisplay, order.orderType, (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice );
		OrderManager::Push( order, contract );
		EClientSocket::placeOrder( order.orderId, contract, order );
	}

	void TwsClient::reqPositionsMulti( int reqId, const std::string& account, const std::string& modelCode )noexcept
	{
		LOG( _logLevel, "({})reqPositionsMulti( '{}', '{}' )", reqId, account, modelCode );
		EClientSocket::reqPositionsMulti( reqId, account, modelCode );
	}
}
