#include "./WrapperLog.h"
#include "../client/TwsClientCo.h"
#include <jde/markets/types/Contract.h>
#include "../../../Framework/source/coroutine/Coroutine.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"

namespace Jde::Markets
{
	struct TwsClientCo;
	using namespace Jde::Coroutine;
	struct ΓM WrapperCo : WrapperLog
	{
		bool error2( int id, int errorCode, str errorMsg )noexcept override;
		void error( int id, int errorCode, str errorMsg )noexcept override;
		void historicalNews( int requestId, str time, str providerCode, str articleId, str headline )noexcept override;
		void historicalNewsEnd( int requestId, bool hasMore )noexcept override;

		void historicalData( TickerId reqId, const ::Bar& bar )noexcept override{ HistoricalData(reqId, bar); }
		void historicalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept override{ HistoricalDataEnd(reqId, startDateStr, endDateStr); }
		bool HistoricalData( TickerId reqId, const ::Bar& bar )noexcept;
		bool HistoricalDataEnd( int reqId, str startDateStr, str endDateStr )noexcept;

		void contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;
		void newsProviders( const vector<NewsProvider>& providers )noexcept override;
		void newsArticle( int reqId, int articleType, str articleText )noexcept override;

		void securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept override;
		void securityDefinitionOptionalParameterEnd( int reqId )noexcept override;
	protected:
		flat_map<int,vector<sp<Contract>>> _contracts; UnorderedMapValue<int,ContractAwaitable::THandle> _contractSingleHandles;
	private:
		flat_map<int,sp<Proto::Results::NewsCollection>> _news;UnorderedMapValue<int,HistoricalNewsAwaitable::THandle> _newsHandles;
		UnorderedSet<NewsProviderAwaitable::THandle> _newsProviderHandles;
		UnorderedMapValue<int,NewsArticleAwaitable::THandle> _newsArticleHandles;
		flat_map<int,up<Proto::Results::OptionExchanges>> _optionParams; UnorderedMapValue<int,HCoroutine> _secDefOptParamHandles;
		UnorderedMapValue<int,HistoryAwait*> _historical;
		flat_map<TickerId,vector<::Bar>> _historicalData;

		friend TwsClientCo; friend HistoricalNewsAwaitable; friend ContractAwaitable; friend NewsProviderAwaitable; friend NewsArticleAwaitable; friend HistoryAwait; friend SecDefOptParamAwaitable;
	};
}