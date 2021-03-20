#include "TickManager.h"
#include "client/TwsClientSync.h"
#include "../../Framework/source/collections/Vector.h"
#include "types/IBException.h"

#define WorkerPtr if( auto p=TickWorker::Instance(); p ) p
#define TwsClientPtr if( _pTwsClient ) _pTwsClient
#define var const auto

namespace Jde::Markets
{
	std::future<Tick> TickManager::Ratios( const ContractPK contractId )noexcept
	{
		auto p=TickWorker::Instance();
		if( !p )
			THROW( Exception("Shutting Down") );
		return p->Ratios( contractId );
	}

	void TickManager::Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->Subscribe( sessionId, clientId,  contractId, fields, snapshot, fnctn );
	}

	void TickManager::CalcImpliedVolatility( uint32 sessionId, uint32 clientId, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->CalcImpliedVolatility( sessionId, contract, optionPrice, underPrice, fnctn );
	}

	void TickManager::CalculateOptionPrice( uint32 sessionId, uint32 clientId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->CalculateOptionPrice( sessionId, contract, volatility, underPrice, fnctn );
	}

	void TickManager::CancelProto( uint sessionId, uint /*clientId*/, ContractPK contractId )noexcept
	{
		WorkerPtr->CancelProto( sessionId, contractId );
	}

	void TickManager::Cancel( Coroutine::Handle h )noexcept
	{
		WorkerPtr->Cancel( h );
	}

	TickManager::Awaitable::Awaitable( const TickParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		TickParams{ params }
	{}

	void TickManager::Awaitable::await_suspend( TickManager::Awaitable::base::Handle h )noexcept
	{
		base::await_suspend( h );
		ASSERT( Tick.ContractId );
		_pPromise = &h.promise();
		if( auto p=TickWorker::Instance(); p )
			p->Subscribe( TickWorker::SubscriptionInfo{ {h, _hClient}, *this} );
	}
	bool TickManager::Awaitable::await_ready()noexcept
	{
		auto p=TickWorker::Instance();
		var isReady = p && p->Test( Tick, Fields );
		if( isReady )
			DBG( "TickManager::await_ready={}"sv, isReady );
		return isReady;
	}

	TickManager::TickWorker::TickWorker( sp<TwsClient> _pParent )noexcept:
		base{"TickWorker"},
		_pTwsClient{ _pParent }
	{}

	sp<TickManager::TickWorker> TickManager::TickWorker::CreateInstance( sp<TwsClient> _pParent )noexcept
	{
		auto p = make_shared<TickManager::TickWorker>( _pParent );
		p->Start();
		_pInstance = p;
		return p;
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

	void TickManager::TickWorker::SetRequestId( TickerId id, ContractPK contractId )noexcept
	{
		unique_lock l{ _valuesMutex };
		DBG( "_tickerContractMap adding [{}]={}"sv, id, contractId );
		_tickerContractMap.emplace( id, contractId );
		_values.try_emplace( contractId, contractId, id );
	}
/*	void TickManager::TickWorker::RemoveRequest( TickerId id )noexcept
	{
		unique_lock l{ _valuesMutex };
		if( auto p=_tickerContractMap.find( id ); p!=_tickerContractMap.end() )
		{
			_tickerContractMap.erase( p );
			if( auto pValue=_values.find(p->second); pValue!=_values.end() )
				pValue->second.TwsRequestId = 0;
		}
	}*/

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
				DBG( "_delays.refresh[{}]={} - Test"sv, get<1>(p->second), get<2>(p->second) );
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

	void TickManager::TickWorker::Push( TickerId tickerId, ETickType tickType, function<void(Tick&)> fnctn )noexcept
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
	void TickManager::TickWorker::Push( TickerId id, ETickType type, double v )noexcept
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetDouble( type, v );} );
	}

	void TickManager::TickWorker::PushPrice( TickerId id, ETickType type, double v/*, const TickAttrib& attribs*/ )noexcept
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetPrice(type, v/*, attribs*/);} );
	}
	void TickManager::TickWorker::Push( TickerId id, ETickType type, int v )noexcept
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetInt(type, v);} );
	}
	void TickManager::TickWorker::Push( TickerId id, ETickType type, const std::string& v )noexcept
	{
		Push( id, type, [type,v](Tick& tick){ tick.SetString(type, v);} );
	}
	void TickManager::TickWorker::Push( TickerId id, time_t timeStamp, const std::string& providerCode, const std::string& articleId, const std::string& headline, const std::string& extraData )noexcept
	{
		News v{ timeStamp, providerCode, articleId, headline, extraData };
		Push( id, ETickType::NewsTick, [v](Tick& tick)mutable{ tick.AddNews(move(v));} );
	}

	void TickManager::TickWorker::Push( TickerId id, ETickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept
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
	bool TickManager::TickWorker::HandleError( int id, int errorCode, const std::string& errorString )noexcept
	{
		ContractPK contractId;
		if( errorCode==10197 )
			return false;
		{
			unique_lock l{ _valuesMutex };
			DBG( "_tickerContractMap={}"sv, _tickerContractMap.size() );
			auto p=_tickerContractMap.find( id );
			if( p==_tickerContractMap.end() )
				return false;
			contractId = p->second;
			DBG( "_tickerContractMap erasing [{}]={}"sv, p->first, p->second );
			_tickerContractMap.erase( p );
			if( auto pValue = _values.find(contractId); pValue!=_values.end() )
				pValue->second.TwsRequestId = 0;
		}
#define FORX(X,Iter) for( auto Iter=X.find(contractId); Iter!=X.end() && Iter->first==contractId; Iter = X.erase(Iter) )
#define FOR(X) FORX(X,p)
#define IBExceptionPtr std::make_exception_ptr(IBException{errorString, errorCode, id, __func__,__FILE__, __LINE__})
		{
			unique_lock l{ _twsSubscriptionMutex };
			FORX(_twsSubscriptions, pSub )
			{
				const TickListSource& s = pSub->second;
				if( s.Source==ESubscriptionSource::Proto )
				{
					unique_lock l2{ _protoSubscriptionMutex };
					FOR(_protoSubscriptions )
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
						get<0>( p->second ).set_exception( IBExceptionPtr );
				}
				else if( s.Source==ESubscriptionSource::Coroutine )
				{
					unique_lock l2{ _subscriptionMutex };
					FOR( _subscriptions )
					{
						auto h = p->second.HCoroutine;
						auto& returnObject = h.promise().get_return_object();
						returnObject.Result = IBExceptionPtr;
						Coroutine::CoroutinePool::Resume( move(h) );
					}
				}
			}
		}
		return true;
	}

	uint index = 0;
	void TickManager::TickWorker::Process()noexcept
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
				DBG( "index={}k"sv, index/1000 );
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
						returnObject.Result = tick;
						DBG( "({})TickManager - Calling resume()."sv, hClient );
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
						tick.AddProto( tickType, messages );
					}
					if( messages.size() )
					{
						for( auto p=range.first; p!=range.second; ++p )
						{
							try
							{
								p->second.Function( messages, contractId );
							}
							catch( const Exception& /*e*/ )
							{
								var sessionId = p->second.SessionId;
								CancelProto( sessionId, contractId, &ul );
							}
						}
					}
					else
						DBG0( "!messages.size()"sv );
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
					DBG( "_delays.emplace[{}]={} - ratios"sv, get<2>(p->second), contractId );
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
					DBG( "{}<{}"sv, ToIsoString(get<1>(value)), ToIsoString(Clock::now()) );
					DBG( "_delays.emplace[{}]={} - ratio timeout"sv, get<2>(p->second), p->first );
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
				DBG( "({})Canceling orphan request {}<now"sv, p->second, ToIsoString(p->first) );
				CancelMarketData( p->second, 0 );
			}
		}
		if( !outgoingTicks.size() )
		{
		/*	if( Clock::now()-LastOutgoing<WakeDuration )
				std::this_thread::yield();
			else */
			{
				std::unique_lock<std::mutex> ul( _mtx );
				_cv.wait_for( ul, WakeDuration );
			}
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
	void TickManager::TickWorker::RemoveTwsSubscription( ESubscriptionSource source, uint id, ContractPK contractId )noexcept
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
			DBG( "({})Erasing _twsSubscription {}."sv, pFound->second.Id, pFound->second.Source );
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
					_pTwsClient->reqMktData( newRequestId, contract, StringUtilities::AddCommas(currentTicks), false, false, {} );
				}
			}
		}
		else
			WARN( "Lost request for contract '{}'"sv, contractId );
	}
	void TickManager::TickWorker::Cancel( Coroutine::Handle h )noexcept
	{
		unique_lock l{ _subscriptionMutex };
		if( auto p = std::find_if(_subscriptions.begin(), _subscriptions.end(), [h](var x){ return x.second.HClient==h;}); p!=_subscriptions.end() )
		{
			DBG( "Cancel({})"sv, h );
			p->second.HCoroutine.destroy();
			var contractId = p->first;
			_subscriptions.erase( p );
			l.unlock();
			//DBG( "_delays.emplace[{}]={}"sv, h, contractId );
			unique_lock l{ _delayMutex };
			_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Coroutine, h, contractId) );
		}
		else
			DBG( "Could not find handle {}."sv, h );
	}
	void TickManager::TickWorker::CancelProto( uint sessionId, ContractPK contractId, unique_lock<mutex>* pLock )noexcept
	{
		auto pl = pLock ? up<unique_lock<mutex>>{} : make_unique<unique_lock<mutex>>( _protoSubscriptionMutex );
		auto remove = [this]( auto p )
		{
			DBG( "_delays.emplace[{}]={} - Proto Cancel"sv, p->second.SessionId, p->first );
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
				DBG( "Could not find proto contractId={} sessionId={}"sv, contractId, sessionId );
		}
		else
			for( auto p = _protoSubscriptions.begin(); p!=_protoSubscriptions.end(); p = sessionId==p->second.SessionId ? remove(p) : std::next(p) );
	}

	#define PREFIX auto pTwsClient = _pTwsClient; if( !pTwsClient ) return; var id = pTwsClient->RequestId(); unique_lock l{ _optionRequestMutex }; _optionRequests.emplace( id, ProtoSubscription{sessionId, 0, fnctn} );
	void TickManager::TickWorker::CalcImpliedVolatility( uint32 sessionId, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept
	{
		PREFIX
		pTwsClient->calculateImpliedVolatility( id, contract, optionPrice, underPrice, {} );
	}
	void TickManager::TickWorker::CalculateOptionPrice( uint32 sessionId, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept
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
		DBG( "_ratioSubscriptions.emplace({}, {})"sv, contractId, ToIsoString(Clock::now()+5s) );
		auto p = _ratioSubscriptions.emplace( contractId, make_tuple( std::promise<Tick>{}, Clock::now()+5s, handle ) );
		return get<0>( p->second ).get_future();
	}

	bool TickManager::TickWorker::AddSubscription( ContractPK contractId, const TickListSource& source, sp<unique_lock<mutex>> pLock )noexcept
	{
		auto newTicks = 0;

		if( !pLock )
			pLock = make_shared<unique_lock<mutex>>( _twsSubscriptionMutex );
		auto requestTicks = GetSubscribedTicks( contractId );
		for_each( source.Ticks.begin(), source.Ticks.end(), [&newTicks, &requestTicks](auto x){newTicks+=requestTicks.emplace(x).second;} );
		DBG("({})Adding _twsSubscription {}."sv, source.Id, source.Source );
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
				DBG( "_tickerContractMap[{}]={}"sv, reqId, contractId );
				if( auto p=_tickerContractMap.emplace( reqId, contractId ); !p.second )
					p.first->second = contractId;
				if( auto pValue = _values.find(contractId); pValue!=_values.end() )
					pValue->second.TwsRequestId = reqId;
				else
					_values.emplace( contractId, Tick{contractId, reqId} );
			}


			::Contract contract;  contract.conId = contractId; contract.exchange = "SMART";
			_pTwsClient->reqMktData( reqId, contract, StringUtilities::AddCommas(requestTicks), false, false, {} );//456=dividends - https://interactivebrokers.github.io/tws-api/tick_types.html
		}
		return newTicks;
	}
	//orphans
	//add/remove subscription.
	void TickManager::TickWorker::CancelMarketData( TickerId oldId, TickerId newId )noexcept
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
				DBG( "_tickerContractMap erasing [{}]={}"sv, p->first, contractId );
				_tickerContractMap.erase( p );
				if( newId )
				{
					DBG( "_tickerContractMap adding [{}]={}"sv, newId, contractId );
					_tickerContractMap.emplace( newId, contractId );
				}
				if( auto pValue=_values.find(contractId); pValue!=_values.end() )
					pValue->second.TwsRequestId = newId;
				else
					ERR( "Lost requestId for contract '{}' with newRequest {}."sv, contractId, newId );
			}
		}
		DBG( "TickWorker::cancelMktData( oldId={}, newId={} )"sv, oldId, newId );
		TwsClientPtr->cancelMktData( oldId, false );
	}

	void TickManager::TickWorker::Subscribe( uint32 sessionId, uint32 clientId, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept
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
					catch( const Exception& /*e*/ )
					{
						CancelProto( sessionId, contractId );
					}
				}
			}
		}
	}
	void TickManager::TickWorker::Subscribe( const SubscriptionInfo& subscription )noexcept
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