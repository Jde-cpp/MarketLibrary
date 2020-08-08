#include <future>
#include "../../../Framework/source/Cache.h"

namespace Jde::Markets
{
	template<typename T>
	struct WrapperPromise
	{
		typedef std::promise<sp<T>> PromiseType;
		typedef std::future<sp<T>> Future;

		bool Contains( ReqId id )const noexcept{ shared_lock l{_promiseMutex}; return _promises.find(id)!=_promises.end(); }
		template<typename E>
		bool Error( ReqId id, const E& e )noexcept;
		Future Promise( ReqId id )noexcept;
		virtual void End( ReqId reqId )noexcept;
	protected:
		map<ReqId,PromiseType> _promises; mutable shared_mutex _promiseMutex;
	};

	template<typename T>
	struct WrapperItem : WrapperPromise<T>
	{
		typedef WrapperPromise<T> Base;
		void Push( ReqId id, sp<T> pValue )noexcept;
	};


	template<typename T>
	struct WrapperData : public WrapperPromise<vector<T>>
	{
		typedef vector<T> Collection;
		typedef WrapperPromise<Collection> Base;
		void Push( ReqId id, const T& value )noexcept;
		void End( ReqId id )noexcept;
	protected:
		map<ReqId,sp<Collection>> _data; mutable mutex _dataMutex;
	};

/*	template<typename T>
	struct WrapperCache final : public WrapperData<T>
	{
		typedef WrapperData<T> Data;
		typedef typename Data::Base Base;
		typedef std::future<sp<vector<T>>> Future;
		std::tuple<Future,bool> Promise( ReqId id, const string& cacheId )noexcept;
		sp<vector<T>> End2( ReqId reqId )noexcept;
		bool Has( ReqId id )const noexcept{ return _cacheIds.find(id)!=_cacheIds.end(); }
	private:
		map<ReqId,string> _cacheIds;
	};
*/
#define var const auto

	template<typename T>
	typename WrapperPromise<T>::Future WrapperPromise<T>::Promise( ReqId id )noexcept
	{
		unique_lock l{_promiseMutex};
		return _promises.emplace_hint( _promises.end(), id, PromiseType{} )->second.get_future();
	}


/*	template<typename T>
	tuple<std::future<sp<vector<T>>>,bool> WrapperCache<T>::Promise( ReqId id, const string& cacheId )noexcept
	{
		unique_lock l{Base::_promiseMutex};
		auto& promise = Base::_promises.emplace_hint( Base::_promises.end(), id, std::promise<sp<vector<T>>>{} )->second;
		bool set = false;
		if( cacheId.size() )
		{
			set = Cache::Has( cacheId );
			if( set )
				promise.set_value( Cache::Get<vector<T>>(cacheId) );
			else
				_cacheIds.emplace( id, cacheId );
		}
		return make_tuple( promise.get_future(), set );
	}

	template<typename T>
	sp<vector<T>> WrapperCache<T>::End2( ReqId reqId )noexcept
	{
		var pIdData = Data::_data.find( reqId );
		var pCacheId = _cacheIds.find( reqId );
		sp<vector<T>> pResult;
		if( pCacheId!=_cacheIds.end() )
		{
			if( pIdData!=Data::_data.end() )
				Cache::Set( pCacheId->second, pResult = pIdData->second );
			else
				pResult = Cache::Get<vector<T>>( pCacheId->second );
			_cacheIds.erase( pCacheId );
		}
		WrapperData<T>::End( reqId );
		return pResult;
	}
*/
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
			_promises.erase( pPromise );
		else
			WARN( "Could not find _promises={}"sv, reqId );
	}
	template<typename T>
	void WrapperData<T>::End( ReqId reqId )noexcept
	{
		lock_guard<std::mutex> lck2{ _dataMutex };
		var pIdBars = _data.find( reqId );
		auto pPromise = Base::_promises.find( reqId );
		if( pIdBars==_data.end() || pPromise==Base::_promises.end() )
			DBG( "Could not find reqId={} in Data"sv, reqId );
		else
		{
			auto& promise = pPromise->second;
			var pBars = pIdBars->second;
			promise.set_value( pBars );
			//LOG( WrapperHistorian::Instance().GetLogLevel(), "End( {}, count={} )", reqId, _data.size() );
			_data.erase( pIdBars );
		}
		Base::End( reqId );
	}
	template<typename T>
	void WrapperData<T>::Push( ReqId reqId, const T& value )noexcept
	{
		//shared_lock l{ _promiseMutex };
		//if( _callbacks.find( reqId )!=_callbacks.end() )//make sure it is sync?
		{
		//	l.unlock();
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