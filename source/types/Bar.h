#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )
struct Bar;

#define Γα ΓM α
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
		Γα ToString( const BarSize::Enum barSize )noexcept->sv;
	};

	namespace Proto{ class MinuteBar; }
	struct ΓM CandleStick
	{
		using Amount=float;
		CandleStick()=default;
		CandleStick( const Proto::MinuteBar& minuteBar )noexcept;
		CandleStick( const ::Bar& bar )noexcept;
		::Bar ToIB( TimePoint time )const noexcept;
		α ToProto()const noexcept->Proto::MinuteBar;
		const CandleStick::Amount Open{0.0};
		const CandleStick::Amount High{0.0};
		const CandleStick::Amount Low{0.0};
		const CandleStick::Amount Close{0.0};
		const Decimal Volume{0};//was uint
	};
	using EDisplay=Proto::Requests::Display;
	namespace TwsDisplay
	{
		using Enum=Proto::Requests::Display;
		static constexpr array<sv,9> StringValues = {"TRADES", "MIDPOINT", "BID", "ASK", "BID_ASK", "HISTORICAL_VOLATILITY", "OPTION_IMPLIED_VOLATILITY", "FEE_RATE", "REBATE_RATE"};
		Γα ToString( const TwsDisplay::Enum display )noexcept->string;
		Γα FromString( sv stringValue )noexcept(false)->TwsDisplay::Enum;
	};
}
#undef Γα