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
	const LogTag& TwsClient::_logLevel{ Logging::TagLevel("tws-requests") };
	void TwsClient::CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)
	{
		if( _pInstance )
			DBG( "Creating new Instance of TwsClient, removing old."sv );
		_pInstance = sp<TwsClient>( new TwsClient(settings, wrapper, pReaderSignal, clientId) );
		TwsProcessor::CreateInstance( _pInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //Make sure thread is still alive, ie not shutting down.  while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();
		DBG( "Connected to Tws Host='{}', Port'{}', Client='{}'"sv, settings.Host, _pInstance->_port, clientId );
	}

	TwsClient::TwsClient( const TwsConnectionSettings& settings, sp<EWrapper> pWrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		EClientSocket( pWrapper.get(), pReaderSignal.get() ),
		_pWrapper{ pWrapper },
		_settings{ settings }
	{
		for( var port : _settings.Ports )
		{
			TRACE( "Attempt to connect to Tws:  {}", port );
			if( eConnect(_settings.Host.c_str(), port, (int)clientId, false) )
			{
				INFO( "connected to Tws:  {}.", port );
				_port = port;
				break;
			}
			else
				INFO( "connect to Tws:  {} failed", port );
		}
		THROW_IF( !_port, "Could not connect to IB {}"sv, _settings );
	}

	sp<WrapperLog> TwsClient::WrapperLogPtr()noexcept
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
		LOG( "reqAccountUpdates( '{}', '{}' )", true, acctCode );
		auto [handle,subscribe] = WrapperLogPtr()->AddAccountUpdate( acctCode, callback );
		if( subscribe )
		 	EClient::reqAccountUpdates( true, string{acctCode} );
		return handle;
	}
	void TwsClient::CancelAccountUpdates( sv acctCode, Handle handle )noexcept
	{
		auto p = _pInstance; if( !p ) return;
		LOG( "({})CancelAccountUpdates( '{}' )", handle, acctCode );
		if( p->WrapperLogPtr()->RemoveAccountUpdate( acctCode, handle) )
		{
			LOG( "reqAccountUpdates( '{}', '{}' )", false, acctCode );
			p->reqAccountUpdates( false, string{acctCode} );
		}
	}
	void TwsClient::reqAccountUpdatesMulti( TickerId reqId, str account, str modelCode, bool ledgerAndNLV )noexcept
	{
		LOG( "({})reqAccountUpdatesMulti( {}, {}, {} )", reqId, account, modelCode, ledgerAndNLV );
		EClient::reqAccountUpdatesMulti( reqId, account, modelCode, ledgerAndNLV );
	}
	void TwsClient::reqExecutions( int reqId, const ExecutionFilter& filter )noexcept
	{
		LOG( "({})reqExecutions( {}, {}, {} )", reqId, filter.m_acctCode, filter.m_time, filter.m_symbol );
		EClient::reqExecutions( reqId, filter );
	}
	void TwsClient::ReqHistoricalData( TickerId reqId, const Contract& contract, Day endDay, Day dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept
	{
		const DateTime endTime{ EndOfDay(FromDays(endDay)) };
		const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
		reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::ToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
	}
	void TwsClient::reqHistoricalData( TickerId reqId, const ::Contract& contract, str endDateTime, str durationStr, str  barSizeSetting, str whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept
	{
		var contractDisplay = contract.localSymbol.size() ? contract.localSymbol : std::to_string( contract.conId );
		var size = WrapperLogPtr()->HistoricalDataRequestSize();
		var send = size<_settings.MaxHistoricalDataRequest;
		LOG( "({})reqHistoricalData( '{}', '{}', '{}', '{}', '{}', useRth='{}', keepUpToDate='{}' ){}", reqId, contractDisplay, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH!=0, keepUpToDate, size/*send ? "" : "*"*/ );
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
	void TwsClient::reqMktData( TickerId reqId, const ::Contract& contract, str genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions )noexcept
	{
		LOG( "({})reqMktData( '{}', '{}', snapshot='{}', regulatorySnaphsot='{}' )", reqId, contract.conId, genericTicks, snapshot, regulatorySnaphsot );
		EClientSocket::reqMktData( reqId, contract, genericTicks, snapshot, regulatorySnaphsot, mktDataOptions );
	}

	void TwsClient::reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol, sv futFopExchange, sv underlyingSecType )noexcept
	{
		LOG( "({})reqSecDefOptParams( '{}', '{}', '{}', {} )", tickerId, underlyingSymbol, futFopExchange, underlyingSecType, underlyingConId );
		EClientSocket::reqSecDefOptParams( tickerId, string(underlyingSymbol), string(futFopExchange), string(underlyingSecType), underlyingConId );
	}
	void TwsClient::reqContractDetails( int reqId, const ::Contract& contract )noexcept
	{
		LOG( "({})reqContractDetails( '{}', '{}', '{}' )", reqId, (contract.conId==0 ? contract.symbol : std::to_string(contract.conId)), contract.secType, contract.lastTradeDateOrContractMonth );
		EClientSocket::reqContractDetails( reqId, contract );
	}
	void TwsClient::reqHeadTimestamp( int tickerId, const ::Contract &contract, str whatToShow, int useRTH, int formatDate )noexcept
	{
		LOG( "({})reqHeadTimestamp( '{}', '{}', useRTH:  '{}', formatDate:  '{}' )", tickerId, contract.conId, whatToShow, useRTH, formatDate );
		EClientSocket::reqHeadTimestamp( tickerId, contract, whatToShow, useRTH, formatDate );
	}
	void TwsClient::reqFundamentalData( TickerId tickerId, const ::Contract &contract, sv reportType )noexcept
	{
		LOG( "({})reqFundamentalData( '{}', '{}' )", tickerId, contract.conId, reportType );
		EClientSocket::reqFundamentalData( tickerId, contract, string{reportType}, TagValueListSPtr{} );
	}
	void TwsClient::reqNewsProviders()noexcept
	{
		LOG( "reqNewsProviders", ReqNewsProvidersLogId );
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
		LOG( "({})reqHistoricalNews( '{}', '{}', '{}', '{}', '{}' )", requestId, conId, providers, startString, endString, totalResults );

		EClientSocket::reqHistoricalNews( requestId, conId, providers, startString, endString, (int)totalResults, nullptr );
	}
	void TwsClient::reqNewsArticle( TickerId requestId, str providerCode, str articleId )noexcept
	{
		LOG( "({})reqNewsArticle( '{}', '{}' )", requestId, providerCode, articleId );
		EClientSocket::reqNewsArticle( requestId, providerCode, articleId, nullptr );
	}

	void TwsClient::reqCurrentTime()noexcept
	{
		LOG( "reqCurrentTime" );
		EClientSocket::reqCurrentTime();
	}

	void TwsClient::reqOpenOrders()noexcept
	{
		LOG( "reqOpenOrders" );
		EClientSocket::reqOpenOrders();
	}
	void TwsClient::reqAllOpenOrders()noexcept
	{
		LOG( "reqAllOpenOrders" );
		EClientSocket::reqAllOpenOrders();
	}
	void TwsClient::reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, str whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept
	{
		LOG( "({})reqRealTimeBars( {}, {}, {} )", id, contract.conId, barSize, whatToShow, useRTH );
		EClientSocket::reqRealTimeBars( id, contract, barSize, whatToShow, useRTH, realTimeBarsOptions );
	}
	void TwsClient::placeOrder( const ::Contract& contract, const ::Order& order )noexcept
	{
		var contractDisplay = format( "({}){}",  contract.symbol, contract.conId );
		LOG( "({})placeOrder( {}, {}, {}@{} )", order.orderId, contractDisplay, order.orderType, (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice );
		OrderManager::Push( order, contract );
		EClientSocket::placeOrder( order.orderId, contract, order );
	}

	void TwsClient::reqPositionsMulti( int reqId, str account, str modelCode )noexcept
	{
		LOG( "({})reqPositionsMulti( '{}', '{}' )", reqId, account, modelCode );
		EClientSocket::reqPositionsMulti( reqId, account, modelCode );
	}
}
