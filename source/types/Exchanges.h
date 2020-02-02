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
		Arca   =0x40000000,//32
		Bats   =0x200000000,
		PinkSheets =0x100000000000
	};

	JDE_MARKETS_EXPORT string to_string( Exchanges exchange )noexcept;
	Exchanges ToExchange( string_view pszName )noexcept;
	JDE_MARKETS_EXPORT TimePoint PreviousTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT TimePoint NextTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( DayIndex day=0 )noexcept;
	inline DayIndex NextTradingDay( DayIndex day )noexcept{ return Chrono::DaysSinceEpoch( NextTradingDay(Chrono::FromDays(day)) ); }

	inline DayIndex CurrentTradingDay( DayIndex day )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }//weekend=monday, monday=monday.
	inline DayIndex CurrentTradingDay()noexcept{ return NextTradingDay( PreviousTradingDay( Chrono::DaysSinceEpoch(Timezone::EasternTimeNow())) ); }//weekend=monday, monday=monday.
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }//weekend=monday, monday=monday.

	JDE_MARKETS_EXPORT bool IsHoliday( const TimePoint& date )noexcept;
	bool IsHoliday( DayIndex day )noexcept;//{return IsHoliday( Chrono::FromDays(date) ); }//TODO calc on day.
	namespace ExchangeTime
	{
		//JDE_MARKETS_EXPORT MinuteIndex MinuteCount( const TimePoint& timePoint )noexcept;
		JDE_MARKETS_EXPORT MinuteIndex MinuteCount( DayIndex day )noexcept;
	}
}