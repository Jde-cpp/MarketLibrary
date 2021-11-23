#pragma once
#include <future>
//#include <.h>
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "TwsClientCache.h"
#include <jde/markets/Exports.h>
#include <jde/markets/types/Contract.h>
#include <jde/markets/types/proto/results.pb.h>

struct EReaderSignal;
struct NewsProvider;
namespace Jde::Markets
{
	struct WrapperSync;
	struct IBException;
	struct TwsConnectionSettings;
	struct OptionsData;
	struct ΓM TwsClientSync : public TwsClientCache
	{
		template<class T> using Container = VectorPtr<T>;
		template<class T> using Future = std::future<Container<T>>;
		static sp<TwsClientSync> CreateInstance( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);

		α CurrentTime()noexcept->TimePoint;
		α HeadTimestamp( const ::Contract &contract, const std::string& whatToShow )noexcept(false)->TimePoint;
		α CheckTimeouts()noexcept->void override;

		//Future<::Bar> ReqHistoricalDataSync( const Contract& contract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false) override;
		//Future<::Bar> ReqHistoricalDataSync( const Contract& contract, time_t start, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept override;

		Future<::ContractDetails> ReqContractDetails( sv symbol )noexcept;
		static Future<::ContractDetails> ReqContractDetails( ContractPK id )noexcept{ auto p=_pSyncInstance; if( p ) return p->ReqContractDetailsInst(id);
			std::promise<VectorPtr<::ContractDetails>> promise;
			promise.set_value( make_shared<vector<::ContractDetails>>() );
			return promise.get_future();
		}
		Future<::ContractDetails> ReqContractDetails( const ::Contract& contract )noexcept;
//		sp<Proto::Results::ExchangeContracts> ReqSecDefOptParamsSmart( ContractPK underlyingConId, sv symbol )noexcept(false);
//		std::future<sp<Proto::Results::OptionExchanges>> ReqSecDefOptParams( ContractPK underlyingConId, sv symbol )noexcept;
		//α reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol=""sv, sv futFopExchange="", sv underlyingSecType="STK" )noexcept override;
		std::future<sp<string>> ReqFundamentalData( const ::Contract &contract, sv reportType )noexcept;
		//Future<NewsProvider> RequestNewsProviders()noexcept;
		std::future<VectorPtr<Proto::Results::Position>> RequestPositions()noexcept(false);
		//std::future<sp<map<string,double>>> ReqRatios( const ::Contract &contract )noexcept;


		//vector<ActiveOrderPtr> ReqAllOpenOrders()noexcept(false);//timeout
		static TwsClientSync& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		static bool IsConnected()noexcept;
		α ReqIds()noexcept->void;
	private:
		Future<::ContractDetails> ReqContractDetailsInst( ContractPK id )noexcept;

		α OnError( TickerId id, int errorCode, const std::string& errorMsg )->void;
		α OnHeadTimestamp( TickerId reqId, TimePoint t )->void;
		sp<WrapperSync> Wrapper()noexcept;
		//static sp<TwsClientSync> _pInstance;
		TwsClientSync( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		Collections::UnorderedMap<TickerId,std::condition_variable> _conditionVariables;
		Collections::UnorderedMap<TickerId,IBException> _errors;
		UnorderedMapValue<TickerId,TimePoint> _headTimestamps;

		//Collections::UnorderedMap<TickerId,std::list<::Bar>> _historicalData;
		static sp<TwsClientSync> _pSyncInstance;
	};
}