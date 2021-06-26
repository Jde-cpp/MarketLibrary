#pragma once
//#include <experimental/coroutine>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

#include "../../Framework/source/threading/Worker.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/coroutine/Coroutine.h"
#include <jde/coroutine/Task.h>
#include "../../Framework/source/coroutine/CoWorker.h"
#include "../../Framework/source/collections/Map.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include "../../Framework/source/collections/UnorderedMapValue.h"
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#include <jde/markets/types/proto/results.pb.h>
#pragma warning( default : 4244 )
#include <jde/markets/types/Tick.h>

namespace Jde::Markets
{
	struct TwsClient;
	class EventManagerTests; class OptionTests;

	using boost::container::flat_map;
	using boost::container::flat_set;
	using boost::container::flat_multimap;
	using Proto::Requests::ETickList; using Proto::Results::ETickType;
	struct JDE_MARKETS_EXPORT TickManager final
	{
		struct TickParams /*~final*/
		{
			Tick::Fields Fields;
			Markets::Tick Tick;
		};

		struct JDE_MARKETS_EXPORT Awaitable final : Coroutine::CancelAwaitable<Coroutine::TaskError<Tick>>, TickParams
		{
			typedef Coroutine::TaskError<Markets::Tick> TTask;
			typedef TTask::TResult TResult;
			typedef Coroutine::CancelAwaitable<TTask> base;
			typedef TTask::promise_type PromiseType;
			typedef coroutine_handle<PromiseType> Handle;
			Awaitable( const TickParams& params, Coroutine::Handle& h )noexcept;
			~Awaitable()=default;
			bool await_ready()noexcept override;
			void await_suspend( base::THandle h )noexcept override;
			TResult await_resume()noexcept override
			{
				base::AwaitResume();
				return _pPromise ? _pPromise->get_return_object().Result : TResult{TickParams::Tick};
			}
		private:
			PromiseType* _pPromise{ nullptr };
		};

		typedef function<void(const vector<Proto::Results::MessageUnion>&, uint32_t)> ProtoFunction;
		static void CalcImpliedVolatility( uint32 sessionId, uint32 clientId,  const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept;
		static void CalculateOptionPrice(  uint32 sessionId, uint32 clientId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept;
		static void Cancel( Coroutine::Handle h )noexcept;
		static auto Subscribe( const TickParams& params, Coroutine::Handle& h )noexcept{ return Awaitable{params, h}; }
		static void Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept;
		static void CancelProto( uint sessionId, uint clientId, ContractPK contractId )noexcept;
		static std::future<Tick> Ratios( const ContractPK contractId )noexcept;


		struct TickWorker final: Coroutine::TCoWorker<TickWorker,Awaitable>
		{
			typedef Coroutine::TCoWorker<TickWorker,Awaitable> base;
			typedef Tick::Fields TickFields;
			struct SubscriptionInfo : base::Handles<>{ TickManager::TickParams Params; };
			static sp<TickWorker> CreateInstance( sp<TwsClient> _pParent )noexcept;
			TickWorker( sp<TwsClient> pParent )noexcept;

			void Push( TickerId id, ETickType type, double v )noexcept;
			void Push( TickerId id, ETickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept;
			void Push( TickerId id, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData )noexcept;
			JDE_MARKETS_EXPORT void PushPrice( TickerId id, ETickType type, double v/*, const TickAttrib& attribs*/ )noexcept;
			void Push( TickerId id, ETickType type, long long v )noexcept;
			void Push( TickerId id, ETickType type, const std::string& v )noexcept;
			bool HandleError( int id, int errorCode, const std::string& errorString )noexcept;
		private:
			void Process()noexcept override;
			void Cancel( Coroutine::Handle h )noexcept;
			void CancelProto( uint hClient, ContractPK contractId, unique_lock<mutex>* pLock=nullptr )noexcept;
			void Subscribe( const SubscriptionInfo& params )noexcept;
			void AddOutgoingField( ContractPK id, ETickType t )noexcept;

			void Push( TickerId id, ETickType type, function<void(Tick&)> fnctn )noexcept;

			bool Test( Tick& tick, TickFields fields )noexcept;
			bool Test( Tick& clientTick, TickFields clientFields, const Tick& latestTick )const noexcept;
			std::future<Tick> Ratios( const ContractPK contractId )noexcept;
			optional<Tick> Get( ContractPK contractId )const noexcept;

			TickerId RequestId( ContractPK contractId )const noexcept;
			ContractPK ContractId( TickerId id )const noexcept{ shared_lock l( _valuesMutex ); auto p=_tickerContractMap.find(id); return p==_tickerContractMap.end() ? 0 : p->second; }
			void SetRequestId( TickerId id, ContractPK contractId )noexcept;
			void RemoveRequest( TickerId id )noexcept;
			flat_map<TickerId,ContractPK> _tickerContractMap; //mutable shared_mutex _tickerContractMapMutex;

			flat_map<ContractPK,Tick> _values; mutable shared_mutex _valuesMutex;
			flat_map<ContractPK,TickFields> _outgoingFields; mutex _outgoingFieldsMutex;

			enum ESubscriptionSource{ Internal/*InternalSubscriptionHandle*/, Coroutine/*clientHandle*/, Proto/*SessionId*/ };
			struct TickListSource{ ESubscriptionSource Source; uint Id; flat_set<ETickList> Ticks; };
			void RemoveTwsSubscription( ESubscriptionSource source, uint id, ContractPK contractId )noexcept;
			flat_set<ETickList> GetSubscribedTicks( ContractPK id )const noexcept;
			bool AddSubscription( ContractPK contractId, const TickListSource& source, sp<unique_lock<mutex>> pLock={} )noexcept;
			flat_multimap<TimePoint,tuple<ESubscriptionSource, uint, ContractPK>> _delays; std::shared_mutex _delayMutex;
			flat_multimap<ContractPK,TickListSource> _twsSubscriptions; std::mutex _twsSubscriptionMutex;

			flat_multimap<ContractPK,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;//Type=0, handleIndex=clientId

			void Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept;
			struct ProtoSubscription
			{
				uint32 SessionId;
				uint32 ClientId;

				TickManager::ProtoFunction Function;
			};
			flat_multimap<ContractPK,ProtoSubscription> _protoSubscriptions; mutex _protoSubscriptionMutex;//Type=1, handleIndex=[SessionId,clientId]

			flat_multimap<ContractPK,tuple<std::promise<Tick>,TimePoint,uint>> _ratioSubscriptions; mutex _ratioSubscriptionMutex;

			void CancelMarketData( TickerId oldId, TickerId newId )noexcept;
			Collections::MapValue<TickerId,TimePoint> _canceledTicks;//TODO move to cache.

			flat_multimap<TimePoint,TickerId> _orphans; mutex _orphanMutex;//TODO make a multimap class
			sp<TwsClient> _pTwsClient;
			TimePoint LastOutgoing;

			void CalcImpliedVolatility( uint32 hClient, const ::Contract& contract, double optionPrice, double underPrice, TickManager::ProtoFunction fnctn )noexcept;
			void CalculateOptionPrice( uint32 hClient, const ::Contract& contract, double volatility, double underPrice, TickManager::ProtoFunction fnctn )noexcept;
			flat_multimap<TickerId,ProtoSubscription> _optionRequests; mutex _optionRequestMutex;

			uint InternalSubscriptionHandle{0};
			friend TickManager;
			friend EventManagerTests; friend OptionTests;
		};
	};

	inline void TickManager::TickWorker::AddOutgoingField( ContractPK id, ETickType t )noexcept
	{
		unique_lock l{ _outgoingFieldsMutex };
		auto& v = _outgoingFields.try_emplace( id ).first->second;
		v.set( t );
		std::unique_lock<std::mutex> lk( _mtx );
		_cv.notify_one();
	}
}

