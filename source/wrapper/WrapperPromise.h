#include <future>
#include "../../../Framework/source/Cache.h"
#include "../types/IBException.h"

namespace Jde::Markets
{
	struct IBException;
	template<typename T>
	struct WrapperPromise
	{
		typedef std::promise<sp<T>> PromiseType;
		typedef std::future<sp<T>> Future;

		bool Contains( ReqId id )const noexcept{ shared_lock l{_promiseMutex}; return _promises.find(id)!=_promises.end(); }
		template<typename E>
		bool Error( ReqId id, const E& e )noexcept;
		Future Promise( ReqId id, Duration timeout )noexcept;
		virtual void End( ReqId reqId )noexcept;
		flat_map<ReqId,TimePoint> _timeouts; mutable shared_mutex _timeoutMutex;
	protected:
		flat_map<ReqId,PromiseType> _promises; mutable shared_mutex _promiseMutex;
	};

	template<typename T>
	struct WrapperItem final : WrapperPromise<T>
	{
		typedef WrapperPromise<T> Base;
		void Push( ReqId id, sp<T> pValue )noexcept;
	};


	template<typename T>
	struct WrapperData final : public WrapperPromise<vector<T>>
	{
		typedef vector<T> Collection;
		typedef WrapperPromise<Collection> Base;
		bool Error( ReqId id, const IBException& e )noexcept;

		void Push( ReqId id, const T& value )noexcept;
		void End( ReqId id )noexcept;
		bool End( const VectorPtr<T>& value );
		void CheckTimeouts()noexcept;
		flat_map<ReqId,TimePoint> _timeouts; mutable shared_mutex _timeoutMutex;
	protected:
		flat_map<ReqId,sp<Collection>> _data; mutable mutex _dataMutex;
	};

#define var const auto

	template<typename T>
	typename WrapperPromise<T>::Future WrapperPromise<T>::Promise( ReqId id, Duration timeout )noexcept
	{
		if( timeout!=Duration::zero() )
		{
			std::unique_lock l{_timeoutMutex};
			_timeouts.emplace( id, Clock::now()+timeout );
		}
		unique_lock l{_promiseMutex};
		return _promises.emplace_hint( _promises.end(), id, PromiseType{} )->second.get_future();
	}

	template<typename T>
	bool WrapperData<T>::Error( ReqId reqId, const IBException& e )noexcept
	{
		{
			lock_guard<std::mutex> l{ _dataMutex };
			if( var pIdBars = _data.find( reqId ); pIdBars!=_data.end() )
			{
				if( pIdBars->second->size()>0 )
					DBG( "({}) - Error with data. {}"sv, reqId,  pIdBars->second->size() );
				_data.erase( pIdBars );
			}
		}
		{
			std::unique_lock l{ _timeoutMutex };
			_timeouts.erase( reqId );
		}
		return Base::Error( reqId, e );
	}

	template<typename T>
	void WrapperData<T>::CheckTimeouts()noexcept
	{
		TickerId reqId = 0;
		{
			std::unique_lock l{ _timeoutMutex };
			for( auto p=_timeouts.begin(); p!=_timeouts.end() && !reqId; ++p )
			{
				//DBG( "timeout={} Clock::now()={}"sv, ToIsoString(timeout), ToIsoString(Clock::now()) );
				if( Clock::now()>p->second )
					reqId = p->first;
			}
		}
		if( reqId )
		{
			DBG( "({}) - Timeout"sv, reqId );
			Error( reqId, IBException{"Timeout", -2, reqId} );
		}
	}

	template<typename T>
	template<typename E>
	bool WrapperPromise<T>::Error( ReqId reqId, const E& e )noexcept
	{
		unique_lock l{ _promiseMutex };
		var pValue = _promises.find( reqId );
		var found = pValue!=_promises.end();
		if( found )
		{
			e.Log();
			auto& promise = pValue->second;
			promise.set_exception( std::make_exception_ptr(e) );
			DBG( "({}) erase exception"sv, reqId );
			_promises.erase( pValue );
		}
		return found;
	}
	template<typename T>
	void WrapperPromise<T>::End( ReqId reqId )noexcept
	{
		unique_lock lck{_promiseMutex};
		auto pPromise = _promises.find( reqId );
		if( pPromise!=_promises.end() )
		{
			//DBG( "({}) erase End"sv, reqId );
			_promises.erase( pPromise );
		}
		else
			WARN( "Could not find _promises={}"sv, reqId );
	}
	template<typename T>
	void WrapperData<T>::End( ReqId reqId )noexcept
	{
		{
			lock_guard<std::mutex> lck2{ _dataMutex };
			var pIdBars = _data.find( reqId );
			shared_lock lck{ Base::_promiseMutex };
			auto pPromise = Base::_promises.find( reqId );
			if( pPromise==Base::_promises.end() )
				ERR( "({})Could not promise"sv, reqId );
			else
			{
				var haveData = pIdBars!=_data.end();
				pPromise->second.set_value( haveData ? pIdBars->second : make_shared<Collection>() );
				if( haveData )
					_data.erase( pIdBars );
			}
			unique_lock l{ _timeoutMutex };
			_timeouts.erase( reqId );
		}
		Base::End( reqId );
	}
	template<typename T>
	bool WrapperData<T>::End( const VectorPtr<T>& value )
	{
		//unique_lock lck2{ _dataMutex };
		unique_lock lck{ Base::_promiseMutex };
		var havePromises = Base::_promises.size()>0;
		for( auto& promise : Base::_promises )
			promise.second.set_value( value );
		Base::_promises.clear();
		return havePromises;
	}
	template<typename T>
	void WrapperData<T>::Push( ReqId reqId, const T& value )noexcept
	{
		{
			unique_lock<std::mutex> lck{ _dataMutex };
			auto pContracts = _data.try_emplace( _data.end(), reqId, sp<vector<T>>{new vector<T>{}} )->second;
			pContracts->push_back( value );
		}
	}

	template<typename T>
	void WrapperItem<T>::Push( ReqId id, sp<T> pValue )noexcept
	{
		{
			unique_lock l{Base::_promiseMutex};
			auto pPromise = Base::_promises.find( id );
			if( pPromise!=Base::_promises.end() )
				pPromise->second.set_value( pValue );
			else
				WARN( "Could not find WrapperItem::promises_requestCallbacks={}"sv, id );
		}
		Base::End( id );
	}
}
#undef var