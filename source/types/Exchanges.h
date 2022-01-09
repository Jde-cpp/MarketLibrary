#pragma once
#include <jde/markets/Exports.h>
#include <jde/markets/TypeDefs.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/ib.pb.h>
#include <jde/markets/types/proto/results.pb.h>
#pragma warning( default : 4244 )
namespace Jde::Markets
{
	#define Φ ΓM α
	struct Contract;
	using SecurityType=Proto::SecurityType;
	using Exchanges = Proto::Exchanges;

	constexpr std::array<sv,31> ExchangeStrings={ "SMART", "NYSE", "NASDAQ", "AMEX", "ARCA", "BATS", "PINK", "VALUE", "IBIS", "CBOE", "ISE", "PSE", "PEARL", "MIAX", "MERCURY", "EDGX", "GEMINI", "BOX", "EMERALD", "NASDAQOM", "NASDAQBX", "PHLX", "CBOE2", "EBS", "IEX", "VENTURE", "ASX", "AEQLIT", "LSEETF", "LSE", "ISLAND" };
	Φ ToString( Exchanges exchange )noexcept->sv;
	Φ ToExchange( sv pszName )noexcept->Exchanges;
	Φ PreviousTradingDay( Day day=0 )noexcept->Day;
	Φ NextTradingDay( Day day )noexcept->Day;
	Ξ PreviousTradingDay( const TimePoint& time )noexcept{ return Chrono::FromDays( PreviousTradingDay(Chrono::ToDays(time)) ); }
	Φ PreviousTradingDay( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept->Day;
	Φ NextTradingDay( TimePoint time )noexcept->TimePoint;

	Ξ CurrentTradingDay( Day day, Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay(day) ); }
	Ξ CurrentTradingDay( Exchanges /*exchange*/=Exchanges::Nyse )noexcept{ return NextTradingDay( PreviousTradingDay() ); }
	Φ CurrentTradingDay( const Markets::Contract& x )noexcept->Day;
	Ξ CurrentTradingDay( const TimePoint& time )noexcept{ return NextTradingDay( PreviousTradingDay(time) ); }
	Φ CurrentTradingDay( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept->Day;
	Φ ClosingTime( const std::vector<Proto::Results::ContractHours>& tradingHours )noexcept(false)->TimePoint;
	Ξ DayLengthMinutes( Exchanges /*exchange*/=Exchanges::Nyse )noexcept->Day{ return 390; }
	α IsOpen()noexcept->bool;//TODO getrid of
	Φ IsOpen( SecurityType type )noexcept->bool;//TODO getrid of
	Φ IsOpen( const Contract& c, bool useRth=false )noexcept->bool;
	Φ IsPreMarket( SecurityType type )noexcept->bool;//TODO getrid of
	α IsRth( const Contract& contract, TimePoint time )noexcept->bool;
	α RthBegin( const Contract& contract, Day day )noexcept->TimePoint;
	α RthEnd( const Contract& contract, Day day )noexcept->TimePoint;
	Φ ExtendedBegin( const Contract& contract, Day day )noexcept->TimePoint;
	α ExtendedEnd( const Contract& contract, Day day )noexcept->TimePoint;

	Φ IsHoliday( const TimePoint& date )noexcept->bool;
	Φ IsHoliday( Day day, Exchanges exchange=Exchanges::Nyse )noexcept->bool;
	namespace ExchangeTime
	{
		Φ MinuteCount( Day day )noexcept->MinuteIndex;
	}
	struct TradingDay
	{
		TradingDay( Day initial, Exchanges exchange ):_value{initial},_exchange{exchange}{ }
		α operator++()->TradingDay&{ do{ ++_value;}while(IsHoliday(_value,_exchange)); return *this; }
		α operator--()->TradingDay&{ do{ --_value;}while(IsHoliday(_value,_exchange)); return *this; }
		α operator-=( Day day )->TradingDay&{ for( Day i = 0; i<day; ++i, --*this ); return *this; }
		operator Day()const noexcept{ return _value; }
	private:
		Day _value;
		Exchanges _exchange;
	};
	α DayCount( TradingDay start, Day end )noexcept->Day;
	α operator-( TradingDay copy, Day x )noexcept->TradingDay;
}
#undef Φ