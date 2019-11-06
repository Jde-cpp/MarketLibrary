#include "stdafx.h"
#include "Bar.h"
//#include "../../../framework/DateTime.h"
#define var const auto

namespace Jde::Markets
{
	const char* BarSize::ToString(const BarSize::Enum barSize )noexcept(false)
	{
		const char* result = "";
		switch( barSize )
		{
		case BarSize::Second:
			result = "1 sec";
			break;
		case BarSize::Second5:
			result = "5 secs";
			break;
		case BarSize::Second15:
			result = "15 secs";
			break;
		case BarSize::Second30:
			result = "30 secs";
			break;
		case BarSize::Minute:
			result = "1 min";
			break;
		case BarSize::Minute2:
			result = "2 mins";
			break;
		case BarSize::Minute3:
			result = "3 mins";
			break;
		case BarSize::Minute5:
			result = "5 mins";
			break;
		case BarSize::Minute15:
			result = "15 mins";
			break;
		case BarSize::Minute30:
			result = "30 mins";
			break;
		case BarSize::Hour:
			result = "1 hour";
			break;
		case BarSize::Day:
			result = "1 day";
			break;
		default:
			THROW( Exception(fmt::format("Unknown Barsize {}", barSize).c_str()) );
		}
		return result;
	}

	Duration BarSize::BarDuration( const BarSize::Enum barSize )noexcept
	{
		Duration duration = 1440min;
		switch( barSize )
		{
		case BarSize::Second:
			duration = 1s;
			break;
		case BarSize::Second5:
			duration = 5s;
			break;
		case BarSize::Second15:
			duration = 15s;
			break;
		case BarSize::Second30:
			duration = 30s;
			break;
		case BarSize::Minute:
			duration = 1min;
			break;
		case BarSize::Minute2:
			duration = 2min;
			break;
		case BarSize::Minute3:
			duration = 3min;
			break;
		case BarSize::Minute5:
			duration = 5min;
			break;
		case BarSize::Minute15:
			duration = 15min;
			break;
		case BarSize::Minute30:
			duration = 30min;
			break;
		case BarSize::Hour:
			duration = 60min;
			break;
		case BarSize::Day:
			duration = 1440min;
			break;
		default:
			ERR( "Unknown Barsize '{}'", barSize );
		}
		return duration;
	}
	uint16 BarSize::BarsPerDay( const BarSize::Enum barSize )noexcept
	{
		constexpr auto perDay = chrono::minutes( (6*60+30) ).count();
		var duration = chrono::duration_cast<chrono::minutes>( BarSize::BarDuration(barSize) ).count();
		return duration > perDay 
			? 1 
			: perDay/duration + (perDay%duration ? 1 : 0);
	}

 	const char* TwsDisplay::StringValues[9] = {"TRADES", "MIDPOINT", "BID", "ASK", "BID_ASK", "HISTORICAL_VOLATILITY", "OPTION_IMPLIED_VOLATILITY", "FEE_RATE", "REBATE_RATE"};
	const char* TwsDisplay::ToString( const TwsDisplay::Enum display )noexcept(false)
	{
		if( display>=sizeof(StringValues) )
			THROW( Exception(fmt::format("Unknown TwsDisplay {}", display).c_str()) );
		return StringValues[display];
	}
	TwsDisplay::Enum TwsDisplay::FromString( string_view stringValue )noexcept(false)
	{
		size_t index=0;
		for( ;index<sizeof(StringValues); ++index )
		{
			if( stringValue==StringValues[index] )
				break;
		}
		if( index==sizeof(StringValues) )
			THROW( Exception(fmt::format("Unknown TwsDisplay {}", stringValue).c_str()) );
		return static_cast<TwsDisplay::Enum>( index );
	}
	std::ostream& operator<<( std::ostream& os, const TwsDisplay::Enum& value )
	{
		os << TwsDisplay::ToString( value );
		return os;
	}

}