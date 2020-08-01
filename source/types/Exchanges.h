#pragma once
#include "../Exports.h"
#include "../TypeDefs.h"
#include "proto/ib.pb.h"
//#include "DateTime.h"

namespace Jde::Markets
{
	//enum class SecurityType : uint8;
	struct Contract;

	using SecurityType=Proto::SecurityType;
	using Exchanges = Proto::Exchanges;
	// enum class Exchanges : uint
	// {
	// 	None   = 0,
	// 	Nyse   = 1,
	// 	Nasdaq = 2,
	// 	Amex   =0x20,
	// 	Smart  =0x20000000,//30
	// 	Arca   =0x40000000,//32
	// 	Bats   =0x200000000,
	// 	PinkSheets =0x100000000000,
	// 	Value = 0x80000000000000
	// };

	JDE_MARKETS_EXPORT string_view ToString( Exchanges exchange )noexcept;
	Exchanges ToExchange( string_view pszName )noexcept;
	JDE_MARKETS_EXPORT TimePoint PreviousTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT TimePoint NextTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( DayIndex day=0 )noexcept;
	inline DayIndex NextTradingDay( DayIndex day )noexcept{ return Chrono::DaysSinceEpoch( NextTradingDay(Chrono::FromDays(day)) ); }

	inline DayIndex CurrentTradingDay( DayIndex day, Exchanges exchange=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }//weekend=monday, monday=monday.
	inline DayIndex CurrentTradingDay( Exchanges exchange=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay( Chrono::DaysSinceEpoch(Timezone::EasternTimeNow())) ); }//weekend=monday, monday=monday.
	inline DayIndex CurrentTradingDay( const Contract& details )noexcept{ return CurrentTradingDay(); }
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }//weekend=monday, monday=monday.
	bool IsOpen()noexcept;
	bool IsOpen( SecurityType type )noexcept;
	bool IsOpen( const Contract& contract )noexcept;
	bool IsPreMarket( SecurityType type )noexcept;
	bool IsRth( const Contract& contract, TimePoint time )noexcept;
	TimePoint RthBegin( const Contract& contract, DayIndex day )noexcept;
	TimePoint RthEnd( const Contract& contract, DayIndex day )noexcept;
	TimePoint ExtendedBegin( const Contract& contract, DayIndex day )noexcept;
	TimePoint ExtendedEnd( const Contract& contract, DayIndex day )noexcept;

	JDE_MARKETS_EXPORT bool IsHoliday( const TimePoint& date )noexcept;
	JDE_MARKETS_EXPORT bool IsHoliday( DayIndex day, Exchanges exchange=Exchanges::Nyse )noexcept;//{return IsHoliday( Chrono::FromDays(date) ); }//TODO calc on day.
	//JDE_MARKETS_EXPORT bool IsHoliday( DayIndex day )noexcept;//{return IsHoliday( Chrono::FromDays(date) ); }//TODO calc on day.
	namespace ExchangeTime
	{
		//JDE_MARKETS_EXPORT MinuteIndex MinuteCount( const TimePoint& timePoint )noexcept;
		JDE_MARKETS_EXPORT MinuteIndex MinuteCount( DayIndex day )noexcept;
	}
}