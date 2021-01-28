#pragma once
#include "../Exports.h"
#include "../TypeDefs.h"
#pragma warning( disable : 4244 )
#include "proto/ib.pb.h"
#include "proto/results.pb.h"
#pragma warning( default : 4244 )
namespace Jde::Markets
{
	struct Contract;
	using SecurityType=Proto::SecurityType;
	using Exchanges = Proto::Exchanges;
	//using namespace Chrono;

	constexpr std::array<std::string_view,26> ExchangeStrings={ "SMART", "NYSE", "NASDAQ", "AMEX", "ARCA", "BATS", "PINK", "VALUE", "IBIS", "CBOE", "ISE", "PSE", "PEARL", "MIAX", "MERCURY", "EDGX", "GEMINI", "BOX", "EMERALD", "NASDAQOM", "NASDAQBX", "PHLX", "CBOE2", "EBS", "IEX", "VENTURE" };
	JDE_MARKETS_EXPORT string_view ToString( Exchanges exchange )noexcept;
	Exchanges ToExchange( string_view pszName )noexcept;
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( DayIndex day=0 )noexcept;
	JDE_MARKETS_EXPORT DayIndex NextTradingDay( DayIndex day )noexcept;
	TimePoint PreviousTradingDay( const TimePoint& time )noexcept;
	JDE_MARKETS_EXPORT DayIndex PreviousTradingDay( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept;
	JDE_MARKETS_EXPORT TimePoint NextTradingDay( TimePoint time )noexcept;

	inline DayIndex CurrentTradingDay( DayIndex day, Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }
	inline DayIndex CurrentTradingDay( Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay() ); }
	JDE_MARKETS_EXPORT DayIndex CurrentTradingDay( const Markets::Contract& x )noexcept;
	inline TimePoint CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }
	JDE_MARKETS_EXPORT DayIndex CurrentTradingDay( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept;
	JDE_MARKETS_EXPORT TimePoint ClosingTime( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept(false);
	inline uint16 DayLengthMinutes( Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return 390; }
	bool IsOpen()noexcept;
	JDE_MARKETS_EXPORT bool IsOpen( SecurityType type )noexcept;
	JDE_MARKETS_EXPORT bool IsOpen( const Contract& contract )noexcept;
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
	struct TradingDay
	{
		TradingDay( DayIndex initial, Exchanges exchange ):_value{initial},_exchange{exchange}{ }
		TradingDay& operator++(){ do{ ++_value;}while(IsHoliday(_value,_exchange)); return *this; }
		TradingDay& operator--(){ do{ --_value;}while(IsHoliday(_value,_exchange)); return *this; }
		TradingDay& operator-=( DayIndex day )
		{
			for( DayIndex i = 0; i<day; ++i, --*this );
			return *this;
		}
		operator DayIndex()const noexcept{ return _value; }
	private:
		DayIndex _value;
		Exchanges _exchange;
	};
	DayIndex DayCount( TradingDay start, DayIndex end )noexcept;
	TradingDay operator-( TradingDay copy, DayIndex x )noexcept;

}