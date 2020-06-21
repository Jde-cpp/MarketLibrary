#include "../Exports.h"
#include "WrapperLog.h"
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "../types/proto/results.pb.h"

namespace Jde::Markets
{
	//
	struct JDE_MARKETS_EXPORT WrapperCache : public WrapperLog
	{
		void AddCacheId( ReqId reqId, const string& cacheId ){ if( cacheId.size() ) _cacheIds.emplace(reqId, cacheId); }
		void contractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;
		void historicalData( TickerId reqId, const ibapi::Bar& bar )noexcept override;
		static void ToBar( const ibapi::Bar& bar, Proto::Results::Bar& proto )noexcept;
		void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)noexcept override;
		void securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept override;
		static Proto::Results::OptionParams ToOptionParam( const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
		//Proto::Results::OptionParams securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
		void securityDefinitionOptionalParameterEnd( int reqId )noexcept override;
	private:
		UnorderedMapValue<ReqId,string> _cacheIds;
		unordered_map<ReqId,sp<vector<ibapi::ContractDetails>>> _details;  mutable shared_mutex _detailsMutex;
		unordered_map<ReqId,sp<vector<Proto::Results::OptionParams>>> _optionParams;  mutable shared_mutex _optionParamsMutex;
		unordered_map<ReqId,sp<vector<Proto::Results::Bar>>> _historicalData;  mutable shared_mutex _historicalDataMutex;
	};
}