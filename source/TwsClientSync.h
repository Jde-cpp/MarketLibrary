#pragma once
#include <future>
#include "../../MarketLibrary/source/TwsClient.h"
#include "Exports.h"
#include "../../Framework/source/collections/UnorderedMapValue.h"
#include "../../Framework/source/collections/UnorderedMap.h"
struct EReaderSignal;

namespace Jde::Markets
{
	struct WrapperSync;
	struct IBException;
	struct TwsConnectionSettings;
	struct OptionsData;
	struct JDE_MARKETS_EXPORT TwsClientSync : public TwsClient
	{
		static void CreateInstance( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		TimePoint CurrentTime()noexcept;
		TimePoint HeadTimestamp( const ibapi::Contract &contract, const std::string& whatToShow )noexcept(false);

		std::future<sp<list<ibapi::Bar>>> ReqHistoricalData( const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate )noexcept(false);
		std::future<sp<list<ibapi::ContractDetails>>> ReqContractDetails( ContractPK id )noexcept;
		std::future<sp<list<ibapi::ContractDetails>>> ReqContractDetails( const ibapi::Contract& contract )noexcept;
		std::future<sp<list<OptionsData>>> ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept(false);
		std::future<sp<string>> ReqFundamentalData( const ibapi::Contract &contract, string_view reportType )noexcept;
		std::future<sp<map<string,double>>> ReqRatios( const ibapi::Contract &contract )noexcept;

		//vector<ActiveOrderPtr> ReqAllOpenOrders()noexcept(false);//timeout
		static TwsClientSync& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
	private:
		void ReqIds()noexcept;
		void OnError( TickerId id, int errorCode, const std::string& errorMsg );
		void OnHeadTimestamp( TickerId reqId, TimePoint t );
		void OnReqHistoricalData( TickerId reqId, sp<list<ibapi::Bar>> pBars );
		sp<WrapperSync> Wrapper()noexcept{ return std::dynamic_pointer_cast<WrapperSync>(_pWrapper); }
		//static sp<TwsClientSync> _pInstance;
		TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		Collections::UnorderedMap<TickerId,condition_variable> _conditionVariables;
		Collections::UnorderedMap<TickerId,IBException> _errors;
		UnorderedMapValue<TickerId,TimePoint> _headTimestamps;

		Collections::UnorderedMap<TickerId,list<ibapi::Bar>> _historicalData;
	};
}