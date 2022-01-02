#include <jde/markets/Exports.h>
#include "../../../Framework/source/coroutine/Coroutine.h"
#include "./WrapperLog.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"
//#include <jde/markets/types/proto/results.pb.h>

namespace Jde::Markets
{
	struct Contract;struct TwsClientCo; struct HistoryAwait; struct HistoricalNewsAwait; struct ContractAwait; struct NewsProviderAwait; struct NewsArticleAwait; struct SecDefOptParamAwait; struct AccountsAwait; struct PlaceOrderAwait;
	using namespace Jde::Coroutine;
	struct ΓM WrapperCo : WrapperLog
	{
		α error2( int id, int errorCode, str errorMsg )noexcept->bool override;
		α error( int id, int errorCode, str errorMsg )noexcept->void override;
		α historicalNews( int requestId, str time, str providerCode, str articleId, str headline )noexcept->void override;
		α historicalNewsEnd( int requestId, bool hasMore )noexcept->void override;

		α historicalData( TickerId reqId, const ::Bar& bar )noexcept->void override{ HistoricalData(reqId, bar); }
		α historicalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept->void override{ HistoricalDataEnd(reqId, startDateStr, endDateStr); }
		α HistoricalData( TickerId reqId, const ::Bar& bar )noexcept->bool;
		α HistoricalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept->bool;

		α contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept->void override;
		α contractDetailsEnd( int reqId )noexcept->void override;
		α managedAccounts( str accountsList )noexcept->void override;

		α newsProviders( const vector<NewsProvider>& providers )noexcept->void override;
		α newsArticle( int reqId, int articleType, str articleText )noexcept->void override;

		α securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept->void override;
		α securityDefinitionOptionalParameterEnd( int reqId )noexcept->void override;
		α OpenOrder( ::OrderId orderId, const ::Contract& contract, const ::Order& order, const ::OrderState& state )noexcept->sp<Proto::Results::OpenOrder>;
	protected:
		flat_map<ReqId,up<vector<sp<::ContractDetails>>>> _requestContracts; UnorderedMapValue<ReqId,HCoroutine> _contractHandles;
	private:
		flat_map<int,up<Proto::Results::NewsCollection>> _news;UnorderedMapValue<int,HCoroutine> _newsHandles;
		UnorderedSet<HCoroutine> _newsProviderHandles;
		UnorderedMapValue<int,HCoroutine> _newsArticleHandles;
		flat_map<ReqId,sp<Proto::Results::OptionExchanges>> _optionParams; UnorderedMapValue<ReqId,HCoroutine> _secDefOptParamHandles;
		UnorderedMapValue<int,HistoryAwait*> _historical;
		flat_map<TickerId,vector<::Bar>> _historicalData;
		HCoroutine _accountHandle;
		UnorderedMapValue<::OrderId,HCoroutine> _orderHandles;

		friend TwsClientCo; friend HistoricalNewsAwait; friend ContractAwait; friend NewsProviderAwait; friend NewsArticleAwait; friend HistoryAwait; friend SecDefOptParamAwait; friend AccountsAwait; friend PlaceOrderAwait;
	};
}