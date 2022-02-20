#include "TwsClient.h"
#include "../TwsProcessor.h"
#include "../data/Accounts.h"
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
	TwsConnectionSettings _settings;
	α TwsClient::MaxHistoricalDataRequest()noexcept->uint8{ return _settings.MaxHistoricalDataRequest; }

	α TwsClient::CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)->void
	{
		if( _pInstance )
			DBG( "Creating new Instance of TwsClient, removing old."sv );

		_pInstance = sp<TwsClient>( new TwsClient(settings, wrapper, pReaderSignal, clientId) );
		TwsProcessor::CreateInstance( _pInstance, pReaderSignal );
		while( !_pInstance->isConnected() ) //Make sure thread is still alive, ie not shutting down.  while( !TwsProcessor::IsConnected() )
			std::this_thread::yield();
		_pInstance->reqIds();
		while( _pInstance->_requestId==0 )
			std::this_thread::yield();

		INFO( "Connected to Tws Host='{}', Port'{}', Client='{}' nextId='{}'"sv, settings.Host, _pInstance->_port, clientId, _pInstance->_requestId );
	}

	TwsClient::TwsClient( const TwsConnectionSettings& settings, sp<EWrapper> pWrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false):
		EClientSocket( pWrapper.get(), pReaderSignal.get() ),
		_pWrapper{ pWrapper }
	{
		_settings = settings;
		AccountAuthorizer::Initialize();
		for( var port : _settings.Ports )
		{
			TRACE( "Attempt to connect to Tws:  {}", port );
			if( eConnect(_settings.Host.c_str(), port, (int)clientId, false) )
			{
				//INFO( "connected to Tws:  {}.", port );
				_port = port;
				break;
			}
			else
				INFO( "connect to Tws:  {} failed", port );
		}
		THROW_IF( !_port, "Could not connect to IB {}", _settings );
	}

	α TwsClient::WrapperLogPtr()noexcept->sp<WrapperLog>
	{
		return std::dynamic_pointer_cast<WrapperLog>( _pWrapper );
	}
	α TwsClient::SetRequestId( TickerId id )noexcept->void
	{
		if( _requestId<id )
			_requestId = id;
	}

	α TwsClient::RequestAccountUpdates( str acctCode, sp<IAccountUpdateHandler> callback )noexcept->uint
	{
		LOG( "reqAccountUpdates( '{}', '{}' )", true, acctCode );
		auto [handle,subscribe] = WrapperLogPtr()->AddAccountUpdate( acctCode, callback );
		if( subscribe )
		 	EClient::reqAccountUpdates( true, string{acctCode} );
		return handle;
	}
	α TwsClient::CancelAccountUpdates( str acctCode, Handle handle )noexcept->void
	{
		auto p = _pInstance; if( !p ) return;
		if( p->WrapperLogPtr()->RemoveAccountUpdate(acctCode, handle) )
		{
			LOG( "reqAccountUpdates( '{}', '{}' )", false, acctCode );
			p->reqAccountUpdates( false, acctCode );
		}
	}
	α TwsClient::reqAccountUpdatesMulti( TickerId reqId, str account, str modelCode, bool ledgerAndNLV )noexcept->void
	{
		LOG( "({})reqAccountUpdatesMulti( {}, {}, {} )", reqId, account, modelCode, ledgerAndNLV );
		EClient::reqAccountUpdatesMulti( reqId, account, modelCode, ledgerAndNLV );
	}
	α TwsClient::reqExecutions( int reqId, const ExecutionFilter& filter )noexcept->void
	{
		LOG( "({})reqExecutions( {}, {}, {} )", reqId, filter.m_acctCode, filter.m_time, filter.m_symbol );
		EClient::reqExecutions( reqId, filter );
	}
	α TwsClient::ReqHistoricalData( TickerId reqId, const Contract& contract, Day endDay, Day dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth, SL sl )noexcept->void
	{
		const DateTime endTime{ EndOfDay(FromDays(endDay)) };
		const string endTimeString{ format("{}{:0>2}{:0>2} {:0>2}:{:0>2}:{:0>2} GMT", endTime.Year(), endTime.Month(), endTime.Day(), endTime.Hour(), endTime.Minute(), endTime.Second()) };
		reqHistoricalData( reqId, *contract.ToTws(), endTimeString, format("{} D", dayCount), string{BarSize::ToString((BarSize::Enum)barSize)}, string{TwsDisplay::ToString(display)}, useRth ? 1 : 0, 2, false, TagValueListSPtr{}, sl );
	}
	α TwsClient::reqHistoricalData( TickerId reqId, const ::Contract& contract, str endDateTime, str durationStr, str  barSizeSetting, str whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions, SL sl )noexcept(false)->void
	{
		ASSERT( durationStr!="0 D" );
		var contractDisplay = contract.localSymbol.size() ? contract.localSymbol : std::to_string( contract.conId );
		var size = WrapperLogPtr()->HistoricalDataRequestSize();
		LOGSL( "({})reqHistoricalData( '{}', '{}', '{}', '{}', '{}', useRth='{}', keepUpToDate='{}' ){}", reqId, contractDisplay, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH!=0, keepUpToDate, size/*send ? "" : "*"*/ );
		if( size>_settings.MaxHistoricalDataRequest )
			throw IBException{ format("Only '{}' historical data requests allowed at one time - {}.", TwsClient::MaxHistoricalDataRequest(), size), 322, reqId };
		WrapperLogPtr()->AddHistoricalDataRequest2( reqId );
		EClient::reqHistoricalData( reqId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, keepUpToDate, chartOptions );
	}
	α TwsClient::reqMktData( TickerId reqId, const ::Contract& contract, str genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions, SL sl )noexcept->void
	{
		LOGSL( "({})reqMktData( '{}', '{}', snapshot='{}', regulatorySnaphsot='{}' )", reqId, contract.conId, genericTicks, snapshot, regulatorySnaphsot );
		EClientSocket::reqMktData( reqId, contract, genericTicks, snapshot, regulatorySnaphsot, mktDataOptions );
	}

	α TwsClient::reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol, sv futFopExchange, sv underlyingSecType )noexcept->void
	{
		LOG( "({})reqSecDefOptParams( '{}', '{}', '{}', {} )", tickerId, underlyingSymbol, futFopExchange, underlyingSecType, underlyingConId );
		EClientSocket::reqSecDefOptParams( tickerId, string(underlyingSymbol), string(futFopExchange), string(underlyingSecType), underlyingConId );
	}
	α TwsClient::reqContractDetails( int reqId, const ::Contract& contract, SL sl )noexcept->void
	{
		LOGSL( "({})reqContractDetails( '{}', '{}', '{}' )", reqId, (contract.conId==0 ? contract.symbol : std::to_string(contract.conId)), contract.secType, contract.lastTradeDateOrContractMonth );
		EClientSocket::reqContractDetails( reqId, contract );
	}
	α TwsClient::reqHeadTimestamp( int tickerId, const ::Contract &contract, str whatToShow, int useRTH, int formatDate )noexcept->void
	{
		LOG( "({})reqHeadTimestamp( '{}', '{}', useRTH:  '{}', formatDate:  '{}' )", tickerId, contract.conId, whatToShow, useRTH, formatDate );
		EClientSocket::reqHeadTimestamp( tickerId, contract, whatToShow, useRTH, formatDate );
	}
	α TwsClient::reqFundamentalData( TickerId tickerId, const ::Contract &contract, sv reportType )noexcept->void
	{
		LOG( "({})reqFundamentalData( '{}', '{}' )", tickerId, contract.conId, reportType );
		EClientSocket::reqFundamentalData( tickerId, contract, string{reportType}, TagValueListSPtr{} );
	}
	α TwsClient::reqNewsProviders()noexcept->void
	{
		LOG( "reqNewsProviders", ReqNewsProvidersLogId );
		EClientSocket::reqNewsProviders();
	}

	α TwsClient::reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start, TimePoint end )noexcept->void
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
	α TwsClient::reqNewsArticle( TickerId requestId, str providerCode, str articleId )noexcept->void
	{
		LOG( "({})reqNewsArticle( '{}', '{}' )", requestId, providerCode, articleId );
		EClientSocket::reqNewsArticle( requestId, providerCode, articleId, nullptr );
	}

	α TwsClient::reqCurrentTime()noexcept->void
	{
		LOG( "reqCurrentTime" );
		EClientSocket::reqCurrentTime();
	}

	α TwsClient::reqOpenOrders()noexcept->void
	{
		LOG( "reqOpenOrders" );
		EClientSocket::reqOpenOrders();
	}
	α TwsClient::reqAllOpenOrders()noexcept->void
	{
		LOG( "reqAllOpenOrders" );
		EClientSocket::reqAllOpenOrders();
	}
	α TwsClient::reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, str whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept->void
	{
		LOG( "({})reqRealTimeBars( {}, {}, {} )", id, contract.conId, barSize, whatToShow, useRTH );
		EClientSocket::reqRealTimeBars( id, contract, barSize, whatToShow, useRTH, realTimeBarsOptions );
	}
	α TwsClient::placeOrder( const ::Contract& contract, const ::Order& order, SL sl )noexcept->void
	{
		var contractDisplay = format( "({}){}",  contract.symbol, contract.conId );
		LOGSL( "({})placeOrder( {}, {}, {}@{} )", order.orderId, contractDisplay, order.orderType, (order.action=="BUY" ? 1 : -1 )*ToDouble(order.totalQuantity), order.lmtPrice );
		OrderManager::Push( order, contract );
		EClientSocket::placeOrder( order.orderId, contract, order );
	}

	α TwsClient::reqPositionsMulti( int reqId, str account, str modelCode )noexcept->void
	{
		LOG( "({})reqPositionsMulti( '{}', '{}' )", reqId, account, modelCode );
		EClientSocket::reqPositionsMulti( reqId, account, modelCode );
	}
}