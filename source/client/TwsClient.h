#pragma once
#include <EClientSocket.h>
#include "../Exports.h"
#include "../types/TwsConnectionSettings.h"
#pragma warning( disable : 4244 )
#include "../types/proto/requests.pb.h"
#pragma warning( default : 4244 )
#include "../TypeDefs.h"
#include "../TickManager.h"

struct EReaderSignal;

namespace ibapi
{
	typedef long OrderId;
}

namespace Jde::Markets
{
	//namespace Private{  TickWorker; }
	struct TwsProcessor; struct TwsConnectionSettings; struct WrapperLog; struct Contract; class ClientConnection;

	struct JDE_MARKETS_EXPORT TwsClient : private EClientSocket
	{
		virtual void CheckTimeouts()noexcept{};
		static void CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static TwsClient& Instance()noexcept{return *_pInstance;}//ASSERT(_pInstance);
		static sp<TwsClient> InstancePtr()noexcept{ return _pInstance; }
		static bool HasInstance()noexcept{ return _pInstance!=nullptr;}
		ibapi::OrderId RequestId()noexcept{ return _requestId++; }
		bool isConnected()const noexcept{ return EClientSocket::isConnected(); }
		void SetRequestId( TickerId id )noexcept;

		void cancelMktData( TickerId reqId, bool log=true )noexcept{ if( log )LOG(_logLevel, "({})cancelMktData()"sv, reqId); EClientSocket::cancelMktData(reqId); }
		void cancelOrder( TickerId reqId )noexcept{ LOG(_logLevel, "({})cancelOrder()"sv, reqId); EClientSocket::cancelOrder(reqId); }
		void cancelPositionsMulti(TickerId reqId)noexcept{ LOG(_logLevel, "({})cancelPositionsMulti()"sv, reqId); EClientSocket::cancelPositionsMulti(reqId); }
		void cancelRealTimeBars( TickerId reqId )noexcept{ LOG(_logLevel, "({})cancelRealTimeBars()"sv, reqId); EClientSocket::cancelRealTimeBars(reqId); }
		void reqIds( int _=1 )noexcept{ LOG0(_logLevel, "reqIds()"sv); EClientSocket::reqIds(_); }
		void reqAccountUpdates( bool subscribe, const string& acctCode )noexcept;
		void reqAccountUpdates( const string& acctCode, function<void(const string&,const string&,const string&,const string&)> callback )noexcept;
		void reqAccountUpdatesMulti(TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV)noexcept;
		void reqExecutions( int reqId, const ExecutionFilter& filter )noexcept;
		void ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex endDay, DayIndex dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept;
		void reqHistoricalData( TickerId reqId, const ::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string& barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept; static constexpr uint32 ReqHistoricalDataLogId = 2998346204;
		void reqPositions()noexcept{ LOG0( _logLevel, "reqPositions()"sv ); EClientSocket::reqPositions(); }
		void reqRealTimeBars(TickerId id, const ::Contract& contract, int barSize, const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions)noexcept;

		void cancelPositions()noexcept{ LOG0( _logLevel, "cancelPositions()"sv ); EClientSocket::cancelPositions(); }
		void reqPositionsMulti( int reqId, const std::string& account, const std::string& modelCode )noexcept;
		void reqManagedAccts()noexcept{ LOG0( _logLevel, "reqManagedAccts()"sv ); EClientSocket::reqManagedAccts(); }
		virtual void reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol=""sv, string_view futFopExchange="", string_view underlyingSecType="STK" )noexcept;
		void reqContractDetails( int reqId, const ::Contract& contract )noexcept;
		void reqHeadTimestamp( int tickerId, const ::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept;
		void reqFundamentalData( TickerId tickerId, const ::Contract &contract, string_view reportType )noexcept;
		void reqNewsProviders()noexcept;	static constexpr uint32 ReqNewsProvidersLogId = 159697286;

		void reqNewsArticle( TickerId requestId, const string& providerCode, const string& articleId )noexcept;
		void reqHistoricalNews( TickerId requestId, ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept;

		void reqCurrentTime()noexcept;
		void reqOpenOrders()noexcept;
		void reqAllOpenOrders()noexcept;
		void placeOrder( const ::Contract& contract, const ::Order& order )noexcept;
	protected:
		shared_ptr<EWrapper> _pWrapper;
		shared_ptr<WrapperLog> WrapperLogPtr()noexcept;
		uint16 _port{0};
		TwsClient( const TwsConnectionSettings& settings, shared_ptr<EWrapper> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static sp<TwsClient> _pInstance;
	private:
		void reqMktData(TickerId id, const ::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions) noexcept;
		TwsConnectionSettings _settings;
		std::atomic<TickerId> _requestId{1};
		ELogLevel _logLevel{ ELogLevel::Debug };

		friend TwsProcessor;
		friend TickManager::TickWorker;
		friend ClientConnection;
	};
}

