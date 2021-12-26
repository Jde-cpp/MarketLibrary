#pragma once
#include <jde/markets/Exports.h>
#include <jde/markets/types/Contract.h>
#include <jde/coroutine/Task.h>
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "awaitables/TwsAwaitable.h"
#include "awaitables/HistoricalDataAwaitable.h"
#include "../data/Accounts.h"
#include "../types/Bar.h"
#include "TwsClient.h"

namespace Jde::Markets
{
	//#define base ITwsAwaitUnique
	struct ΓM HistoricalNewsAwait final : ITwsAwaitUnique//<sp<Proto::Results::HistoricalNewsCollection>>
	{
		HistoricalNewsAwait( function<void(ReqId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		α await_suspend( HCoroutine h )noexcept->void override;
	private:
		function<void(ReqId, sp<TwsClient>)> _fnctn;
	};

	struct ΓM ContractAwait final : ITwsAwaitShared//::ContractDetails
	{
		ContractAwait( ContractPK id, function<void(ReqId, sp<TwsClient>)> f, bool single=true )noexcept:_fnctn{f},_id{id},_single{single}{}
		ContractAwait( function<void(ReqId, sp<TwsClient>)> f, bool single=true )noexcept:ContractAwait{ 0, f, single }{}
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
	private:
		α CacheId()noexcept->string{ return format( Contract::CacheDetailsFormat, _id ); }
		α ToString()noexcept->string{ return _id ? std::to_string( _id ) : "Symbol query"; }
		function<void(ReqId, sp<TwsClient>)> _fnctn;
		sp<::ContractDetails> _pCache;
		const ContractPK _id;
		const bool _single;
	};
	struct ΓM NewsProviderAwait final : ITwsAwaitShared//<sp<map<string,string>>
	{
		NewsProviderAwait( function<void(sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		bool await_ready()noexcept override;
		α await_suspend( HCoroutine h )noexcept->void override;
		AwaitResult await_resume()noexcept override;
	private:
		string CacheId()noexcept{ return "NewsProviders"; }
		function<void(sp<TwsClient>)> _fnctn;
		sp<map<string,string>> _pCache;
	};
	struct ΓM NewsArticleAwait final : ITwsAwaitUnique
	{
		NewsArticleAwait( function<void(ReqId, sp<TwsClient>)> f )noexcept:_fnctn{f}{}
		α await_suspend( HCoroutine h )noexcept->void override;
	private:
		function<void(ReqId, sp<TwsClient>)> _fnctn;
	};
#undef base
	struct ΓM Tws : TwsClient
	{
		Tws( const TwsConnectionSettings& settings, sp<WrapperCo> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )noexcept(false);
		α AddParam( coroutine_handle<>&& h )noexcept->TickerId;
		α WrapperPtr()noexcept->sp<WrapperCo>;

		Ω HistoricalData( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept{ return HistoryAwait{pContract, end, dayCount, barSize, display, useRth}; }
		Ω HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )noexcept->HistoricalNewsAwait;
		Ω ContractDetail( ContractPK conId )noexcept->ContractAwait;
		Ω ContractDetail( const ::Contract& c )noexcept->ContractAwait;
		Ω ContractDetails( const ::Contract& c )noexcept->ContractAwait;
		Ω NewsProviders()noexcept->NewsProviderAwait;
		Ω NewsArticle( str providerCode, str articleId )noexcept->NewsArticleAwait;
		Ω SecDefOptParams( ContractPK underlyingConId, bool smart=false )noexcept{ return SecDefOptParamAwait{underlyingConId, smart}; }
		Ω InstancePtr()noexcept->sp<Tws>{ return dynamic_pointer_cast<Tws>( TwsClient::InstancePtr() ); }
		Ω RequestAllOpenOrders()noexcept{ return AllOpenOrdersAwait{}; }
		Ω ReqManagedAccts()noexcept{ return AccountsAwait{}; }
	private:
		friend ITwsAwait;
	};
}