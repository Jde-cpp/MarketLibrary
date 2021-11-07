#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )
struct Bar;

#define Γα JDE_MARKETS_EXPORT auto
namespace Jde::Markets
{
	Γα ConvertIBDate( str time, optional<bool> ymdFormat={} )noexcept->time_t;
	Γα ToIBDate( TimePoint time )noexcept->string;
	using EBarSize=Proto::Requests::BarSize;
	namespace BarSize
	{
		using Enum=Proto::Requests::BarSize;

		Γα BarDuration( const BarSize::Enum barSize )noexcept->Duration;
		Γα BarsPerDay( const BarSize::Enum barSize )noexcept->uint16;
		//α ToString( const BarSize::Enum barSize )noexcept(false)->sv;
		Γα ToString( const BarSize::Enum barSize )noexcept->sv;
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
	namespace TwsDisplay
	{
		using Enum=Proto::Requests::Display;
		static constexpr array<sv,9> StringValues = {"TRADES", "MIDPOINT", "BID", "ASK", "BID_ASK", "HISTORICAL_VOLATILITY", "OPTION_IMPLIED_VOLATILITY", "FEE_RATE", "REBATE_RATE"};
		Γα ToString( const TwsDisplay::Enum display )noexcept->string;
		Γα FromString( sv stringValue )noexcept(false)->TwsDisplay::Enum;
	};
	//std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value );
}
#undef Γα