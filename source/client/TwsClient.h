#pragma once
#ifdef _MSC_VER
	#pragma push_macro("assert")
	#undef assert
	#include <platformspecific.h>
	#pragma pop_macro("assert")
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder-ctor"
#include <Decimal.h>
#include <EClientSocket.h>
#pragma clang diagnostic pop

#include <jde/markets/Exports.h>
#include "../types/TwsConnectionSettings.h"
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )
#include <jde/markets/TypeDefs.h>
#include "../TickManager.h"

struct EReaderSignal;

namespace ibapi
{
	typedef long OrderId;
}

namespace Jde::Markets
{
	struct IAccountUpdateHandler;
	struct TwsProcessor; struct TwsConnectionSettings; struct WrapperLog; struct Contract; class ClientConnection; struct PlaceOrderAwait;

	struct ΓM TwsClient : private EClientSocket
	{
		β CheckTimeouts()noexcept->void{};
		Ω CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false)->void;
		Ω Instance()noexcept->TwsClient&{return *_pInstance;}//ASSERT(_pInstance);
		Ω InstancePtr()noexcept->sp<TwsClient>{ return _pInstance; }
		Ω HasInstance()noexcept->bool{ return _pInstance!=nullptr;}
		Ω RequestId()noexcept->ibapi::OrderId{ return Instance()._requestId++; }
		α isConnected()const noexcept->bool{ return EClientSocket::isConnected(); }
		α SetRequestId( TickerId id )noexcept->void;

		α cancelMktData( TickerId reqId )noexcept->void{ LOG("({})cancelMktData()", reqId); EClientSocket::cancelMktData(reqId); }
		α cancelOrder( TickerId reqId )noexcept->void{ LOG( "({})cancelOrder()", reqId); EClientSocket::cancelOrder(reqId); }
		α cancelPositionsMulti(TickerId reqId)noexcept->void{ LOG( "({})cancelPositionsMulti()", reqId); EClientSocket::cancelPositionsMulti(reqId); }
		α cancelRealTimeBars( TickerId reqId )noexcept->void{ LOG( "({})cancelRealTimeBars()", reqId); EClientSocket::cancelRealTimeBars(reqId); }
		α reqIds( int _=1 )noexcept->void{ LOG( "reqIds()" ); EClientSocket::reqIds(_); }
		α RequestAccountUpdates( sv acctCode, sp<IAccountUpdateHandler> )noexcept->Handle;
		Ω CancelAccountUpdates( sv acctCode, Handle handle )noexcept->void;
		α reqAccountUpdatesMulti(TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV)noexcept->void;
		α reqExecutions( int reqId, const ExecutionFilter& filter )noexcept->void;
		α ReqHistoricalData( TickerId reqId, const Contract& contract, Day endDay, Day dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept->void;
		α reqHistoricalData( TickerId reqId, const ::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept->void; static constexpr uint32 ReqHistoricalDataLogId = 1595149123;
		α reqPositions()noexcept->void{ LOG( "reqPositions()" ); EClientSocket::reqPositions(); }
		α reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept->void;

		α cancelPositions()noexcept->void{ LOG( "cancelPositions()" ); EClientSocket::cancelPositions(); }
		α reqPositionsMulti( int reqId, const std::string& account, const std::string& modelCode )noexcept->void;
		α reqManagedAccts()noexcept->void{ LOG( "reqManagedAccts()" ); EClientSocket::reqManagedAccts(); }
		β reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol=""sv, sv futFopExchange="", sv underlyingSecType="STK" )noexcept->void;
		α reqContractDetails( int reqId, const ::Contract& contract )noexcept->void;
		α reqHeadTimestamp( int tickerId, const ::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept->void;
		α reqFundamentalData( TickerId tickerId, const ::Contract &contract, sv reportType )noexcept->void;
		α reqNewsProviders()noexcept->void;	static constexpr uint32 ReqNewsProvidersLogId = 159697286;

		α reqNewsArticle( TickerId requestId, str providerCode, str articleId )noexcept->void;
		α reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept->void;

		α reqCurrentTime()noexcept->void;
		α reqOpenOrders()noexcept->void;
		α reqAllOpenOrders()noexcept->void;
	protected:
		sp<EWrapper> _pWrapper;
		sp<WrapperLog> WrapperLogPtr()noexcept;
		uint16 _port{0};
		TwsClient( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static sp<TwsClient> _pInstance;
	private:
		α placeOrder( const ::Contract& contract, const ::Order& order )noexcept->void;
		α reqMktData(TickerId id, const ::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions)noexcept->void;
		TwsConnectionSettings _settings;
		std::atomic<TickerId> _requestId{0};
		static const LogTag& _logLevel;
		flat_set<string> _accountUpdates; shared_mutex _accountUpdateMutex;
		friend TwsProcessor; friend TickManager::TickWorker; friend ClientConnection; friend PlaceOrderAwait;
	};
}