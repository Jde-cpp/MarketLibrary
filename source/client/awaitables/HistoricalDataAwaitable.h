#pragma once
#include "../TwsClient.h"
#include "TwsAwaitable.h"
#include "../../types/Bar.h"

namespace Jde::Markets
{
	using ContractPtr_=sp<const Contract>;
	struct ΓM HistoryAwait final : ITwsAwaitShared//vector<::Bar>
	{
		using base = ITwsAwaitShared;
		HistoryAwait( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, SL sl )noexcept:HistoryAwait{ pContract, end, dayCount, barSize, display, useRth, 0, sl }{}
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override{ return _dataPtr ? AwaitResult{ static_pointer_cast<void>(_dataPtr) } : base::await_resume(); }

	private:
		HistoryAwait( ContractPtr_ pContract, Day end, Day dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, time_t start, SL sl )noexcept:
			ITwsAwaitShared{ sl },
			_pContract{ pContract }, _end{ end }, _dayCount{ dayCount }, _start{ start }, _barSize{ barSize }, _display{ display }, _useRth{ useRth }
		{}
		α Missing()noexcept->vector<tuple<Day,Day>>;
		α AsyncFetch( HCoroutine h )noexcept->Task;
		α SetData(bool force=false)noexcept->bool;
		α SetTwsResults( ibapi::OrderId reqId, const vector<::Bar>& bars )->void;

		ContractPtr_ _pContract;
		Day _end;
		Day _dayCount;
		time_t _start;
		Proto::Requests::BarSize _barSize;
		TwsDisplay::Enum _display;
		const bool _useRth;
		sp<vector<::Bar>> _dataPtr;
		flat_map<Day,VectorPtr<sp<::Bar>>> _cache;
		HCoroutine _hCoroutine;
		vector<ibapi::OrderId> _twsRequests;
		friend WrapperCo;
	};
}