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
	inline fs::path File( const Contract& contract, uint16 year, uint8 month, uint8 day, bool complete=true )noexcept{ return BarData::Path(contract)/( IO::FileUtilities::DateFileName(year,month,day)+fmt::format("{}.dat.xz", complete ? ""sv : "_partial"sv) ); }
	inline fs::path File( const Contract& contract, DayIndex day )noexcept{ DateTime date{FromDays(day)}; return File(contract, date.Year(), date.Month(), date.Day()); }

	inline bool HavePath()noexcept(false){ return Settings::Global().Have("barPath"); }

	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> TryLoad( const Contract& contract, DayIndex start, DayIndex end )noexcept;
	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> Load( const Contract& contract, DayIndex start, DayIndex end )noexcept(false);
	MapPtr<DayIndex,VectorPtr<CandleStick>> Load( const fs::path& path, string_view symbol, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
	sp<Proto::BarFile> Load( const fs::path& path )noexcept(false);
	void ForEachFile( const Contract& contract, const function<void(const fs::path&,DayIndex, DayIndex)>& fnctn, DayIndex start, DayIndex end, string_view prefix=""sv )noexcept;
	JDE_MARKETS_EXPORT void Save( const Contract&, map<DayIndex,vector<sp<ibapi::Bar>>>& rthBars )noexcept;
	JDE_MARKETS_EXPORT void Save( const Contract& contract, const map<DayIndex,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,TimePoint_>> pExcluded=nullptr, bool checkExisting=false, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
}}