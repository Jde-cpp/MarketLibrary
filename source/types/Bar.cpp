#include "Bar.h"
#include <bar.h>
#pragma warning( disable : 4244 )
#include "./proto/bar.pb.h"
#pragma warning( default : 4244 )
#include <jde/Assert.h>
#include <jde/Exception.h>
#include <jde/Log.h>
#include <jde/Str.h>


#define var const auto

namespace Jde
{
	using EBarSize=Markets::Proto::Requests::BarSize;
	time_t Markets::ConvertIBDate( str time, optional<bool> ymdFormatOptional )noexcept
	{
		var ymdFormat = ymdFormatOptional.value_or( time.size()==8 );
		ASSERT( (ymdFormat && time.size()==8) || time.size()==10 );
		return ymdFormat
			? DateTime( (uint16)stoi(time.substr(0,4)), (uint8)stoi(time.substr(4,2)), (uint8)stoi(time.substr(6,2)) ).TimeT()
			: stoi(time);
	}
	string Markets::ToIBDate( TimePoint timePoint )noexcept
	{
		const DateTime time{ timePoint };
		return time.Hour()==0 && time.Minute()==0 && time.Second()==0
			? fmt::format( "{}{:0>2}{:0>2}", time.Year(), time.Month(), time.Day() )
			: std::to_string( time.TimeT() );
	}
}
namespace Jde::Markets
{
/*	sv BarSize::TryToString(const BarSize::Enum barSize )noexcept
	{
		auto value = "1 hour"sv;
		Try( [barSize, &value](){ value = ToString(barSize);} );
		return value;
	}
*/
	α BarSize::ToString( const BarSize::Enum barSize )noexcept->sv
	{
		sv result = "1 hour";
		if( barSize!=EBarSize::Hour )
		{
			if( barSize==EBarSize::Second ) result = "1 sec";
			else if( barSize==EBarSize::Second5 ) result = "5 secs";
			else if( barSize==EBarSize::Second15 ) result = "15 secs";
			else if( barSize==EBarSize::Second30 ) result = "30 secs";
			else if( barSize==EBarSize::Minute ) result = "1 min";
			else if( barSize==EBarSize::Minute2 ) result = "2 mins";
			else if( barSize==EBarSize::Minute3 ) result = "3 mins";
			else if( barSize==EBarSize::Minute5 ) result = "5 mins";
			else if( barSize==EBarSize::Minute15 ) result = "15 mins";
			else if( barSize==EBarSize::Minute30 ) result = "30 mins";
			else if( barSize==EBarSize::Hour ) result = "1 hour";
			else if( barSize==EBarSize::Week ) result = "1 week";
			else if( barSize==EBarSize::Day ) result = "1 day";
			else if( barSize==EBarSize::Month ) result = "1 month";
			else WARN( "Unknown Barsize {}", barSize );
		}
		return result;
	}

	Duration BarSize::BarDuration( const BarSize::Enum barSize )noexcept
	{
		Duration duration = 1440min;//can go off of enum value
		switch( barSize )
		{
		case EBarSize::Second:
			duration = 1s;
			break;
		case EBarSize::Second5:
			duration = 5s;
			break;
		case EBarSize::Second15:
			duration = 15s;
			break;
		case EBarSize::Second30:
			duration = 30s;
			break;
		case EBarSize::Minute:
			duration = 1min;
			break;
		case EBarSize::Minute2:
			duration = 2min;
			break;
		case EBarSize::Minute3:
			duration = 3min;
			break;
		case EBarSize::Minute5:
			duration = 5min;
			break;
		case EBarSize::Minute15:
			duration = 15min;
			break;
		case EBarSize::Minute30:
			duration = 30min;
			break;
		case EBarSize::Hour:
			duration = 60min;
			break;
		case EBarSize::Day:
			duration = 1440min;
			break;
		default:
			ERR( "Unknown Barsize '{}'"sv, barSize );
		}
		return duration;
	}
	using namespace std::chrono;
	uint16 BarSize::BarsPerDay( const BarSize::Enum barSize )noexcept
	{
		constexpr auto perDay = minutes( (6*60+30) ).count();
		var duration = duration_cast<minutes>( BarSize::BarDuration(barSize) ).count();
		return duration > perDay
			? 1
			: perDay/duration + (perDay%duration ? 1 : 0);
	}

	CandleStick::CandleStick( const Proto::MinuteBar& minuteBar )noexcept:
		Open{ minuteBar.first_traded_price() },
		High{ minuteBar.highest_traded_price() },
		Low{ minuteBar.lowest_traded_price() },
		Close{ minuteBar.last_traded_price() },
		Volume{ minuteBar.volume() }
	{}
	CandleStick::CandleStick( const ::Bar& bar )noexcept:
		Open{ static_cast<CandleStick::Amount>(bar.open) },
		High{ static_cast<CandleStick::Amount>(bar.high) },
		Low{ static_cast<CandleStick::Amount>(bar.low) },
		Close{ static_cast<CandleStick::Amount>(bar.close) },
		Volume{ static_cast<uint32>(bar.volume) }
	{}
	α CandleStick::ToIB( TimePoint time )const noexcept->::Bar
	{
		return ::Bar{ ToIBDate(time), (double)High, (double)Low, (double)Open, (double)Close, 0, ToDecimal(Volume), 0 };
	}
	α CandleStick::ToProto()const noexcept->Proto::MinuteBar
	{
		Proto::MinuteBar proto;
		proto.set_first_traded_price( static_cast<float>(Open) );
		proto.set_highest_traded_price( static_cast<float>(High) );
		proto.set_lowest_traded_price( static_cast<float>(Low) );
		proto.set_last_traded_price( static_cast<float>(Close) );
		proto.set_volume( Volume );
		return proto;
	}


	α TwsDisplay::ToString( const TwsDisplay::Enum display )noexcept->string
	{
		return Str::FromEnum( TwsDisplay::StringValues, display );
	}
	α TwsDisplay::FromString( sv stringValue )noexcept(false)->TwsDisplay::Enum
	{
		size_t index=0;
		for( ;index<sizeof(StringValues); ++index )
		{
			if( stringValue==StringValues[index] )
				break;
		}
		if( index==sizeof(StringValues) )
			THROW( "Unknown TwsDisplay {}", stringValue );
		return static_cast<TwsDisplay::Enum>( index );
	}
	//std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value )
	//{
	//	os << TwsDisplay::ToString( value );
	//	return os;
	//}
}