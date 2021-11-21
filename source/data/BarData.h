#pragma once
//#include "boost/container/flat_set.hpp"
#include <jde/io/File.h>
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/coroutine/Awaitable.h"

struct Bar;
#define Φ ΓM auto

namespace Jde::Markets
{
	using namespace Coroutine;
	namespace Proto{ class BarFile; }
	struct Contract; using ContractPtr_=sp<const Contract>;
	struct CandleStick;
	// enum class EBarSize : int;
	// enum class EDisplay : int;
	using EBarSize=Proto::Requests::BarSize;
	using EDisplay=Proto::Requests::Display;
}
namespace Jde::Markets::BarData
{
	using FileFunction=function<void(path,Day, Day)>;
	Φ Path( const Contract& contract )noexcept(false)->fs::path;
	α Path()noexcept(false)->fs::path;
	Ξ File( const Contract& contract, uint16 year, uint8 month, uint8 day, bool complete=true )noexcept{ return BarData::Path(contract)/( IO::FileUtilities::DateFileName(year,month,day)+format("{}.dat.xz", complete ? ""sv : "_partial"sv) ); }
	Ξ File( const Contract& contract, Day day )noexcept{ DateTime date{Chrono::FromDays(day)}; return File(contract, date.Year(), date.Month(), date.Day()); }
	Φ FindExisting( const Contract& contract, Day start=0, Day end=0, sv prefix={}, flat_map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->flat_set<Day>;
	Ξ HavePath()noexcept{ return Settings::Global().Have("barPath"); }
	α ApplySplit( const Contract& contract, uint multiplier )noexcept->void;

	α ForEachFile( const Contract& contract, FileFunction fnctn, Day start, Day end, sv prefix=""sv )noexcept->void;

	Φ TryLoad( const Contract& contract, Day start, Day end )noexcept->sp<flat_map<Day,VectorPtr<CandleStick>>>;
	Φ Load( const Contract& contract, Day start, Day end )noexcept(false)->sp<flat_map<Day,VectorPtr<CandleStick>>>;
	Φ CoLoad( ContractPtr_ contract, Day start, Day end )noexcept(false)->AWrapper;//map<Day,VectorPtr<CandleStick>>
	α Load( path path, sv symbol, const flat_map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->sp<flat_map<Day,VectorPtr<CandleStick>>>;
	α Load( fs::path path2, string symbol2 )noexcept->AWrapper;
	α Load( path path )noexcept(false)->sp<Proto::BarFile>;

	Φ Save( const Contract&, flat_map<Day,vector<sp<Bar>>>& rthBars )noexcept->void;
	Φ Save( const Contract& contract, const flat_map<Day,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,optional<TimePoint>>> pExcluded=nullptr, bool checkExisting=false, const flat_map<string,sp<Proto::BarFile>>* pPartials=nullptr )noexcept(false)->void;

	α Combine( const Contract& contract, Day day, vector<sp<Bar>>& fromBars, EBarSize toBarSize, EBarSize fromBarSize=EBarSize::Minute, bool useRth=true )->VectorPtr<sp<::Bar>>;
}
#undef Φ