#include "./WrapperLog.h"
#include "../client/TwsClientCo.h"
#include <jde/markets/types/Contract.h>
#include "../../../Framework/source/coroutine/Coroutine.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"

namespace Jde::Markets
{
	struct TwsClientCo;
	using namespace Jde::Coroutine;
	struct JDE_MARKETS_EXPORT WrapperCo : WrapperLog
	{
		bool error2( int id, int errorCode, str errorMsg )noexcept override;
		void error( int id, int errorCode, str errorMsg )noexcept override;
		//using base=WrapperSync;
		//void AddParam( TickerId id, coroutine_handle<>&& h )noexcept{ _params.emplace( id, move(h) ); }

		void historicalNews( int requestId, str time, str providerCode, str articleId, str headline )noexcept override;
		void historicalNewsEnd( int requestId, bool hasMore )noexcept override;

		void contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;
		void newsProviders( const vector<NewsProvider>& providers )noexcept override;
		void newsArticle( int reqId, int articleType, str articleText )noexcept override;
	protected:
		flat_map<int,vector<sp<Contract>>> _contracts; UnorderedMapValue<int,ContractAwaitable::THandle> _contractSingleHandles;
	private:
		flat_map<int,sp<Proto::Results::NewsCollection>> _news;UnorderedMapValue<int,HistoricalNewsAwaitable::THandle> _newsHandles;
		UnorderedSet<NewsProviderAwaitable::THandle> _newsProviderHandles;
		UnorderedMapValue<int,NewsArticleAwaitable::THandle> _newsArticleHandles;
		friend TwsClientCo; friend HistoricalNewsAwaitable; friend ContractAwaitable; friend NewsProviderAwaitable; friend NewsArticleAwaitable;
	};
}