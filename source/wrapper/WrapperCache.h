#include <jde/markets/Exports.h>
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "WrapperCo.h"

namespace Jde::Markets
{
	//
	struct ΓM WrapperCache : public WrapperCo
	{
		α AddCacheId( ReqId reqId, str cacheId )->void{ if( cacheId.size() ) _cacheIds.emplace(reqId, cacheId); }//reqContractDetails.OPT.CALL.20230120.330.0.SPY
		α contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept->void override;
		α contractDetailsEnd( int reqId )noexcept->void override;

	private:
		UnorderedMapValue<ReqId,string> _cacheIds;
		unordered_map<ReqId,sp<vector<::ContractDetails>>> _details;  mutable shared_mutex _detailsMutex;
		unordered_map<ReqId,sp<vector<Proto::Results::Bar>>> _historicalData;  mutable shared_mutex _historicalDataMutex;
	};
}
