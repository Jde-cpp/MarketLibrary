#pragma once
#include "../Exports.h"
#include "../TypeDefs.h"
#include "proto/ib.pb.h"

namespace Jde::Markets
{
	//enum class SecurityType : uint8;
	struct Contract;
	using SecurityType=Proto::SecurityType;
	using Exchanges = Proto::Exchanges;
	using namespace Chrono;
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
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( DayIndex day=0 )noexcept;
	JDE_MARKETS_EXPORT DayIndex NextTradingDay( DayIndex day )noexcept;
	inline TimePoint PreviousTradingDay( const TimePoint& time )noexcept{ return FromDays(PreviousTradingDay(DaysSinceEpoch(time)) ); }
	inline TimePoint NextTradingDay( const TimePoint& time )noexcept{ return FromDays( NextTradingDay(DaysSinceEpoch(time)) ); }

	inline DayIndex CurrentTradingDay( DayIndex day, Exchanges exchange=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }
	inline DayIndex CurrentTradingDay( Exchanges exchange=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay() ); }
	inline DayIndex CurrentTradingDay( const Contract& details )noexcept{ return CurrentTradingDay(); }
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }
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
	JDE_MARKETS_EXPORT bool IsHoliday( DayIndex day, Exchanges exchange=Exchanges::Nyse )noexcept;//{return IsHoliday( Chrono::FromDays(date) ); }
	namespace ExchangeTime
	{
		JDE_MARKETS_EXPORT MinuteIndex MinuteCount( DayIndex day )noexcept;
	}
}