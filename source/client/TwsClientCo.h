#pragma once
#include <jde/markets/Exports.h>
#include <jde/markets/types/Contract.h>
#include <jde/coroutine/Task.h>
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "awaitables/TwsAwaitable.h"
#include "awaitables/HistoricalDataAwaitable.h"
#include "../types/Bar.h"
#include "TwsClient.h"

namespace Jde::Markets
{
	struct JDE_MARKETS_EXPORT HistoricalNewsAwaitable final : ITwsAwaitableImpl//<sp<Proto::Results::HistoricalNewsCollection>>
	{
		HistoricalNewsAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		//bool await_ready()noexcept override;TODO cache results...
		void await_suspend( typename base::THandle h )noexcept override;
		private:
			function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};
	struct JDE_MARKETS_EXPORT ContractAwaitable final : ITwsAwaitableImpl//<sp<Contract>>
	{
		using base = ITwsAwaitableImpl;
		ContractAwaitable( ContractPK id, function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f},_id{id}{}
		bool await_ready()noexcept override;
		void await_suspend( typename base::THandle h )noexcept override;
		typename base::TResult await_resume()noexcept override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
		string CacheId()noexcept{ return format("ContractDetails.{}", _id); }
		sp<Contract> _pCache;
		ContractPK _id;
	};
	struct JDE_MARKETS_EXPORT NewsProviderAwaitable final : ITwsAwaitableImpl//<sp<map<string,string>>
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
	struct JDE_MARKETS_EXPORT NewsArticleAwaitable final : ITwsAwaitableImpl
	{
		NewsArticleAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		void await_suspend( typename base::THandle h )noexcept override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};

	struct JDE_MARKETS_EXPORT TwsClientCo : TwsClient
	{
		TwsClientCo( const TwsConnectionSettings& settings, shared_ptr<WrapperCo> wrapper, shared_ptr<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		TickerId AddParam( coroutine_handle<>&& h )noexcept;
		sp<WrapperCo> WrapperPtr()noexcept;

		Ω HistoricalData( sp<Contract> pContract, DayIndex end, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept{ return HistoricalDataAwaitable{pContract, end, dayCount, barSize, display, useRth}; }
		Ω HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept->HistoricalNewsAwaitable;
		Ω ContractDetails( ContractPK conId )noexcept->ContractAwaitable;
		Ω NewsProviders()noexcept->NewsProviderAwaitable;
		Ω NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwaitable;
		Ω SecDefOptParams( ContractPK underlyingConId, bool smart=false )noexcept{ return SecDefOptParamAwaitable{underlyingConId, smart}; }
		Ω InstancePtr()noexcept->sp<TwsClientCo>{ return dynamic_pointer_cast<TwsClientCo>( TwsClient::InstancePtr() ); }
	private:
		friend ITwsAwaitable;
	};
}