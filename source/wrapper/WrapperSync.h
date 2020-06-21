#pragma once
#include "../../../Framework/source/collections/Queue.h"
#include "../Exports.h"
#include "WrapperCache.h"
#include "WrapperPromise.h"
#include "../types/proto/results.pb.h"

namespace Jde::Markets
{
	//struct OptionsData{ const std::string Exchange; int UnderlyingConId; const std::string TradingClass; const std::string Multiplier; const std::set<std::string> Expirations; const std::set<double> Strikes; };
	struct JDE_MARKETS_EXPORT WrapperSync : public WrapperCache
	{
		typedef function<void(TickerId id, int errorCode, const std::string& errorMsg)> ErrorCallback;
		typedef function<void(bool)> DisconnectCallback; void AddDisconnectCallback( const DisconnectCallback& callback )noexcept;
		typedef function<void(TimePoint)> CurrentTimeCallback; void AddCurrentTime( CurrentTimeCallback& fnctn )noexcept;
		typedef function<void(TimePoint)> HeadTimestampCallback; void AddHeadTimestamp( TickerId reqId, const HeadTimestampCallback& fnctn, const ErrorCallback& errorFnctn )noexcept;
		typedef vector<ibapi::Bar> ReqHistoricalData; //typedef function<void(sp<ReqHistoricalData>)> ReqHistoricalDataCallback;
		//typedef function<void(TickerId)> ReqIdCallback;
		typedef function<void()> EndCallback;

		void AddOpenOrderEnd( EndCallback& )noexcept;
		void AddRatioTick( TickerId tickerId, string_view key, double value )noexcept;
		std::shared_future<TickerId> ReqIdsPromise()noexcept;
		WrapperData<ibapi::Bar>::Future ReqHistoricalDataPromise( ReqId reqId )noexcept;
		WrapperData<ibapi::ContractDetails>::Future ContractDetailsPromise( ReqId reqId )noexcept;
		WrapperData<Proto::Results::OptionParams>::Future SecDefOptParamsPromise( ReqId reqId )noexcept;
		WrapperItem<string>::Future FundamentalDataPromise( ReqId reqId )noexcept;
		WrapperItem<map<string,double>>::Future RatioPromise( ReqId reqId )noexcept;
	protected:
		map<ReqId,HeadTimestampCallback> _headTimestamp; mutable mutex _headTimestampMutex;
		unordered_map<ReqId,ErrorCallback> _errorCallbacks; mutable mutex _errorCallbacksMutex;
		void error( int id, int errorCode, const std::string& errorString )noexcept override{error2( id, errorCode, errorString );};
		bool error2( int id, int errorCode, const std::string& errorString )noexcept;
		bool historicalDataSync( TickerId reqId, const ibapi::Bar& bar )noexcept;
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
		void historicalData( TickerId reqId, const ibapi::Bar& bar )noexcept override;
		void historicalDataEnd( int reqId, const std::string& startDateStr, const std::string& endDateStr )noexcept override;

		//void reqId(int reqId, const std::string& startDateStr, const std::string& endDateStr)noexcept override;
		void nextValidId( ibapi::OrderId orderId)noexcept override;
		void openOrderEnd()noexcept override;
	public:
		void securityDefinitionOptionalParameter( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept override;
		bool securityDefinitionOptionalParameterSync( int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations, const std::set<double>& strikes )noexcept;
		void securityDefinitionOptionalParameterEnd( int reqId )noexcept override;
		bool securityDefinitionOptionalParameterEndSync( int reqId )noexcept;
		void contractDetails( int reqId, const ibapi::ContractDetails& contractDetails )noexcept override;
		void contractDetailsEnd( int reqId )noexcept override;
	protected:

		WrapperData<ibapi::ContractDetails> _detailsData;
	private:
		void SendCurrentTime( const TimePoint& time )noexcept;
		forward_list<CurrentTimeCallback> _currentTimeCallbacks; mutable shared_mutex _currentTimeCallbacksMutex;
		forward_list<DisconnectCallback> _disconnectCallbacks; mutable shared_mutex _disconnectCallbacksMutex;

		//map<ReqId,ReqHistoricalDataCallback> _requestCallbacks; mutable shared_mutex _requestCallbacksMutex;
		//map<ReqId,sp<ReqHistoricalData>> _requestData; mutable mutex _requestDataMutex;
		sp<std::promise<TickerId>> _requestIdsPromisePtr; sp<std::shared_future<TickerId>> _requestIdsFuturePtr; mutable shared_mutex _requestIdsPromiseMutex;
		WrapperData<ibapi::Bar> _historicalData;
		WrapperData<Proto::Results::OptionParams> _optionsData;

		WrapperItem<string> _fundamentalData;
		WrapperItem<map<string,double>> _ratioData; map<TickerId,map<string,double>> _ratioValues; mutable mutex _ratioMutex;

		//QueueValue<ReqIdCallback> _requestIds;
		QueueValue<EndCallback> _openOrderEnds;
	};
}