#pragma once
#ifdef _MSC_VER
	#pragma push_macro("assert")
	#undef assert
	#include <platformspecific.h>
	#pragma pop_macro("assert")
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder-ctor"
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
	struct TwsProcessor; struct TwsConnectionSettings; struct WrapperLog; struct Contract; class ClientConnection;

	struct ΓM TwsClient : private EClientSocket
	{
		virtual void CheckTimeouts()noexcept{};
		static void CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static TwsClient& Instance()noexcept{return *_pInstance;}//ASSERT(_pInstance);
		static sp<TwsClient> InstancePtr()noexcept{ return _pInstance; }
		static bool HasInstance()noexcept{ return _pInstance!=nullptr;}
		ibapi::OrderId RequestId()noexcept{ return _requestId++; }
		bool isConnected()const noexcept{ return EClientSocket::isConnected(); }
		void SetRequestId( TickerId id )noexcept;

		void cancelMktData( TickerId reqId, bool log=true )noexcept{ if( log )LOG( "({})cancelMktData()", reqId); EClientSocket::cancelMktData(reqId); }
		void cancelOrder( TickerId reqId )noexcept{ LOG( "({})cancelOrder()", reqId); EClientSocket::cancelOrder(reqId); }
		void cancelPositionsMulti(TickerId reqId)noexcept{ LOG( "({})cancelPositionsMulti()", reqId); EClientSocket::cancelPositionsMulti(reqId); }
		void cancelRealTimeBars( TickerId reqId )noexcept{ LOG( "({})cancelRealTimeBars()", reqId); EClientSocket::cancelRealTimeBars(reqId); }
		void reqIds( int _=1 )noexcept{ LOG( "reqIds()" ); EClientSocket::reqIds(_); }
		Handle RequestAccountUpdates( sv acctCode, sp<IAccountUpdateHandler> )noexcept;
		static void CancelAccountUpdates( sv acctCode, Handle handle )noexcept;
		void reqAccountUpdatesMulti(TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV)noexcept;
		void reqExecutions( int reqId, const ExecutionFilter& filter )noexcept;
		void ReqHistoricalData( TickerId reqId, const Contract& contract, Day endDay, Day dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept;
		void reqHistoricalData( TickerId reqId, const ::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept; static constexpr uint32 ReqHistoricalDataLogId = 1595149123;
		void reqPositions()noexcept{ LOG( "reqPositions()" ); EClientSocket::reqPositions(); }
		void reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept;

		void cancelPositions()noexcept{ LOG( "cancelPositions()" ); EClientSocket::cancelPositions(); }
		void reqPositionsMulti( int reqId, const std::string& account, const std::string& modelCode )noexcept;
		void reqManagedAccts()noexcept{ LOG( "reqManagedAccts()" ); EClientSocket::reqManagedAccts(); }
		virtual void reqSecDefOptParams( TickerId tickerId, int underlyingConId, sv underlyingSymbol=""sv, sv futFopExchange="", sv underlyingSecType="STK" )noexcept;
		void reqContractDetails( int reqId, const ::Contract& contract )noexcept;
		void reqHeadTimestamp( int tickerId, const ::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept;
		void reqFundamentalData( TickerId tickerId, const ::Contract &contract, sv reportType )noexcept;
		void reqNewsProviders()noexcept;	static constexpr uint32 ReqNewsProvidersLogId = 159697286;

		void reqNewsArticle( TickerId requestId, str providerCode, str articleId )noexcept;
		void reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept;

		void reqCurrentTime()noexcept;
		void reqOpenOrders()noexcept;
		void reqAllOpenOrders()noexcept;
		void placeOrder( const ::Contract& contract, const ::Order& order )noexcept;
	protected:
		sp<EWrapper> _pWrapper;
		sp<WrapperLog> WrapperLogPtr()noexcept;
		uint16 _port{0};
		TwsClient( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static sp<TwsClient> _pInstance;
	private:
		void reqMktData(TickerId id, const ::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions) noexcept;
		TwsConnectionSettings _settings;
		std::atomic<TickerId> _requestId{1};
		static const LogTag& _logLevel;
		flat_set<string> _accountUpdates; shared_mutex _accountUpdateMutex;
		friend TwsProcessor;
		friend TickManager::TickWorker;
		friend ClientConnection;
	};
}