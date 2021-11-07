#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#include "../types/Exchanges.h"
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )
#include "../../../Framework/source/math/MathUtilities.h"
#include "../../../Framework/source/coroutine/Awaitable.h"

#define Γα JDE_MARKETS_EXPORT auto
struct Bar;
namespace Jde::Markets
{
	
	struct Contract;
	using ContractPtr_=sp<const Contract>;
namespace HistoricalDataCache
{
	Γα ReqHistoricalData( const Contract& contract, DayIndex end, uint dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept->MapPtr<DayIndex,VectorPtr<sp<::Bar>>>;
	Γα ReqHistoricalData2( const Contract& contract, DayIndex end, uint dayCount, Proto::Requests::BarSize barSize, Proto::Requests::Display display, bool useRth )noexcept->flat_map<DayIndex,VectorPtr<sp<::Bar>>>;
	Γα Push( const Contract& contract, Proto::Requests::Display display, Proto::Requests::BarSize barSize, bool useRth, const vector<::Bar>& bars, DayIndex end, DayIndex subDayCount )noexcept->void;

	struct StatCount : public Math::StatResult<double>{ StatCount( Math::StatResult<double> x, uint y ):Math::StatResult<double>{x}, Count{y}{} const uint Count;};
	struct StatAwaitable : IAwaitable
	{
		StatAwaitable( ContractPtr_ pContract, double days, DayIndex start, DayIndex end )noexcept:IAwaitable{"ReqStats"}, ContractPtr{pContract}, Days{days}, Start{start}, End{end}{}
		Γα await_ready()noexcept->bool override;
		Γα await_suspend( HCoroutine h )noexcept->void override;
		Γα await_resume()noexcept->Task2::TResult override;
	private:
		α Count()const noexcept->DayIndex;
		ContractPtr_ ContractPtr; double Days; DayIndex Start, End, FullDays; uint16 Minutes; string CacheId;
		std::variant<sp<HistoricalDataCache::StatCount>,std::exception_ptr> _result;
	};
	Ξ ReqStats( ContractPtr_ pContract, double days, DayIndex start, DayIndex end=CurrentTradingDay() )noexcept{ return StatAwaitable{ pContract, days, start, end }; }
}}
#undef Γα