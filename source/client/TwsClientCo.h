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
	struct ΓM HistoricalNewsAwaitable final : ITwsAwaitableImpl//<sp<Proto::Results::HistoricalNewsCollection>>
	{
		HistoricalNewsAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		//bool await_ready()noexcept override;TODO cache results...
		α await_suspend( typename base::THandle h )noexcept->void override;
		private:
			function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};
	struct ΓM ContractAwaitable final : ITwsAwaitableImpl//ContractPtr_
	{
		using base = ITwsAwaitableImpl;
		ContractAwaitable( ContractPK id, function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f},_id{id}{}
		bool await_ready()noexcept override;
		α await_suspend( typename base::THandle h )noexcept->void override;
		typename base::TResult await_resume()noexcept override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
		string CacheId()noexcept{ return format("ContractDetails.{}", _id); }
		sp<Contract> _pCache;
		ContractPK _id;
	};
	struct ΓM NewsProviderAwaitable final : ITwsAwaitableImpl//<sp<map<string,string>>
	{
		using base = ITwsAwaitableImpl;
		NewsProviderAwaitable( function<void(sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		bool await_ready()noexcept override;
		α await_suspend( typename base::THandle h )noexcept->void override;
		TaskResult await_resume()noexcept override;
	private:
		string CacheId()noexcept{ return "NewsProviders"; }
		function<void(sp<TwsClient>)> _fnctn;
		sp<map<string,string>> _pCache;
	};
	struct ΓM NewsArticleAwaitable final : ITwsAwaitableImpl
	{
		NewsArticleAwaitable( function<void(ibapi::OrderId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		α await_suspend( typename base::THandle h )noexcept->void override;
	private:
		function<void(ibapi::OrderId, sp<TwsClient>)> _fnctn;
	};

	struct ΓM Tws : TwsClient
	{
		Tws( const TwsConnectionSettings& settings, sp<WrapperCo> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		α AddParam( coroutine_handle<>&& h )noexcept->TickerId;
		α WrapperPtr()noexcept->sp<WrapperCo>;

		Ω HistoricalData( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept{ return HistoryAwait{pContract, end, dayCount, barSize, display, useRth}; }
		Ω HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept->HistoricalNewsAwaitable;
		Ω ContractDetails( ContractPK conId )noexcept->ContractAwaitable;
		Ω ContractDetails( sp<::Contract> c )noexcept->ContractAwaitable;
		Ω NewsProviders()noexcept->NewsProviderAwaitable;
		Ω NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwaitable;
		Ω SecDefOptParams( ContractPK underlyingConId, bool smart=false )noexcept{ return SecDefOptParamAwaitable{underlyingConId, smart}; }
		Ω InstancePtr()noexcept->sp<Tws>{ return dynamic_pointer_cast<Tws>( TwsClient::InstancePtr() ); }
	private:
		friend ITwsAwaitable;
	};
}