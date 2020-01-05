#include "Exchanges.h"
#define var const auto

namespace Jde::Markets
{
	using namespace Chrono;
	string to_string( Exchanges exchange )noexcept
	{
		string value;
		switch( exchange )
		{
		case Exchanges::Nyse:
			value = "Nyse";
			break;
		case Exchanges::Nasdaq:
			value = "Nasdaq";
			break;
		case Exchanges::Amex:
			value = "Amex";
			break;
		case Exchanges::Smart:
			value = "Smart";
			break;
		case Exchanges::Arca:
			value = "Arca";
			break;
		case Exchanges::Bats:
			value = "Bats";
			break;
		default:
			GetDefaultLogger()->error( "Unknown exchange {}", (uint)exchange );
		}
		return value;
	}
	Exchanges ToExchange( string_view pszName )noexcept
	{
		const CIString name( pszName.data(), pszName.size() );
		Exchanges value = Exchanges::None;
		if( name=="Nyse" )
			value = Exchanges::Nyse;
		else if( name=="Nasdaq" )
			value = Exchanges::Nasdaq;
		else if( name=="Amex" )
			value = Exchanges::Amex;
		else if( name=="Smart" || name.size()==0 )
			value = Exchanges::Smart;
		else if( name=="Arca" )
			value = Exchanges::Arca;
		else if( name=="Bats" )
			value = Exchanges::Bats;
		else
			GetDefaultLogger()->error( "Unknown exchange {}", pszName );
		return value;
	}

	bool IsHoliday( const TimePoint& time )noexcept
	{
		const DateTime date{time};
		bool isHoliday = date.DayOfWk()==DayOfWeek::Saturday || date.DayOfWk()==DayOfWeek::Sunday;
		if( !isHoliday )
		{
			var year = date.Year();
			var day = date.Day();
			var month= date.Month();
			if( year==2020 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==20;
				else if( month==2 )
					isHoliday = day==17;
				else if( month==4 )
					isHoliday = day==10;
				else if( month==5 )
					isHoliday = day==25;
				else if( month==7 )
					isHoliday = day==3;
				else if( month==9 )
					isHoliday = day==7;
				else if( month==11 )
					isHoliday = day==26;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2019 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==21;
				else if( month==2 )
					isHoliday = day==18;
				else if( month==4 )
					isHoliday = day==19;
				else if( month==5 )
					isHoliday = day==27;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==2;
				else if( month==11 )
					isHoliday = day==28;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2018 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==15;
				else if( month==2 )
					isHoliday = day==19;
				else if( month==3 )
					isHoliday = day==30;
				else if( month==5 )
					isHoliday = day==28;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==3;
				else if( month==11 )
					isHoliday = day==22;
				else if( month==12 )
					isHoliday = day==25 || day==5;  //5=bush Death
			}
			else if( year==2017 )
			{
				if( month == 1 )
					isHoliday = day==2 || day==16;
				else if( month==2 )
					isHoliday = day==20;
				else if( month==4 )
					isHoliday = day==14;
				else if( month==5 )
					isHoliday = day==29;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==4;
				else if( month==11 )
					isHoliday = day==23;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2016 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==18;
				else if( month==2 )
					isHoliday = day==15;
				else if( month==3 )
					isHoliday = day==25;
				else if( month==5 )
					isHoliday = day==30;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==5;
				else if( month==11 )
					isHoliday = day==24;
				else if( month==12 )
					isHoliday = day==26;
			}
			else if( year==2015 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==19;
				else if( month==2 )
					isHoliday = day==16;
				else if( month==4 )
					isHoliday = day==3;
				else if( month==5 )
					isHoliday = day==25;
				else if( month==7 )
					isHoliday = day==3;
				else if( month==9 )
					isHoliday = day==7;
				else if( month==11 )
					isHoliday = day==26;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2014 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==20;
				else if( month==2 )
					isHoliday = day==17;
				else if( month==4 )
					isHoliday = day==18;
				else if( month==5 )
					isHoliday = day==26;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==1;
				else if( month==11 )
					isHoliday = day==27;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2013 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==21;
				else if( month==2 )
					isHoliday = day==18;
				else if( month==3 )
					isHoliday = day==29;
				else if( month==5 )
					isHoliday = day==27;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==2;
				else if( month==11 )
					isHoliday = day==28;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2012 )
			{
				if( month == 1 )
					isHoliday = day==2 || day==16;
				else if( month==2 )
					isHoliday = day==20;
				else if( month==4 )
					isHoliday = day==6;
				else if( month==5 )
					isHoliday = day==28;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==3;
				else if( month==10 )
					isHoliday = day==29 || day==30;//not holiday just no ib history
				else if( month==11 )
					isHoliday = day==22;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2011 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==17;
				else if( month==2 )
					isHoliday = day==21;
				else if( month==4 )
					isHoliday = day==22;
				else if( month==5 )
					isHoliday = day==30;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==5;
				else if( month==11 )
					isHoliday = day==24;
				else if( month==12 )
					isHoliday = day==26;
			}
			else if( year==2010 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==18;
				else if( month==2 )
					isHoliday = day==15;
				else if( month==4 )
					isHoliday = day==2;
				else if( month==5 )
					isHoliday = day==31;
				else if( month==7 )
					isHoliday = day==5;
				else if( month==9 )
					isHoliday = day==6;
				else if( month==11 )
					isHoliday = day==25;
				else if( month==12 )
					isHoliday = day==24;
			}
			else if( year==2009 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==19;
				else if( month==2 )
					isHoliday = day==16;
				else if( month==4 )
					isHoliday = day==10;
				else if( month==5 )
					isHoliday = day==25;
				else if( month==7 )
					isHoliday = day==3;
				else if( month==9 )
					isHoliday = day==7;
				else if( month==11 )
					isHoliday = day==26;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2008 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==21;
				else if( month==2 )
					isHoliday = day==18;
				else if( month==3 )
					isHoliday = day==21;
				else if( month==5 )
					isHoliday = day==26;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==1;
				else if( month==11 )
					isHoliday = day==27;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2007 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==2 || day==15;
				else if( month==2 )
					isHoliday = day==19 || day==8;/*todo remove 8??*/
				else if( month==4 )
					isHoliday = day==6;
				else if( month==5 )
					isHoliday = day==28;
				else if( month==7 )
					isHoliday = day==4 || day==2; /*2 have no values*/
				else if( month==9 )
					isHoliday = day==3;
				else if( month==11 )
					isHoliday = day==22;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2006 )
			{
				if( month == 1 )
					isHoliday = day==2 || day==16;
				else if( month==2 )
					isHoliday = day==20;
				else if( month==4 )
					isHoliday = day==14;
				else if( month==5 )
					isHoliday = day==29;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==4;
				else if( month==11 )
					isHoliday = day==23;
				else if( month==12 )
					isHoliday = day==25;
			}
			else if( year==2005 )
			{
				if( month == 1 )
					isHoliday = day==17;
				else if( month==2 )
					isHoliday = day==21;
				else if( month==3 )
					isHoliday = day==25;
				else if( month==5 )
					isHoliday = day==30;
				else if( month==7 )
					isHoliday = day==4;
				else if( month==9 )
					isHoliday = day==5;
				else if( month==11 )
					isHoliday = day==24;
				else if( month==12 )
					isHoliday = day==26;
			}
			else if( year==2004 )
			{
				if( month == 1 )
					isHoliday = day==1 || day==29 || day==30;/*29 & 30 no data for most stocks*/
				else if( month==2 )
					isHoliday = day==16 || day==9 || day==10;
				else if( month==3 )
					isHoliday = day==24;/*no data for alot*/
				else if( month==4 )
					isHoliday = day==9;
				else if( month==5 )
					isHoliday = day==31;
				else if( month==6 )
					isHoliday = day==11;//Regan's Death
				else if( month==7 )
					isHoliday = day==5 || day==12;/*no data on 12th*/
				else if( month==9 )
					isHoliday = day==6;
				else if( month==11 )
					isHoliday = day==25;
				else if( month==12 )
					isHoliday = day==24;
			}
			else
				ASSERT_DESC( false, fmt::format("date not implemented {}.", year) );
		}
		return isHoliday;
	}

	DayIndex PreviousTradingDay( DayIndex day )noexcept
	{
		if( day==0 )
			day = Chrono::DaysSinceEpoch( Timezone::EasternTimeNow() );
		return Chrono::DaysSinceEpoch( PreviousTradingDay(Chrono::FromDays(day)) );
	}
	TimePoint PreviousTradingDay( const TimePoint& time )noexcept
	{
		//auto start = time;
		auto previous = time-std::chrono::hours( 24 );
		for( ; IsHoliday(previous); previous-=std::chrono::hours(24)  );
		return previous;
	}
	TimePoint NextTradingDay( const TimePoint& time )noexcept
	{
		auto next = time+std::chrono::hours( 24 );;
		for( ; IsHoliday(next); next+=std::chrono::hours(24)  );
		return next;

	}
	mutex _lock;
	MinuteIndex ExchangeTime::MinuteCount( DayIndex day )noexcept
	{
		if( day==16062 )
			return 352;
		constexpr array<DayIndex,35> values = {12748,13112,13332,13476,13697,13840,14063,14237,14452,14602,14939,15303,15524,15667,15698,15889,16038,16063,16254,16402,16428,16766,16793,17130,17350,17494,17715,17858,17889,18080,18229,18254,18593,18620,18957};
		auto pValue = std::lower_bound( values.begin(), values.end(), day );
		return pValue==values.end() || *pValue!=day ? 390 : 210;
#if 0
		static map<DayIndex,uint16> shortDays;
		if( shortDays.size()==0 )
		{
			unique_lock l{_lock};
			map<DayIndex,uint16> temp;
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2004,11,26).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2005,11,23).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2006,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2006,11,24).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2007,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2007,11,23)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2008,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2008,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2009,7,27)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2009,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2010,11,26)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2011,11,25)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2012,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2012,11,23)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2012,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2013,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2013,11,29)), 210 );
			//temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2013,12,23), 352 );//352
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2013,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2014,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2014,11,28)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2014,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2015,11,27)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2015,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2016,11,25)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2017,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2017,11,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2018,7,3)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2018,11,23)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2018,12,24)), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2019,7,3).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2019,11,29).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2019,12,24).GetTimePoint()), 210 );
 			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2020,11,27).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2020,12,24).GetTimePoint()), 210 );
			temp.emplace_hint( temp.end(), DaysSinceEpoch(DateTime(2021,11,26).GetTimePoint()), 210 );
			ostringstream os;
			for( var& [day,size] : temp )
				os << day <<",";
			DBG0( os.str() );
			shortDays = temp;
		}
		return shortDays.find(day)==shortDays.end() ? 390 : shortDays.find(day)->second;
#endif
	}
}