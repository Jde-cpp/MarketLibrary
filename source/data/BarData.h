#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#include "../types/proto/bar.pb.h"
#include "../../../Framework/source/Settings.h"

namespace Jde::Markets
{
	struct Contract;
	struct CandleStick;
namespace BarData
{
	JDE_MARKETS_EXPORT fs::path Path( const Contract& contract )noexcept(false);
	inline fs::path Path()noexcept(false){ return Settings::Global().Get<fs::path>("barPath"); }
	inline bool HavePath()noexcept(false){ return Settings::Global().Have("barPath"); }

	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> TryReqHistoricalData( const Contract& contract, DayIndex start, DayIndex end )noexcept;
	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> ReqHistoricalData( const Contract& contract, DayIndex start, DayIndex end )noexcept(false);
	JDE_MARKETS_EXPORT void Save( const Contract&, map<DayIndex,vector<sp<ibapi::Bar>>>& rthBars )noexcept;
	JDE_MARKETS_EXPORT void Save( const Contract& contract, const map<DayIndex,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,TimePoint_>> pExcluded=nullptr, bool checkExisting=false, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
}}