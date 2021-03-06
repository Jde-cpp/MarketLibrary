#include "BarData.h"
#include <jde/io/File.h>
#include <jde/Str.h>
#include <jde/Assert.h>
#include <jde/markets/types/Contract.h>
#include "../client/TwsClientSync.h"
#include "../types/Bar.h"
#pragma warning( disable : 4244 )
#include "../types/proto/bar.pb.h"
#pragma warning( default : 4244 )
#include "../../../XZ/source/XZ.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../../../Framework/source/io/ProtoUtilities.h"

#define var const auto
#define _client TwsClientSync::Instance()

namespace Jde::Markets
{
	using namespace Chrono;
	fs::path BarData::Path()noexcept(false)
	{
		var path = Settings::Global().Get<fs::path>( "barPath" );
		return path.empty() ? IApplication::Instance().ApplicationDataFolder()/"securities" : path;
	}
	fs::path BarData::Path( const Contract& contract )noexcept(false){ ASSERT(contract.PrimaryExchange!=Exchanges::Smart) return Path()/Str::ToLower(string{ToString(contract.PrimaryExchange)})/Str::Replace(contract.Symbol, " ", "_"); }

	sp<Proto::BarFile> BarData::Load( path path )noexcept(false)
	{
		var pBytes = IO::Zip::XZ::Read( path );
		auto pFile = pBytes ? make_shared<Proto::BarFile>() : sp<Proto::BarFile>();
		if( pFile )
		{
			google::protobuf::io::CodedInputStream input{ (const uint8*)pBytes->data(), (int)pBytes->size() };
			if( !pFile->MergePartialFromCodedStream(&input) )
				THROW( IOException("barFile.MergePartialFromCodedStream returned false") );
		}
		else
		{
			ERR( "'{}' has 0 bytes."sv, path.string() );
			fs::remove( path );
			DBG( "Deleted '{}'."sv, path.string() );
		}
		return pFile;
	}

	MapPtr<DayIndex,VectorPtr<CandleStick>> BarData::Load( path path, sv symbol, const map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)
	{
		auto pResults = make_shared<map<DayIndex,VectorPtr<CandleStick>>>();

		var pFile = pPartials && pPartials->find(path.string())!=pPartials->end() ? pPartials->find(path.string())->second : Load( path );
		if( !pFile )
			return pResults;

		DayIndex dayCount = pFile->days_size();
		for( DayIndex i=0; i<dayCount; ++i )
		{
			var& day = pFile->days(i);
			var dayIndex = day.day();
			uint barCount = day.bars_size();
			if( IsHoliday(dayIndex) )
			{
				ERR( "({}) - {} has {} with {} bars but it is a holiday."sv, symbol, path.string(), DateDisplay(dayIndex), barCount );
				continue;
			}
			auto& pDays = pResults->try_emplace( pResults->end(), dayIndex, make_shared<vector<CandleStick>>() )->second;
			if( pDays->size() )
			{
				ERR( "({}) - {} has {} 2x."sv, symbol, path.string(), DateDisplay(dayIndex) );
				continue;
			}
			if( barCount>ExchangeTime::MinuteCount(dayIndex) )
			{
				ERR( "{} for {} has {} bars."sv, symbol, DateDisplay(dayIndex), barCount );
				barCount = ExchangeTime::MinuteCount( dayIndex );
			}
			pDays->reserve( barCount );
			for( int iBar=0; iBar<barCount; ++iBar )
				pDays->push_back( CandleStick(day.bars(iBar)) );
		}
		return pResults;
	}
	tuple<DayIndex,DayIndex> ExtractTimeframe( path path, TimePoint earliestDate, uint8 yearsToKeep=3 )
	{
		static uint16 currentYear=DateTime().Year();
		var [year,month,day] = IO::FileUtilities::ExtractDate( path );
		if( year==0 )
			return make_tuple( 0, 0 );

		var start = CurrentTradingDay( year==currentYear-yearsToKeep ? earliestDate : ToTimePoint(year, std::max(month,(uint8)1), std::max(day,(uint8)1)) );
		var end = day ? start+24h-1s : EndOfMonth( ToTimePoint(year, month ? month : 12, 1) );
		return make_tuple( DaysSinceEpoch(start), DaysSinceEpoch(end) );
	}

	void BarData::ForEachFile( const Contract& contract, const function<void(path,DayIndex, DayIndex)>& fnctn, DayIndex start, DayIndex endInput, sv prefix )noexcept//fnctn better not throw
	{
		var root = BarData::Path( contract );
		if( !fs::exists(root) )
			return;
		var pEntries = IO::FileUtilities::GetDirectory( root );
		var end = endInput ? endInput : std::numeric_limits<DayIndex>::max();
		for( var& entry : *pEntries )
		{
			var path = entry.path();
			if( 	(prefix.size() && path.stem().string().find(prefix)!=0)
				|| (!prefix.size() && std::isalpha(path.stem().string()[0])) )
			{
				continue;
			}
			auto issueDate = contract.IssueDate;
			if( issueDate==TimePoint::max() )
				issueDate = DateTime{2010,1,1}.GetTimePoint();
			ASSERT( issueDate!=TimePoint::max() );
			var [fileStart,fileEnd] = ExtractTimeframe( path, issueDate );
			if( fileEnd==0 )
			{
				DBG( "could not find ent to '{}' prefix='{}'"sv, path.stem().string(), prefix );
				continue;
			}
			if( start<=fileEnd && end>=fileStart )
				fnctn( path, fileStart, fileEnd );
		}
	}

	MapPtr<DayIndex,VectorPtr<CandleStick>> BarData::TryLoad( const Contract& contract, DayIndex start, DayIndex end )noexcept
	{
		try
		{
			return Load( contract, start, end );
		}
		catch( const Exception& e )
		{
			e.Log();
		}
		return make_shared<map<DayIndex,VectorPtr<CandleStick>>>();
	}
	MapPtr<DayIndex,VectorPtr<CandleStick>> BarData::Load( const Contract& contract, DayIndex start, DayIndex end )noexcept(false)
	{
		//var root = BarData::Path( contract );
		auto pResults = make_shared<map<DayIndex,VectorPtr<CandleStick>>>();
		auto fnctn = [&]( path path, DayIndex, DayIndex )
		{
			var pFileValues = Load( path, contract.Symbol );
			for( var& [day,pBars] : *pFileValues )
			{
				if( day>=start && day<=end )
					pResults->try_emplace( pResults->end(), day, pBars );
			}
		};
		//if( HavePath() )
			ForEachFile( contract, fnctn, start, end );
		return pResults;
	}

	void BarData::Save( const Contract& contract, map<DayIndex,vector<sp<::Bar>>>& rthBars )noexcept
	{
		var current = CurrentTradingDay( contract );
		var exclude = Clock::now()>ExtendedEnd( contract, current ) ? 0 : current;
		map<DayIndex,VectorPtr<CandleStick>> days;
		for( var& [day, bars] : rthBars )
		{
			if( day==exclude )
				continue;
			auto pSaveBars = days.emplace( day, make_shared<vector<CandleStick>>() ).first->second;
			for( var& pBar : bars )
				pSaveBars->push_back( CandleStick{*pBar} );
		}
		Try( [&](){ Save( contract, days ); } );
	}

	void BarData::ApplySplit( const Contract& contract, uint multiplier )noexcept
	{
		auto fnctn = [&]( path path, DayIndex, DayIndex )
		{
			var pExisting = Load( path );//BarFile
			Proto::BarFile newFile;
			for( int i=0; i<pExisting->days_size(); ++i )
			{
				var& day = pExisting->days( i );
				auto pDays = newFile.add_days();
				for( int j=0; j<day.bars_size(); ++j )
				{
					auto bar = day.bars( j );
					auto pNew = pDays->add_bars();
					pNew->set_first_traded_price( bar.first_traded_price() );
					pNew->set_highest_traded_price( bar.highest_traded_price() );
					pNew->set_lowest_traded_price( bar.lowest_traded_price() );
					pNew->set_last_traded_price( bar.last_traded_price() );
					pNew->set_volume( bar.volume() );
				}
			}
			IO::Zip::XZ::Write( path, IO::Proto::ToString(newFile) );
		};
		ForEachFile( contract, fnctn, 0, CurrentTradingDay() );
	}

	void BarData::Save( const Contract& contract, const map<DayIndex,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,TimePoint_>> pExcluded, bool checkExisting, const map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)
	{
		const DateTime now{ CurrentTradingDay(Clock::now()) };
		auto getFileName = [&now]( const DateTime& itemDate )->string
		{
			var year = std::max( now.Year()-3, itemDate.Year() );
			var month = now.Year()==itemDate.Year() ? itemDate.Month() : 0;
			auto day = month==0 || now.Month()!=itemDate.Month() ? 0 : itemDate.Day();
			return IO::FileUtilities::DateFileName( year, month, day );
		};
		auto addDay = []( Proto::BarFile& proto, DayIndex day, const vector<CandleStick>& bars )
		{
			auto pDay = proto.add_days();
			pDay->set_day( day );
			for( var& bar : bars )
				*pDay->add_bars() = bar.ToProto();
		};
		map<string,Proto::BarFile> files;
		for( var& [day, pBars] : days )
		{
			if( !pBars->size() )//How did this happen?
				continue;
			var fileName = getFileName( DateTime{FromDays(day)} );
			auto& barFile = files.try_emplace( files.end(), fileName, Proto::BarFile{} )->second;
			addDay( barFile, day, *pBars );
		}
		//is partial?, should consolidate
		for( auto& [fileName,proto] : files )
		{
			var barPath = Path( contract );
			set<DayIndex> existingDays;
			set<string> combinedFiles;
			auto addExisting = [&,&proto2=proto]( path path, DayIndex _=0, DayIndex _2=0)noexcept
			{
				combinedFiles.emplace( path.string() );
				var pExisting = Load( path, contract.Symbol, pPartials );
				for( var& [day, pBars] : *pExisting )
				{
					if( days.find(day)!=days.end() )
						continue;
					existingDays.emplace( day );
					addDay( proto2, day, *pBars );
				}
			};
			if( checkExisting )
			{
				{ auto path = barPath/(fileName+".dat.xz"); if( fs::exists(path) ) addExisting(path); }
				{ auto path = barPath/(fileName+"_partial.dat.xz"); if( fs::exists(path) ) addExisting(path);  }
			}
			bool complete = true;
			if( pExcluded )
			{
				var [start,end] = ExtractTimeframe( fileName, contract.IssueDate );
				for( auto day = start; day<=end; day = NextTradingDay(day) )
				{
					if( days.find(day)!=days.end() || existingDays.find(day)!=existingDays.end() )
						continue;
					auto excluded = false;
					//(AEM) is incomplete because of '01/26/04' 12443
					for( var& [startEx,endEx] : *pExcluded )
					{
						var startExcludedDay = DaysSinceEpoch( startEx );
						var endExcludedDay = DaysSinceEpoch( endEx.value_or(TimePoint{}) );
						excluded = ( !endExcludedDay && startExcludedDay==day ) || ( endExcludedDay && startExcludedDay<=day && endExcludedDay>=day );
						if( excluded )
							break;
					}
					if( !excluded )
					{
						ForEachFile( contract, addExisting, start, end );
						complete = existingDays.find( day )!=existingDays.end();
						if( !complete )
						{
							WARN( "({}) {}.dat.xz is incomplete because of '{}'"sv, contract.Symbol, fileName, DateDisplay(day) );
							break;
						}
					}
				}
			}
			string completeFileName = fileName+(complete ? "" : "_partial")+".dat.xz";
			try
			{
				if( !fs::exists(barPath) )
				{
					fs::create_directories( barPath );
					DBG( "Created directory '{}'"sv, barPath.string() );
				}
				fs::path tempPath{ barPath/("~"+completeFileName) };
				IO::Zip::XZ::Write( tempPath, IO::Proto::ToString(proto) );
				const fs::path path{ barPath/completeFileName };
				fs::rename( tempPath, path );
				for( var& combinedFile : combinedFiles )
				{
					if( combinedFile==path.string() )
						continue;
					DBG( "Removing combined file '{}'"sv, combinedFile );
					fs::remove( combinedFile );
				}
			}
			catch( const fs::filesystem_error& e )
			{
				throw IOException( move(e) );
			}
		}
	}

	flat_set<DayIndex> BarData::FindExisting( const Contract& contract, DayIndex start2, DayIndex end2, sv prefix, map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)
	{
		flat_set<DayIndex> existing;
		list<fs::path> consolodate;
		const DateTime today{ DateTime::Today() };
		auto fnctn = [&]( path path, DayIndex fileStart, DayIndex fileEnd )
		{
			if( Str::EndsWith(path.stem().stem().string(), "_partial") /*|| (end2==18269 && path.stem().stem().string()=="2019")*/  )
			{
				var pExisting = pPartials ? Jde::Find( *pPartials, path.string() ) : nullptr;
				auto pPartial = pExisting ? pExisting : BarData::Load( path );
				if( !pPartial )
				{
					fs::remove( path );
					DBG( "removed empty file '{}'"sv, path.string() );
					return;
				}
				if( pPartials && !pExisting )
					pPartials->emplace( path.string(), pPartial );
				for( auto i=0; i<pPartial->days_size(); ++i )
				{
					var day2 = pPartial->days(i).day();
					if( pPartial->days(i).bars_size()>0 )
						existing.emplace( day2 );
				}
			}
			else
			{
				for( auto date=fileStart; date<=fileEnd; date=NextTradingDay(date) )
					existing.emplace( date );
			}
		};
		BarData::ForEachFile( contract, fnctn, start2, end2, prefix );

		return existing;
	}
}