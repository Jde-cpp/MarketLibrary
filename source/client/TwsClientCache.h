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
	struct JDE_MARKETS_EXPORT TwsClientCache : public TwsClientCo
	{
		//static TwsClientCache& Instance()noexcept;//{ ASSERT(_pInstance); return *_pInstance;}
		TwsClientCache( const TwsConnectionSettings& settings, shared_ptr<WrapperCache> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		shared_ptr<WrapperCache> Wrapper()noexcept;
		static ::Contract ToContract( sv symbol, DayIndex dayIndex, Proto::SecurityRight isCall, double strike=0 )noexcept;
		void ReqContractDetails( TickerId reqId, const ::Contract& contract )noexcept;
		//void ReqSecDefOptParams( TickerId reqId, ContractPK underlyingConId, sv symbol )noexcept;
		//void ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex current, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept(false);
		//void RequestNewsProviders()noexcept;
		//void reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept;
		//virtual std::future<VectorPtr<::Bar>> ReqHistoricalDataSync( const Contract& contract, DayIndex end, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, bool useCache )noexcept(false)=0;
		//virtual std::future<VectorPtr<::Bar>> ReqHistoricalDataSync( const Contract& contract, time_t start, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept=0;

		//static void reqAllOpenOrders()noexcept{ BASE.reqAllOpenOrders(); }
		//static bool isConnected()noexcept{ auto p=TwsClient::InstancePtr(); return p ? (*p).isConnected() : false; }
	};
}