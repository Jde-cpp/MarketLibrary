#include <jde/markets/Exports.h>
#include "../../../Framework/source/coroutine/Coroutine.h"
#include "./WrapperLog.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"
//#include <jde/markets/types/proto/results.pb.h>

namespace Jde::Markets
{
	struct Contract;struct TwsClientCo; struct HistoryAwait; struct HistoricalNewsAwait; struct ContractAwait; struct NewsProviderAwait; struct NewsArticleAwait; struct SecDefOptParamAwait; struct AccountsAwait; struct PlaceOrderAwait; struct HeadTimestampAwait;
	using namespace Jde::Coroutine;
#define $ noexcept->void override
	struct ΓM WrapperCo : WrapperLog
	{
		α error2( int id, int errorCode, str errorMsg )noexcept->bool override;
		α error( int id, int errorCode, str errorMsg, str advancedOrderRejectJson )$;
		α headTimestamp( int reqId, str headTimestamp )$;
		α historicalNews( int requestId, str time, str providerCode, str articleId, str headline )$;
		α historicalNewsEnd( int requestId, bool hasMore )$;

		α historicalData( TickerId reqId, const ::Bar& bar )$;
		α historicalDataEnd( int reqId, str startDateStr, str endDateStr )$;

		α contractDetails( int reqId, const ::ContractDetails& contractDetails )$;
		α contractDetailsEnd( int reqId )$;
		α managedAccounts( str accountsList )$;

		α newsProviders( const vector<NewsProvider>& providers )$;
		α newsArticle( int reqId, int articleType, str articleText )$;

		α securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )$;
		α securityDefinitionOptionalParameterEnd( int reqId )$;
		α OpenOrder( ::OrderId orderId, const ::Contract& contract, const ::Order& order, const ::OrderState& state )noexcept->sp<Proto::Results::OpenOrder>;
	protected:
		flat_map<ReqId,up<vector<sp<::ContractDetails>>>> _requestContracts; UnorderedMapValue<ReqId,HCoroutine> _contractHandles;
	private:
		flat_map<int,up<Proto::Results::NewsCollection>> _news;UnorderedMapValue<int,HCoroutine> _newsHandles;
		UnorderedSet<HCoroutine> _newsProviderHandles;
		UnorderedMapValue<int,HCoroutine> _newsArticleHandles;
		flat_map<ReqId,sp<Proto::Results::OptionExchanges>> _optionParams; UnorderedMapValue<ReqId,HCoroutine> _secDefOptParamHandles;
		UnorderedMapValue<int,HistoryAwait*> _historical;
		HCoroutine _accountHandle;
		UnorderedMapValue<::OrderId,HCoroutine> _orderHandles;
		UnorderedMapValue<ReqId,HCoroutine> _headTimestampHandles;

		friend TwsClientCo; friend HistoricalNewsAwait; friend ContractAwait; friend NewsProviderAwait; friend NewsArticleAwait; friend HistoryAwait; friend SecDefOptParamAwait; friend AccountsAwait; friend PlaceOrderAwait; friend HeadTimestampAwait;
	};
}
#undef $