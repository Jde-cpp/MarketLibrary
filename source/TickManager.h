#pragma once
#include <experimental/coroutine>
#include "../../Framework/source/threading/Worker.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/coroutine/Coroutine.h"
#include "../../Framework/source/coroutine/Task.h"
#include "../../Framework/source/coroutine/CoWorker.h"
#include "../../Framework/source/collections/Map.h"
#include "../../Framework/source/collections/UnorderedSet.h"
#include "../../Framework/source/collections/UnorderedMapValue.h"
#include <boost/container/flat_set.hpp>
#include "types/proto/requests.pb.h"
#include "types/proto/results.pb.h"
#include "types/Tick.h"

namespace Jde::Markets{ struct TwsClient; }
namespace Jde::Markets
{
	using std::experimental::coroutine_handle;
	using Proto::Requests::ETickList; using Proto::Results::ETickType;
	struct JDE_MARKETS_EXPORT TickManager final
	{
		struct TickParams /*~final*/
		{
			Tick::Fields Fields;
			Markets::Tick Tick;
		};

		struct JDE_MARKETS_EXPORT Awaitable final : Coroutine::CancelAwaitable<Coroutine::Task<Tick>>, TickParams
		{
			typedef Coroutine::CancelAwaitable<Coroutine::Task<Markets::Tick>> base;
			typedef Coroutine::Task<Markets::Tick>::promise_type PromiseType;
			typedef coroutine_handle<PromiseType> Handle;
			Awaitable( const TickParams& params, Coroutine::Handle& h )noexcept;
			~Awaitable()=default;
			bool await_ready()noexcept;
			void await_suspend( Awaitable::Handle h )noexcept;
			Markets::Tick await_resume()noexcept
			{
				DBG( "({})TickManager::Awaitable::await_resume"sv, std::this_thread::get_id() );
				if( _pPromise )
				{
					const auto& result = _pPromise->get_return_object().Result;
					return result;
				}
				return Tick;
			}
		private:
			PromiseType* _pPromise{nullptr};
			//optional<Tick> _tick;
			//void End( Awaitable::Handle h, const Tick* pTick )noexcept; std::once_flag _singleEnd;
		};

		typedef function<void(const vector<const Proto::Results::MessageUnion>&, uint32_t)> ProtoFunction;
		static void CalcImpliedVolatility( uint hClient, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept;
		static void CalculateOptionPrice(  uint hClient, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept;
		static void Cancel( Coroutine::Handle h )noexcept;
		static auto Subscribe( const TickParams& params, Coroutine::Handle& h )noexcept{ return Awaitable{params, h}; }
		static void Subscribe( uint hClient, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept;
		static void CancelProto( uint hClient, ContractPK contractId )noexcept;
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
			void PushPrice( TickerId id, ETickType type, double v/*, const TickAttrib& attribs*/ )noexcept;
			void Push( TickerId id, ETickType type, int v )noexcept;
			void Push( TickerId id, ETickType type, const std::string& v )noexcept;

		private:
			void Process()noexcept override;
			void Cancel( Coroutine::Handle h )noexcept;
			void CancelProto( uint hClient, ContractPK contractId )noexcept;
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
			void AddSubscription(ContractPK contractId, const TickListSource& source )noexcept;
			flat_multimap<TimePoint,tuple<ESubscriptionSource, uint, ContractPK>> _delays; std::shared_mutex _delayMutex;
			flat_multimap<ContractPK,TickListSource> _twsSubscriptions; std::mutex _twsSubscriptionMutex;

			flat_multimap<ContractPK,SubscriptionInfo> _subscriptions; mutex _subscriptionMutex;//Type=0, handleIndex=clientId

			void Subscribe( uint hClient, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept;
			flat_multimap<ContractPK,tuple<uint,TickManager::ProtoFunction>> _protoSubscriptions; mutex _protoSubscriptionMutex;//Type=1, handleIndex=[SessionId,clientId]

			flat_multimap<ContractPK,tuple<std::promise<Tick>,TimePoint,uint>> _ratioSubscriptions; mutex _ratioSubscriptionMutex;

			void CancelMarketData( TickerId oldId, TickerId newId )noexcept;
			Collections::MapValue<TickerId,TimePoint> _canceledTicks;//TODO move to cache.

			flat_multimap<TimePoint,TickerId> _orphans; mutex _orphanMutex;//TODO make a multimap class
			sp<TwsClient> _pTwsClient;
			TimePoint LastOutgoing;

			void CalcImpliedVolatility( uint hClient, const ::Contract& contract, double optionPrice, double underPrice, TickManager::ProtoFunction fnctn )noexcept;
			void CalculateOptionPrice( uint hClient, const ::Contract& contract, double volatility, double underPrice, TickManager::ProtoFunction fnctn )noexcept;
			flat_multimap<TickerId,tuple<uint,TickManager::ProtoFunction>> _optionRequests; mutex _optionRequestMutex;

			uint InternalSubscriptionHandle{0};
			friend TickManager;
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

