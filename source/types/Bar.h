#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#pragma warning( disable : 4244 )
#include "./proto/requests.pb.h"
#pragma warning( default : 4244 )
struct Bar;

namespace Jde::Markets
{
	JDE_MARKETS_EXPORT time_t ConvertIBDate( const string& time, const optional<bool>& ymdFormat={} )noexcept;
	JDE_MARKETS_EXPORT string ToIBDate( TimePoint time )noexcept;
	using EBarSize=Proto::Requests::BarSize;
	struct JDE_MARKETS_EXPORT BarSize
	{
		using Enum=Proto::Requests::BarSize;

		static Duration BarDuration( const BarSize::Enum barSize )noexcept;
		static uint16 BarsPerDay( const BarSize::Enum barSize )noexcept;
		static string_view ToString(const BarSize::Enum barSize )noexcept(false);
		static string_view TryToString(const BarSize::Enum barSize )noexcept;
	};

	namespace Proto{ class MinuteBar; }
	struct JDE_MARKETS_EXPORT CandleStick
	{
		typedef float Amount;
		CandleStick()=default;
		CandleStick( const Proto::MinuteBar& minuteBar )noexcept;
		CandleStick( const ::Bar& bar )noexcept;
		::Bar ToIB( TimePoint time )const noexcept;
		Proto::MinuteBar ToProto()const noexcept;
		const CandleStick::Amount Open{0.0};
		const CandleStick::Amount High{0.0};
		const CandleStick::Amount Low{0.0};
		const CandleStick::Amount Close{0.0};
		const uint32 Volume{0};
	};
	using EDisplay=Proto::Requests::Display;
	struct JDE_MARKETS_EXPORT TwsDisplay
	{
		using Enum=Proto::Requests::Display;
		static constexpr array<string_view,9> StringValues = {"TRADES", "MIDPOINT", "BID", "ASK", "BID_ASK", "HISTORICAL_VOLATILITY", "OPTION_IMPLIED_VOLATILITY", "FEE_RATE", "REBATE_RATE"};
		static string_view ToString( const TwsDisplay::Enum display )noexcept(false);
		static TwsDisplay::Enum FromString( string_view stringValue )noexcept(false);
	};
	std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value );
}