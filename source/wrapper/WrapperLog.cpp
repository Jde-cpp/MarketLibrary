#include "WrapperLog.h"

#define var const auto
#define _client TwsClient::Instance()
namespace Jde::Markets
{
	void WrapperLog::error( int id, int errorCode, const std::string& errorMsg )noexcept
	{
		if( _historicalDataRequests.erase(id) )
			LOG( _logLevel, "({})_historicalDataRequests.erase(){}"sv, id, _historicalDataRequests.size() );

		LOG( _logLevel, "({})WrapperLog::error( {}, {} )"sv, id, errorCode, errorMsg );
		if( errorCode==509 )
			ERR( "Disconnected:  {} - {}"sv, errorCode, errorMsg );
	}
	void WrapperLog::connectAck()noexcept{ LOG0( _logLevel, "WrapperLog::connectAck()"sv); }
	/***********TO Implement************/
	void WrapperLog::accountDownloadEnd( const std::string& accountName )noexcept { LOG( _logLevel, "WrapperLog::accountDownloadEnd( {} )"sv, accountName); }
	void WrapperLog::connectionClosed()noexcept { LOG0(_logLevel, "WrapperLog::connectionClosed()"sv); }
	void WrapperLog::bondContractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept  { LOG( _logLevel, "WrapperLog::bondContractDetails( {}, {} )"sv, reqId, contractDetails.contract.conId); }
	void WrapperLog::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		LOG( _logLevel, "({})WrapperLog::contractDetails( '{}', '{}', '{}', '{}', '{}' )"sv, reqId, contractDetails.contract.conId, contractDetails.contract.localSymbol, contractDetails.contract.symbol, contractDetails.contract.right, contractDetails.realExpirationDate );
	}
	void WrapperLog::contractDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "({})WrapperLog::contractDetailsEnd()"sv, reqId ); }
	void WrapperLog::execDetails( int reqId, const ibapi::Contract& contract, const Execution& execution )noexcept{ LOG( _logLevel, "({})WrapperLog::execDetails( {}, {}, {}, {}, {} )"sv, reqId, contract.symbol, execution.acctNumber, execution.side, execution.shares, execution.price );	}
	void WrapperLog::execDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::execDetailsEnd( {} )"sv, reqId ); }
	void WrapperLog::historicalData( TickerId reqId, const ::Bar& bar )noexcept
	{
		LOG( ELogLevel::Trace, "({})WrapperLog::historicalData( '{}', count: '{}', volume: '{}', wap: '{}', open: '{}', close: '{}', high: '{}', low: '{}' )"sv, reqId, bar.time, bar.count, bar.volume, bar.wap, bar.open, bar.close, bar.high, bar.low );
	}
	void WrapperLog::historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept
	{
		_historicalDataRequests.erase( reqId );
		var size = _historicalDataRequests.size();
		var format = startDateStr.size() || endDateStr.size() ? "({}){}WrapperLog::historicalDataEnd( {}, {} )"sv : "({})WrapperLog::historicalDataEnd(){}"sv;
		LOG( _logLevel, format, reqId, size, startDateStr, endDateStr );
	}
	void WrapperLog::managedAccounts( const std::string& accountsList )noexcept{ DBG( "WrapperLog::managedAccounts( {} )"sv, accountsList ); }
	void WrapperLog::nextValidId( ::OrderId orderId )noexcept{ LOG( ELogLevel::Information, "WrapperLog::nextValidId( '{}' )"sv, orderId ); }
#pragma region Order
	void WrapperLog::orderStatus( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice )noexcept{ DBG( "WrapperLog::orderStatus( {}, {}, {}/{} )"sv, orderId, status, filled, filled+remaining ); }
	string toString( const ::Order& order ){ return fmt::format( "{}x{}" , (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice ); };
	void WrapperLog::openOrder( ::OrderId orderId, const ibapi::Contract& contract, const ::Order& order, const ::OrderState& orderState )noexcept{LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )"sv, orderId, contract.symbol, toString(order), orderState.status ); }
	void WrapperLog::openOrderEnd()noexcept{ LOG0( _logLevel, "WrapperLog::openOrderEnd()"sv); }
#pragma endregion
	void WrapperLog::realtimeBar( TickerId reqId, long time, double open, double high, double low, double close, long volume, double wap, int count )noexcept{}
	void WrapperLog::receiveFA(faDataType pFaDataType, const std::string& cxml)noexcept{ LOG( _logLevel, "WrapperLog::receiveFA( {}, {} )"sv, pFaDataType, cxml); }
	void WrapperLog::tickByTickAllLast(int reqId, int tickType, time_t time, double price, int /*size*/, const TickAttribLast& /*attribs*/, const std::string& /*exchange*/, const std::string& /*specialConditions*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickAllLast( {}, {}, {}, {} )"sv, reqId, tickType, time, price);  }
	void WrapperLog::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, int /*bidSize*/, int /*askSize*/, const TickAttribBidAsk& /*attribs*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickBidAsk( {}, {}, {}, {} )"sv, reqId, time, bidPrice, askPrice); }
	void WrapperLog::tickByTickMidPoint(int reqId, time_t time, double midPoint)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickMidPoint( {}, {}, {} )"sv, reqId, time, midPoint); }
	void WrapperLog::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& /*headline*/, const std::string& /*extraData*/)noexcept{ LOG( _logLevel, "WrapperLog::tickNews( {}, {}, {}, {} )"sv, tickerId, timeStamp, providerCode, articleId); }
	void WrapperLog::tickReqParams( int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::tickReqParams( {}, {}, {}, {} )"sv, tickerId, minTick, bboExchange, snapshotPermissions ); }
	void WrapperLog::updateAccountValue(const std::string& key, const std::string& val, const std::string& currency, const std::string& accountName)noexcept{ LOG( _logLevel, "updateAccountValue( {}, {}, {}, {} )"sv, key, val, currency, accountName); }
	void WrapperLog::updatePortfolio( const ibapi::Contract& contract, double position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName )noexcept{ LOG( _logLevel, "WrapperLog::updatePortfolio( {}, {}, {}, {}, {}, {}, {}, {} )"sv, contract.symbol, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName); }
	void WrapperLog::updateAccountTime(const std::string& timeStamp)noexcept{ LOG( ELogLevel::Trace, "WrapperLog::updateAccountTime( {} )"sv, timeStamp); }
	void WrapperLog::updateMktDepth(TickerId id, int position, int operation, int side, double /*price*/, int /*size*/)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepth( {}, {}, {}, {} )"sv, id, position, operation, side); }
	void WrapperLog::updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation, int /*side*/, double /*price*/, int /*size*/, bool isSmartDepth)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepthL2( {}, {}, {}, {}, {} )"sv, id, position, marketMaker, operation, isSmartDepth); }
	void WrapperLog::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch)noexcept{ LOG( _logLevel, "WrapperLog::updateNewsBulletin( {}, {}, {}, {} )"sv, msgId, msgType, newsMessage, originExch); }
	void WrapperLog::scannerParameters(const std::string& xml)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )"sv, xml ); }
	void WrapperLog::scannerData(int reqId, int rank, const ::ContractDetails& contractDetails, const std::string& distance, const std::string& /*benchmark*/, const std::string& /*projection*/, const std::string& /*legsStr*/)noexcept{ LOG( _logLevel, "WrapperLog::scannerData( {}, {}, {}, {} )"sv, reqId, rank, contractDetails.contract.conId, distance ); }
	void WrapperLog::scannerDataEnd(int reqId)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )"sv, reqId ); }
	void WrapperLog::currentTime( long time )noexcept
	{
		LOG( _logLevel, "WrapperLog::currentTime( {} )"sv, ToIsoString(Clock::from_time_t(time)) );
	}
	void WrapperLog::accountSummaryEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::accountSummaryEnd( {} )"sv, reqId ); }
	void WrapperLog::positionMultiEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::positionMultiEnd( {} )"sv, reqId ); }
#pragma region accountUpdateMulti
	void WrapperLog::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMulti( {}, {}, {}, {}, {}, {} )"sv, reqId, account, modelCode, key, value, currency ); }
	void WrapperLog::accountUpdateMultiEnd( int reqId )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMultiEnd( {} )"sv, reqId ); }
#pragma endregion
	void WrapperLog::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameter( '{}', '{}', '{}', '{}', '{}', '{}', '{}' )"sv, reqId, exchange, underlyingConId, tradingClass, multiplier, expirations.size(), strikes.size() );
	}
	void WrapperLog::securityDefinitionOptionalParameterEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameterEnd( {} )"sv, reqId ); }
	void WrapperLog::fundamentalData(TickerId reqId, const std::string& data)noexcept{ LOG( _logLevel, "WrapperLog::fundamentalData( {}, {} )"sv, reqId, data ); }
	void WrapperLog::deltaNeutralValidation(int reqId, const ::DeltaNeutralContract& deltaNeutralContract)noexcept{ LOG( _logLevel, "WrapperLog::deltaNeutralValidation({},{})"sv, reqId, deltaNeutralContract.conId); }
	void WrapperLog::marketDataType( TickerId tickerId, int marketDataType )noexcept{ }
	void WrapperLog::commissionReport( const CommissionReport& commissionReport )noexcept{ LOG( _logLevel, "WrapperLog::commissionReport( {} )"sv, commissionReport.commission ); }
#pragma region Poision
	void WrapperLog::position( const std::string& account, const ibapi::Contract& contract, double position, double avgCost )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {}, {}, {} )"sv, account, contract.conId, contract.symbol, position, avgCost ); }
	void WrapperLog::positionEnd()noexcept{ LOG0( _logLevel, "WrapperLog::positionEnd()"sv ); }
#pragma endregion
	void WrapperLog::accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& /*curency*/)noexcept{ LOG( _logLevel, "WrapperLog::accountSummary({},{},{},{})"sv, reqId, account, tag, value); }
	void WrapperLog::verifyMessageAPI( const std::string& apiData )noexcept{ LOG( _logLevel, "WrapperLog::verifyMessageAPI({})"sv, apiData); }
	void WrapperLog::verifyCompleted( bool isSuccessful, const std::string& errorText)noexcept{ LOG( _logLevel, "WrapperLog::verifyCompleted( {},{} )"sv, isSuccessful, errorText); }
	void WrapperLog::displayGroupList( int reqId, const std::string& groups )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupList( {}, {} )"sv, reqId, groups); }
	void WrapperLog::displayGroupUpdated( int reqId, const std::string& contractInfo )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupUpdated( {},{} )"sv, reqId, contractInfo); }
	void WrapperLog::verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange)noexcept{ LOG( _logLevel, "WrapperLog::verifyAndAuthMessageAPI({},{})"sv, apiData, xyzChallange); }

	void WrapperLog::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText)noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {} )"sv, isSuccessful, errorText); }
	void WrapperLog::positionMulti( int reqId, const std::string& account,const std::string& modelCode, const ibapi::Contract& contract, double /*pos*/, double /*avgCost*/)noexcept{ LOG( _logLevel, "WrapperLog::positionMulti( {}, {}, {}, {} )"sv, reqId, account, modelCode, contract.conId); }
	void WrapperLog::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)noexcept{ LOG( _logLevel, "WrapperLog::softDollarTiers( {}, {} )"sv, reqId, tiers.size()); }
	void WrapperLog::familyCodes(const std::vector<FamilyCode> &familyCodes)noexcept{ LOG( _logLevel, "WrapperLog::familyCodes( {} )"sv, familyCodes.size()); }
	void WrapperLog::symbolSamples(int reqId, const std::vector<::ContractDescription> &contractDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::symbolSamples( {}, {} )"sv, reqId, contractDescriptions.size()); }
	void WrapperLog::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::mktDepthExchanges( {} )"sv, depthMktDataDescriptions.size()); }
	void WrapperLog::smartComponents(int reqId, const SmartComponentsMap& theMap)noexcept{ LOG( _logLevel, "WrapperLog::smartComponents( {}, {} )"sv, reqId, theMap.size()); }
	void WrapperLog::newsProviders(const std::vector<NewsProvider> &newsProviders)noexcept{ LOG( _logLevel, "WrapperLog::newsProviders( {} )"sv, newsProviders.size()); }
	void WrapperLog::newsArticle(int requestId, int articleType, const std::string& articleText)noexcept{ LOG( _logLevel, "WrapperLog::newsArticle( {}, {}, {} )"sv, requestId, articleType, articleText); }
	void WrapperLog::historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& /*headline*/)noexcept{ LOG( _logLevel, "WrapperLog::historicalNews( {}, {}, {}, {} )"sv, requestId, time, providerCode, articleId); }
	void WrapperLog::historicalNewsEnd(int requestId, bool hasMore)noexcept{ LOG( _logLevel, "WrapperLog::historicalNewsEnd( {}, {} )"sv, requestId, hasMore); }
	void WrapperLog::headTimestamp(int reqId, const std::string& headTimestamp)noexcept{ LOG( _logLevel, "WrapperLog::headTimestamp( {}, {} )"sv, reqId, headTimestamp); }
	void WrapperLog::histogramData(int reqId, const HistogramDataVector& data)noexcept{ LOG( _logLevel, "WrapperLog::histogramData( {}, {} )"sv, reqId, data.size()); }
	void WrapperLog::historicalDataUpdate(TickerId reqId, const ::Bar& bar)noexcept{ LOG( _logLevel, "WrapperLog::historicalDataUpdate( {}, {} )"sv, reqId, bar.time); }
	void WrapperLog::rerouteMktDataReq(int reqId, int conid, const std::string& exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDataReq( {}, {}, {} )"sv, reqId, conid, exchange); }
	void WrapperLog::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDepthReq( {}, {}, {} )"sv, reqId, conid, exchange); }
	void WrapperLog::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements)noexcept{ LOG( _logLevel, "WrapperLog::marketRule( {}, {} )"sv, marketRuleId, priceIncrements.size()); }
	void WrapperLog::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)noexcept{ LOG( _logLevel, "WrapperLog::pnl( {}, {}, {}, {} )"sv, reqId, dailyPnL, unrealizedPnL, realizedPnL); }
	void WrapperLog::pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double /*realizedPnL*/, double /*value*/)noexcept{ LOG( _logLevel, "WrapperLog::pnlSingle( {}, {}, {}, {} )"sv, reqId, pos, dailyPnL, unrealizedPnL); }
	void WrapperLog::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done)noexcept{ LOG( _logLevel, "WrapperLog::historicalTicks( {}, {}, {} )"sv, reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksBidAsk( int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::historicalTicksBidAsk( {}, {}, {} )"sv, reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksLast( int reqId, const std::vector<HistoricalTickLast>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {} )"sv, reqId, ticks.size(), done ); }
	void WrapperLog::tickEFP( TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints, double /*totalDividends*/, int /*holdDays*/, const std::string& /*futureLastTradeDate*/, double /*dividendImpact*/, double /*dividendsToLastTradeDate*/ )noexcept{ LOG( _logLevel, "WrapperLog::tickEFP( {}, {}, {}, {} )"sv, tickerId, tickType, basisPoints, formattedBasisPoints ); }
	void WrapperLog::tickGeneric( TickerId tickerId, TickType tickType, double value )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickGeneric( tickerId='{}', field='{}', value='{}' )"sv, tickerId, tickType, value );
	}
	void WrapperLog::tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickOptionComputation( tickerId='{}', tickType='{}', tickAttrib='{}', impliedVol='{}', delta='{}', optPrice='{}', pvDividend='{}', gamma='{}', vega='{}', theta='{}', undPrice='{}' )"sv, tickerId, tickType, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice );
	}
	void WrapperLog::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attribs )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickPrice( tickerId='{}', field='{}', price='{}' )"sv, tickerId, field, price );
	}
	void WrapperLog::tickSize( TickerId tickerId, TickType field, int size )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickSize( tickerId='{}', field='{}', size='{}' )"sv, tickerId, field, size );
	}
	void WrapperLog::tickString( TickerId tickerId, TickType tickType, const std::string& value )noexcept
	{
		LOG( _tickLevel, "WrapperLog::tickString( tickerId='{}', field='{}', value='{}' )"sv, tickerId, tickType, value );
	}
	void WrapperLog::tickSnapshotEnd( int reqId )noexcept{LOG( _tickLevel, "WrapperLog::tickSnapshotEnd( tickerId='{}' )"sv, reqId);}
	void WrapperLog::winError( const std::string& str, int lastError)noexcept{ LOG( _logLevel, "({})noexcept{}."sv, lastError, str ); }
	void WrapperLog::orderBound( long long orderId, int apiClientId, int apiOrderId )noexcept{ LOG( _logLevel, "WrapperLog::orderBound( {}, {}, {} )"sv, orderId, apiClientId, apiOrderId ); }
	void WrapperLog::completedOrder(const ibapi::Contract& contract, const ::Order& order, const ::OrderState& orderState)noexcept{LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )"sv, contract.symbol, toString(order), orderState.status );}
	void WrapperLog::completedOrdersEnd()noexcept{LOG0( _logLevel, "WrapperLog::completedOrdersEnd()"sv); }

}