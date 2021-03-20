#pragma once
#include <EWrapper.h>
#include "../Exports.h"
#include "../../../Framework/source/collections/UnorderedSet.h"
#include "../TickManager.h"

namespace Jde::Markets
{
	struct IAccountUpdateHandler
	{
		virtual bool UpdateAccountValue( sv key, sv val, sv currency, sv accountName )noexcept=0;
		virtual bool PortfolioUpdate( const Proto::Results::PortfolioUpdate& update )noexcept=0;
		virtual void AccountDownloadEnd( sv accountName )noexcept=0;
	};
	struct JDE_MARKETS_EXPORT WrapperLog : public EWrapper
	{
		static bool IsStatusMessage( int errorCode ){ return errorCode==165  || (errorCode>2102 && errorCode<2108); }
		void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib)noexcept override;
		void tickSize( TickerId tickerId, TickType field, int size)noexcept override;
		void tickGeneric(TickerId tickerId, TickType tickType, double value)noexcept override;
		void tickString(TickerId tickerId, TickType tickType, const std::string& value)noexcept override;
		void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints, double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate)noexcept override;
		void tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData)noexcept override;
		void tickSnapshotEnd( int reqId)noexcept override;
		void tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions)noexcept override;
		void tickByTickAllLast(int reqId, int tickType, time_t time, double price, int size, const TickAttribLast& tickAttribLast, const std::string& exchange, const std::string& specialConditions)noexcept override;
		void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, int bidSize, int askSize, const TickAttribBidAsk& tickAttribBidAsk)noexcept override;
		void tickByTickMidPoint(int reqId, time_t time, double midPoint)noexcept override;
		void orderStatus( ::OrderId orderId, const std::string& status, double filled, double remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)noexcept override;
		void openOrder( ::OrderId orderId, const ::Contract&, const ::Order&, const ::OrderState&)noexcept override;
		void openOrderEnd()noexcept override;
		void winError( const std::string& str, int lastError)noexcept override;
		void connectionClosed()noexcept override;
		void updateAccountValue(const std::string& key, const std::string& val,	const std::string& currency, const std::string& accountName)noexcept override;
		bool updateAccountValue2( sv key, sv val, sv currency, sv accountName )noexcept;
		void updatePortfolio( const ::Contract& contract, double position,	double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)noexcept override;
		void updateAccountTime(const std::string& timeStamp)noexcept override;
		void accountDownloadEnd(const std::string& accountName)noexcept override;
		void nextValidId( ::OrderId orderId )noexcept override;
		void contractDetails( int reqId, const ::ContractDetails& contractDetails)noexcept override;
		void bondContractDetails( int reqId, const ::ContractDetails& contractDetails)noexcept  override;
		void contractDetailsEnd( int reqId)noexcept override;
		void execDetails( int reqId, const ::Contract& contract, const Execution& execution)noexcept override;
		void execDetailsEnd( int reqId)noexcept override;
		void error( int id, int errorCode, const std::string& errorMsg )noexcept override;
		bool error2( int id, int errorCode, const std::string& errorMsg )noexcept;
		void updateMktDepth(TickerId id, int position, int operation, int side, double price, int size)noexcept override;
		void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation, int side, double price, int size, bool isSmartDepth)noexcept override;
		void updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch)noexcept override;
		void managedAccounts( const std::string& accountsList)noexcept override;
		void receiveFA(faDataType pFaDataType, const std::string& cxml)noexcept override;
		void historicalData(TickerId reqId, const ::Bar& bar)noexcept override;
		void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)noexcept override;
		void scannerParameters(const std::string& xml)noexcept override;
		void scannerData(int reqId, int rank, const ::ContractDetails& contractDetails, const std::string& distance, const std::string& benchmark, const std::string& projection,	const std::string& legsStr)noexcept override;
		void scannerDataEnd(int reqId)noexcept override;
		void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,	long volume, double wap, int count)noexcept override;
		void currentTime(long time)noexcept override;
		void fundamentalData(TickerId reqId, const std::string& data)noexcept override;
		void deltaNeutralValidation(int reqId, const ::DeltaNeutralContract& deltaNeutralContract)noexcept override;
		void marketDataType( TickerId reqId, int marketDataType)noexcept override;
		void commissionReport( const CommissionReport& commissionReport)noexcept override;
		void position( const std::string& account, const ::Contract& contract, double position, double avgCost)noexcept override;
		void positionEnd()noexcept override;
		void accountSummary( int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& curency)noexcept override;
		void accountSummaryEnd( int reqId)noexcept override;
		void verifyMessageAPI( const std::string& apiData)noexcept override;
		void verifyCompleted( bool isSuccessful, const std::string& errorText)noexcept override;
		void displayGroupList( int reqId, const std::string& groups)noexcept override;
		void displayGroupUpdated( int reqId, const std::string& contractInfo)noexcept override;
		void verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange)noexcept override;
		void verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText)noexcept override;
		void connectAck()noexcept override;
		void positionMulti( int reqId, const std::string& account,const std::string& modelCode, const ::Contract& contract, double pos, double avgCost)noexcept override;
		void positionMultiEnd( int reqId)noexcept override;
		void accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency)noexcept override;
		void accountUpdateMultiEnd( int reqId)noexcept override;
		void securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes)noexcept override;
		void securityDefinitionOptionalParameterEnd(int reqId)noexcept override;
		void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)noexcept override;
		void familyCodes(const std::vector<FamilyCode> &familyCodes)noexcept override;
		void symbolSamples(int reqId, const std::vector<::ContractDescription> &contractDescriptions)noexcept override;
		void mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)noexcept override;
		void smartComponents(int reqId, const SmartComponentsMap& theMap)noexcept override;
		void newsProviders(const std::vector<NewsProvider> &newsProviders)noexcept override;
		void newsArticle(int requestId, int articleType, const std::string& articleText)noexcept override;
		void historicalNews(int requestId, const std::string& time, const std::string& providerCode, const std::string& articleId, const std::string& headline)noexcept override;
		void historicalNewsEnd(int requestId, bool hasMore)noexcept override;
		void headTimestamp(int reqId, const std::string& headTimestamp)noexcept override;
		void histogramData(int reqId, const HistogramDataVector& data)noexcept override;
		void historicalDataUpdate(TickerId reqId, const ::Bar& bar)noexcept override;
		void rerouteMktDataReq(int reqId, int conid, const std::string& exchange)noexcept override;
		void rerouteMktDepthReq(int reqId, int conid, const std::string& exchange)noexcept override;
		void marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements)noexcept override;
		void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL)noexcept override;
		void pnlSingle(int reqId, int pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value)noexcept override;
		void historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done)noexcept override;
		void historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done)noexcept override;
		void historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done)noexcept override;
		void orderBound(long long orderId, int apiClientId, int apiOrderId)noexcept override;
		void completedOrder(const ::Contract& contract, const ::Order& order, const ::OrderState& orderState)noexcept override;
		void completedOrdersEnd()noexcept override;
		//TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice
		void tickOptionComputation(::TickerId tickerId, ::TickType tickType, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)noexcept override;
		void replaceFAEnd( int /*reqId*/, const std::string& /*text*/ )noexcept override{};

		ELogLevel GetLogLevel()const noexcept{ return _logLevel; }
		uint HistoricalDataRequestSize()const noexcept{ return _historicalDataRequests.size(); }
		void AddHistoricalDataRequest2( TickerId id )noexcept{ _historicalDataRequests.emplace(id); }
		tuple<uint,bool> AddAccountUpdate( sv accountNumber, sp<IAccountUpdateHandler> callback )noexcept;
		bool RemoveAccountUpdate( sv account, uint handle )noexcept;
	protected:
		UnorderedSet<TickerId> _historicalDataRequests;
		ELogLevel _logLevel{ ELogLevel::Debug };
		ELogLevel _tickLevel{ ELogLevel::Trace };
		sp<TickManager::TickWorker> _pTickWorker;
		flat_map<string, flat_map<Handle,sp<IAccountUpdateHandler>>> _accountUpdateCallbacks; static uint _accountUpdateHandle; flat_map<string,flat_map<string,tuple<string,string>>> _accountUpdateCache; flat_map<string,flat_map<ContractPK,Proto::Results::PortfolioUpdate>> _accountPortfolioUpdates; shared_mutex _accountUpdateCallbackMutex;		friend TickManager::TickWorker;
	};
}