#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#include "boost/container/flat_set.hpp"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/coroutine/Awaitable.h"
#include <jde/io/File.h>

struct Bar;
#define 🚪 JDE_MARKETS_EXPORT auto

namespace Jde::Markets
{
	using namespace Coroutine;
	namespace Proto{ class BarFile; }
	struct Contract;
	struct CandleStick;
	using boost::container::flat_set;
namespace BarData
{
	🚪 Path( const Contract& contract )noexcept(false)->fs::path;
	fs::path Path()noexcept(false);
	inline fs::path File( const Contract& contract, uint16 year, uint8 month, uint8 day, bool complete=true )noexcept{ return BarData::Path(contract)/( IO::FileUtilities::DateFileName(year,month,day)+format("{}.dat.xz", complete ? ""sv : "_partial"sv) ); }
	inline fs::path File( const Contract& contract, DayIndex day )noexcept{ DateTime date{Chrono::FromDays(day)}; return File(contract, date.Year(), date.Month(), date.Day()); }
	🚪 FindExisting( const Contract& contract, DayIndex start=0, DayIndex end=0, sv prefix={}, map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->flat_set<DayIndex>;
	inline bool HavePath()noexcept{ return Settings::Global().Have("barPath"); }
	void ApplySplit( const Contract& contract, uint multiplier )noexcept;

	🚪 TryLoad( const Contract& contract, DayIndex start, DayIndex end )noexcept->MapPtr<DayIndex,VectorPtr<CandleStick>>;
	🚪 Load( const Contract& contract, DayIndex start, DayIndex end )noexcept(false)->MapPtr<DayIndex,VectorPtr<CandleStick>>;
	🚪 CoLoad( const Contract& contract, DayIndex start, DayIndex end )noexcept(false)->AWrapper;//map<DayIndex,VectorPtr<CandleStick>>
	α Load( path path, sv symbol, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->MapPtr<DayIndex,VectorPtr<CandleStick>>;
	α Load( fs::path path2, string symbol2 )noexcept->AWrapper;
	sp<Proto::BarFile> Load( path path )noexcept(false);
	void ForEachFile( const Contract& contract, const function<void(path,DayIndex, DayIndex)>& fnctn, DayIndex start, DayIndex end, sv prefix=""sv )noexcept;
	🚪 Save( const Contract&, flat_map<DayIndex,vector<sp<Bar>>>& rthBars )noexcept->void;
	🚪 Save( const Contract& contract, const map<DayIndex,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,TimePoint_>> pExcluded=nullptr, bool checkExisting=false, const map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->void;
}}
#undef 🚪