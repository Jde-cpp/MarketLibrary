#pragma once
#include "../Exports.h"
#include "../types/TwsConnectionSettings.h"

struct EReaderSignal;

namespace Jde::Markets
{
	struct TwsConnectionSettings;
	struct JDE_MARKETS_EXPORT TwsClient : public EClientSocket
	{
		static void CreateInstance( const TwsConnectionSettings& settings, sp<EWrapper> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		static TwsClient& Instance()noexcept{return *_pInstance;}//ASSERT(_pInstance);
		static bool HasInstance()noexcept{ return _pInstance!=nullptr;}
		ibapi::OrderId RequestId()noexcept{ return _requestId++; }
		void SetRequestId( TickerId id )noexcept;

		void cancelMktData( TickerId reqId )noexcept{ LOG(_logLevel, "cancelMktData( '{}' )"sv, reqId); EClientSocket::cancelMktData(reqId); }
		void cancelOrder( TickerId reqId )noexcept{ LOG(_logLevel, "cancelOrder( '{}' )"sv, reqId); EClientSocket::cancelOrder(reqId); }
		void cancelPositionsMulti(TickerId reqId)noexcept{ LOG(_logLevel, "cancelPositionsMulti( '{}' )"sv, reqId); EClientSocket::cancelPositionsMulti(reqId); }

		void reqIds( int _=1 )noexcept{ LOG0(_logLevel, "reqIds()"sv); EClientSocket::reqIds(_); }
		void reqAccountUpdates( bool subscribe, const string& acctCode )noexcept{ LOG(_logLevel, "reqAccountUpdates( '{}', '{}' )"sv, subscribe, acctCode); EClientSocket::reqAccountUpdates( subscribe, acctCode ); }
		void reqAccountUpdatesMulti(TickerId reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV)noexcept;
		void reqHistoricalData( TickerId tickerId, const ibapi::Contract& contract, const std::string& endDateTime, const std::string& durationStr, const std::string&  barSizeSetting, const std::string& whatToShow, int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions )noexcept;
		void reqPositions()noexcept{ LOG0( _logLevel, "reqPositions()"sv ); EClientSocket::reqPositions(); }
		void reqManagedAccts()noexcept{ LOG0( _logLevel, "reqManagedAccts()"sv ); EClientSocket::reqManagedAccts(); }
		void reqMktData( TickerId tickerId, const ibapi::Contract& contract, const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions )noexcept;
		virtual void reqSecDefOptParams( TickerId tickerId, int underlyingConId, string_view underlyingSymbol=""sv, string_view futFopExchange="", string_view underlyingSecType="STK" )noexcept;
		void reqContractDetails( int reqId, const ibapi::Contract& contract )noexcept;
		void reqHeadTimestamp( int tickerId, const ibapi::Contract &contract, const std::string& whatToShow, int useRTH, int formatDate )noexcept;
		void reqFundamentalData( TickerId tickerId, const ibapi::Contract &contract, string_view reportType )noexcept;
		void reqCurrentTime()noexcept;
		void reqOpenOrders()noexcept;
		void reqAllOpenOrders()noexcept;
		void placeOrder( const ibapi::Contract& contract, const ibapi::Order& order )noexcept;
	protected:
		shared_ptr<EWrapper> _pWrapper;
		uint16 _port{0};
		TwsClient( const TwsConnectionSettings& settings, shared_ptr<EWrapper> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);

	private:
		static sp<TwsClient> _pInstance;
		TwsConnectionSettings _settings;
		std::atomic<TickerId> _requestId{0};
		ELogLevel _logLevel{ ELogLevel::Debug };
	};
}

