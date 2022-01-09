#include "WrapperLog.h"
#include <CommissionReport.h>
#include <TwsSocketClientErrors.h>
#include "../../../Framework/source/Cache.h"
#include "../OrderManager.h"
#include "../client/TwsClient.h"
#include "../types/Bar.h"

#define var const auto
#define _client auto pClient = TwsClient::InstancePtr(); if( pClient ) (*pClient)

namespace Jde::Markets
{
	unique_lock<shared_mutex>* _pUpdateLock{ nullptr };

	const LogTag& WrapperLog::_logLevel{ Logging::TagLevel("tws-results") };
	const LogTag& WrapperLog::_historicalLevel{ Logging::TagLevel("tws-hist") };
	const LogTag& WrapperLog::_tickLevel{ Logging::TagLevel("tws-tick") };

	α WrapperLog::error2( int id, int code, str m )noexcept->bool
	{
		WrapperLog::error( id, code, m );
		if( id>0 && code!=322 )
		/*auto t =*/ OrderManager::Push( id, code, m );//todo find out if found.
		return id==-1 || _pTickWorker->HandleError( id, code, m );
	}
#define $ noexcept->void
	α WrapperLog::error( int id, int errorCode, str errorMsg )$
	{
		LOG_IFT( _historicalDataRequests.erase(id),  _historicalLevel, "({})_historicalDataRequests.erase(){}", id, _historicalDataRequests.size() );
		LOG_IF( errorCode!=2106, "({})WrapperLog::error( {}, {} )", id, errorCode, errorMsg );
		ERR_IF( errorCode==SOCKET_EXCEPTION.code(), "Disconnected:  {} - {}", errorCode, errorMsg );
		ERR_IF( errorCode==UNSUPPORTED_VERSION.code(), "Disconnected:  {} - {}", errorCode, errorMsg );
		ERR_IF( errorCode==NOT_CONNECTED.code(), "Not Connected:  {} - {}", errorCode, errorMsg );
	}
	α WrapperLog::connectAck()${ LOG( "WrapperLog::connectAck()"); }

	α WrapperLog::connectionClosed()${ LOG( "WrapperLog::connectionClosed()" ); }
	α WrapperLog::bondContractDetails( int reqId, const ::ContractDetails& contractDetails )${ LOG( "WrapperLog::bondContractDetails( {}, {} )", reqId, contractDetails.contract.conId); }
	α WrapperLog::contractDetails( int reqId, const ::ContractDetails& contractDetails )$
	{
		LOG( "({})WrapperLog::contractDetails( '{}', '{}', '{}', '{}', '{}' )", reqId, contractDetails.contract.conId, contractDetails.contract.localSymbol, contractDetails.contract.symbol, contractDetails.contract.right, contractDetails.realExpirationDate );
	}
	α WrapperLog::contractDetailsEnd( int reqId )${ LOG( "({})WrapperLog::contractDetailsEnd()", reqId ); }
	α WrapperLog::execDetails( int reqId, const ::Contract& contract, const Execution& execution )${ LOG( "({})WrapperLog::execDetails( {}, {}, {}, {}, {} )", reqId, contract.symbol, execution.acctNumber, execution.side, execution.shares, execution.price );	}
	α WrapperLog::execDetailsEnd( int reqId )${ LOG( "WrapperLog::execDetailsEnd( {} )", reqId ); }
	α WrapperLog::historicalData( TickerId reqId, const ::Bar& bar )$
	{
		LOGT( _historicalLevel, "({})hstrclData( '{}', count: '{}', volume: '{}', wap: '{}', open: '{}', close: '{}', high: '{}', low: '{}' )", reqId, bar.time.starts_with("20") ? DateDisplay(DateTime{ConvertIBDate(bar.time)}) : Chrono::Display(ConvertIBDate(bar.time)), bar.count, ToDouble(bar.volume), ToDouble(bar.wap), bar.open, bar.close, bar.high, bar.low );
	}
	α WrapperLog::historicalDataEnd( int reqId, str startDateStr, str endDateStr )$
	{
		_historicalDataRequests.erase( reqId );
		var size = _historicalDataRequests.size();
		if( startDateStr.size() || endDateStr.size() )
			LOGT( _historicalLevel, "({}){}WrapperLog::historicalDataEnd( {}, {} )", reqId, size, startDateStr, endDateStr );
		else
			LOGT( _historicalLevel, "({})WrapperLog::historicalDataEnd(){}", reqId, size, startDateStr, endDateStr );
	}
	α WrapperLog::managedAccounts( str accountsList )${ LOG( "WrapperLog::managedAccounts( {} )"sv, accountsList ); }
	α WrapperLog::nextValidId( ::OrderId orderId )${ LOG( "WrapperLog::nextValidId( '{}' )", orderId ); }
#pragma region Order
	α WrapperLog::orderStatus( ::OrderId orderId, str status, ::Decimal filled, ::Decimal remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, str whyHeld, double mktCapPrice )$
	{
		OrderManager::Push( orderId, status, ToDouble(filled), ToDouble(remaining), avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice );
		LOG( "WrapperLog::orderStatus( {}, {}, {}/{} )"sv, orderId, status, ToDouble(filled), ToDouble(filled)+ToDouble(remaining) );
	}
	α toString( const ::Order& order ){ return fmt::format( "{}x{}" , (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice ); };
	α WrapperLog::openOrder( ::OrderId orderId, const ::Contract& contract, const ::Order& order, const ::OrderState& orderState )$
	{
		OrderManager::Push( order, contract, orderState );
		LOG( "WrapperLog::openOrder( {}, {}@{}, {} )", orderId, contract.symbol, toString(order), orderState.status );
	}
	α WrapperLog::openOrderEnd()${ LOG( "WrapperLog::openOrderEnd()"); }
#pragma endregion
	α WrapperLog::realtimeBar( TickerId reqId, long time, double open, double high, double low, double close, ::Decimal volume, ::Decimal wap, int count )${}
	α WrapperLog::receiveFA(faDataType pFaDataType, str cxml)${ LOG( "WrapperLog::receiveFA( {}, {} )", pFaDataType, cxml); }
	α WrapperLog::tickByTickAllLast(int reqId, int tickType, time_t time, double price, ::Decimal /*size*/, const TickAttribLast& /*attribs*/, str /*exchange*/, str /*specialConditions*/)${ LOG( "WrapperLog::tickByTickAllLast( {}, {}, {}, {} )", reqId, tickType, time, price);  }
	α WrapperLog::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, ::Decimal /*bidSize*/, ::Decimal /*askSize*/, const TickAttribBidAsk& /*attribs*/)${ LOG( "WrapperLog::tickByTickBidAsk( {}, {}, {}, {} )", reqId, time, bidPrice, askPrice); }
	α WrapperLog::tickByTickMidPoint(int reqId, time_t time, double midPoint)${ LOG( "WrapperLog::tickByTickMidPoint( {}, {}, {} )", reqId, time, midPoint); }
	α WrapperLog::tickReqParams( int tickerId, double minTick, str bboExchange, int snapshotPermissions )${ LOGL( ELogLevel::Trace, "WrapperLog::tickReqParams( {}, {}, {}, {} )", tickerId, minTick, bboExchange, snapshotPermissions ); }
	α WrapperLog::updateAccountValue2( sv key, sv val, sv currency, sv accountName )noexcept->bool
	{
		unique_lock l{ _accountUpdateCallbackMutex };
		_pUpdateLock = &l;

		bool haveCallback = false;
		if( auto p  = _accountUpdateCallbacks.find(string{accountName}); p!=_accountUpdateCallbacks.end() )
		{
			haveCallback = p->second.size();
			std::for_each( p->second.begin(), p->second.end(), [&](auto x){ x.second->UpdateAccountValue(key, val, currency, accountName); } );
		}
		auto& accountValues = _accountUpdateCache.try_emplace( string{accountName} ).first->second;
		accountValues[string{key}] = make_tuple( string{val}, string{currency} );
		return haveCallback;
	}
	α WrapperLog::updateAccountValue( str key, str val, str currency, str accountName )$
	{
		LOG( "updateAccountValue( {}, {}, {}, {} )", key, val, currency, accountName );
		updateAccountValue2( key, val, currency, accountName );
	}
	α WrapperLog::accountDownloadEnd( str accountName )$
	{
		LOG( "WrapperLog::accountDownloadEnd( {} )", accountName);
		shared_lock l{ _accountUpdateCallbackMutex };
		if( auto p  = _accountUpdateCallbacks.find(accountName); p!=_accountUpdateCallbacks.end() )
			std::for_each( p->second.begin(), p->second.end(), [&](auto& x){ x.second->AccountDownloadEnd(accountName); } );
	}

	α WrapperLog::updatePortfolio( const ::Contract& contract, ::Decimal position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, str accountNumber )$
	{
		LOGL( ELogLevel::Trace, "WrapperLog::updatePortfolio( {}, {}, {}, {}, {}, {}, {}, {} )", contract.symbol, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountNumber);
		Proto::Results::PortfolioUpdate update;
		unique_lock l{ _accountUpdateCallbackMutex };
		auto p  = _accountUpdateCallbacks.find( string{accountNumber} );
		if( p==_accountUpdateCallbacks.end() || p->second.empty() )
		{
			if( p!=_accountUpdateCallbacks.end() )
				_accountPortfolioUpdates.erase( string{accountNumber} );
			LOGL( ELogLevel::Trace, "WrapperLog::updatePortfolio - no callbacks canceling" );
			return TwsClient::CancelAccountUpdates( accountNumber, 0 );
		}

		Contract myContract{ contract };
		update.set_allocated_contract( myContract.ToProto().release() );
		update.set_position( ToDouble(position) );
		update.set_market_price( marketPrice );
		update.set_market_value( marketValue );
		update.set_average_cost( averageCost );
		update.set_unrealized_pnl( unrealizedPNL );
		update.set_realized_pnl( realizedPNL );
		update.set_account_number( accountNumber );
		if( myContract.SecType==SecurityType::Option )
		{
			const string cacheId{ format("reqContractDetails.{}", myContract.Symbol) };
			if( Cache::Has(cacheId) )
			{
				var details = Cache::Get<vector<::ContractDetails>>( cacheId );
				if( details->size()==1 )
					update.mutable_contract()->set_underlying_id( details->front().underConId );
				else
					WARN( "'{}' returned multiple securities"sv, myContract.Symbol );
			}
		}
		std::for_each( p->second.begin(), p->second.end(), [&](auto x){ x.second->PortfolioUpdate(update); } );
		_accountPortfolioUpdates[accountNumber][contract.conId]=update;
	}
	α WrapperLog::updateAccountTime(str timeStamp)${ LOGL( ELogLevel::Trace, "WrapperLog::updateAccountTime( {} )", timeStamp); }
	α WrapperLog::updateMktDepth(TickerId id, int position, int operation, int side, double /*price*/, ::Decimal /*size*/)${ LOG( "WrapperLog::updateMktDepth( {}, {}, {}, {} )", id, position, operation, side); }
	α WrapperLog::updateMktDepthL2(TickerId id, int position, str marketMaker, int operation, int /*side*/, double /*price*/, ::Decimal /*size*/, bool isSmartDepth)${ LOG( "WrapperLog::updateMktDepthL2( {}, {}, {}, {}, {} )", id, position, marketMaker, operation, isSmartDepth); }
	α WrapperLog::updateNewsBulletin(int msgId, int msgType, str newsMessage, str originExch)${ LOG( "WrapperLog::updateNewsBulletin( {}, {}, {}, {} )", msgId, msgType, newsMessage, originExch); }
	α WrapperLog::scannerParameters(str xml)${ LOG( "WrapperLog::scannerDataEnd( {} )", xml ); }
	α WrapperLog::scannerData(int reqId, int rank, const ::ContractDetails& contractDetails, str distance, str /*benchmark*/, str /*projection*/, str /*legsStr*/)${ LOG( "WrapperLog::scannerData( {}, {}, {}, {} )", reqId, rank, contractDetails.contract.conId, distance ); }
	α WrapperLog::scannerDataEnd(int reqId)${ LOG( "WrapperLog::scannerDataEnd( {} )", reqId ); }
	α WrapperLog::currentTime( long time )$
	{
		LOG( "WrapperLog::currentTime( {} )", ToIsoString(Clock::from_time_t(time)) );
	}
	α WrapperLog::accountSummaryEnd( int reqId )${ LOG( "WrapperLog::accountSummaryEnd( {} )", reqId ); }
	α WrapperLog::positionMultiEnd( int reqId )${ LOG( "WrapperLog::positionMultiEnd( {} )", reqId ); }
#pragma region accountUpdateMulti
	α WrapperLog::accountUpdateMulti( int reqId, str account, str modelCode, str key, str value, str currency )${ LOGL( ELogLevel::Trace, "WrapperLog::accountUpdateMulti( {}, {}, {}, {}, {}, {} )", reqId, account, modelCode, key, value, currency ); }
	α WrapperLog::accountUpdateMultiEnd( int reqId )${ LOGL( ELogLevel::Trace, "WrapperLog::accountUpdateMultiEnd( {} )", reqId ); }
#pragma endregion
	α WrapperLog::securityDefinitionOptionalParameter(int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )$
	{
		LOG( "WrapperLog::securityDefinitionOptionalParameter( {}, '{}', {}, '{}', {}, {}, {} )", reqId, exchange, underlyingConId, tradingClass, multiplier, expirations.size(), strikes.size() );
	}
	α WrapperLog::securityDefinitionOptionalParameterEnd( int reqId )${ LOG( "WrapperLog::securityDefinitionOptionalParameterEnd( {} )", reqId ); }
	α WrapperLog::fundamentalData(TickerId reqId, str data)${ LOG( "WrapperLog::fundamentalData( {}, {} )", reqId, data ); }
	α WrapperLog::deltaNeutralValidation(int reqId, const ::DeltaNeutralContract& deltaNeutralContract)${ LOG( "WrapperLog::deltaNeutralValidation({},{})", reqId, deltaNeutralContract.conId); }
	α WrapperLog::marketDataType( TickerId tickerId, int marketDataType )${ }
	α WrapperLog::commissionReport( const CommissionReport& commissionReport )${ LOG( "WrapperLog::commissionReport( {} )", commissionReport.commission ); }
#pragma region Poision
	α WrapperLog::position( str account, const ::Contract& contract, ::Decimal position, double avgCost )${ LOG( "WrapperLog::position( {}, {}, {}, {}, {} )", account, contract.conId, contract.symbol, position, avgCost ); }
	α WrapperLog::positionEnd()${ LOG( "WrapperLog::positionEnd()" ); }
#pragma endregion
	α WrapperLog::accountSummary( int reqId, str account, str tag, str value, str /*curency*/)${ LOG( "WrapperLog::accountSummary({},{},{},{})", reqId, account, tag, value); }
	α WrapperLog::verifyMessageAPI( str apiData )${ LOG( "WrapperLog::verifyMessageAPI({})", apiData); }
	α WrapperLog::verifyCompleted( bool isSuccessful, str errorText)${ LOG( "WrapperLog::verifyCompleted( {},{} )", isSuccessful, errorText); }
	α WrapperLog::displayGroupList( int reqId, str groups )${ LOG( "WrapperLog::displayGroupList( {}, {} )", reqId, groups); }
	α WrapperLog::displayGroupUpdated( int reqId, str contractInfo )${ LOG( "WrapperLog::displayGroupUpdated( {},{} )", reqId, contractInfo); }
	α WrapperLog::verifyAndAuthMessageAPI( str apiData, str xyzChallange)${ LOG( "WrapperLog::verifyAndAuthMessageAPI({},{})", apiData, xyzChallange); }

	α WrapperLog::verifyAndAuthCompleted( bool isSuccessful, str errorText)${ LOG( "WrapperLog::position( {}, {} )", isSuccessful, errorText); }
	α WrapperLog::positionMulti( int reqId, str account,str modelCode, const ::Contract& contract, ::Decimal /*pos*/, double /*avgCost*/)${ LOG( "WrapperLog::positionMulti( {}, {}, {}, {} )", reqId, account, modelCode, contract.conId); }
	α WrapperLog::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)${ LOG( "WrapperLog::softDollarTiers( {}, {} )", reqId, tiers.size()); }
	α WrapperLog::familyCodes(const std::vector<FamilyCode> &familyCodes)${ LOG( "WrapperLog::familyCodes( {} )", familyCodes.size()); }
	α WrapperLog::symbolSamples(int reqId, const std::vector<::ContractDescription> &contractDescriptions)${ LOG( "WrapperLog::symbolSamples( {}, {} )", reqId, contractDescriptions.size()); }
	α WrapperLog::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)${ LOG( "WrapperLog::mktDepthExchanges( {} )", depthMktDataDescriptions.size()); }
	α WrapperLog::smartComponents(int reqId, const SmartComponentsMap& theMap)${ LOG( "WrapperLog::smartComponents( {}, {} )", reqId, theMap.size()); }
	α WrapperLog::newsProviders(const std::vector<NewsProvider> &newsProviders)${ LOG( "WrapperLog::newsProviders( {} )", newsProviders.size()); }
	α WrapperLog::newsArticle(int requestId, int articleType, str articleText)${ LOG( "WrapperLog::newsArticle( {}, {}, {} )", requestId, articleType, articleText); }
	α WrapperLog::historicalNews(int requestId, str time, str providerCode, str articleId, str /*headline*/)${ LOG( "WrapperLog::historicalNews( {}, {}, {}, {} )", requestId, time, providerCode, articleId); }
	α WrapperLog::historicalNewsEnd(int requestId, bool hasMore)${ LOG( "WrapperLog::historicalNewsEnd( {}, {} )", requestId, hasMore); }
	α WrapperLog::headTimestamp(int reqId, str headTimestamp)${ LOG( "WrapperLog::headTimestamp( {}, {} )", reqId, headTimestamp); }
	α WrapperLog::histogramData(int reqId, const HistogramDataVector& data)${ LOG( "WrapperLog::histogramData( {}, {} )", reqId, data.size()); }
	α WrapperLog::historicalDataUpdate(TickerId reqId, const ::Bar& bar)${ LOG( "WrapperLog::historicalDataUpdate( {}, {} )", reqId, bar.time); }
	α WrapperLog::rerouteMktDataReq(int reqId, int conid, str exchange)${ LOG( "WrapperLog::rerouteMktDataReq( {}, {}, {} )", reqId, conid, exchange); }
	α WrapperLog::rerouteMktDepthReq(int reqId, int conid, str exchange)${ LOG( "WrapperLog::rerouteMktDepthReq( {}, {}, {} )", reqId, conid, exchange); }
	α WrapperLog::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements)${ LOG( "WrapperLog::marketRule( {}, {} )", marketRuleId, priceIncrements.size()); }
	α WrapperLog::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)${ LOG( "WrapperLog::pnl( {}, {}, {}, {} )", reqId, dailyPnL, unrealizedPnL, realizedPnL); }
	α WrapperLog::pnlSingle(int reqId, ::Decimal pos, double dailyPnL, double unrealizedPnL, double /*realizedPnL*/, double /*value*/)${ LOG( "WrapperLog::pnlSingle( {}, {}, {}, {} )", reqId, pos, dailyPnL, unrealizedPnL); }
	α WrapperLog::historicalSchedule( int reqId, str startDateTime, str endDateTime, str timeZone, const vector<HistoricalSession>& sessions )${ LOG( "({})WrapperLog::historicalSchedule( {}, {}, {}, {} )", reqId, startDateTime, endDateTime, timeZone, sessions.size() ); }
	α WrapperLog::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done)${ LOG( "WrapperLog::historicalTicks( {}, {}, {} )", reqId, ticks.size(), done); }
	α WrapperLog::historicalTicksBidAsk( int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done )${ LOG( "WrapperLog::historicalTicksBidAsk( {}, {}, {} )", reqId, ticks.size(), done); }
	α WrapperLog::historicalTicksLast( int reqId, const std::vector<HistoricalTickLast>& ticks, bool done )${ LOG( "WrapperLog::position( {}, {}, {} )", reqId, ticks.size(), done ); }
	α WrapperLog::tickEFP( TickerId tickerId, TickType tickType, double basisPoints, str formattedBasisPoints, double /*totalDividends*/, int /*holdDays*/, str /*futureLastTradeDate*/, double /*dividendImpact*/, double /*dividendsToLastTradeDate*/ )${ LOG( "WrapperLog::tickEFP( {}, {}, {}, {} )", tickerId, tickType, basisPoints, formattedBasisPoints ); }
	α WrapperLog::tickGeneric( TickerId t, TickType type, double v )$
	{
		LOGT( _tickLevel, "({})WrapperLog::tickGeneric( type='{}', value='{}' )", t, type, v );
		_pTickWorker->PushPrice( t, (ETickType)type, v );
	}
	α WrapperLog::tickOptionComputation( TickerId t, TickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )$
	{
		LOGT( _tickLevel, "({})WrapperLog::tickOptionComputation( type='{}', tickAttrib='{}', impliedVol='{}', delta='{}', optPrice='{}', pvDividend='{}', gamma='{}', vega='{}', theta='{}', undPrice='{}' )", t, type, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice );
		_pTickWorker->Push( t, (ETickType)type, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice );
	}
	α WrapperLog::tickPrice( TickerId t, TickType type, double v, const TickAttrib& attribs )$
	{
		LOGT( _tickLevel, "({})WrapperLog::tickPrice( type='{}', price='{}' )", t, type, v );
		_pTickWorker->PushPrice( t, (ETickType)type, v );
	}
	α WrapperLog::tickSize( TickerId t, TickType type, ::Decimal v )$
	{
		LOGT( _tickLevel, "({})WrapperLog::tickSize( type='{}', size='{}' )", t, type, v );
		_pTickWorker->Push( t, (ETickType)type, v );
	}
	α WrapperLog::tickString( TickerId t, TickType type, str v )$
	{
		LOGT( _tickLevel, "({})WrapperLog::tickString( type='{}', value='{}' )", t, type, v );
		_pTickWorker->Push( t, (ETickType)type, v );
	}
	α WrapperLog::tickNews( int tickerId, time_t timeStamp, str providerCode, str articleId, str headline, str extraData )$
	{
		LOG( "WrapperLog::tickNews( {}, {}, {}, {} )", tickerId, timeStamp, providerCode, articleId);
		_pTickWorker->Push( tickerId, timeStamp, providerCode, articleId, headline, extraData );
	}
	α WrapperLog::tickSnapshotEnd( int t )${LOGT( _tickLevel, "WrapperLog::tickSnapshotEnd( t='{}' )", t);}
	α WrapperLog::winError( str str, int lastError)${ LOG( "({})${}.", lastError, str ); }
	α WrapperLog::orderBound( long long orderId, int apiClientId, int apiOrderId )${ LOG( "WrapperLog::orderBound( {}, {}, {} )", orderId, apiClientId, apiOrderId ); }
	α WrapperLog::completedOrder( const ::Contract& contract, const ::Order& order, const ::OrderState& orderState )$
	{
		OrderManager::Push( order, contract, orderState );

		LOG( "WrapperLog::openOrder( {}, {}@{}, {} )", contract.symbol, toString(order), orderState.status );
	}
	α WrapperLog::completedOrdersEnd()${LOG( "WrapperLog::completedOrdersEnd()"); }
	Handle WrapperLog::_accountUpdateHandle{0};
	α WrapperLog::AddAccountUpdate( sv account, sp<IAccountUpdateHandler> callback )noexcept->tuple<uint,bool>
	{
		unique_lock l{ _accountUpdateCallbackMutex };
		_pUpdateLock = &l;
		var handle = ++_accountUpdateHandle;
		auto& handleCallbacks = _accountUpdateCallbacks.try_emplace( string{account} ).first->second;
		handleCallbacks.emplace( handle, callback );
		var newCall = handleCallbacks.size()==1;
		if( var p  = _accountPortfolioUpdates.find( string{account} ); p!=_accountPortfolioUpdates.end() )
		{
			for( var& contractUpdate : p->second )
				callback->PortfolioUpdate( contractUpdate.second );
		}
		if( var p=_accountUpdateCache.find(string{account}); !newCall && p!=_accountUpdateCache.end() )
		{
			for( var& [key,values] : p->second )
				callback->UpdateAccountValue( key, get<0>(values), get<1>(values), account );
			callback->AccountDownloadEnd( account );
		}

		_pUpdateLock = nullptr;
		return make_tuple( handle, newCall );
	}

	α WrapperLog::RemoveAccountUpdate( sv account, uint handle )noexcept->bool
	{
		bool cancel = true;
		var pLock = _pUpdateLock ? up<unique_lock<shared_mutex>>{} : make_unique<unique_lock<shared_mutex>>( _accountUpdateCallbackMutex );
		if( auto p  = _accountUpdateCallbacks.find(string{account}); p!=_accountUpdateCallbacks.end() )
		{
			if( handle )
				p->second.erase( handle );
			else
				p->second.clear();
			cancel = p->second.empty();
		}
		return cancel;
	}
	α WrapperLog::wshMetaData( int reqId, str dataJson )$
	{
		LOG( "WrapperLog::wshMetaData( {}, {} )", reqId, dataJson );
	}
	α WrapperLog::wshEventData( int reqId, str dataJson )$
	{
		LOG( "WrapperLog::wshEventData( {}, {} )", reqId, dataJson );
	}
}