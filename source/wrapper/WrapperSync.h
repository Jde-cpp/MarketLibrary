#pragma once
#include "../../../Framework/source/collections/Queue.h"
#include "../../../Framework/source/threading/Thread.h"
#include <jde/markets/Exports.h>
#include "WrapperCo.h"
#include "WrapperPromise.h"
#include <jde/markets/types/proto/results.pb.h>

namespace Jde::Settings{ struct Container; }
struct EReaderSignal;
namespace Jde::Markets
{
	struct TwsClientSync;
	struct ΓM WrapperSync : WrapperCo, std::enable_shared_from_this<WrapperSync>
	{
		WrapperSync()noexcept;
		~WrapperSync();
		α Shutdown()noexcept->void;
		virtual sp<TwsClientSync> CreateClient( uint twsClientId )noexcept(false);
		typedef function<void(TickerId id, int errorCode, str errorMsg)> ErrorCallback;
		typedef function<void(bool)> DisconnectCallback; α AddDisconnectCallback( const DisconnectCallback& callback )noexcept->void;
		typedef function<void(TimePoint)> CurrentTimeCallback; α AddCurrentTime( CurrentTimeCallback& fnctn )noexcept->void;
		typedef function<void(TimePoint)> HeadTimestampCallback; α AddHeadTimestamp( TickerId reqId, const HeadTimestampCallback& fnctn, const ErrorCallback& errorFnctn )noexcept->void;
		typedef function<void()> EndCallback;

		α AddOpenOrderEnd( EndCallback& )noexcept->void;
		std::shared_future<TickerId> ReqIdsPromise()noexcept;
		WrapperData<NewsProvider>::Future NewsProviderPromise()noexcept{ return _newsProviderData.Promise(static_cast<ReqId>(Threading::GetThreadId()), 5s); }
		std::future<VectorPtr<Proto::Results::Position>> PositionPromise()noexcept;
		WrapperItem<Proto::Results::OptionExchanges>::Future SecDefOptParamsPromise( ReqId reqId )noexcept;
		WrapperItem<string>::Future FundamentalDataPromise( ReqId reqId, Duration duration )noexcept;
		WrapperItem<map<string,double>>::Future RatioPromise( ReqId reqId, Duration duration )noexcept;
		α CheckTimeouts()noexcept->void;
	protected:
		map<ReqId,HeadTimestampCallback> _headTimestamp; mutable mutex _headTimestampMutex;
		std::unordered_map<ReqId,ErrorCallback> _errorCallbacks; mutable mutex _errorCallbacksMutex;
		α error( int id, int errorCode, str errorString )noexcept->void override{error2( id, errorCode, errorString );};
		bool error2( int id, int errorCode, str errorString )noexcept override;
	private:
		α currentTime( long time )noexcept->void override;
		α fundamentalData(TickerId reqId, str data)noexcept->void override;
		α headTimestamp( int reqId, str headTimestamp )noexcept->void override;
		α nextValidId( ::OrderId orderId)noexcept->void override;
		α openOrderEnd()noexcept->void override;
		α position( str account, const ::Contract& contract, ::Decimal position, double avgCost )noexcept->void override;
		α positionEnd()noexcept->void override;
	protected:
		sp<TwsClientSync> _pClient;

	private:
		sp<EReaderSignal> _pReaderSignal;
		α SendCurrentTime( const TimePoint& time )noexcept->void;
		std::forward_list<CurrentTimeCallback> _currentTimeCallbacks; mutable shared_mutex _currentTimeCallbacksMutex;
		std::forward_list<DisconnectCallback> _disconnectCallbacks; mutable shared_mutex _disconnectCallbacksMutex;
		sp<std::promise<TickerId>> _requestIdsPromisePtr; sp<std::shared_future<TickerId>> _requestIdsFuturePtr; mutable shared_mutex _requestIdsPromiseMutex;
		WrapperItem<Proto::Results::OptionExchanges> _optionFutures; map<int,sp<Proto::Results::OptionExchanges>> _optionData;

		WrapperItem<string> _fundamentalData;
		WrapperItem<map<string,double>> _ratioData; map<TickerId,map<string,double>> _ratioValues; mutable mutex _ratioMutex;
		WrapperData<NewsProvider> _newsProviderData;
		sp<std::promise<VectorPtr<Proto::Results::Position>>> _positionPromisePtr; mutable shared_mutex _positionPromiseMutex; VectorPtr<Proto::Results::Position> _positionsPtr;
	};
}