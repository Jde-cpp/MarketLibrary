#include <jde/markets/Exports.h>
#include "WrapperCo.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include <jde/markets/types/proto/results.pb.h>
#include <jde/markets/TypeDefs.h>
namespace Jde::Markets
{
	//
	struct JDE_MARKETS_EXPORT WrapperCache : public WrapperCo
	{
		void AddCacheId( ReqId reqId, str cacheId ){ if( cacheId.size() ) _cacheIds.emplace(reqId, cacheId); }//reqContractDetails.OPT.CALL.20230120.330.0.SPY
		void contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;
		//void historicalData( TickerId reqId, const ::Bar& bar )noexcept override;
		//static void ToBar( const ::Bar& bar, Proto::Results::Bar& proto )noexcept;
		//void historicalDataEnd(int reqId, str startDateStr, str endDateStr)noexcept override;
		//void securityDefinitionOptionalParameter( int reqId, str exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept override;
		//static Proto::Results::ExchangeContracts ToOptionParam( sv exchange, int underlyingConId, str tradingClass, str multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
		//void securityDefinitionOptionalParameterEnd( int reqId )noexcept override;
	private:
		UnorderedMapValue<ReqId,string> _cacheIds;
		unordered_map<ReqId,sp<vector<::ContractDetails>>> _details;  mutable shared_mutex _detailsMutex;
		//flat_map<int,up<Proto::Results::OptionExchanges>> _optionParams;
		unordered_map<ReqId,sp<vector<Proto::Results::Bar>>> _historicalData;  mutable shared_mutex _historicalDataMutex;
	};
}