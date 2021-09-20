#include "WrapperLog.h"
#include <CommissionReport.h>
#include "../../../Framework/source/Cache.h"
#include "../OrderManager.h"
#include "../client/TwsClient.h"

#define var const auto
#define _client auto pClient = TwsClient::InstancePtr(); if( pClient ) (*pClient)
namespace Jde::Markets
{
	ELogLevel WrapperLog::_logLevel{ Logging::TagLevel("wrapper", [](auto l){ WrapperLog::SetLevel(l);}, ELogLevel::Debug) };
	ELogLevel WrapperLog::_historicalLevel{ Logging::TagLevel("wrapperHist", [](auto l){ WrapperLog::SetHistoricalLevel(l);}) };
	ELogLevel WrapperLog::_tickLevel{ Logging::TagLevel("wrapperTick", [](auto l){ WrapperLog::SetTickLevel(l);}) };

	bool WrapperLog::error2( int id, int errorCode, str errorMsg )noexcept
	{
		WrapperLog::error( id, errorCode, errorMsg );
		return id==-1 || _pTickWorker->HandleError( id, errorCode, errorMsg );
	}
	void WrapperLog::error( int id, int errorCode, str errorMsg )noexcept
	{
		LOG_IF( _historicalDataRequests.erase(id),  _historicalLevel, "({})_historicalDataRequests.erase(){}", id, _historicalDataRequests.size() );
		LOG_IF( errorCode!=2106, _logLevel, "({})WrapperLog::error( {}, {} )", id, errorCode, errorMsg );
		LOG_IF( errorCode==509, ELogLevel::Error, "Disconnected:  {} - {}", errorCode, errorMsg );
	}
	void WrapperLog::connectAck()noexcept{ LOG( _logLevel, "WrapperLog::connectAck()"); }

	void WrapperLog::connectionClosed()noexcept { LOG(_logLevel, "WrapperLog::connectionClosed()"); }
	void WrapperLog::bondContractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept  { LOG( _logLevel, "WrapperLog::bondContractDetails( {}, {} )", reqId, contractDetails.contract.conId); }
	void WrapperLog::contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept
	{
		LOG( _logLevel, "({})WrapperLog::contractDetails( '{}', '{}', '{}', '{}', '{}' )", reqId, contractDetails.contract.conId, contractDetails.contract.localSymbol, contractDetails.contract.symbol, contractDetails.contract.right, contractDetails.realExpirationDate );
	}
	void WrapperLog::contractDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "({})WrapperLog::contractDetailsEnd()", reqId ); }
	void WrapperLog::execDetails( int reqId, const ::Contract& contract, const Execution& execution )noexcept{ LOG( _logLevel, "({})WrapperLog::execDetails( {}, {}, {}, {}, {} )", reqId, contract.symbol, execution.acctNumber, execution.side, execution.shares, execution.price );	}
	void WrapperLog::execDetailsEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::execDetailsEnd( {} )", reqId ); }
	void WrapperLog::historicalData( TickerId reqId, const ::Bar& bar )noexcept
	{
		LOG( _historicalLevel, "({})WrapperLog::historicalData( '{}', count: '{}', volume: '{}', wap: '{}', open: '{}', close: '{}', high: '{}', low: '{}' )", reqId, bar.time, bar.count, bar.volume, bar.wap, bar.open, bar.close, bar.high, bar.low );
	}
	void WrapperLog::historicalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept
	{
		_historicalDataRequests.erase( reqId );
		var size = _historicalDataRequests.size();
		var format = startDateStr.size() || endDateStr.size() ? "({}){}WrapperLog::historicalDataEnd( {}, {} )"sv : "({})WrapperLog::historicalDataEnd(){}"sv;
		Logging::Log( Logging::MessageBase(format, _historicalLevel, MY_FILE, __func__, __LINE__), reqId, size, startDateStr, endDateStr );
	}
	void WrapperLog::managedAccounts( str accountsList )noexcept{ DBG( "WrapperLog::managedAccounts( {} )"sv, accountsList ); }
	void WrapperLog::nextValidId( ::OrderId orderId )noexcept{ LOG( ELogLevel::Information, "WrapperLog::nextValidId( '{}' )", orderId ); }
#pragma region Order
	void WrapperLog::orderStatus( ::OrderId orderId, str status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, str whyHeld, double mktCapPrice )noexcept
	{
		OrderManager::Push( orderId, status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, clientId, whyHeld, mktCapPrice );
		DBG( "WrapperLog::orderStatus( {}, {}, {}/{} )"sv, orderId, status, filled, filled+remaining );
	}
	string toString( const ::Order& order ){ return fmt::format( "{}x{}" , (order.action=="BUY" ? 1 : -1 )*order.totalQuantity, order.lmtPrice ); };
	void WrapperLog::openOrder( ::OrderId orderId, const ::Contract& contract, const ::Order& order, const ::OrderState& orderState )noexcept
	{
		OrderManager::Push( order, contract, orderState );
		LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )", orderId, contract.symbol, toString(order), orderState.status );
	}
	void WrapperLog::openOrderEnd()noexcept{ LOG( _logLevel, "WrapperLog::openOrderEnd()"); }
#pragma endregion
	void WrapperLog::realtimeBar( TickerId reqId, long time, double open, double high, double low, double close, long volume, double wap, int count )noexcept{}
	void WrapperLog::receiveFA(faDataType pFaDataType, str cxml)noexcept{ LOG( _logLevel, "WrapperLog::receiveFA( {}, {} )", pFaDataType, cxml); }
	void WrapperLog::tickByTickAllLast(int reqId, int tickType, time_t time, double price, long long /*size*/, const TickAttribLast& /*attribs*/, str /*exchange*/, str /*specialConditions*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickAllLast( {}, {}, {}, {} )", reqId, tickType, time, price);  }
	void WrapperLog::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, long long /*bidSize*/, long long /*askSize*/, const TickAttribBidAsk& /*attribs*/)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickBidAsk( {}, {}, {}, {} )", reqId, time, bidPrice, askPrice); }
	void WrapperLog::tickByTickMidPoint(int reqId, time_t time, double midPoint)noexcept{ LOG( _logLevel, "WrapperLog::tickByTickMidPoint( {}, {}, {} )", reqId, time, midPoint); }
	void WrapperLog::tickReqParams( int tickerId, double minTick, str bboExchange, int snapshotPermissions )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::tickReqParams( {}, {}, {}, {} )", tickerId, minTick, bboExchange, snapshotPermissions ); }
	bool WrapperLog::updateAccountValue2( sv key, sv val, sv currency, sv accountName )noexcept
	{
		unique_lock l{ _accountUpdateCallbackMutex };
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
	void WrapperLog::updateAccountValue( str key, str val, str currency, str accountName )noexcept
	{
		LOG( _logLevel, "updateAccountValue( {}, {}, {}, {} )", key, val, currency, accountName );
		updateAccountValue2( key, val, currency, accountName );
	}
	void WrapperLog::accountDownloadEnd( str accountName )noexcept
	{
		LOG( _logLevel, "WrapperLog::accountDownloadEnd( {} )", accountName);
		shared_lock l{ _accountUpdateCallbackMutex };
		if( auto p  = _accountUpdateCallbacks.find(accountName); p!=_accountUpdateCallbacks.end() )
			std::for_each( p->second.begin(), p->second.end(), [&](auto& x){ x.second->AccountDownloadEnd(accountName); } );
	}

	void WrapperLog::updatePortfolio( const ::Contract& contract, double position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, str accountNumber )noexcept
	{
		LOG( ELogLevel::Trace, "WrapperLog::updatePortfolio( {}, {}, {}, {}, {}, {}, {}, {} )", contract.symbol, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountNumber);
		Proto::Results::PortfolioUpdate update;
		unique_lock l{ _accountUpdateCallbackMutex };
		auto p  = _accountUpdateCallbacks.find( string{accountNumber} );
		if( p==_accountUpdateCallbacks.end() || p->second.empty() )
		{
			if( p!=_accountUpdateCallbacks.end() )
				_accountPortfolioUpdates.erase( string{accountNumber} );
			LOG( ELogLevel::Trace, "WrapperLog::updatePortfolio - no callbacks canceling" );
			return TwsClient::CancelAccountUpdates( accountNumber, 0 );
		}

		Contract myContract{ contract };
		update.set_allocated_contract( myContract.ToProto(true).get() );
		update.set_position( position );
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
	void WrapperLog::updateAccountTime(str timeStamp)noexcept{ LOG( ELogLevel::Trace, "WrapperLog::updateAccountTime( {} )", timeStamp); }
	void WrapperLog::updateMktDepth(TickerId id, int position, int operation, int side, double /*price*/, long long /*size*/)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepth( {}, {}, {}, {} )", id, position, operation, side); }
	void WrapperLog::updateMktDepthL2(TickerId id, int position, str marketMaker, int operation, int /*side*/, double /*price*/, long long /*size*/, bool isSmartDepth)noexcept{ LOG( _logLevel, "WrapperLog::updateMktDepthL2( {}, {}, {}, {}, {} )", id, position, marketMaker, operation, isSmartDepth); }
	void WrapperLog::updateNewsBulletin(int msgId, int msgType, str newsMessage, str originExch)noexcept{ LOG( _logLevel, "WrapperLog::updateNewsBulletin( {}, {}, {}, {} )", msgId, msgType, newsMessage, originExch); }
	void WrapperLog::scannerParameters(str xml)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )", xml ); }
	void WrapperLog::scannerData(int reqId, int rank, const ::ContractDetails& contractDetails, str distance, str /*benchmark*/, str /*projection*/, str /*legsStr*/)noexcept{ LOG( _logLevel, "WrapperLog::scannerData( {}, {}, {}, {} )", reqId, rank, contractDetails.contract.conId, distance ); }
	void WrapperLog::scannerDataEnd(int reqId)noexcept{ LOG( _logLevel, "WrapperLog::scannerDataEnd( {} )", reqId ); }
	void WrapperLog::currentTime( long time )noexcept
	{
		LOG( _logLevel, "WrapperLog::currentTime( {} )", ToIsoString(Clock::from_time_t(time)) );
	}
	void WrapperLog::accountSummaryEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::accountSummaryEnd( {} )", reqId ); }
	void WrapperLog::positionMultiEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::positionMultiEnd( {} )", reqId ); }
#pragma region accountUpdateMulti
	void WrapperLog::accountUpdateMulti( int reqId, str account, str modelCode, str key, str value, str currency )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMulti( {}, {}, {}, {}, {}, {} )", reqId, account, modelCode, key, value, currency ); }
	void WrapperLog::accountUpdateMultiEnd( int reqId )noexcept{ LOG( ELogLevel::Trace, "WrapperLog::accountUpdateMultiEnd( {} )", reqId ); }
#pragma endregion
	void WrapperLog::securityDefinitionOptionalParameter(int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept
	{
		LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameter( '{}', '{}', '{}', '{}', '{}', '{}', '{}' )", reqId, exchange, underlyingConId, tradingClass, multiplier, expirations.size(), strikes.size() );
	}
	void WrapperLog::securityDefinitionOptionalParameterEnd( int reqId )noexcept{ LOG( _logLevel, "WrapperLog::securityDefinitionOptionalParameterEnd( {} )", reqId ); }
	void WrapperLog::fundamentalData(TickerId reqId, str data)noexcept{ LOG( _logLevel, "WrapperLog::fundamentalData( {}, {} )", reqId, data ); }
	void WrapperLog::deltaNeutralValidation(int reqId, const ::DeltaNeutralContract& deltaNeutralContract)noexcept{ LOG( _logLevel, "WrapperLog::deltaNeutralValidation({},{})", reqId, deltaNeutralContract.conId); }
	void WrapperLog::marketDataType( TickerId tickerId, int marketDataType )noexcept{ }
	void WrapperLog::commissionReport( const CommissionReport& commissionReport )noexcept{ LOG( _logLevel, "WrapperLog::commissionReport( {} )", commissionReport.commission ); }
#pragma region Poision
	void WrapperLog::position( str account, const ::Contract& contract, double position, double avgCost )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {}, {}, {} )", account, contract.conId, contract.symbol, position, avgCost ); }
	void WrapperLog::positionEnd()noexcept{ LOG( _logLevel, "WrapperLog::positionEnd()" ); }
#pragma endregion
	void WrapperLog::accountSummary( int reqId, str account, str tag, str value, str /*curency*/)noexcept{ LOG( _logLevel, "WrapperLog::accountSummary({},{},{},{})", reqId, account, tag, value); }
	void WrapperLog::verifyMessageAPI( str apiData )noexcept{ LOG( _logLevel, "WrapperLog::verifyMessageAPI({})", apiData); }
	void WrapperLog::verifyCompleted( bool isSuccessful, str errorText)noexcept{ LOG( _logLevel, "WrapperLog::verifyCompleted( {},{} )", isSuccessful, errorText); }
	void WrapperLog::displayGroupList( int reqId, str groups )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupList( {}, {} )", reqId, groups); }
	void WrapperLog::displayGroupUpdated( int reqId, str contractInfo )noexcept{ LOG( _logLevel, "WrapperLog::displayGroupUpdated( {},{} )", reqId, contractInfo); }
	void WrapperLog::verifyAndAuthMessageAPI( str apiData, str xyzChallange)noexcept{ LOG( _logLevel, "WrapperLog::verifyAndAuthMessageAPI({},{})", apiData, xyzChallange); }

	void WrapperLog::verifyAndAuthCompleted( bool isSuccessful, str errorText)noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {} )", isSuccessful, errorText); }
	void WrapperLog::positionMulti( int reqId, str account,str modelCode, const ::Contract& contract, double /*pos*/, double /*avgCost*/)noexcept{ LOG( _logLevel, "WrapperLog::positionMulti( {}, {}, {}, {} )", reqId, account, modelCode, contract.conId); }
	void WrapperLog::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)noexcept{ LOG( _logLevel, "WrapperLog::softDollarTiers( {}, {} )", reqId, tiers.size()); }
	void WrapperLog::familyCodes(const std::vector<FamilyCode> &familyCodes)noexcept{ LOG( _logLevel, "WrapperLog::familyCodes( {} )", familyCodes.size()); }
	void WrapperLog::symbolSamples(int reqId, const std::vector<::ContractDescription> &contractDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::symbolSamples( {}, {} )", reqId, contractDescriptions.size()); }
	void WrapperLog::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)noexcept{ LOG( _logLevel, "WrapperLog::mktDepthExchanges( {} )", depthMktDataDescriptions.size()); }
	void WrapperLog::smartComponents(int reqId, const SmartComponentsMap& theMap)noexcept{ LOG( _logLevel, "WrapperLog::smartComponents( {}, {} )", reqId, theMap.size()); }
	void WrapperLog::newsProviders(const std::vector<NewsProvider> &newsProviders)noexcept{ LOG( _logLevel, "WrapperLog::newsProviders( {} )", newsProviders.size()); }
	void WrapperLog::newsArticle(int requestId, int articleType, str articleText)noexcept{ LOG( _logLevel, "WrapperLog::newsArticle( {}, {}, {} )", requestId, articleType, articleText); }
	void WrapperLog::historicalNews(int requestId, str time, str providerCode, str articleId, str /*headline*/)noexcept{ LOG( _logLevel, "WrapperLog::historicalNews( {}, {}, {}, {} )", requestId, time, providerCode, articleId); }
	void WrapperLog::historicalNewsEnd(int requestId, bool hasMore)noexcept{ LOG( _logLevel, "WrapperLog::historicalNewsEnd( {}, {} )", requestId, hasMore); }
	void WrapperLog::headTimestamp(int reqId, str headTimestamp)noexcept{ LOG( _logLevel, "WrapperLog::headTimestamp( {}, {} )", reqId, headTimestamp); }
	void WrapperLog::histogramData(int reqId, const HistogramDataVector& data)noexcept{ LOG( _logLevel, "WrapperLog::histogramData( {}, {} )", reqId, data.size()); }
	void WrapperLog::historicalDataUpdate(TickerId reqId, const ::Bar& bar)noexcept{ LOG( _logLevel, "WrapperLog::historicalDataUpdate( {}, {} )", reqId, bar.time); }
	void WrapperLog::rerouteMktDataReq(int reqId, int conid, str exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDataReq( {}, {}, {} )", reqId, conid, exchange); }
	void WrapperLog::rerouteMktDepthReq(int reqId, int conid, str exchange)noexcept{ LOG( _logLevel, "WrapperLog::rerouteMktDepthReq( {}, {}, {} )", reqId, conid, exchange); }
	void WrapperLog::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements)noexcept{ LOG( _logLevel, "WrapperLog::marketRule( {}, {} )", marketRuleId, priceIncrements.size()); }
	void WrapperLog::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)noexcept{ LOG( _logLevel, "WrapperLog::pnl( {}, {}, {}, {} )", reqId, dailyPnL, unrealizedPnL, realizedPnL); }
	void WrapperLog::pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double /*realizedPnL*/, double /*value*/)noexcept{ LOG( _logLevel, "WrapperLog::pnlSingle( {}, {}, {}, {} )", reqId, pos, dailyPnL, unrealizedPnL); }
	void WrapperLog::historicalTicks(int reqId, const std::vector<HistoricalTick>& ticks, bool done)noexcept{ LOG( _logLevel, "WrapperLog::historicalTicks( {}, {}, {} )", reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksBidAsk( int reqId, const std::vector<HistoricalTickBidAsk>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::historicalTicksBidAsk( {}, {}, {} )", reqId, ticks.size(), done); }
	void WrapperLog::historicalTicksLast( int reqId, const std::vector<HistoricalTickLast>& ticks, bool done )noexcept{ LOG( _logLevel, "WrapperLog::position( {}, {}, {} )", reqId, ticks.size(), done ); }
	void WrapperLog::tickEFP( TickerId tickerId, TickType tickType, double basisPoints, str formattedBasisPoints, double /*totalDividends*/, int /*holdDays*/, str /*futureLastTradeDate*/, double /*dividendImpact*/, double /*dividendsToLastTradeDate*/ )noexcept{ LOG( _logLevel, "WrapperLog::tickEFP( {}, {}, {}, {} )", tickerId, tickType, basisPoints, formattedBasisPoints ); }
	void WrapperLog::tickGeneric( TickerId t, TickType type, double v )noexcept
	{
		LOG( _tickLevel, "({})WrapperLog::tickGeneric( type='{}', value='{}' )", t, type, v );
		_pTickWorker->PushPrice( t, (ETickType)type, v );
	}
	void WrapperLog::tickOptionComputation( TickerId t, TickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept
	{
		LOG( _tickLevel, "({})WrapperLog::tickOptionComputation( type='{}', tickAttrib='{}', impliedVol='{}', delta='{}', optPrice='{}', pvDividend='{}', gamma='{}', vega='{}', theta='{}', undPrice='{}' )", t, type, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice );
		_pTickWorker->Push( t, (ETickType)type, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice );
	}
	void WrapperLog::tickPrice( TickerId t, TickType type, double v, const TickAttrib& attribs )noexcept
	{
		LOG( _tickLevel, "({})WrapperLog::tickPrice( type='{}', price='{}' )", t, type, v );
		_pTickWorker->PushPrice( t, (ETickType)type, v );
	}
	void WrapperLog::tickSize( TickerId t, TickType type, long long v )noexcept
	{
		LOG( _tickLevel, "({})WrapperLog::tickSize( type='{}', size='{}' )", t, type, v );
		_pTickWorker->Push( t, (ETickType)type, v );
	}
	void WrapperLog::tickString( TickerId t, TickType type, str v )noexcept
	{
		LOG( _tickLevel, "({})WrapperLog::tickString( type='{}', value='{}' )", t, type, v );
		_pTickWorker->Push( t, (ETickType)type, v );
	}
	void WrapperLog::tickNews( int tickerId, time_t timeStamp, str providerCode, str articleId, str headline, str extraData )noexcept
	{
		LOG( _logLevel, "WrapperLog::tickNews( {}, {}, {}, {} )", tickerId, timeStamp, providerCode, articleId);
		_pTickWorker->Push( tickerId, timeStamp, providerCode, articleId, headline, extraData );
	}
	void WrapperLog::tickSnapshotEnd( int t )noexcept{LOG( _tickLevel, "WrapperLog::tickSnapshotEnd( t='{}' )", t);}
	void WrapperLog::winError( str str, int lastError)noexcept{ LOG( _logLevel, "({})noexcept{}.", lastError, str ); }
	void WrapperLog::orderBound( long long orderId, int apiClientId, int apiOrderId )noexcept{ LOG( _logLevel, "WrapperLog::orderBound( {}, {}, {} )", orderId, apiClientId, apiOrderId ); }
	void WrapperLog::completedOrder( const ::Contract& contract, const ::Order& order, const ::OrderState& orderState )noexcept
	{
		OrderManager::Push( order, contract, orderState );

		LOG( _logLevel, "WrapperLog::openOrder( {}, {}@{}, {} )", contract.symbol, toString(order), orderState.status );
	}
	void WrapperLog::completedOrdersEnd()noexcept{LOG( _logLevel, "WrapperLog::completedOrdersEnd()"); }
	Handle WrapperLog::_accountUpdateHandle{0};
	unique_lock<shared_mutex>* _pUpdateLock{ nullptr };
	tuple<uint,bool> WrapperLog::AddAccountUpdate( sv account, sp<IAccountUpdateHandler> callback )noexcept
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

	bool WrapperLog::RemoveAccountUpdate( sv account, uint handle )noexcept
	{
		bool cancel = true;
		var pLock = _pUpdateLock ? unique_ptr<unique_lock<shared_mutex>>{} : make_unique<unique_lock<shared_mutex>>( _accountUpdateCallbackMutex );
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
	void WrapperLog::wshMetaData( int reqId, str dataJson )noexcept
	{
		LOG( _logLevel, "WrapperLog::wshMetaData( {}, {} )", reqId, dataJson );
	}
	void WrapperLog::wshEventData( int reqId, str dataJson )noexcept
	{
		LOG( _logLevel, "WrapperLog::wshEventData( {}, {} )", reqId, dataJson );
	}
}