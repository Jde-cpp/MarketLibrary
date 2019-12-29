#pragma once
#include "../Exports.h"
#include "../TypeDefs.h"
//#include "DateTime.h"

namespace Jde::Markets
{
	enum class Exchanges : uint
	{
		None   = 0,
		Nyse   = 1,
		Nasdaq = 2,
		Amex   =0x20,
		Smart  =0x20000000,//30
		Arca   =0x40000000,
		Bats   =0x200000000
	};

	JDE_MARKETS_EXPORT string to_string( Exchanges exchange )noexcept;
	Exchanges ToExchange( string_view pszName )noexcept;
	JDE_MARKETS_EXPORT TimePoint PreviousTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT TimePoint NextTradingDay( const TimePoint& time )noexcept;
	inline DayIndex PreviousTradingDay( DayIndex day )noexcept{ return Chrono::DaysSinceEpoch( PreviousTradingDay(Chrono::FromDays(day)) ); }
	inline DayIndex NextTradingDay( DayIndex day )noexcept{ return Chrono::DaysSinceEpoch( NextTradingDay(Chrono::FromDays(day)) ); }

	inline DayIndex CurrentTradingDay( DayIndex day )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }//weekend=monday, monday=monday.
	inline DayIndex CurrentTradingDay()noexcept{ return NextTradingDay( PreviousTradingDay( Chrono::DaysSinceEpoch(Timezone::EasternTimeNow())) ); }//weekend=monday, monday=monday.
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }//weekend=monday, monday=monday.

	JDE_MARKETS_EXPORT bool IsHoliday( const TimePoint& date )noexcept;
	inline bool IsHoliday( DayIndex date )noexcept{return IsHoliday( Chrono::FromDays(date) ); }
	namespace ExchangeTime
	{
		//JDE_MARKETS_EXPORT MinuteIndex MinuteCount( const TimePoint& timePoint )noexcept;
		JDE_MARKETS_EXPORT MinuteIndex MinuteCount( DayIndex day )noexcept;
	}
}