#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#include "./proto/requests.pb.h"
namespace ibapi{struct Bar;}
namespace Jde::Markets
{
	time_t ConvertIBDate( const string& time, const optional<bool>& ymdFormat={} )noexcept;
	string ToIBDate( TimePoint time )noexcept;
	using EBarSize=Proto::Requests::BarSize;
	struct JDE_MARKETS_EXPORT BarSize
	{
		//enum Enum{ None=0, Second=1, Second5=2, Second15=3, Second30=4, Minute=5, Minute2=6, Minute3=7, Minute5=8, Minute15=9, Minute30=10, Hour=11, Day=12, Week=13, Month=14, Month3=14, Year=15 };
		using Enum=Proto::Requests::BarSize;

		static Duration BarDuration( const BarSize::Enum barSize )noexcept;
		static uint16 BarsPerDay( const BarSize::Enum barSize )noexcept;
		static string_view ToString(const BarSize::Enum barSize )noexcept(false);
		static string_view TryToString(const BarSize::Enum barSize )noexcept;
	};

	namespace Proto{ class MinuteBar; }
	struct JDE_MARKETS_EXPORT CandleStick
	{
		CandleStick()=default;
		CandleStick( const Proto::MinuteBar& minuteBar )noexcept;
		CandleStick( const ::Bar& bar )noexcept;
		//CandleStick& operator=(const CandleStick&)=default;
		::Bar ToIB( TimePoint time )const noexcept;
		Proto::MinuteBar ToProto()const noexcept;
		const Amount Open{0.0};
		const Amount High{0.0};
		const Amount Low{0.0};
		const Amount Close{0.0};
		const uint32 Volume{0};
	};
	using EDisplay=Proto::Requests::Display;
	struct JDE_MARKETS_EXPORT TwsDisplay
	{
		//enum Enum{ Trades, Midpoint, Bid, Ask, BidAsk, HistoricalVolatility, OptionImpliedVolatility, FeeRate, RebateRate };
		using Enum=Proto::Requests::Display;
		static constexpr array<string_view,9> StringValues = {"TRADES", "MIDPOINT", "BID", "ASK", "BID_ASK", "HISTORICAL_VOLATILITY", "OPTION_IMPLIED_VOLATILITY", "FEE_RATE", "REBATE_RATE"};
		static string_view ToString( const TwsDisplay::Enum display )noexcept(false);
		static TwsDisplay::Enum FromString( string_view stringValue )noexcept(false);
	};
	std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value );
}