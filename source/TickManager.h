﻿#pragma once
#include <jde/markets/Exports.h>
#include <jde/markets/types/Tick.h>
#include "../../Framework/source/collections/Map.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/coroutine/CoWorker.h"


#define Φ ΓM auto
namespace Jde::Markets
{
	struct TwsClient; class EventManagerTests; class OptionTests;

	struct RatioAwait : IAwait
	{
		RatioAwait( ContractPK id ):_contractId{ id }{}
		Φ await_ready()noexcept->bool;
		Φ await_suspend( HCoroutine )noexcept->void;
		Φ await_resume()noexcept->AwaitResult;
	private:
		ContractPK _contractId;
		up<Tick> _value;
	};

	struct ΓM TickManager final
	{
		struct TickParams /*~final*/
		{
			function<bool(const Markets::Tick&)> Trigger;
			Markets::Tick Tick;
		};

		struct ΓM Awaitable final : CancelAwait, TickParams
		{
			using base=CancelAwait;
			Awaitable( const TickParams& params, Handle& h )ι;
			α await_ready()ι->bool override;
			α await_suspend( HCoroutine h )ι->void override;
			α await_resume()ι->AwaitResult override{ base::AwaitResume(); return _pPromise ? move(_pPromise->get_return_object().Result()) : AwaitResult{ mu<Markets::Tick>(TickParams::Tick) }; }
		private:
			Task::promise_type* _pPromise{ nullptr };
		};

		using ProtoFunction=function<void(const vector<Proto::Results::MessageUnion>&)>;
		Ω CalcImpliedVolatility( uint32 sessionId, uint32 clientId,  const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept->void;
		Ω CalculateOptionPrice(  uint32 sessionId, uint32 clientId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept->void;
		Ω Cancel( Handle h )noexcept->optional<Tick>;
		Ω Subscribe( const TickParams& params, Handle& h )noexcept{ return Awaitable{params, h}; }
		Ω Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept(false)->void;
		Ω CancelProto( uint sessionId, uint clientId, ContractPK contractId )noexcept->void;
		Ω Ratios( ContractPK c )noexcept{ return RatioAwait{c}; }


		struct TickWorker final: TCoWorker<TickWorker,Awaitable>
		{
			typedef TCoWorker<TickWorker,Awaitable> base;
			typedef Tick::Fields TickFields;
			struct SubscriptionInfo : base::Handles<>{ TickManager::TickParams Params; };
			static sp<TickWorker> CreateInstance( sp<TwsClient> _pParent )noexcept;
			TickWorker( sp<TwsClient> pParent )noexcept;

			α Push( TickerId id, ETickType type, Decimal v )noexcept->void;
			α Push( TickerId id, ETickType type, double v )noexcept->void;
			α Push( TickerId id, ETickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept->void;
			α Push( TickerId id, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData )noexcept->void;
			Φ PushPrice( TickerId id, ETickType type, double v/*, const TickAttrib& attribs*/ )noexcept->void;
			α Push( TickerId id, ETickType type, long long v )noexcept->void;
			α Push( TickerId id, ETickType type, const std::string& v )noexcept->void;
			α HandleError( int id, int errorCode, const std::string& errorString )noexcept->bool;
			Ω SGet( ContractPK contractId )noexcept->optional<Tick>;
		private:
			Φ Process()noexcept->void override;
			α Cancel( Handle h )noexcept->optional<Tick>;
			α CancelProto( uint hClient, ContractPK contractId, unique_lock<mutex>* pLock=nullptr )noexcept->void;
			α Subscribe( const SubscriptionInfo& params )noexcept->void;
			α AddOutgoingField( ContractPK id, ETickType t )noexcept->void;

			α Push( TickerId id, ETickType type, function<void(Tick&)> fnctn )noexcept->void;

			α Test( Tick& tick, function<bool(const Markets::Tick&)> test )noexcept->bool;
			α Test( Tick& clientTick, function<bool(const Markets::Tick&)> test, const Tick& latestTick )const noexcept->bool;
			α Ratios( ContractPK contractId )noexcept->Task;
			α Get( ContractPK contractId )const noexcept->optional<Tick>;

			α RequestId( ContractPK contractId )const noexcept->TickerId;
			α ContractId( TickerId id )const noexcept->ContractPK{ shared_lock l( _valuesMutex ); auto p=_tickerContractMap.find(id); return p==_tickerContractMap.end() ? 0 : p->second; }
			α SetRequestId( TickerId id, ContractPK contractId )noexcept->void;
			α RemoveRequest( TickerId id )noexcept->void;
			flat_map<TickerId,ContractPK> _tickerContractMap;

			flat_map<ContractPK,Tick> _values; mutable shared_mutex _valuesMutex;
			flat_map<ContractPK,TickFields> _outgoingFields; mutex _outgoingFieldsMutex;

			enum ESubscriptionSource{ Internal/*InternalSubscriptionHandle*/, Coroutine/*clientHandle*/, Proto/*SessionId*/ };
			struct TickListSource{ ESubscriptionSource Source; uint Id; flat_set<ETickList> Ticks; };
			α RemoveTwsSubscription( ESubscriptionSource source, uint id, ContractPK contractId )noexcept->void;
			flat_set<ETickList> GetSubscribedTicks( ContractPK id )const noexcept;
			α AddSubscription( ContractPK contractId, const TickListSource& source, sp<unique_lock<mutex>> pLock={} )noexcept(false)->bool;
			flat_multimap<TimePoint,tuple<ESubscriptionSource, uint, ContractPK>> _delays; std::shared_mutex _delayMutex;
			flat_multimap<ContractPK,TickListSource> _twsSubscriptions; std::mutex _twsSubscriptionMutex;

			flat_multimap<ContractPK,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;//Type=0, handleIndex=clientId

			α Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept(false)->void;
			struct ProtoSubscription
			{
				uint32 SessionId;
				uint32 ClientId;

				TickManager::ProtoFunction Function;
			};
			flat_multimap<ContractPK,ProtoSubscription> _protoSubscriptions; mutex _protoSubscriptionMutex;//Type=1, handleIndex=[SessionId,clientId]

			flat_multimap<ContractPK,tuple<HCoroutine,TimePoint,uint>> _ratioSubscriptions; mutex _ratioSubscriptionMutex;

			α CancelMarketData( TickerId oldId, TickerId newId )noexcept->void;
			Collections::MapValue<TickerId,TimePoint> _canceledTicks;//TODO move to cache.

			flat_multimap<TimePoint,TickerId> _orphans; shared_mutex _orphanMutex;//TODO make a multimap class
			sp<TwsClient> _pTwsClient;
			TimePoint LastOutgoing;

			α CalcImpliedVolatility( uint32 hClient, const ::Contract& contract, double optionPrice, double underPrice, TickManager::ProtoFunction fnctn )noexcept->void;
			α CalculateOptionPrice( uint32 hClient, const ::Contract& contract, double volatility, double underPrice, TickManager::ProtoFunction fnctn )noexcept->void;
			flat_multimap<TickerId,ProtoSubscription> _optionRequests; mutex _optionRequestMutex;

			uint InternalSubscriptionHandle{0};
			friend TickManager; friend EventManagerTests; friend OptionTests; friend RatioAwait;
		};
	};

	inline α TickManager::TickWorker::AddOutgoingField( ContractPK id, ETickType t )noexcept->void
	{
		unique_lock l{ _outgoingFieldsMutex };
		auto& v = _outgoingFields.try_emplace( id ).first->second;
		v.set( t );
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.notify_one();
	}
}
#undef Φ