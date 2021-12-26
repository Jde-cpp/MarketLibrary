#pragma once
#include <EWrapper.h>
#include <jde/markets/Exports.h>
#include <jde/markets/types/proto/results.pb.h>
#include "../../../Framework/source/collections/UnorderedSet.h"
#include "../TickManager.h"

namespace Jde{ struct LogTag;}
namespace Jde::Markets
{
	struct IAccountUpdateHandler
	{
		β UpdateAccountValue( sv key, sv val, sv currency, sv accountName )noexcept->bool=0;
		β PortfolioUpdate( const Proto::Results::PortfolioUpdate& update )noexcept->bool=0;
		β AccountDownloadEnd( sv accountName )noexcept->void=0;
	};
#define $ noexcept->void override
	struct ΓM WrapperLog : public EWrapper
	{
		static bool IsStatusMessage( int errorCode ){ return errorCode==165  || (errorCode>2102 && errorCode<2108); }
		void tickPrice( TickerId tickerId, TickType field, double price, const ::TickAttrib& attrib )noexcept override;
		void tickSize( TickerId tickerId, TickType field, ::Decimal size )noexcept override;
		void tickGeneric( TickerId tickerId, TickType tickType, double value )noexcept override;
		void tickString( TickerId tickerId, TickType tickType, str value )noexcept override;
		void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, str formattedBasisPoints, double totalDividends, int holdDays, str futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate)noexcept override;
		void tickNews(int tickerId, time_t timeStamp, str providerCode, str articleId, str headline, str extraData)noexcept override;
		void tickSnapshotEnd( int reqId)noexcept override;
		void tickReqParams(int tickerId, double minTick, str bboExchange, int snapshotPermissions)noexcept override;
		void tickByTickAllLast(int reqId, int tickType, time_t time, double price, ::Decimal size, const TickAttribLast& tickAttribLast, str exchange, str specialConditions)noexcept override;
		void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, ::Decimal bidSize, ::Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)noexcept override;
		void tickByTickMidPoint(int reqId, time_t time, double midPoint)noexcept override;
		void orderStatus( ::OrderId orderId, str status, ::Decimal filled, ::Decimal remaining, double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId, str whyHeld, double mktCapPrice)noexcept override;
		void openOrder( ::OrderId orderId, const ::Contract&, const ::Order&, const ::OrderState&)noexcept override;
		void openOrderEnd()noexcept override;
		void winError( str str, int lastError)noexcept override;
		void connectionClosed()noexcept override;
		void updateAccountValue(str key, str val,	str currency, str accountName)noexcept override;
		bool updateAccountValue2( sv key, sv val, sv currency, sv accountName )noexcept;
		void updatePortfolio( const ::Contract& contract, ::Decimal position, double marketPrice, double marketValue, double averageCost, double unrealizedPNL, double realizedPNL, str accountName)noexcept override;
		void updateAccountTime(str timeStamp)noexcept override;
		void accountDownloadEnd(str accountName)noexcept override;
		void nextValidId( ::OrderId orderId )noexcept override;
		void contractDetails( int reqId, const ::ContractDetails& contractDetails)noexcept override;
		void bondContractDetails( int reqId, const ::ContractDetails& contractDetails)noexcept  override;
		void contractDetailsEnd( int reqId)noexcept override;
		void execDetails( int reqId, const ::Contract& contract, const Execution& execution)noexcept override;
		void execDetailsEnd( int reqId)noexcept override;
		void error( int id, int errorCode, str errorMsg )noexcept override;
		β error2( int id, int errorCode, str errorMsg )noexcept->bool;
		void updateMktDepth(TickerId id, int position, int operation, int side, double price, ::Decimal size)noexcept override;
		void updateMktDepthL2(TickerId id, int position, str marketMaker, int operation, int side, double price, ::Decimal size, bool isSmartDepth)noexcept override;
		void updateNewsBulletin(int msgId, int msgType, str newsMessage, str originExch)noexcept override;
		void managedAccounts( str accountsList)noexcept override;
		void receiveFA(faDataType pFaDataType, str cxml)noexcept override;
		void historicalData(TickerId reqId, const ::Bar& bar)noexcept override;
		void historicalDataEnd(int reqId, str startDateStr, str endDateStr)noexcept override;
		void scannerParameters(str xml)noexcept override;
		void scannerData(int reqId, int rank, const ::ContractDetails& contractDetails, str distance, str benchmark, str projection,	str legsStr)noexcept override;
		void scannerDataEnd(int reqId)noexcept override;
		void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close, ::Decimal volume, ::Decimal wap, int count)noexcept override;
		void currentTime(long time)noexcept override;
		void fundamentalData(TickerId reqId, str data)noexcept override;
		void deltaNeutralValidation(int reqId, const ::DeltaNeutralContract& deltaNeutralContract)noexcept override;
		void marketDataType( TickerId reqId, int marketDataType)noexcept override;
		void commissionReport( const CommissionReport& commissionReport)noexcept override;
		void position( str account, const ::Contract& contract, ::Decimal position, double avgCost )noexcept override;
		void positionEnd()noexcept override;
		void accountSummary( int reqId, str account, str tag, str value, str curency)noexcept override;
		void accountSummaryEnd( int reqId)noexcept override;
		void verifyMessageAPI( str apiData)noexcept override;
		void verifyCompleted( bool isSuccessful, str errorText)noexcept override;
		void displayGroupList( int reqId, str groups)noexcept override;
		void displayGroupUpdated( int reqId, str contractInfo)noexcept override;
		void verifyAndAuthMessageAPI( str apiData, str xyzChallange)noexcept override;
		void verifyAndAuthCompleted( bool isSuccessful, str errorText)noexcept override;
		void connectAck()noexcept override;
		void positionMulti( int reqId, str account,str modelCode, const ::Contract& contract, ::Decimal pos, double avgCost)noexcept override;
		void positionMultiEnd( int reqId)noexcept override;
		void accountUpdateMulti( int reqId, str account, str modelCode, str key, str value, str currency)noexcept override;
		void accountUpdateMultiEnd( int reqId)noexcept override;
		void securityDefinitionOptionalParameter(int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes)noexcept override;
		void securityDefinitionOptionalParameterEnd(int reqId)noexcept override;
		void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers)noexcept override;
		void familyCodes( const std::vector<FamilyCode> &familyCodes )noexcept override;
		void symbolSamples( int reqId, const std::vector<::ContractDescription> &contractDescriptions )noexcept override;
		void mktDepthExchanges( const std::vector<DepthMktDataDescription> &depthMktDataDescriptions )noexcept override;
		void smartComponents( int reqId, const SmartComponentsMap& theMap )noexcept override;
		void newsProviders( const std::vector<NewsProvider> &newsProviders )noexcept override;
		void newsArticle( int requestId, int articleType, str articleText )noexcept override;
		void historicalNews( int requestId, str time, str providerCode, str articleId, str headline )noexcept override;
		void historicalNewsEnd( int requestId, bool hasMore )noexcept override;
		void headTimestamp( int reqId, str headTimestamp )noexcept override;
		void histogramData( int reqId, const HistogramDataVector& data )noexcept override;
		void historicalDataUpdate( TickerId reqId, const ::Bar& bar )noexcept override;
		void rerouteMktDataReq( int reqId, int conid, str exchange )noexcept override;
		void rerouteMktDepthReq( int reqId, int conid, str exchange )noexcept override;
		void marketRule( int marketRuleId, const std::vector<PriceIncrement> &priceIncrements )noexcept override;
		void pnl( int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL )noexcept override;
		void pnlSingle( int reqId, ::Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL, double value )noexcept override;
		α historicalSchedule( int reqId, str startDateTime, str endDateTime, str timeZone, const vector<HistoricalSession>& sessions )$;
		α historicalTicks( int reqId, const std::vector<HistoricalTick> &ticks, bool done )$;
		α historicalTicksBidAsk( int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done )$;
		α historicalTicksLast( int reqId, const std::vector<HistoricalTickLast> &ticks, bool done )$;
		α orderBound( long long orderId, int apiClientId, int apiOrderId )$;
		α completedOrder( const ::Contract& contract, const ::Order& order, const ::OrderState& orderState )$;
		α completedOrdersEnd()$;
		void tickOptionComputation( ::TickerId tickerId, ::TickType tickType, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept override;
		void replaceFAEnd( int /*reqId*/, str /*text*/ )noexcept override{};
		void wshMetaData( int reqId, str dataJson )noexcept override;
		void wshEventData( int reqId, str dataJson )noexcept override;

		//ELogLevel GetLogLevel()const noexcept{ return _logLevel; }
		uint HistoricalDataRequestSize()const noexcept{ return _historicalDataRequests.size(); }
		void AddHistoricalDataRequest2( TickerId id )noexcept{ _historicalDataRequests.emplace(id); }
		tuple<uint,bool> AddAccountUpdate( sv accountNumber, sp<IAccountUpdateHandler> callback )noexcept;
		bool RemoveAccountUpdate( sv account, uint handle )noexcept;

	protected:
		UnorderedSet<TickerId> _historicalDataRequests;
		static const LogTag& _logLevel;
		static const LogTag& _historicalLevel;
		static const LogTag& _tickLevel;
		sp<TickManager::TickWorker> _pTickWorker;
		flat_map<string, flat_map<Handle,sp<IAccountUpdateHandler>>> _accountUpdateCallbacks; static uint _accountUpdateHandle; flat_map<string,flat_map<string,tuple<string,string>>> _accountUpdateCache; flat_map<string,flat_map<ContractPK,Proto::Results::PortfolioUpdate>> _accountPortfolioUpdates; shared_mutex _accountUpdateCallbackMutex;		friend TickManager::TickWorker;
	};
}
#undef $