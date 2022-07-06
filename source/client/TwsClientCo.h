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
	struct ΓM HistoricalNewsAwait final : ITwsAwaitUnique//<sp<Proto::Results::HistoricalNewsCollection>>
	{
		HistoricalNewsAwait( function<void(ReqId, sp<TwsClient>)> f, SRCE )ι:ITwsAwaitUnique{sl},_fnctn{f}{}
		α await_suspend( HCoroutine h )ι->void override;
	private:
		function<void(ReqId, sp<TwsClient>)> _fnctn;
	};
#define Base ITwsAwaitShared
	struct ΓM ContractAwait final : Base
	{
		using base=Base;
		ContractAwait( ContractPK id, function<void(ReqId, sp<TwsClient>)> f, bool single=true, SRCE )ι:Base{sl}, _fnctn{f},_id{id},_single{single}{}
		ContractAwait( function<void(ReqId, sp<TwsClient>)> f, bool single=true, SRCE )ι:ContractAwait{ 0, f, single, sl }{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
	private:
		α CacheId()ι->string{ return format( Contract::CacheDetailsFormat, _id ); }
		α ToString()ι->string{ return _id ? std::to_string( _id ) : "Symbol query"; }
		function<void(ReqId, sp<TwsClient>)> _fnctn;
		sp<::ContractDetails> _pCache;
		const ContractPK _id;
		const bool _single;
	};
	struct ΓM NewsProviderAwait final : Base//<sp<map<string,string>>
	{
		using base=Base;
		NewsProviderAwait( function<void(sp<TwsClient>)> f )ι:_fnctn{f}{}
		bool await_ready()ι override;
		α await_suspend( HCoroutine h )ι->void override;
		AwaitResult await_resume()ι override;
	private:
		string CacheId()ι{ return "NewsProviders"; }
		function<void(sp<TwsClient>)> _fnctn;
		sp<map<string,string>> _pCache;
	};
	struct ΓM NewsArticleAwait final : ITwsAwaitUnique
	{
		NewsArticleAwait( function<void(ReqId, sp<TwsClient>)> f, SRCE )ι:ITwsAwaitUnique{sl}, _fnctn{f}{}
		α await_suspend( HCoroutine h )ι->void override;
	private:
		function<void(ReqId, sp<TwsClient>)> _fnctn;
	};
#undef base
	struct ΓM Tws : TwsClient
	{
		Tws( const TwsConnectionSettings& settings, sp<WrapperCo> wrapper, sp<EReaderSignal>& pReaderSignal, uint clientId )ε;
		Ω InstancePtr()ι->sp<Tws>{ return dynamic_pointer_cast<Tws>( TwsClient::InstancePtr() ); }

		α AddParam( coroutine_handle<>&& h )ι->TickerId;
		α WrapperPtr()ι->sp<WrapperCo>;

		Ω HistoricalData( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, SRCE )ι{ return HistoryAwait{pContract, end, dayCount, barSize, display, useRth, sl}; }
		Ω HistoricalNews( ContractPK conId, const vector<string>& providerCodes, uint totalResults, TimePoint start={}, TimePoint end={} )ι->HistoricalNewsAwait;
		Ω ContractDetail( ContractPK conId )ι->ContractAwait;
		Ω ContractDetail( const ::Contract& c )ι->ContractAwait;
		Ω ContractDetails( const ::Contract& c )ι->ContractAwait;
		Ω NewsProviders()ι->NewsProviderAwait;
		Ω NewsArticle( str providerCode, str articleId )ι->NewsArticleAwait;
		Ω SecDefOptParams( ContractPK underlyingConId, bool smart=false )ι{ return SecDefOptParamAwait{underlyingConId, smart}; }
		Ω PlaceOrder( sp<::Contract> c, ::Order o, string blockId, double stop=0, double stopLimit=0, SRCE )ι{ return PlaceOrderAwait{ c, move(o), move(blockId), stop, stopLimit, sl }; }
		Ω RequestAllOpenOrders( SRCE )ι{ return AllOpenOrdersAwait{ sl }; }
		Ω ReqManagedAccts()ι{ return AccountsAwait{}; }
		Ω HeadTimestamp( sp<::Contract> c, sv whatToShow )ι{ return HeadTimestampAwait{ c, whatToShow }; }

	private:
		friend ITwsAwait;
	};
}