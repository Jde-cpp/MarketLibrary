#pragma once
#include <future>
//#include <.h>
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "TwsClientCache.h"
#include "../Exports.h"
#include "../types/Contract.h"
#include "../types/proto/results.pb.h"

struct EReaderSignal;
struct NewsProvider;
namespace Jde::Markets
{
	struct WrapperSync;
	struct IBException;
	struct TwsConnectionSettings;
	struct OptionsData;
	struct JDE_MARKETS_EXPORT TwsClientSync : public TwsClientCache
	{
		template<class T> using Container = VectorPtr<T>;
		template<class T> using Future = std::future<Container<T>>;
		static void CreateInstance( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		TimePoint CurrentTime()noexcept;
		TimePoint HeadTimestamp( const ::Contract &contract, const std::string& whatToShow )noexcept(false);
		void CheckTimeouts()noexcept override;

		Future<::Bar> ReqHistoricalDataSync( const Contract& contract, DayIndex end, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false) override;
		Future<::Bar> ReqHistoricalDataSync( const Contract& contract, time_t start, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept override;

		Future<::ContractDetails> ReqContractDetails( string_view symbol )noexcept;
		Future<::ContractDetails> ReqContractDetails( ContractPK id )noexcept;
		Future<::ContractDetails> ReqContractDetails( const ::Contract& contract )noexcept;
		Proto::Results::OptionParams ReqSecDefOptParamsSmart( ContractPK underlyingConId, string_view symbol )noexcept(false);
		Future<Proto::Results::OptionParams> ReqSecDefOptParams( ContractPK underlyingConId, string_view symbol )noexcept;
		//void reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol=""sv, string_view futFopExchange="", string_view underlyingSecType="STK" )noexcept override;
		std::future<sp<string>> ReqFundamentalData( const ::Contract &contract, string_view reportType )noexcept;
		Future<NewsProvider> RequestNewsProviders( ReqId sessionId )noexcept;
		std::future<VectorPtr<Proto::Results::Position>> RequestPositions()noexcept(false);
		std::future<sp<map<string,double>>> ReqRatios( const ::Contract &contract )noexcept;


		//vector<ActiveOrderPtr> ReqAllOpenOrders()noexcept(false);//timeout
		static TwsClientSync& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		static bool IsConnected()noexcept;
	private:
		void ReqIds()noexcept;
		void OnError( TickerId id, int errorCode, const std::string& errorMsg );
		void OnHeadTimestamp( TickerId reqId, TimePoint t );
		shared_ptr<WrapperSync> Wrapper()noexcept;
		//static sp<TwsClientSync> _pInstance;
		TwsClientSync( const TwsConnectionSettings& settings, shared_ptr<WrapperSync> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		Collections::UnorderedMap<TickerId,std::condition_variable> _conditionVariables;
		Collections::UnorderedMap<TickerId,IBException> _errors;
		UnorderedMapValue<TickerId,TimePoint> _headTimestamps;

		Collections::UnorderedMap<TickerId,list<::Bar>> _historicalData;
	};
}