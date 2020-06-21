#pragma once
// #include <future>
#include "../../../MarketLibrary/source/types/Bar.h"
#include "TwsClient.h"
#include "../Exports.h"
#include "../types/proto/requests.pb.h"

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
		void ReqHistoricalData( TickerId reqId, ContractPK contractId, DayIndex current, uint dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useCache, bool useRth )noexcept;

		//UnorderedMapValue<ReqId,string> _cacheIds;
	};
}