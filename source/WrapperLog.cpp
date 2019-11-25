#include "stdafx.h"
#include "WrapperLog.h"
//#include "types/messages/IBEnums.h"

namespace Jde::Markets
{
	void WrapperLog::error( int id, int errorCode, const std::string& errorMsg )noexcept 
	{ //callback reqHistoricalData...
		LOG( _logLevel, "WrapperLog::error( {}, {}, {} )", id, errorCode, errorMsg ); 
	}
	void WrapperLog::connectAck()noexcept{ LOG0( _logLevel, "WrapperLog::connectAck()"); }
	/***********TO Implement************/
	void WrapperLog::accountDownloadEnd( const std::string& accountName )noexcept { LOG( _logLevel, "WrapperLog::accountDownloadEnd( {} )"sv, accountName); }
	void WrapperLog::connectionClosed()noexcept { LOG0(_logLevel, "WrapperLog::connectionClosed()"); }
	void WrapperLog::bondContractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept  { LOG( _logLevel, "WrapperLog::bondContractDetails( {}, {} )", reqId, contractDetails.contract.conId); }
	void WrapperLog::contractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept
	{
		LOG( _logLevel, "WrapperLog::contractDetails( '{}', ('{}', '{}', '{}', '{}', '{}') )", reqId, contractDetails.contract.conId, contractDetails.contract.localSymbol, contractDetails.contract.symbol, contractDetails.contract.right, contractDetails.realExpirationDate );
	}
	void WrapperLog::contractDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::contractDetailsEnd( '{}' )", reqId ); }
	void WrapperLog::execDetails( int reqId, const ibapi::Contract& contract, const Execution& execution )noexcept{ LOG( _logLevel, "WrapperLog::execDetails( {}, {}, {}, {}, {}, {} )", reqId, contract.symbol, execution.acctNumber, execution.side, execution.shares, execution.price );	}
	void WrapperLog::execDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::execDetailsEnd( {} )", reqId ); }
	void WrapperLog::historicalData( TickerId reqId, const ibapi::Bar& bar )noexcept
	{ 
		LOG( ELogLevel::Trace, "WrapperLog::historicalData( '{}', ['{}', count: '{}', volume: '{}', wap: '{}', open: '{}', close: '{}', high: '{}', low: '{}'] )", reqId, bar.time, bar.count, bar.volume, bar.wap, bar.open, bar.close, bar.high, bar.low ); 
	}
	void WrapperLog::historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept{ LOG( _logLevel, "WrapperLog::historicalDataEnd( {}, {}, {} )", reqId, startDateStr, endDateStr); }
	void WrapperLog::managedAccounts( const std::string& accountsList )noexcept{ DBG( "WrapperLog::managedAccounts( {} )", accountsList ); }
	void WrapperLog::nextValidId( ibapi::OrderId orderId )noexcept{ LOG( ELogLevel::Information, "WrapperLog::nextValidId( {} )", orderId ); }
#pragma region Order
	void WrapperLog::orderStatus( ibapi::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept{ DBG( "WrapperLog::orderStatus( {}, {}, {}/{} )", orderId, status, filled, filled+remaining ); }
	string toString( const ibapi::Order& order ){ return fmt::format( "{}x{}" , (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice ); };
	void WrapperLog::openOrder( ibapi::OrderId orderId, const ibapi::Contract& contract, const ibapi::Order& order, const ibapi::OrderState& orderState )noexcept{LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )", orderId, contract.symbol, toString(order), orderState.status ); }
	void WrapperLog::openOrderEnd()noexcept{ LOG0( _logLevel, "WrapperLog::openOrderEnd()"); }
#pragma endregion
	void WrapperLog::realtimeBar( TickerId reqId, long time, double open, double high, double low, double close, long volume, double wap, int count )noexcept{}
	void WrapperLog::receiveFA(faDataType pFaDataType, const std::string& cxml)noexcept{ LOG( _logLevel, "WrapperLog::receiveFA( {}, {} )", pFaDataType, cxml); }
	void WrapperLog::tickByTickAllLast(int reqId, int tickType, time_t time, double price, int /*size*/, const TickAttribLast& /*attribs*/, const std::string& /*exchange*/, const std::string& /*specialConditions*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickAllLast( {}, {}, {}, {} )", reqId, tickType, time, price);  }
	void WrapperLog::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, int /*bidSize*/, int /*askSize*/, const TickAttribBidAsk& /*attribs*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickBidAsk( {}, {}, {}, {} )", reqId, time, bidPrice, askPrice); }
	void WrapperLog::tickByTickMidPoint(int reqId, time_t time, double midPoint)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickMidPoint( {}, {}, {} )", reqId, time, midPoint); }
	void WrapperLog::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& /*headline*/, const std::string& /*extraData*/)noexcept{ LOG( _logLevel, "WrapperLog::tickNews( {}, {}, {}, {} )", tickerId, timeStamp, providerCode, articleId); }
	void WrapperLog::tickReqParams( int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::tickReqParams( {}, {}, {}, {} )", tickerId, minTick, bboExchange, snapshotPermissions ); }
	void WrapperLog::updateAccountValue(const std::string& key, const std::string& val, const std::string& currency, const std::string& accountName)noexcept{ LOG( _logLevel, "updateAccountValue( {}, {}, {}, {} )", key, val, currency, accountName); }
	void WrapperLog::updatePortfolio( const ibapi::Contract& contract, double position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName )noexcept{ LOG( _logLevel, "WrapperLog::updatePortfolio( {}, {}, {}, {}, {}, {}, {}, {} )", contract.symbol, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName); }
	void WrapperLog::updateAccountTime(const std::string& timeStamp)noexcept{ LOG( ELogLevel::Trace, "WrapperLog::updateAccountTime( {} )", timeStamp); }
	void WrapperLog::updateMktDepth(TickerId id, int position, int operation, int side, double /*price*/, int /*size*/)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepth( {}, {}, {}, {} )", id, position, operation, side); }
	void WrapperLog::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation, int /*side*/, double /*price*/, int /*size*/, bool isSmartDepth)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepthL2( {}, {}, {}, {}, {} )", id, position, marketMaker, operation, isSmartDepth); }
	void WrapperLog::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch)noexcept{ LOG( _logLevel, "WrapperLog::updateNewsBulletin( {}, {}, {}, {} )", msgId, msgType, newsMessage, originExch); }
	void WrapperLog::scannerParameters(const std::string& xml)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )", xml ); }
	void WrapperLog::scannerData(int reqId, int rank, const ibapi::ContractDetails& contractDetails, const std::string& distance, const std::string& /*benchmark*/, const std::string& /*projection*/, const std::string& /*legsStr*/)noexcept{ LOG( _logLevel, "WrapperLog::scannerData( {}, {}, {}, {} )", reqId, rank, contractDetails.contract.conId, distance ); }
	void WrapperLog::scannerDataEnd(int reqId)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )", reqId ); }
	void WrapperLog::currentTime( long time )noexcept
	{ 
		LOG( _logLevel, "WrapperLog::currentTime( {} )", ToIsoString(Clock::from_time_t(time)) );
		
	}
	void WrapperLog::accountSummaryEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::accountSummaryEnd( {} )", reqId ); }
	void WrapperLog::positionMultiEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::positionMultiEnd( {} )", reqId ); }
#pragma region accountUpdateMulti
	void WrapperLog::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMulti( {}, {}, {}, {}, {}, {} )", reqId, account, modelCode, key, value, currency ); }
	void WrapperLog::accountUpdateMultiEnd( int reqId )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMultiEnd( {} )", reqId ); }
#pragma endregion
	void WrapperLog::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{ 
		LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameter( '{}', '{}', '{}', '{}', '{}', '{}', '{}' )", reqId, exchange, underlyingConId, tradingClass, multiplier, expirations.size(), strikes.size() ); 
	}
	void WrapperLog::securityDefinitionOptionalParameterEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameterEnd( {} )", reqId ); }
	void WrapperLog::fundamentalData(TickerId reqId, const std::string& data)noexcept{ LOG( _logLevel, "WrapperLog::fundamentalData( {}, {} )", reqId, data ); }
	void WrapperLog::deltaNeutralValidation(int reqId, const ibapi::DeltaNeutralContract& deltaNeutralContract)noexcept{ LOG( _logLevel, "WrapperLog::deltaNeutralValidation({},{})", reqId, deltaNeutralContract.conId); }
	void WrapperLog::marketDataType( TickerId tickerId, int marketDataType )noexcept{ }
	void WrapperLog::commissionReport( const CommissionReport& commissionReport )noexcept{ LOG( _logLevel, "WrapperLog::commissionReport( {} )", commissionReport.commission ); }
#pragma region Poision
	void WrapperLog::position( const std::string& account, const ibapi::Contract& contract, double position, double avgCost )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {}, {}, {} )", account, contract.conId, contract.symbol, position, avgCost ); }
	void WrapperLog::positionEnd()noexcept{ LOG0( _logLevel, "WrapperLog::positionEnd()" ); }
#pragma endregion
	void WrapperLog::accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& /*curency*/)noexcept{ LOG( _logLevel, "WrapperLog::accountSummary({},{},{},{})", reqId, account, tag, value); }
	void WrapperLog::verifyMessageAPI( const std::string& apiData )noexcept{ LOG( _logLevel, "WrapperLog::verifyMessageAPI({})", apiData); }
	void WrapperLog::verifyCompleted( bool isSuccessful, const std::string& errorText)noexcept{ LOG( _logLevel, "WrapperLog::verifyCompleted( {},{} )", isSuccessful, errorText); }
	void WrapperLog::displayGroupList( int reqId, const std::string& groups )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupList( {}, {} )", reqId, groups); }
	void WrapperLog::displayGroupUpdated( int reqId, const std::string& contractInfo )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupUpdated( {},{} )", reqId, contractInfo); }
	void WrapperLog::verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange)noexcept{ LOG( _logLevel, "WrapperLog::verifyAndAuthMessageAPI({},{})", apiData, xyzChallange); }

	void WrapperLog::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText)noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {} )", isSuccessful, errorText); }
	void WrapperLog::positionMulti( int reqId, const std::string& account,const std::string& modelCode, const ibapi::Contract& contract, double /*pos*/, double /*avgCost*/)noexcept{ LOG( _logLevel, "WrapperLog::positionMulti( {}, {}, {}, {} )", reqId, account, modelCode, contract.conId); }
	void WrapperLog::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)noexcept{ LOG( _logLevel, "WrapperLog::softDollarTiers( {}, {} )", reqId, tiers.size()); }
	void WrapperLog::familyCodes(const std::vector<FamilyCode> &familyCodes)noexcept{ LOG( _logLevel, "WrapperLog::familyCodes( {} )", familyCodes.size()); }
	void WrapperLog::symbolSamples(int reqId, const std::vector<ibapi::ContractDescription> &contractDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::symbolSamples( {}, {} )", reqId, contractDescriptions.size()); }
	void WrapperLog::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::mktDepthExchanges( {} )", depthMktDataDescriptions.size()); }
	void WrapperLog::smartComponents(int reqId, const SmartComponentsMap& theMap)noexcept{ LOG( _logLevel, "WrapperLog::smartComponents( {}, {} )", reqId, theMap.size()); }
	void WrapperLog::newsProviders(const std::vector<NewsProvider> &newsProviders)noexcept{ LOG( _logLevel, "WrapperLog::newsProviders( {} )", newsProviders.size()); }
	void WrapperLog::newsArticle(int requestId, int articleType, const std::string& articleText)noexcept{ LOG( _logLevel, "WrapperLog::newsArticle( {}, {}, {} )", requestId, articleType, articleText); }
	void WrapperLog::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& /*headline*/)noexcept{ LOG( _logLevel, "WrapperLog::historicalNews( {}, {}, {}, {} )", requestId, time, providerCode, articleId); }
	void WrapperLog::historicalNewsEnd(int requestId, bool hasMore)noexcept{ LOG( _logLevel, "WrapperLog::historicalNewsEnd( {}, {} )", requestId, hasMore); }
	void WrapperLog::headTimestamp(int reqId, const std::string& headTimestamp)noexcept{ LOG( _logLevel, "WrapperLog::headTimestamp( {}, {} )", reqId, headTimestamp); }
	void WrapperLog::histogramData(int reqId, const HistogramDataVector& data)noexcept{ LOG( _logLevel, "WrapperLog::histogramData( {}, {} )", reqId, data.size()); }
	void WrapperLog::historicalDataUpdate(TickerId reqId, const ibapi::Bar& bar)noexcept{ LOG( _logLevel, "WrapperLog::historicalDataUpdate( {}, {} )", reqId, bar.time); }
	void WrapperLog::rerouteMktDataReq(int reqId, int conid, const std::string& exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDataReq( {}, {}, {} )", reqId, conid, exchange); }
	void WrapperLog::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDepthReq( {}, {}, {} )", reqId, conid, exchange); }
	void WrapperLog::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements)noexcept{ LOG( _logLevel, "WrapperLog::marketRule( {}, {} )", marketRuleId, priceIncrements.size()); }
	void WrapperLog::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)noexcept{ LOG( _logLevel, "WrapperLog::pnl( {}, {}, {}, {} )", reqId, dailyPnL, unrealizedPnL, realizedPnL); }
	void WrapperLog::pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double /*realizedPnL*/, double /*value*/)noexcept{ LOG( _logLevel, "WrapperLog::pnlSingle( {}, {}, {}, {} )", reqId, pos, dailyPnL, unrealizedPnL); }
	void WrapperLog::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done)noexcept{ LOG( _logLevel, "WrapperLog::historicalTicks( {}, {}, {} )", reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksBidAsk( int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::historicalTicksBidAsk( {}, {}, {} )", reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksLast( int reqId, const std::vector<HistoricalTickLast>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {} )", reqId, ticks.size(), done ); }
	void WrapperLog::tickEFP( TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints, double /*totalDividends*/, int /*holdDays*/, const std::string& /*futureLastTradeDate*/, double /*dividendImpact*/, double /*dividendsToLastTradeDate*/ )noexcept{ LOG( _logLevel, "WrapperLog::tickEFP( {}, {}, {}, {} )", tickerId, tickType, basisPoints, formattedBasisPoints ); }
	void WrapperLog::tickGeneric( TickerId tickerId, TickType tickType, double value )noexcept
	{ 
		LOG( _tickLevel, "WrapperLog::tickGeneric( tickerId='{}', field='{}', value='{}' )", tickerId, tickType, value ); 
	}
	void WrapperLog::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept
	{ 
		LOG( _tickLevel, "WrapperLog::tickOptionComputation( tickerId='{}', tickType='{}', impliedVol='{}', delta='{}', optPrice='{}', pvDividend='{}', gamma='{}', vega='{}', theta='{}', undPrice='{}' )", tickerId, tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice ); 
	}
	void WrapperLog::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attribs )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickPrice( tickerId='{}', field='{}', price='{}' )", tickerId, field, price ); 
	}
	void WrapperLog::tickSize( TickerId tickerId, TickType field, int size )noexcept
	{ 
		LOG( _tickLevel, "WrapperLog::tickSize( tickerId='{}', field='{}', size='{}' )", tickerId, field, size ); 
	}
	void WrapperLog::tickString( TickerId tickerId, TickType tickType, const std::string& value )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickString( tickerId='{}', field='{}', value='{}' )", tickerId, tickType, value ); 
	}
	void WrapperLog::tickSnapshotEnd( int reqId )noexcept{}
	void WrapperLog::winError( const std::string& str, int lastError)noexcept{ LOG( _logLevel, "({})noexcept{}.", lastError, str ); }
	void WrapperLog::orderBound( long long orderId, int apiClientId, int apiOrderId )noexcept{ LOG( _logLevel, "WrapperLog::orderBound( {}, {}, {} )", orderId, apiClientId, apiOrderId ); }
	void WrapperLog::completedOrder(const ibapi::Contract& contract, const ibapi::Order& order, const ibapi::OrderState& orderState)noexcept{LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )", contract.symbol, toString(order), orderState.status );}
	void WrapperLog::completedOrdersEnd()noexcept{LOG0( _logLevel, "WrapperLog::completedOrdersEnd()"); }

}