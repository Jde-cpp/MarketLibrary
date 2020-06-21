#pragma once
#include <future>
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "TwsClientCache.h"
#include "../Exports.h"
#include "../types/Contract.h"
#include "../types/proto/results.pb.h"

struct EReaderSignal;

namespace Jde::Markets
{
	struct WrapperSync;
	struct IBException;
	struct TwsConnectionSettings;
	struct OptionsData;
	struct JDE_MARKETS_EXPORT TwsClientSync : public TwsClientCache
	{
		template<class T> using Container = sp<std::vector<T>>;
		template<class T> using Future = std::future<sp<std::vector<T>>>;
		static void CreateInstance( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		TimePoint CurrentTime()noexcept;
		TimePoint HeadTimestamp( const ibapi::Contract &contract, const std::string& whatToShow )noexcept(false);

		//Future<ibapi::Bar> ReqHistoricalData( const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate )noexcept(false);
		Future<ibapi::Bar> ReqHistoricalData( ContractPK contractId, DayIndex endDay, uint dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool cache=false, bool useRTH=true )noexcept;
		Future<ibapi::ContractDetails> ReqContractDetails( string_view symbol )noexcept;
		Future<ibapi::ContractDetails> ReqContractDetails( ContractPK id )noexcept;
		Future<ibapi::ContractDetails> ReqContractDetails( const ibapi::Contract& contract )noexcept;
		Proto::Results::OptionParams ReqSecDefOptParamsSmart( ContractPK underlyingConId, string_view symbol )noexcept(false);
		Future<Proto::Results::OptionParams> ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept;
		//void reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol=""sv, string_view futFopExchange="", string_view underlyingSecType="STK" )noexcept override;
		std::future<sp<string>> ReqFundamentalData( const ibapi::Contract &contract, string_view reportType )noexcept;
		std::future<sp<map<string,double>>> ReqRatios( const ibapi::Contract &contract )noexcept;

		//vector<ActiveOrderPtr> ReqAllOpenOrders()noexcept(false);//timeout
		static TwsClientSync& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
	private:
		void ReqIds()noexcept;
		void OnError( TickerId id, int errorCode, const std::string& errorMsg );
		void OnHeadTimestamp( TickerId reqId, TimePoint t );
		//void OnReqHistoricalData( TickerId reqId, sp<list<ibapi::Bar>> pBars );
		shared_ptr<WrapperSync> Wrapper()noexcept;
		//static sp<TwsClientSync> _pInstance;
		TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		Collections::UnorderedMap<TickerId,condition_variable> _conditionVariables;
		Collections::UnorderedMap<TickerId,IBException> _errors;
		UnorderedMapValue<TickerId,TimePoint> _headTimestamps;

		Collections::UnorderedMap<TickerId,list<ibapi::Bar>> _historicalData;
	};
}