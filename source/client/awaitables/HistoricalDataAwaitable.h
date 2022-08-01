#pragma once
#include "../TwsClient.h"
#include "TwsAwaitable.h"
#include <jde/markets/types/Contract.h>
#include "../../types/IBException.h"
#include "../../types/Bar.h"

namespace Jde::Markets
{
	struct HistoryException;
	using ContractPtr_=sp<const Contract>;
	struct ΓM HistoryAwait final : ITwsAwaitShared//vector<::Bar>
	{
		using base = ITwsAwaitShared;
		HistoryAwait( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, SL sl )ι:HistoryAwait{ pContract, end, dayCount, barSize, display, useRth, 0, sl }{}
		α await_ready()ι->bool override;
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override;
		~HistoryAwait();
	private:
		HistoryAwait( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, time_t start, SL sl )ι;
		α Missing()ι->vector<tuple<Day,Day>>;
		α AsyncFetch( HCoroutine h )ι->Task;
		α SetData(bool force=false)ι->bool;
		α SetTwsResults( ibapi::OrderId reqId, const vector<::Bar>& bars )ι->void;

		const Contract _contract;//sp<Contract> was getting lost in SetTwsResults after tws request
		Day _end;
		Day _dayCount;
		time_t _start;
		Proto::Requests::BarSize _barSize;
		TwsDisplay::Enum _display;
		const bool _useRth;
		sp<vector<::Bar>> _pData;
		flat_map<Day,VectorPtr<sp<::Bar>>> _cache;
		HCoroutine _hCoroutine;
		vector<ibapi::OrderId> _twsRequests;
		friend WrapperCo; friend HistoryException;
	};

	struct HistoryException final : IBException
	{
		HistoryException( IException&& ib, const HistoryAwait& x ):IBException{move(ib)},_end{x._end},_dayCount{x._dayCount}, _barSize{x._barSize}, _display{x._display}, _useRth{x._useRth}
		{}
		virtual ~HistoryException(){ Log(); SetLevel( ELogLevel::NoLog ); }
		α IBMessage()Ι->string override{ return format( "({})[{}] - '{}' days={}, {} {} useRth={} - {}", RequestId, (_int)Code, DateDisplay(_end), _dayCount, BarSize::ToString(_barSize), TwsDisplay::ToString(_display), _useRth, what() ); }

		const Day _end;
		const Day _dayCount;
		const Proto::Requests::BarSize _barSize;
		const TwsDisplay::Enum _display;
		const bool _useRth;
	};
}