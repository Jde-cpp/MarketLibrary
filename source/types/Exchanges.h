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

	JDE_MARKETS_EXPORT string to_string( Exchanges exchange );
	Exchanges ToExchange( string_view pszName );
	JDE_MARKETS_EXPORT TimePoint PreviousTradingDay( const TimePoint& time );
	JDE_MARKETS_EXPORT TimePoint NextTradingDay( const TimePoint& time );
	inline TimePoint CurrentTradingDay( const TimePoint& time ){ return NextTradingDay( PreviousTradingDay(time) ); }//weekend=monday, monday=monday.
	JDE_MARKETS_EXPORT bool IsHoliday( const TimePoint& date );
	namespace ExchangeTime
	{
		JDE_MARKETS_EXPORT MinuteIndex MinuteCount( const TimePoint& timePoint );
	}
}