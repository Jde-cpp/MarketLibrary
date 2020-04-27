#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
//#include "./proto/MinuteBar.pb.h"
namespace ibapi{struct Bar;}
namespace Jde::Markets
{
	struct JDE_MARKETS_EXPORT BarSize
	{
		enum Enum{ None=0, Second=1, Second5=2, Second15=3, Second30=4, Minute=5, Minute2=6, Minute3=16, Minute5=7, Minute15=8, Minute30=9, Hour=10, Day=11, Week=12, Month=13, Month3=14, Year=15 };
		static Duration BarDuration( const BarSize::Enum barSize )noexcept;
		static uint16 BarsPerDay( const BarSize::Enum barSize )noexcept;
		static const char* ToString(const BarSize::Enum barSize )noexcept(false);
	};

	struct JDE_MARKETS_EXPORT TwsDisplay
	{
		enum Enum{ Trades, Midpoint, Bid, Ask, BidAsk, HistoricalVolatility, OptionImpliedVolatility, FeeRate, RebateRate };
		static const char* StringValues[9];
		static const char* ToString(const TwsDisplay::Enum display )noexcept(false);
		static TwsDisplay::Enum FromString( string_view stringValue )noexcept(false);
	};
	std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value );
}