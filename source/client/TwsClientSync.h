#pragma once
#include <future>
//#include <.h>
#include "../../../Framework/source/collections/UnorderedMapValue.h"
#include "../../../Framework/source/collections/UnorderedMap.h"
#include "TwsClientCo.h"
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
	struct ΓM TwsClientSync : public Tws
	{
		template<class T> using Container = VectorPtr<T>;
		template<class T> using Future = std::future<Container<T>>;
		static sp<TwsClientSync> CreateInstance( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);

		α CurrentTime()noexcept->TimePoint;
		α HeadTimestamp( const ::Contract &contract, const std::string& whatToShow )noexcept(false)->TimePoint;
		α CheckTimeouts()noexcept->void override;

		std::future<sp<string>> ReqFundamentalData( const ::Contract &contract, sv reportType )noexcept;
		std::future<VectorPtr<Proto::Results::Position>> RequestPositions()noexcept(false);

		static TwsClientSync& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		static bool IsConnected()noexcept;
		α ReqIds()noexcept->void;
	private:

		α OnError( TickerId id, int errorCode, const std::string& errorMsg )->void;
		α OnHeadTimestamp( TickerId reqId, TimePoint t )->void;
		sp<WrapperSync> Wrapper()noexcept;
		TwsClientSync( const TwsConnectionSettings& settings, sp<WrapperSync> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		Collections::UnorderedMap<TickerId,std::condition_variable> _conditionVariables;
		Collections::UnorderedMap<TickerId,IBException> _errors;
		UnorderedMapValue<TickerId,TimePoint> _headTimestamps;

		static sp<TwsClientSync> _pSyncInstance;
	};
}