#pragma once
#include "../Exports.h"
#include "../TypeDefs.h"
#include "proto/ib.pb.h"

namespace Jde::Markets
{
	struct Contract;
	using SecurityType=Proto::SecurityType;
	using Exchanges = Proto::Exchanges;
	using namespace Chrono;

	constexpr std::array<std::string_view,9> ExchangeStrings={ "Smart", "Nyse", "Nasdaq", "Amex", "Arca", "Bats", "PINK", "Value", "IBIS" };
	JDE_MARKETS_EXPORT string_view ToString( Exchanges exchange )noexcept;
	Exchanges ToExchange( string_view pszName )noexcept;
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( DayIndex day=0 )noexcept;
	JDE_MARKETS_EXPORT DayIndex NextTradingDay( DayIndex day )noexcept;
	inline TimePoint PreviousTradingDay( const TimePoint& time )noexcept{ return FromDays(PreviousTradingDay(DaysSinceEpoch(time)) ); }
	inline TimePoint NextTradingDay( const TimePoint& time )noexcept{ return FromDays( NextTradingDay(DaysSinceEpoch(time)) ); }

	inline DayIndex CurrentTradingDay( DayIndex day, Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }
	inline DayIndex CurrentTradingDay( Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay() ); }
	inline DayIndex CurrentTradingDay( const Contract& /*details*/ )noexcept{ return CurrentTradingDay(); }
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }
	bool IsOpen()noexcept;
	JDE_MARKETS_EXPORT bool IsOpen( SecurityType type )noexcept;
	bool IsOpen( const Contract& contract )noexcept;
	JDE_MARKETS_EXPORT bool IsPreMarket( SecurityType type )noexcept;
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