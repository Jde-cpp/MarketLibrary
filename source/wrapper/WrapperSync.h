#pragma once
#include "../../../Framework/source/collections/Queue.h"
#include "../../../Framework/source/threading/Thread.h"
#include "../Exports.h"
#include "WrapperCache.h"
#include <Contract.h>
#include "WrapperPromise.h"
#include "../types/proto/results.pb.h"

namespace Jde::Settings{ struct Container; }
struct EReaderSignal;
namespace Jde::Markets
{
	struct TwsClientSync;
	//struct OptionsData{ const std::string Exchange; int UnderlyingConId; const std::string TradingClass; const std::string Multiplier; const std::set<std::string> Expirations; const std::set<double> Strikes; };
	struct JDE_MARKETS_EXPORT WrapperSync : public WrapperCache, std::enable_shared_from_this<WrapperSync>
	{
		WrapperSync()noexcept;
		virtual sp<TwsClientSync> CreateClient( uint twsClientId )noexcept;
		typedef function<void(TickerId id, int errorCode, const std::string& errorMsg)> ErrorCallback;
		typedef function<void(bool)> DisconnectCallback; void AddDisconnectCallback( const DisconnectCallback& callback )noexcept;
		typedef function<void(TimePoint)> CurrentTimeCallback; void AddCurrentTime( CurrentTimeCallback& fnctn )noexcept;
		typedef function<void(TimePoint)> HeadTimestampCallback; void AddHeadTimestamp( TickerId reqId, const HeadTimestampCallback& fnctn, const ErrorCallback& errorFnctn )noexcept;
		typedef vector<::Bar> ReqHistoricalData; //typedef function<void(sp<ReqHistoricalData>)> ReqHistoricalDataCallback;
		//typedef function<void(TickerId)> ReqIdCallback;
		typedef function<void()> EndCallback;

		void AddOpenOrderEnd( EndCallback& )noexcept;
		void AddRatioTick( TickerId tickerId, string_view key, double value )noexcept;
		std::shared_future<TickerId> ReqIdsPromise()noexcept;
		WrapperData<::Bar>::Future ReqHistoricalDataPromise( ReqId reqId, Duration duration )noexcept;
		WrapperData<::ContractDetails>::Future ContractDetailsPromise( ReqId reqId )noexcept;
		WrapperData<NewsProvider>::Future NewsProviderPromise()noexcept{ return _newsProviderData.Promise(Threading::ThreadId, 5s); }
		std::future<VectorPtr<Proto::Results::Position>> PositionPromise()noexcept;
		WrapperItem<Proto::Results::OptionExchanges>::Future SecDefOptParamsPromise( ReqId reqId )noexcept;
		WrapperItem<string>::Future FundamentalDataPromise( ReqId reqId, Duration duration )noexcept;
		WrapperItem<map<string,double>>::Future RatioPromise( ReqId reqId, Duration duration )noexcept;
		void CheckTimeouts()noexcept;
	protected:
		map<ReqId,HeadTimestampCallback> _headTimestamp; mutable mutex _headTimestampMutex;
		unordered_map<ReqId,ErrorCallback> _errorCallbacks; mutable mutex _errorCallbacksMutex;
		void error( int id, int errorCode, const std::string& errorString )noexcept override{error2( id, errorCode, errorString );};
		bool error2( int id, int errorCode, const std::string& errorString )noexcept;
		bool historicalDataSync( TickerId reqId, const ::Bar& bar )noexcept;
		bool historicalDataEndSync( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept;

		bool TickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib )noexcept;
		bool TickSize( TickerId tickerId, TickType field, int size )noexcept;
		bool TickString( TickerId tickerId, TickType tickType, const std::string& value )noexcept;
		bool TickGeneric( TickerId ibReqId, TickType field, double value )noexcept;
	private:
		void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib )noexcept override{TickPrice(tickerId,field, price, attrib);}
		void tickSize( TickerId tickerId, TickType field, int size )noexcept override{ TickSize( tickerId, field, size ); }
		void tickString( TickerId tickerId, TickType field, const std::string& value )noexcept override{TickString(tickerId, field, value);}
		void tickGeneric( TickerId tickerId, TickType field, double value )noexcept override{TickGeneric(tickerId, field, value);}

		void currentTime( long time )noexcept override;
		void fundamentalData(TickerId reqId, const std::string& data)noexcept override;
		void headTimestamp( int reqId, const std::string& headTimestamp )noexcept override;
		void historicalData( TickerId reqId, const ::Bar& bar )noexcept override;
		void historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept override;

		//void reqId(int reqId, const std::string& startDateStr, const std::string& endDateStr)noexcept override;
		void nextValidId( ::OrderId orderId)noexcept override;
		void newsProviders( const std::vector<NewsProvider>& newsProviders )noexcept override;
//		void newsProviders( const std::vector<NewsProvider>& providers, bool isCache )noexcept;

		void openOrderEnd()noexcept override;
		void position( const std::string& account, const ::Contract& contract, double position, double avgCost )noexcept override;
		void positionEnd()noexcept override;
	public:
		void securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept override;
		bool securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
		void securityDefinitionOptionalParameterEnd( int reqId )noexcept override;
		bool securityDefinitionOptionalParameterEndSync( int reqId )noexcept;
		void contractDetails( int reqId, const ::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;

	protected:
		WrapperData<::ContractDetails> _detailsData;
		sp<TwsClientSync> _pClient;

	private:
		shared_ptr<EReaderSignal> _pReaderSignal;
		void SendCurrentTime( const TimePoint& time )noexcept;
		std::forward_list<CurrentTimeCallback> _currentTimeCallbacks; mutable shared_mutex _currentTimeCallbacksMutex;
		std::forward_list<DisconnectCallback> _disconnectCallbacks; mutable shared_mutex _disconnectCallbacksMutex;

		//map<ReqId,ReqHistoricalDataCallback> _requestCallbacks; mutable shared_mutex _requestCallbacksMutex;
		//map<ReqId,sp<ReqHistoricalData>> _requestData; mutable mutex _requestDataMutex;
		sp<std::promise<TickerId>> _requestIdsPromisePtr; sp<std::shared_future<TickerId>> _requestIdsFuturePtr; mutable shared_mutex _requestIdsPromiseMutex;
		WrapperData<::Bar> _historicalData;
		WrapperItem<Proto::Results::OptionExchanges> _optionFutures; map<int,sp<Proto::Results::OptionExchanges>> _optionData;

		WrapperItem<string> _fundamentalData;
		WrapperItem<map<string,double>> _ratioData; map<TickerId,map<string,double>> _ratioValues; mutable mutex _ratioMutex;

		//QueueValue<ReqIdCallback> _requestIds;
		//QueueValue<EndCallback> _openOrderEnds;
		WrapperData<NewsProvider> _newsProviderData;
		sp<std::promise<VectorPtr<Proto::Results::Position>>> _positionPromisePtr; mutable shared_mutex _positionPromiseMutex; VectorPtr<Proto::Results::Position> _positionsPtr;
	};
}