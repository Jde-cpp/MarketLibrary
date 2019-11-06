#include "stdafx.h"
#include "Exchanges.h"
#define var const auto

namespace Jde::Markets
{
	string to_string( Exchanges exchange )
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
	Exchanges ToExchange( string_view pszName )
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

	bool IsHoliday( const TimePoint& time )
	{
		const DateTime date{time};
		bool isHoliday = date.DayOfWk()==DayOfWeek::Saturday || date.DayOfWk()==DayOfWeek::Sunday;
		if( !isHoliday )
		{
			var year = date.Year();
			var day = date.Day();
			var month= date.Month();
			if( year==2019 )
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
				THROW( Exception("date not implemented {}.", year) );
				
				//isHoliday = true;
				//
		}
		return isHoliday;
	}

	TimePoint PreviousTradingDay( const TimePoint& time )
	{
		//auto start = time;
		auto previous = time-std::chrono::hours( 24 );
		for( ; IsHoliday(previous); previous-=std::chrono::hours(24)  );
		return previous;
	}
	TimePoint NextTradingDay( const TimePoint& time )
	{
		auto next = time+std::chrono::hours( 24 );;
		for( ; IsHoliday(next); next+=std::chrono::hours(24)  );
		return next;
	
	}
	MinuteIndex ExchangeTime::MinuteCount( const TimePoint& timePoint )
	{
		MinuteIndex minuteCount = 390;
		DateTime date( timePoint );
		var year = date.Year(); var month = date.Month(); var day = date.Day();
		if( year==2006 )
		{
			if( month==11 && day==24 )
				minuteCount = 210;
		}
		else if( year==2005 )
		{
			if( month==11 && day==25 )
				minuteCount = 210;
		}
		else if( year==2004 )
		{
			if( month==11 && day==26 )
				minuteCount = 210;
		}
		return minuteCount;
	}
}