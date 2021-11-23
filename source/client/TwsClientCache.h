#pragma once
#include <future>
#include <jde/markets/Exports.h>
#include <jde/markets/types/proto/requests.pb.h>
#include "TwsClientCo.h"
#include "../types/Bar.h"
#include "../types/Exchanges.h"


//struct EReaderSignal;

namespace Jde::Markets
{
	struct WrapperCache;
	//#define BASE auto p=TwsClient::InstancePtr(); if( p ) (*p)
	struct TwsConnectionSettings;
	struct Contract;
	struct ΓM TwsClientCache : public Tws
	{
		//static TwsClientCache& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		TwsClientCache( const TwsConnectionSettings& settings, sp<WrapperCache> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		α Wrapper()noexcept->sp<WrapperCache>;
		Ω ToContract( sv symbol, Day dayIndex, Proto::SecurityRight isCall, double strike=0 )noexcept->::Contract;
		α ReqContractDetails( TickerId reqId, const ::Contract& contract )noexcept->void;
		//void ReqSecDefOptParams( TickerId reqId, ContractPK underlyingConId, sv symbol )noexcept;
		//void ReqHistoricalData( TickerId reqId, const Contract& contract, Day current, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept(false);
		//void RequestNewsProviders()noexcept;
		//void reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept;
		//virtual std::future<VectorPtr<::Bar>> ReqHistoricalDataSync( const Contract& contract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false)=0;
		//virtual std::future<VectorPtr<::Bar>> ReqHistoricalDataSync( const Contract& contract, time_t start, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept=0;

		//static void reqAllOpenOrders()noexcept{ BASE.reqAllOpenOrders(); }
		//static bool isConnected()noexcept{ auto p=TwsClient::InstancePtr(); return p ? (*p).isConnected() : false; }
	};
}