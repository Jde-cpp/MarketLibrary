#include "TickManager.h"
#include "client/TwsClientSync.h"
#include "../../Framework/source/collections/Vector.h"

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

	void TickManager::Subscribe( uint hClient, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->Subscribe( hClient, contractId, fields, snapshot, fnctn );
	}

	void TickManager::CalcImpliedVolatility( uint hClient, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->CalcImpliedVolatility( hClient, contract, optionPrice, underPrice, fnctn );
	}

	void TickManager::CalculateOptionPrice( uint hClient, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept
	{
		WorkerPtr->CalculateOptionPrice( hClient, contract, volatility, underPrice, fnctn );
	}

	void TickManager::CancelProto( uint hClient, ContractPK contractId )noexcept
	{
		WorkerPtr->CancelProto( hClient, contractId );
	}

	void TickManager::Cancel( Coroutine::Handle h )noexcept
	{
		WorkerPtr->Cancel( h );
	}

	TickManager::Awaitable::Awaitable( const TickParams& params, Coroutine::Handle& h )noexcept:
		base{ h },
		TickParams{ params }
	{}

	void TickManager::Awaitable::await_suspend( Awaitable::Handle h )noexcept
	{
		_pPromise = &h.promise();
		if( auto p=TickWorker::Instance(); p )
			p->Subscribe( TickWorker::SubscriptionInfo{ {h, _hClient}, *this} );
	}
	bool TickManager::Awaitable::await_ready()noexcept
	{
		auto p=TickWorker::Instance();
		var isReady = p && p->Test( Tick, Fields );
		DBG( "isReady={}"sv, isReady );
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
		var tick = Get( contractId );
		return tick ? tick->TwsRequestId : 0;
	}

	void TickManager::TickWorker::SetRequestId( TickerId id, ContractPK contractId )noexcept
	{
		unique_lock l{ _valuesMutex };
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
		Push( id, ETickType::NEWS_TICK, [v](Tick& tick)mutable{ tick.AddNews(move(v));} );
	}

	void TickManager::TickWorker::Push( TickerId id, ETickType type, int tickAttrib, double impliedVol, double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice )noexcept
	{
		Proto::Results::MessageUnion msg;
		if( auto p = _optionRequests.find(id); p!=_optionRequests.end() )
		{
			var handle = get<0>( p->second );
			const uint32_t clientId = (uint32_t)handle;
			const uint32_t sessionId = (uint32_t)(handle >> 32);
			msg.set_allocated_option_calculation( OptionComputation{ tickAttrib==0, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice }.ToProto(clientId, type).release() );
			unique_lock l{ _optionRequestMutex };
			get<1>( p->second )( {msg}, sessionId );
			_optionRequests.erase( p );
		}
		else
			WARN( "({})Could not find option request"sv, id );
	}

	void TickManager::TickWorker::Process()noexcept
	{
		optional<tuple<Tick,TickFields>> outgoing;
		{
			unique_lock l{_outgoingFieldsMutex};
			if( auto pOutgoingField=_outgoingFields.begin(); pOutgoingField!=_outgoingFields.end() )
			{
				var contractId = pOutgoingField->first;
				shared_lock l{_valuesMutex};
				if( auto pValue = _values.find(contractId); pValue!=_values.end() )
				{
					var& fields = pOutgoingField->second;
					auto& tick = pValue->second;
					outgoing = make_tuple( tick, fields );
					if( fields[ETickType::NEWS_TICK] )
						tick.NewsPtr->clear();
				}
				else
					CRITICAL( "({})Could not find contract in values."sv, contractId );
				_outgoingFields.erase( pOutgoingField );
			}
		}
		if( outgoing )
		{
			var& tick = get<0>( outgoing.value() );
			var& fields = get<1>( outgoing.value() );
			var contractId = tick.ContractId;
			bool haveSubscription;
			{//coroutine Subscriptions
				unique_lock l{ _subscriptionMutex };
				auto range = _subscriptions.equal_range( contractId );
				haveSubscription = range.first!=range.second;
				for( auto p = range.first; p!=range.second; )
				{
					auto& params = p->second.Params;
					auto& clientTick = params.Tick;
					auto clientFields = params.Fields;
					bool resume = Test( clientTick, clientFields, tick );
					if( resume )
					{
						auto h = p->second.HCoroutine;
						var hClient = p->second.HClient;
						p = _subscriptions.erase( p );
						l.unlock();
						{
							unique_lock l2{ _delayMutex };
							_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Coroutine, hClient, contractId) );
						}
						auto& returnObject = h.promise().get_return_object();
						returnObject.Result = tick;
						DBG0( "Calling h.resume()."sv );
						h.resume();//TODO put in another thread.
					}
					else
						p = next( p );
				}
			}
			{//Proto Subscriptions
				unique_lock l{ _protoSubscriptionMutex };
				auto range = _protoSubscriptions.equal_range( contractId );
				for( auto p = range.first; p!=range.second; ++p )
				{
					haveSubscription = true;
					auto f = fields;
					vector<const Proto::Results::MessageUnion> messages;
					for( uint i = 0; f.any() && i < f.size(); ++i )
					{
						if( !f[i] )
							continue;
						f.reset( i );
						var tickType = (ETickType)i;
						tick.AddProto( tickType, messages );
					}
					get<1>(p->second)( messages, contractId );
				}
			}
			{//Ratio Subscriptions
				unique_lock l{ _ratioSubscriptionMutex };
				auto range = _ratioSubscriptions.equal_range( contractId );
				haveSubscription = haveSubscription || range.first!=range.second;
				for( auto p = range.first; p!=range.second; ++p )
				{
					if( !tick.HasRatios() )
						continue;
					get<0>(p->second).set_value( tick );
					_ratioSubscriptions.erase( p );
					unique_lock l{ _delayMutex };
					_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Internal, get<2>(p->second), contractId) );
				}
			}
			LastOutgoing = Clock::now();
			unique_lock l{ _orphanMutex };
			if( !haveSubscription && std::find_if(_orphans.begin(), _orphans.end(), [&tick](auto x){return tick.ContractId==x.second;})==_orphans.end() )
			{
				shared_lock l{ _delayMutex };
				var haveDelay = std::find_if( _delays.begin(), _delays.end(), [contractId](auto x){return contractId==get<2>(x.second);} )==_delays.end();
				if( !haveDelay && !_canceledTicks.contains(tick.TwsRequestId) )
					_orphans.emplace( Clock::now()+3s, tick.TwsRequestId );
			}
		}
		{//ratio timeouts
			unique_lock l{ _ratioSubscriptionMutex };
			for( auto p = _ratioSubscriptions.begin(); p!=_ratioSubscriptions.end(); )
			{
				auto& value = p->second;
				var remove = get<1>(value)>Clock::now();
				if( remove )
				{
					get<0>(value).set_exception( std::make_exception_ptr(Exception("Timeout")) );
					unique_lock l{ _delayMutex };
					_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Internal, get<2>(p->second), p->first) );
				}
				p = remove ? _ratioSubscriptions.erase( p ) : next( p );
			}
		}
		{
			unique_lock l{ _delayMutex };
			for( auto p = _delays.begin(); p!=_delays.end() && p->first<Clock::now();  )
			{
				var& value = p->second;
				RemoveTwsSubscription( get<0>(value), get<1>(value), get<2>(value) );
				p = _delays.erase( p );
			}
		}
		{
			unique_lock l{ _orphanMutex };
			for( auto p = _orphans.begin(); p!=_orphans.end() && p->first<Clock::now(); p = _orphans.erase( p ) )
			{
				DBG( "({})Canceling orphan request"sv, p->second );
				CancelMarketData( p->second, 0 );
			}
		}
		if( !outgoing )
		{
			if( Clock::now()-LastOutgoing<WakeDuration )
				std::this_thread::yield();
			else
			{
				std::unique_lock<std::mutex> lk( _mtx );
				_cv.wait_for( lk, WakeDuration );
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
		auto p = range.first, pFound = range.second;
		for( ; p!=range.second; ++p )
		{
			var itemSource = p->second;
			previousTicks.insert( itemSource.Ticks.begin(), itemSource.Ticks.end() );
			if( itemSource.Source==source && itemSource.Id==id )
				pFound = p;
		}
		if( pFound!=range.second )
		{
			_twsSubscriptions.erase( pFound );
			var currentTicks = GetSubscribedTicks( contractId );
			var requestId = RequestId( contractId );
			if( !requestId )
				WARN( "Lost requestId for contract '{}'"sv, contractId );
			if( requestId && currentTicks.size()<previousTicks.size() )
			{
				auto newRequestId = currentTicks.size() ? _pTwsClient->RequestId() : 0;
				CancelMarketData( requestId, newRequestId );
				if( newRequestId )
				{
					::Contract contract; contract.conId = contractId;
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
			unique_lock l{ _delayMutex };
			_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Coroutine, h, contractId) );
		}
		else
			DBG( "Could not find handle {}."sv, h );
	}
	void TickManager::TickWorker::CancelProto( uint hClient, ContractPK contractId )noexcept
	{
		{
			unique_lock l{ _delayMutex };
			_delays.emplace( Clock::now()+3s, make_tuple(ESubscriptionSource::Proto, hClient, contractId) );
		}
		unique_lock l{ _protoSubscriptionMutex };
		auto range = _protoSubscriptions.equal_range( contractId );
		auto p = std::find_if( range.first, range.second, [hClient](auto x){ return get<0>(x.second)==hClient; } );
		if( p!=range.second )
			_protoSubscriptions.erase( p );
		else
			DBG( "Could not find proto contractId={} hClient={}"sv, contractId, hClient );
	}

	#define PREFIX auto pTwsClient = _pTwsClient; if( !pTwsClient ) return; var id = pTwsClient->RequestId(); unique_lock l{ _optionRequestMutex }; _optionRequests.emplace( id, make_tuple(hClient, fnctn) );
	void TickManager::TickWorker::CalcImpliedVolatility( uint hClient, const ::Contract& contract, double optionPrice, double underPrice, ProtoFunction fnctn )noexcept
	{
		PREFIX
		pTwsClient->calculateImpliedVolatility( id, contract, optionPrice, underPrice, {} );
	}
	void TickManager::TickWorker::CalculateOptionPrice( uint hClient, const ::Contract& contract, double volatility, double underPrice, ProtoFunction fnctn )noexcept
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
		unique_lock	l{ _twsSubscriptionMutex };
		flat_set<ETickList> requestTicks = GetSubscribedTicks( contractId );

		const flat_set ticks{ ETickList::MiscStats, ETickList::Fundamentals, ETickList::Dividends };
		var handle = ++InternalSubscriptionHandle;
		AddSubscription( contractId, TickListSource{ESubscriptionSource::Internal, handle, ticks} );

		auto p = _ratioSubscriptions.emplace( contractId, make_tuple( std::promise<Tick>{}, Clock::now()+5s, handle ) );
		return get<0>( p->second ).get_future();
	}

	void TickManager::TickWorker::AddSubscription( ContractPK contractId, const TickListSource& source )noexcept
	{
		auto newTicks = 0;

		unique_lock	l{ _twsSubscriptionMutex };
		auto requestTicks = GetSubscribedTicks( contractId );
		for_each( source.Ticks.begin(), source.Ticks.end(), [&newTicks, &requestTicks](auto x){newTicks+=requestTicks.emplace(x).second;} );
		_twsSubscriptions.emplace( contractId, source );
		l.unlock();
		if( newTicks )
		{
			var currentRequestId = requestTicks.size()>source.Ticks.size() ? RequestId( contractId ) : 0;
			var reqId = _pTwsClient->RequestId();
			if( currentRequestId )
				CancelMarketData( currentRequestId, reqId );
			else
			{
				unique_lock l{ _valuesMutex };
				if( auto p=_tickerContractMap.emplace( reqId, contractId ); !p.second )
					p.first->second = contractId;
				_values.try_emplace( contractId, contractId, reqId );
			}


			::Contract contract;  contract.conId = contractId; contract.exchange = "SMART";
			_pTwsClient->reqMktData( reqId, contract, StringUtilities::AddCommas(requestTicks), false, false, {} );//456=dividends - https://interactivebrokers.github.io/tws-api/tick_types.html
		}
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
				_tickerContractMap.erase( p );
				if( auto pValue=_values.find(p->second); pValue!=_values.end() )
					pValue->second.TwsRequestId = newId;
			}
		}
		TwsClientPtr->cancelMktData( oldId );
	}

	void TickManager::TickWorker::Subscribe( uint hClient, ContractPK contractId, const flat_set<ETickList>& fields, bool snapshot, ProtoFunction fnctn )noexcept
	{
		{
			unique_lock l{ _protoSubscriptionMutex };
			_protoSubscriptions.emplace( contractId, make_tuple(hClient,fnctn) );
		}
		AddSubscription( contractId, TickListSource{ESubscriptionSource::Proto, hClient, fields} );
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