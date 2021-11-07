#pragma once
#include "../TwsClient.h"
#include "TwsAwaitable.h"
#include "../../types/Bar.h"

namespace Jde::Markets
{
	using ContractPtr_=sp<const Contract>;
	struct JDE_MARKETS_EXPORT HistoricalDataAwaitable final : ITwsAwaitableImpl//sp<vector<::Bar>>
	{
		using base = ITwsAwaitableImpl;
		HistoricalDataAwaitable( ContractPtr_ pContract, DayIndex end, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )noexcept:HistoricalDataAwaitable{ pContract, end, dayCount, barSize, display, useRth, 0 }{}
		bool await_ready()noexcept override;
		void await_suspend( HCoroutine h )noexcept override;
		TaskResult await_resume()noexcept override;
		α AddTws( ibapi::OrderId reqId, const vector<::Bar>& bars )->void;
	private:
		HistoricalDataAwaitable( ContractPtr_ pContract, DayIndex end, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth, time_t start )noexcept:
			_contractPtr{ pContract }, _end{ end }, _dayCount{ dayCount }, _start{ start }, _barSize{ barSize }, _display{ display }, _useRth{ useRth }
		{}
		α Missing()noexcept->vector<tuple<DayIndex,DayIndex>>;
		α AsyncFetch( HCoroutine h )noexcept->Task2;
		bool SetData(bool force=false)noexcept;
		ContractPtr_ _contractPtr;
		DayIndex _end;
		DayIndex _dayCount;
		time_t _start;
		Proto::Requests::BarSize _barSize;
		TwsDisplay::Enum _display;
		const bool _useRth;
		sp<vector<::Bar>> _dataPtr;
		flat_map<DayIndex,VectorPtr<sp<::Bar>>> _cache;
		HCoroutine _hCoroutine;
		vector<ibapi::OrderId> _twsRequests;
		friend WrapperCo;
	};
}