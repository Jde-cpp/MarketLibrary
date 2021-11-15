#include "TickManager.h"
#include "client/TwsClientSync.h"
#include "../../Framework/source/collections/Vector.h"
#include <jde/Str.h>
#include "types/IBException.h"


#define WorkerPtr if( auto p=TickWorker::Instance(); p ) p
#define TwsClientPtr if( _pTwsClient ) _pTwsClient
#define var const auto

namespace Jde::Markets
{
	static const LogTag& _logLevel{ Logging::TagLevel("tick") };
	α TickManager::Ratios( const ContractPK contractId )noexcept->std::future<Tick>
	{
		auto p=TickWorker::Instance();
		THROW_IF( !p, "Shutting Down" );
		return p->Ratios( contractId );
	}

	α TickManager::Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept->void
	{
		WorkerPtr->Subscribe( sessionId, clientId,  contractId, fields, snapshot, fnctn );
	}

	α TickManager::CalcImpliedVolatility( uint32 sessionId, uint32 clientId, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept->void
	{
		WorkerPtr->CalcImpliedVolatility( sessionId, contract, optionPrice, underPrice, fnctn );
	}

	α TickManager::CalculateOptionPrice( uint32 sessionId, uint32 clientId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept->void
	{
		WorkerPtr->CalculateOptionPrice( sessionId, contract, volatility, underPrice, fnctn );
	}

	α TickManager::CancelProto( uint sessionId, uint /*clientId*/, ContractPK contractId )noexcept->void
	{
		WorkerPtr->CancelProto( sessionId, contractId );
	}

	α TickManager::Cancel( Coroutine::Handle h )noexcept->void
	{
		WorkerPtr->Cancel( h );
	}

	TickManager::Awaitable::Awaitable( const TickParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		TickParams{ params }
	{}

	α TickManager::Awaitable::await_suspend( coroutine_handle<Task2::promise_type> h )noexcept->void
	{
		base::await_suspend( h );
		_pPromise = &h.promise();
		if( auto p=TickWorker::Instance(); p )
			p->Subscribe( TickWorker::SubscriptionInfo{ {h, _hClient}, *this} );
	}
	bool TickManager::Awaitable::await_ready()noexcept
	{
		auto p=TickWorker::Instance();
		var isReady = p && p->Test( Tick, Fields );
		if( isReady )
			LOG( "TickManager::await_ready={}"sv, isReady );
		return isReady;
	}

	TickManager::TickWorker::TickWorker( sp<TwsClient> _pParent )noexcept:
		base{"TickWorker"},
		_pTwsClient{ _pParent }
	{}

	sp<TickManager::TickWorker> TickManager::TickWorker::CreateInstance( sp<TwsClient> _pParent )noexcept
	{
		auto p = _pInstance = make_shared<TickManager::TickWorker>( _pParent );
		_pInstance->Start();
		return dynamic_pointer_cast<TickManager::TickWorker>( p );
	}

	optional<Tick> TickManager::TickWorker::Get( ContractPK contractId )const noexcept
	{
		shared_lock l{ _valuesMutex };
		auto p = _values.find( contractId );
		return p==_values.end() ? optional<Tick>{} : p->second;
	}
	TickerId TickManager::TickWorker::RequestId( ContractPK contractId )const noexcept
	{
		shared_lock l{ _valuesMutex };
		auto p = _values.find( contractId );
		return p==_values.end() ? 0 : p->second.TwsRequestId;
	}

	α TickManager::TickWorker::SetRequestId( TickerId id, ContractPK contractId )noexcept->void
	{
		unique_lock l{ _valuesMutex };
		LOG( "_tickerContractMap adding [{}]={}"sv, id, contractId );
		_tickerContractMap.emplace( id, contractId );
		_values.try_emplace( contractId, contractId, id );
	}

	bool TickManager::TickWorker::Test( Tick& clientTick, TickFields fields )noexcept
	{
		var contractId = clientTick.ContractId;
		var tick = Get( contractId );
		bool change = tick && TickManager::TickWorker::Test( clientTick, fields, tick.value() );
		if( change )
		{
			unique_lock l2{ _delayMutex };
			if( auto p = std::find_if(_delays.begin(), _delays.end(), [contractId](auto x){return contractId==get<2>(x.second);}); p!=_delays.end() )
			{
				LOG( "_delays.refresh[{}]={} - Test"sv, get<1>(p->second), get<2>(p->second) );
				var value = p->second;
				_delays.erase( p );
				_delays.emplace( Clock::now()+3s, value );
			}
		}
		return change;
	}

	bool TickManager::TickWorker::Test( Tick& clientTick, TickFields clientFields, const Tick& latestTick )const noexcept
	{
		bool haveUpdate = false;
		for( uint i = 0; clientFields.any() && i < clientFields.size() && !haveUpdate; ++i )
		{
			if( !clientFields[i] )
				continue;
			clientFields.reset( i );
			var tickType = (ETickType)i;
			haveUpdate = latestTick.IsSet(tickType) && !clientTick.FieldEqual( latestTick, tickType );
		}
		if( haveUpdate )
			clientTick = latestTick;
		return haveUpdate;
	}

	α TickManager::TickWorker::Push( TickerId tickerId, ETickType tickType, function<void(Tick&)> fnctn )noexcept->void
	{
		auto contractId = ContractId( tickerId );
		if( contractId )
		{
			shared_lock l( _valuesMutex );
			if( auto pValue=_values.find(contractId); pValue!=_values.end() )
			{
				fnctn( pValue->second );
				l.unlock();
				AddOutgoingField( contractId, tickType );
			}
		}
		else if( _canceledTicks.try_emplace(tickerId, Clock::now()+120s) )
		{
			ERR( "({})Could not find contract for ticker, canceling"sv, tickerId );
			CancelMarketData( tickerId, 0 );
		}
	}
	α TickManager::TickWorker::Push( TickerId id, ETickType type, Decimal v )noexcept->void
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetDecimal( type, v );} );
	}
	α TickManager::TickWorker::Push( TickerId id, ETickType type, double v )noexcept->void
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetDouble( type, v );} );
	}

	α TickManager::TickWorker::PushPrice( TickerId id, ETickType type, double v/*, const TickAttrib& attribs*/ )noexcept->void
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetPrice(type, v/*, attribs*/);} );
	}
	α TickManager::TickWorker::Push( TickerId id, ETickType type, long long v )noexcept->void
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetInt(type, v);} );
	}
	α TickManager::TickWorker::Push( TickerId id, ETickType type, str v )noexcept->void
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetString(type, v);} );
	}
	α TickManager::TickWorker::Push( TickerId id, time_t timeStamp, str providerCode, str articleId, str headline, str extraData )noexcept->void
	{
		News v{ timeStamp, providerCode, articleId, headline, extraData };
		Push( id, ETickType::NewsTick, [v](Tick& tick)mutable{ tick.AddNews(move(v));} );
	}

	α TickManager::TickWorker::Push( TickerId id, ETickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept->void
	{
		Proto::Results::MessageUnion msg;
		OptionComputation optionComp{ tickAttrib==0, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice };
		unique_lock l{ _optionRequestMutex };


		if( auto p = _optionRequests.find(id); p!=_optionRequests.end() )
		{
			const uint32_t clientId = p->second.ClientId;
			const uint32_t sessionId = p->second.SessionId;
			msg.set_allocated_option_calculation( optionComp.ToProto( clientId, type ).release() );
			unique_lock l{ _optionRequestMutex };
			p->second.Function( {msg}, sessionId );
			_optionRequests.erase( p );
		}
		else
			Push( id, type, [type, x=move(optionComp)](Tick& tick)mutable{tick.SetOptionComputation(type, move(x));} );
	}
	bool TickManager::TickWorker::HandleError( int id, int errorCode, str errorString )noexcept
	{
		ContractPK contractId;
		if( errorCode==10197 )
			return false;
		{
			unique_lock l{ _valuesMutex };
			LOG( "_tickerContractMap={}"sv, _tickerContractMap.size() );
			auto p=_tickerContractMap.find( id );
			if( p==_tickerContractMap.end() )
				return false;
			contractId = p->second;
			LOG( "_tickerContractMap erasing [{}]={}"sv, p->first, p->second );
			_tickerContractMap.erase( p );
			if( auto pValue = _values.find(contractId); pValue!=_values.end() )
				pValue->second.TwsRequestId = 0;
		}
#define FORX(X,Iter) for( auto Iter=X.find(contractId); Iter!=X.end() && Iter->first==contractId; Iter = X.erase(Iter) )
#define FOR(X) FORX(X,p)
//#define IBExceptionPtr make_shared<IBException>( errorString, errorCode, id )
		{
			unique_lock l{ _twsSubscriptionMutex };
			for( auto pSub=_twsSubscriptions.find(contractId); pSub!=_twsSubscriptions.end() && pSub->first==contractId; pSub = _twsSubscriptions.erase(pSub) )
			{
				const TickListSource& s = pSub->second;
				if( s.Source==ESubscriptionSource::Proto )
				{
					unique_lock l2{ _protoSubscriptionMutex };
					for( auto p = _protoSubscriptions.find(contractId); p!=_protoSubscriptions.end() && p->first==contractId; p = _protoSubscriptions.erase(p) )
					{
						Proto::Results::MessageUnion message;
						auto pError = make_unique<Proto::Results::Error>(); pError->set_request_id(contractId); pError->set_code(errorCode); pError->set_message(errorString);
						message.set_allocated_error( pError.release() );
						p->second.Function( {message}, contractId );
					}
				}
				else if( s.Source==ESubscriptionSource::Internal )
				{
					unique_lock l2{ _ratioSubscriptionMutex };
					FOR( _ratioSubscriptions )
						get<0>( p->second ).set_exception( std::make_exception_ptr(IBException{errorString, errorCode, id}) );
				}
				else if( s.Source==ESubscriptionSource::Coroutine )
				{
					unique_lock l2{ _subscriptionMutex };
					FOR( _subscriptions )
					{
						auto h = p->second.HCoroutine;
						auto& returnObject = h.promise().get_return_object();
						returnObject.SetResult( std::dynamic_pointer_cast<IException>(make_shared<IBException>(errorString, errorCode, id)) );
						Coroutine::CoroutinePool::Resume( move(h) );
					}
				}
			}
		}
		return true;
	}

	uint index = 0;
	α TickManager::TickWorker::Process()noexcept->void
	{
		vector<tuple<Tick,TickFields>> outgoingTicks;
		{
			unique_lock ul{_outgoingFieldsMutex};
			for( auto pOutgoingField=_outgoingFields.begin(); pOutgoingField!=_outgoingFields.end(); ++pOutgoingField )
			{
				var contractId = pOutgoingField->first;
				shared_lock l2{_valuesMutex};
				if( auto pValue = _values.find(contractId); pValue!=_values.end() )
				{
					var& fields = pOutgoingField->second;
					auto& tick = pValue->second;
					if( fields[ETickType::BidSize] )
						TRACE( "{}", tick.BidSize );
					outgoingTicks.push_back( {tick, fields} );
					if( fields[ETickType::NewsTick] )
						tick.NewsPtr->clear();
				}
				else
					CRITICAL( "({})Could not find contract in values."sv, contractId );
			}
			_outgoingFields.clear();
		}
		if( outgoingTicks.size() )
		{
			if( ++index %1000 == 0 )
				LOG( "index={}k"sv, index/1000 );
		}
		for( var& outgoing : outgoingTicks )
		{

			var& tick = get<0>( outgoing );
			var& fields = get<1>( outgoing );
			var contractId = tick.ContractId;
			bool haveSubscription;
			{//coroutine Subscriptions
				unique_lock ul{ _subscriptionMutex };
				auto range = _subscriptions.equal_range( contractId );
				haveSubscription = range.first!=range.second;
				for( auto p = range.first; p!=_subscriptions.end() && p->first==contractId; )
				{
					auto& params = p->second.Params;
					auto& clientTick = params.Tick;
					auto clientFields = params.Fields;
					bool resume = Test( clientTick, clientFields, tick );
					if( resume )
					{
						auto h = p->second.HCoroutine;
						var hClient = p->second.HClient;
						{
							unique_lock ul2{ _delayMutex };
							_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Coroutine, hClient, contractId) );
						}
						auto& returnObject = h.promise().get_return_object();
						returnObject.SetResult( make_shared<Tick>(tick) );
						LOG( "({})TickManager - Calling resume()."sv, hClient );
						Coroutine::CoroutinePool::Resume( move(h)/*, move(p->second.ThreadParam)*/ );
						p = _subscriptions.erase( p );
					}
					else
						p = next( p );
				}
			}
			{//Proto Subscriptions
				unique_lock ul{ _protoSubscriptionMutex };//only shared.
				auto range = _protoSubscriptions.equal_range( contractId );
				if( range.first!=range.second )
				{
					haveSubscription = true;
					vector<Proto::Results::MessageUnion> messages;
					auto f = fields;
					for( uint i = 0; f.any() && i < f.size(); ++i )
					{
						if( !f[i] )
							continue;
						f.reset( i );
						var tickType = (ETickType)i;
						if( tickType==ETickType::BidSize )
							TRACE( "{}", tick.BidSize );
						tick.AddProto( tickType, messages );
					}
					if( messages.size() )
					{
						for( auto p=range.first; p!=range.second; ++p )
						{
							try
							{
								CHECK( p->second.Function );//not sure why
								p->second.Function( messages, contractId );
							}
							catch( const IException& )
							{
								var sessionId = p->second.SessionId;
								CancelProto( sessionId, contractId, &ul );
							}
						}
					}
					else
						LOG( "!messages.size()"sv );
				}
			}
			{//Ratio Subscriptions
				unique_lock ul{ _ratioSubscriptionMutex };
				auto range = _ratioSubscriptions.equal_range( contractId );
				haveSubscription = haveSubscription || range.first!=range.second;
				for( auto p = range.first; p!=range.second; ++p )
				{
					if( !tick.HasRatios() )
						continue;
					get<0>(p->second).set_value( tick );
					_ratioSubscriptions.erase( p );
					unique_lock ul2{ _delayMutex };
					LOG( "_delays.emplace[{}]={} - ratios"sv, get<2>(p->second), contractId );
					_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Internal, get<2>(p->second), contractId) );
				}
			}
			LastOutgoing = Clock::now();
			unique_lock ul{ _orphanMutex };
			if( !haveSubscription && std::find_if(_orphans.begin(), _orphans.end(), [&tick](auto x){return tick.ContractId==x.second;})==_orphans.end() )
			{
				shared_lock l2{ _delayMutex };
				var haveDelay = std::find_if( _delays.begin(), _delays.end(), [contractId](auto x){return contractId==get<2>(x.second);} )!=_delays.end();
				if( !haveDelay && !_canceledTicks.contains(tick.TwsRequestId) && tick.TwsRequestId )
					_orphans.emplace( Clock::now()+3s, tick.TwsRequestId );
			}
		}
		{//ratio timeouts
			unique_lock ul{ _ratioSubscriptionMutex };
			for( auto p = _ratioSubscriptions.begin(); p!=_ratioSubscriptions.end(); )
			{
				auto& value = p->second;
				var remove = get<1>(value)<Clock::now();
				if( remove )
				{
					LOG( "{}<{}"sv, ToIsoString(get<1>(value)), ToIsoString(Clock::now()) );
					LOG( "_delays.emplace[{}]={} - ratio timeout"sv, get<2>(p->second), p->first );
					get<0>(value).set_exception( std::make_exception_ptr(Exception("Timeout")) );
					unique_lock ul2{ _delayMutex };
					_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Internal, get<2>(p->second), p->first) );
				}
				p = remove ? _ratioSubscriptions.erase( p ) : next( p );
			}
		}
		{
			unique_lock ul{ _delayMutex };
			for( auto p = _delays.begin(); p!=_delays.end() && p->first<Clock::now();  )
			{
				var& value = p->second;
				RemoveTwsSubscription( get<0>(value), get<1>(value), get<2>(value) );
				p = _delays.erase( p );
			}
		}
		{
			unique_lock ul{ _orphanMutex };
			for( auto p = _orphans.begin(); p!=_orphans.end() && p->first<Clock::now(); p = _orphans.erase( p ) )
			{
				LOG( "({})Canceling orphan request {}<now"sv, p->second, ToIsoString(p->first) );
				CancelMarketData( p->second, 0 );
			}
		}
		if( !outgoingTicks.size() )
		{
			std::unique_lock<std::mutex> ul( _mtx );
			_cv.wait_for( ul, WakeDuration );
			TRACE( "Woken" );
		}
	}
	flat_set<ETickList> TickManager::TickWorker::GetSubscribedTicks( ContractPK id )const noexcept
	{
		flat_set<ETickList> ticks;
		auto range = _twsSubscriptions.equal_range( id );
		for( auto p=range.first; p!=range.second; ++p )
			ticks.insert( p->second.Ticks.begin(), p->second.Ticks.end() );
		return ticks;
	}
	α TickManager::TickWorker::RemoveTwsSubscription( ESubscriptionSource source, uint id, ContractPK contractId )noexcept->void
	{
		unique_lock	l{ _twsSubscriptionMutex };
		flat_set<ETickList> previousTicks;
		auto range = _twsSubscriptions.equal_range( contractId );
		auto pFound = range.second;
		for( auto p = range.first; p!=range.second; ++p )
		{
			var itemSource = p->second;
			previousTicks.insert( itemSource.Ticks.begin(), itemSource.Ticks.end() );
			if( itemSource.Source==source && itemSource.Id==id )
				pFound = p;
		}
		if( pFound!=range.second )
		{
			LOG( "({})Erasing _twsSubscription {}."sv, pFound->second.Id, pFound->second.Source );
			_twsSubscriptions.erase( pFound );
			var currentTicks = GetSubscribedTicks( contractId );
			var requestId = RequestId( contractId );
			if( !requestId )
				WARN( "Lost requestId for contract '{}'."sv, contractId );
			if( requestId && currentTicks.size()<previousTicks.size() )
			{
				auto newRequestId = currentTicks.size() ? _pTwsClient->RequestId() : 0;
				CancelMarketData( requestId, newRequestId );
				if( newRequestId )
				{
					::Contract contract; contract.conId = contractId; contract.exchange = "SMART";
					_pTwsClient->reqMktData( newRequestId, contract, Str::AddCommas(currentTicks), false, false, {} );
				}
			}
		}
		else
			WARN( "Lost request for contract '{}'"sv, contractId );
	}
	α TickManager::TickWorker::Cancel( Coroutine::Handle h )noexcept->void
	{
		unique_lock l{ _subscriptionMutex };
		if( auto p = std::find_if(_subscriptions.begin(), _subscriptions.end(), [h](var x){ return x.second.HClient==h;}); p!=_subscriptions.end() )
		{
			LOG( "Cancel({})"sv, h );
			p->second.HCoroutine.destroy();
			var contractId = p->first;
			_subscriptions.erase( p );
			l.unlock();
			unique_lock l{ _delayMutex };
			_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Coroutine, h, contractId) );
		}
		else
			LOG( "Could not find handle {}."sv, h );
	}
	α TickManager::TickWorker::CancelProto( uint sessionId, ContractPK contractId, unique_lock<mutex>* pLock )noexcept->void
	{
		auto pl = pLock ? up<unique_lock<mutex>>{} : make_unique<unique_lock<mutex>>( _protoSubscriptionMutex );
		auto remove = [this]( auto p )
		{
			LOG( "_delays.emplace[{}]={} - Proto Cancel"sv, p->second.SessionId, p->first );
			unique_lock l{ _delayMutex };
			_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Proto, p->second.SessionId, p->first) );
			return _protoSubscriptions.erase( p );
		};
		if( contractId )
		{
			auto range = _protoSubscriptions.equal_range( contractId );
			auto p = std::find_if( range.first, range.second, [sessionId](auto x){ return x.second.SessionId==sessionId; } );
			if( p!=range.second )
				remove( p );
			else
				LOG( "Could not find proto contractId={} sessionId={}"sv, contractId, sessionId );
		}
		else
			for( auto p = _protoSubscriptions.begin(); p!=_protoSubscriptions.end(); p = sessionId==p->second.SessionId ? remove(p) : std::next(p) );
	}

	#define PREFIX auto pTwsClient = _pTwsClient; if( !pTwsClient ) return; var id = pTwsClient->RequestId(); unique_lock l{ _optionRequestMutex }; _optionRequests.emplace( id, ProtoSubscription{sessionId, 0, fnctn} );
	α TickManager::TickWorker::CalcImpliedVolatility( uint32 sessionId, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept->void
	{
		PREFIX
		pTwsClient->calculateImpliedVolatility( id, contract, optionPrice, underPrice, {} );
	}
	α TickManager::TickWorker::CalculateOptionPrice( uint32 sessionId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept->void
	{
		PREFIX
		pTwsClient->calculateOptionPrice( id, contract, volatility, underPrice, {} );
	}

	std::future<Tick> TickManager::TickWorker::Ratios( const ContractPK contractId )noexcept
	{
		auto pTwsClient = _pTwsClient;
		var tick = Get( contractId );
		if( tick && tick.value().HasRatios() )
		{
			std::promise<Tick> promise;
			promise.set_value( tick.value() );
			return promise.get_future();
		}
		//don't already have.
		auto pLock = make_shared<unique_lock<mutex>>( _twsSubscriptionMutex );
		flat_set<ETickList> requestTicks = GetSubscribedTicks( contractId );

		const flat_set ticks{ ETickList::MiscStats, ETickList::Fundamentals/*, ETickList::Dividends*/ };
		var handle = ++InternalSubscriptionHandle;
		AddSubscription( contractId, TickListSource{ESubscriptionSource::Internal, handle, ticks}, pLock );

		unique_lock l{ _ratioSubscriptionMutex };
		LOG( "_ratioSubscriptions.emplace({}, {})"sv, contractId, ToIsoString(Clock::now()+5s) );
		auto p = _ratioSubscriptions.emplace( contractId, make_tuple( std::promise<Tick>{}, Clock::now()+5s, handle ) );
		return get<0>( p->second ).get_future();
	}

	bool TickManager::TickWorker::AddSubscription( ContractPK contractId, const TickListSource& source, sp<unique_lock<mutex>> pLock )noexcept
	{
		auto newTicks = 0;

		if( !pLock )
			pLock = make_shared<unique_lock<mutex>>( _twsSubscriptionMutex );
		auto requestTicks = GetSubscribedTicks( contractId );
		std::for_each( source.Ticks.begin(), source.Ticks.end(), [&newTicks, &requestTicks](auto x){newTicks+=requestTicks.emplace(x).second;} );
		LOG( "({})Adding _twsSubscription {}."sv, source.Id, source.Source );
		_twsSubscriptions.emplace( contractId, source );
		pLock = nullptr;
		if( newTicks )
		{
			var currentRequestId = requestTicks.size()>source.Ticks.size() ? RequestId( contractId ) : 0;
			var reqId = _pTwsClient->RequestId();
			if( currentRequestId )
				CancelMarketData( currentRequestId, reqId );
			else
			{
				unique_lock l{ _valuesMutex };
				LOG( "_tickerContractMap[{}]={}"sv, reqId, contractId );
				if( auto p=_tickerContractMap.emplace( reqId, contractId ); !p.second )
					p.first->second = contractId;
				if( auto pValue = _values.find(contractId); pValue!=_values.end() )
					pValue->second.TwsRequestId = reqId;
				else
					_values.emplace( contractId, Tick{contractId, reqId} );
			}


			::Contract contract;  contract.conId = contractId; contract.exchange = "SMART";
			_pTwsClient->reqMktData( reqId, contract, Str::AddCommas(requestTicks), false, false, {} );//456=dividends - https://interactivebrokers.github.io/tws-api/tick_types.html
		}
		return newTicks;
	}
	//orphans
	//add/remove subscription.
	α TickManager::TickWorker::CancelMarketData( TickerId oldId, TickerId newId )noexcept->void
	{
		{
			var now = Clock::now();
			unique_lock l{ _canceledTicks.Mutex };
			_canceledTicks.LockEmplace( l, oldId, now+120s );
			for( auto p=_canceledTicks.begin(l); p!=_canceledTicks.end(l);  )
				p = p->second<now ? _canceledTicks.erase( l, p ) : std::next( p );
		}
		{
			unique_lock l{ _valuesMutex };
			if( auto p=_tickerContractMap.find( oldId ); p!=_tickerContractMap.end() )
			{
				var contractId = p->second;
				LOG( "_tickerContractMap erasing [{}]={}"sv, p->first, contractId );
				_tickerContractMap.erase( p );
				if( newId )
				{
					LOG( "_tickerContractMap adding [{}]={}"sv, newId, contractId );
					_tickerContractMap.emplace( newId, contractId );
				}
				if( auto pValue=_values.find(contractId); pValue!=_values.end() )
					pValue->second.TwsRequestId = newId;
				else
					ERR( "Lost requestId for contract '{}' with newRequest {}."sv, contractId, newId );
			}
		}
		LOG( "TickWorker::cancelMktData( oldId={}, newId={} )"sv, oldId, newId );
		TwsClientPtr->cancelMktData( oldId, false );
	}

	α TickManager::TickWorker::Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept->void
	{
		{
			unique_lock l{ _protoSubscriptionMutex };
			_protoSubscriptions.emplace( contractId, ProtoSubscription{sessionId, clientId, fnctn} );
		}
		if( !AddSubscription(contractId, TickListSource{ESubscriptionSource::Proto, sessionId, fields}) )
		{
			if( auto pTick = Get( contractId ); pTick )
			{
				vector<Proto::Results::MessageUnion> messages;
				auto f = pTick->SetFields();
				for( uint i = 0; f.any() && i < f.size(); ++i )
				{
					if( !f[i] )
						continue;
					f.reset( i );
					pTick->AddProto( (ETickType)i, messages );
				}
				if( messages.size() )
				{
					try
					{
						fnctn( messages, contractId );
					}
					catch( const IException& )
					{
						CancelProto( sessionId, contractId );
					}
				}
			}
		}
	}
	α TickManager::TickWorker::Subscribe( const SubscriptionInfo& subscription )noexcept->void
	{
		var& params = subscription.Params;
		var contractId = params.Tick.ContractId;
		{
			unique_lock l{ _subscriptionMutex };
			_subscriptions.emplace( contractId, subscription );
		}
		AddSubscription( contractId, TickListSource{ESubscriptionSource::Coroutine, subscription.HClient, {ETickList::PlPrice}} );
	}
}