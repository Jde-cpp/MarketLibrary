#pragma once
#include <jde/markets/Exports.h>
#include <jde/markets/types/Contract.h>
#include <jde/coroutine/Task.h>
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "TwsClient.h"

namespace Jde::Markets
{
	using namespace Jde::Coroutine;
	struct WrapperCo; struct TwsClientCo;

	struct ITwsAwaitable
	{
		ITwsAwaitable()noexcept;
	protected:
		sp<WrapperCo> WrapperPtr()noexcept;
		sp<TwsClientCo> _pTws;
	};
	//template<class T>
	struct ITwsAwaitableImpl : ITwsAwaitable, IAwaitable<Task2>
	{
		using base=IAwaitable<Task2>;
		bool await_ready()noexcept override{ return !_pTws; }
		void await_suspend( typename base::THandle h )noexcept override{ base::await_suspend( h ); _pPromise = &h.promise(); };
		typename base::TResult await_resume()noexcept override{ base::AwaitResume(); return move(_pPromise->get_return_object().Result); }
	protected:
		typename base::TPromise* _pPromise{ nullptr };
	};

	struct HistoricalNewsAwaitable final : ITwsAwaitableImpl//<sp<Proto::Results::HistoricalNewsCollection>>
	{
		HistoricalNewsAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		//bool await_ready()noexcept override;TODO cache results...
		void await_suspend( typename base::THandle h )noexcept override;
		private:
			function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};
	struct ContractAwaitable final : ITwsAwaitableImpl//<sp<Contract>>
	{
		using base = ITwsAwaitableImpl;//<sp<Contract>>;
		ContractAwaitable( ContractPK id, function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		bool await_ready()noexcept override;
		void await_suspend( typename base::THandle h )noexcept override;
		typename base::TResult await_resume()noexcept override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
		string CacheId()noexcept{ return format("ContractDetails.{}", _id); }
		sp<Contract> _pCache;
		ContractPK _id;
	};
	struct NewsProviderAwaitable final : ITwsAwaitableImpl//<sp<map<string,string>>
	{
		using base = ITwsAwaitableImpl;
		NewsProviderAwaitable( function<void(sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		bool await_ready()noexcept override;
		void await_suspend( typename base::THandle h )noexcept override;
		TaskResult await_resume()noexcept override;
	private:
		string CacheId()noexcept{ return "NewsProviders"; }
		function<void(sp<TwsClient>)> _fnctn;
		sp<map<string,string>> _pCache;
	};
	struct NewsArticleAwaitable final : ITwsAwaitableImpl
	{
		NewsArticleAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		void await_suspend( typename base::THandle h )noexcept override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};

	struct JDE_MARKETS_EXPORT TwsClientCo : public TwsClient
	{
		TwsClientCo( const TwsConnectionSettings& settings, shared_ptr<WrapperCo> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		TickerId AddParam( coroutine_handle<>&& h )noexcept;
		sp<WrapperCo> WrapperPtr()noexcept;

		Ω HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept->HistoricalNewsAwaitable;
		Ω ContractDetails( ContractPK conId )noexcept->ContractAwaitable;
		Ω NewsProviders()noexcept->NewsProviderAwaitable;
		Ω NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwaitable;


		Ω InstancePtr()noexcept->sp<TwsClientCo>{ return dynamic_pointer_cast<TwsClientCo>( TwsClient::InstancePtr() ); }
	private:
		friend ITwsAwaitable;
	};
}