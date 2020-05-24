#pragma once
// #include <future>
// #include "../../Framework/source/collections/UnorderedMap.h"
#include "TwsClient.h"
#include "../Exports.h"
// #include "types/proto/results.pb.h"

//struct EReaderSignal;

namespace Jde::Markets
{
	struct WrapperCache;
	enum class SecurityRight : uint8;
	 struct TwsConnectionSettings;
	// struct OptionsData;
	struct JDE_MARKETS_EXPORT TwsClientCache : public TwsClient
	{
		//static TwsClientCache& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		TwsClientCache( const TwsConnectionSettings& settings, shared_ptr<WrapperCache> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		shared_ptr<WrapperCache> Wrapper()noexcept;
		static ibapi::Contract ToContract( string_view symbol, DayIndex dayIndex, SecurityRight isCall )noexcept;
		void ReqContractDetails( TickerId cacheReqId, const ibapi::Contract& contract )noexcept;
		void ReqSecDefOptParams( TickerId reqId, ContractPK underlyingConId, string_view symbol )noexcept;

		//UnorderedMapValue<ReqId,string> _cacheIds;
	};
}