#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
//#include "../types/proto/bar.pb.h"
#include "boost/container/flat_set.hpp"
#include "../../../Framework/source/Settings.h"
#include <jde/io/File.h>

struct Bar;
namespace Jde::Markets
{
	namespace Proto{ class BarFile; }
	struct Contract;
	struct CandleStick;
	using boost::container::flat_set;
namespace BarData
{
	JDE_MARKETS_EXPORT fs::path Path( const Contract& contract )noexcept(false);
	fs::path Path()noexcept(false);
	inline fs::path File( const Contract& contract, uint16 year, uint8 month, uint8 day, bool complete=true )noexcept{ return BarData::Path(contract)/( IO::FileUtilities::DateFileName(year,month,day)+format("{}.dat.xz", complete ? ""sv : "_partial"sv) ); }
	inline fs::path File( const Contract& contract, DayIndex day )noexcept{ DateTime date{Chrono::FromDays(day)}; return File(contract, date.Year(), date.Month(), date.Day()); }
	JDE_MARKETS_EXPORT flat_set<DayIndex> FindExisting( const Contract& contract, DayIndex start=0, DayIndex end=0, sv prefix={}, map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
	inline bool HavePath()noexcept{ return Settings::Global().Have("barPath"); }
	void ApplySplit( const Contract& contract, uint multiplier )noexcept;

	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> TryLoad( const Contract& contract, DayIndex start, DayIndex end )noexcept;
	JDE_MARKETS_EXPORT MapPtr<DayIndex,VectorPtr<CandleStick>> Load( const Contract& contract, DayIndex start, DayIndex end )noexcept(false);
	MapPtr<DayIndex,VectorPtr<CandleStick>> Load( path path, sv symbol, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
	sp<Proto::BarFile> Load( path path )noexcept(false);
	void ForEachFile( const Contract& contract, const function<void(path,DayIndex, DayIndex)>& fnctn, DayIndex start, DayIndex end, sv prefix=""sv )noexcept;
	JDE_MARKETS_EXPORT void Save( const Contract&, map<DayIndex,vector<sp<Bar>>>& rthBars )noexcept;
	JDE_MARKETS_EXPORT void Save( const Contract& contract, const map<DayIndex,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,TimePoint_>> pExcluded=nullptr, bool checkExisting=false, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false);
}}