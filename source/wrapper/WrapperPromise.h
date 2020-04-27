
namespace Jde::Markets
{
	template<typename T>
	struct WrapperPromise
	{
		typedef std::promise<sp<T>> PromiseType;
		typedef std::future<sp<T>> Future;

		//virtual void Push( ReqId id, const T& value )noexcept=0;
		bool Contains( ReqId id )const noexcept{ shared_lock l{_promiseMutex}; return _promises.find(id)!=_promises.end(); }
		bool Error( ReqId id, const Exception& e )noexcept;
		Future Promise( ReqId id )noexcept;
		virtual void End( ReqId reqId )noexcept;
	protected:
		map<ReqId,PromiseType> _promises; mutable shared_mutex _promiseMutex;
	};
	template<typename T>
	struct WrapperData : WrapperPromise<list<T>>
	{
		typedef list<T> Collection;
		typedef WrapperPromise<list<T>> Base;
		void Push( ReqId id, const T& value )noexcept;
		void End( ReqId id )noexcept;
	private:
		map<ReqId,sp<Collection>> _data; mutable mutex _dataMutex;
	};

	template<typename T>
	struct WrapperItem : WrapperPromise<T>
	{
		typedef WrapperPromise<T> Base;
		void Push( ReqId id, sp<T> pValue )noexcept;
	};

#define var const auto

	template<typename T>
	typename WrapperPromise<T>::Future WrapperPromise<T>::Promise( ReqId id )noexcept
	{
		unique_lock l{_promiseMutex};
		return _promises.emplace_hint( _promises.end(), id, PromiseType{} )->second.get_future();
	}
	template<typename T>
	bool WrapperPromise<T>::Error( ReqId reqId, const Exception& e )noexcept
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
			WARN( "Could not find _requestCallbacks={}"sv, reqId );
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
			auto pContracts = _data.try_emplace( _data.end(), reqId, sp<list<T>>{ new list<T>{} } )->second;
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