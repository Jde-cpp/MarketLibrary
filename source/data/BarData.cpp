#include "BarData.h"
#include <bar.h>
#include <jde/Str.h>
#include <jde/Assert.h>
#include <jde/io/File.h>
#include <jde/markets/types/Contract.h>

#include "../../../Framework/source/collections/Collections.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../XZ/source/XZ.h"
#include "../client/TwsClientSync.h"
#include "../types/Bar.h"
#include "../types/Exchanges.h"
#pragma warning( disable : 4244 )
#include "../types/proto/bar.pb.h"
#pragma warning( default : 4244 )

#define var const auto
#define _client TwsClientSync::Instance()

namespace Jde::Markets
{
	using namespace Chrono;
	static var& _logLevel{ Logging::TagLevel("mrk-hist") };
	using ResultsFunction=function<void(const map<Day,VectorPtr<CandleStick>>&,Day,Day)>;

	fs::path BarData::Path()noexcept(false)
	{
		var path = Settings::Getɛ<fs::path>( "marketHistorian/barPath" );
		return path.empty() ? IApplication::Instance().ApplicationDataFolder()/"securities" : path;
	}
	fs::path BarData::Path( const Contract& contract )noexcept(false){ ASSERT(contract.PrimaryExchange!=Exchanges::Smart) return Path()/Str::ToLower(string{ToString(contract.PrimaryExchange)})/Str::Replace(contract.Symbol, " ", "_"); }

	sp<Proto::BarFile> BarData::Load( path path )noexcept(false)
	{
		return Future<Proto::BarFile>( IO::Zip::XZ::ReadProto<Proto::BarFile>(path) ).get();
	}

	α BarData::Load( fs::path path_, string symbol2 )noexcept->AWrapper
	{
		return AWrapper{ [path=move(path_), symbol=move(symbol2)]( HCoroutine h )->Task
		{
			try
			{
				LOG( "Reading {}", path.string() );
				AwaitResult t = co_await IO::Zip::XZ::ReadProto<Proto::BarFile>( move(path) );
				var pFile = t.SP<Proto::BarFile>();
				Day dayCount = pFile->days_size();
				auto pResults = make_shared<map<Day,VectorPtr<CandleStick>>>();
				for( Day i=0; i<dayCount; ++i )
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
				h.promise().get_return_object().SetResult( pResults );
			}
			catch( IException& e )
			{
				h.promise().get_return_object().SetResult( e.Clone() );
			}
			h.resume();
		}};
	}


	α BarData::Load( path path, sv symbol, const flat_map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)->sp<flat_map<Day,VectorPtr<CandleStick>>>
	{
		auto pResults = ms<flat_map<Day,VectorPtr<CandleStick>>>();

		var pFile = pPartials && pPartials->find(path.string())!=pPartials->end() ? pPartials->find(path.string())->second : Load( path );
		if( !pFile )
			return pResults;

		Day dayCount = pFile->days_size();
		for( Day i=0; i<dayCount; ++i )
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
	static uint16 currentYear=DateTime{}.Year();
	tuple<Day,Day> ExtractTimeframe( path path, TimePoint earliestDate, uint8 yearsToKeep=3 )
	{
		var [year,month,day] = IO::FileUtilities::ExtractDate( path );
		if( year==0 )
			return make_tuple( 0, 0 );

		var start = CurrentTradingDay( year==currentYear-yearsToKeep ? earliestDate : ToTimePoint(year, std::max(month,(uint8)1), std::max(day,(uint8)1)) );
		var end = day ? start+24h-1s : EndOfMonth( ToTimePoint(year, month ? month : 12, 1) );
		ASSERT( start<end );
		return make_tuple( ToDays(start), ToDays(end) );
	}

	vector<tuple<fs::path,Day,Day>> ApplicableFiles( path root, TimePoint issueDate, Day start, Day endInput, sv prefix={} )
	{
		vector<tuple<fs::path,Day,Day>> paths;
		if( !fs::exists(root) )
			return paths;
		var pEntries = IO::FileUtilities::GetDirectory( root );
		var end = endInput ? endInput : std::numeric_limits<Day>::max();
		for( var& entry : *pEntries )
		{
			var path = entry.path();
			if( (prefix.size() && path.stem().string().find(prefix)!=0) || (!prefix.size() && std::isalpha(path.stem().string()[0])) )
				continue;
			if( issueDate==TimePoint::max() )
				issueDate = DateTime{2010,1,1}.GetTimePoint();
			ASSERT( issueDate!=TimePoint::max() );
			var [fileStart,fileEnd] = ExtractTimeframe( path, issueDate );
			CONTINUE_IF( fileEnd==0, "could not find end to '{}' prefix='{}'", path.stem().string(), prefix );
			if( start<=fileEnd && end>=fileStart )
				paths.push_back( make_tuple(path, fileStart, fileEnd) );
		}
		return paths;
	}
	struct BarFilesAwaitable : IAwait
	{
		using base=IAwait;
		BarFilesAwaitable( const Contract& contract, Day start, Day endInput, ResultsFunction f )noexcept(false):
			_function{ f },
			_files{ ApplicableFiles(BarData::Path(contract), contract.IssueDate, start, endInput) },
			_symbol{ contract.Symbol }
		{
			LOG( "({})BarFilesAwaitable::BarFilesAwaitable( {}, {}, fileSize={} )", _symbol, DateDisplay(start), DateDisplay(endInput), _files.size() );
		}
		~BarFilesAwaitable(){ LOG( "({})~BarFilesAwaitable()", _symbol ); }
		α await_ready()noexcept->bool override{ return _files.empty(); }
		α await_suspend( HCoroutine h )noexcept->void override
		{
			base::await_suspend( h );
			_h = move( h );
			auto f = [this]()mutable->Task
			{
				for( var [path,start,end] : _files )
				{
					try
					{
						auto a = BarData::Load( move(path), _symbol );
						auto result = co_await a;
						auto pData = result.SP<map<Day,VectorPtr<CandleStick>>>();
						_function( *pData, start, end );
					}
					catch( IException& e )
					{
						try
						{
							if( IO::FileSize(path)>0 ) e.Throw();
							fs::remove( path ); INFO( "{} empty removing", path );
						}
						catch( IException& e )
						{
							_pException = e.Clone();
							break;
						}
					}
				}
				_h.resume();
			};
			f();
		}
		α await_resume()noexcept->AwaitResult override
		{
			AwaitResume();
			return _pException ? AwaitResult{ _pException } : AwaitResult{ sp<void>{} };
		}
	private:
		ResultsFunction _function;
		vector<tuple<fs::path,Day,Day>> _files;
		sp<IException> _pException;
		Coroutine::HCoroutine _h;
		string _symbol;
	};
	α ForEachFile( const Contract& contract, Day start, Day endInput, ResultsFunction f )noexcept{ return BarFilesAwaitable{contract, start, endInput, f}; }
	α BarData::ForEachFile( const Contract& contract, FileFunction fnctn, Day start, Day endInput, sv prefix )noexcept->void//fnctn better not throw
	{
		for( var& [path,fileStart,fileEnd] : ApplicableFiles(BarData::Path(contract), contract.IssueDate, start, endInput, prefix) )
			fnctn( path, fileStart, fileEnd );
	}

	α BarData::TryLoad( const Contract& contract, Day start, Day end )noexcept->sp<flat_map<Day,VectorPtr<CandleStick>>>
	{
		try
		{
			return Load( contract, start, end );
		}
		catch( const IException& )
		{}
		return ms<flat_map<Day,VectorPtr<CandleStick>>>();
	}
	α BarData::Load( const Contract& contract, Day start, Day end )noexcept(false)->sp<flat_map<Day,VectorPtr<CandleStick>>>
	{
		auto pResults = ms<flat_map<Day,VectorPtr<CandleStick>>>();
		auto fnctn = [&]( path path, Day, Day )noexcept
		{
			var pFileValues = Load( path, (sv)contract.Symbol );
			for( var& [day,pBars] : *pFileValues )
			{
				if( day>=start && day<=end )
					pResults->try_emplace( pResults->end(), day, pBars );
			}
		};
		ForEachFile( contract, fnctn, start, end );
		return pResults;
	}
	α BarData::CoLoad( ContractPtr_ pContract, Day start, Day end )noexcept(false)->AWrapper
	{
		return AWrapper( [pContract, start, end]( HCoroutine h )->Task
		{
			LOG( "BarData::CoLoad( ({}) {}-{} )", pContract->Symbol, DateDisplay(start), DateDisplay(end) );
			auto p = new map<Day,VectorPtr<CandleStick>>();
			auto f = [pResults=p,start,end]( const map<Day,VectorPtr<CandleStick>>& bars, Day, Day )
			{
				for( var& dayBars : bars )
				{
					var day = dayBars.first; var pBars = dayBars.second;
					if( day>=start && day<=end )
						pResults->try_emplace( pResults->end(), day, pBars );
				}
			};
			try
			{
				( co_await ForEachFile(*pContract, start, end, f) ).CheckError();
				h.promise().get_return_object().SetResult( make_shared<map<Day,VectorPtr<CandleStick>>>(*p) );
			}
			catch( IException& e )
			{
				h.promise().get_return_object().SetResult( e.Clone() );
			}
			h.resume();
		} );
	}

	void BarData::Save( const Contract& contract, flat_map<Day,vector<sp<::Bar>>>& rthBars )noexcept
	{
		var current = CurrentTradingDay( contract );
		var exclude = Clock::now()>ExtendedEnd( contract, current ) ? 0 : current;
		flat_map<Day,VectorPtr<CandleStick>> days;
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
		auto fnctn = [&]( path path, Day, Day )noexcept
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

	α BarData::Save( const Contract& contract, const flat_map<Day,VectorPtr<CandleStick>>& days, VectorPtr<tuple<TimePoint,optional<TimePoint>>> pExcluded, bool checkExisting, const flat_map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)->void
	{
		const DateTime now{ CurrentTradingDay(Clock::now()) };
		auto getFileName = [&now]( const DateTime& itemDate )->string
		{
			var year = std::max( now.Year()-3, itemDate.Year() );
			var month = now.Year()==itemDate.Year() ? itemDate.Month() : 0;
			auto day = month==0 || now.Month()!=itemDate.Month() ? 0 : itemDate.Day();
			return IO::FileUtilities::DateFileName( year, month, day );
		};
		auto addDay = []( Proto::BarFile& proto, Day day, const vector<CandleStick>& bars )
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
			set<Day> existingDays;
			set<string> combinedFiles;
			auto addExisting = [&,&proto2=proto]( path path, Day _=0, Day _2=0)noexcept
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
						var startExcludedDay = ToDays( startEx );
						var endExcludedDay = ToDays( endEx.value_or(TimePoint{}) );
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
			catch( fs::filesystem_error& e )
			{
				throw IOException( move(e) );
			}
		}
	}

	α BarData::FindExisting( const Contract& contract, Day start2, Day end2, sv prefix, flat_map<string,sp<Proto::BarFile>>* pPartials )noexcept(false)->flat_set<Day>
	{
		flat_set<Day> existing;
		const DateTime today{ DateTime::Today() };
		auto fnctn = [&]( path path, Day fileStart, Day fileEnd )
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

	α BarData::Combine( const Contract& contract, Day day, vector<sp<Bar>>& fromBars, EBarSize toBarSize, EBarSize fromBarSize, bool useRth )->VectorPtr<sp<::Bar>>
	{
		ASSERT( toBarSize%fromBarSize==0 );
		auto pToBars = ms<vector<sp<Bar>>>();
		var start = useRth ? RthBegin( contract, day ) : ExtendedBegin( contract, day );
		var end = Min( Clock::now(), useRth ? RthEnd(contract, day) : ExtendedEnd(contract, day) );
		auto ppBar = fromBars.begin();
		var duration = toBarSize==EBarSize::Day ? end-start : BarSize::BarDuration( toBarSize );  ASSERT( toBarSize<=EBarSize::Day );
		var minuteCount = std::chrono::duration_cast<std::chrono::minutes>( duration ).count();
		for( auto barStart = start, barEnd=Clock::from_time_t( (Clock::to_time_t(start)/60+minuteCount)/minuteCount*minuteCount*60 ); barEnd<=end; barStart=barEnd, barEnd+=duration )//1st barEnd for hour is 10am not 10:30am
		{
			::Bar combined{ ToIBDate(barStart), 0, std::numeric_limits<double>::max(), 0, 0, 0, 0, 0 };
			double sum = 0;
			for( ;ppBar!=fromBars.end() && Clock::from_time_t(ConvertIBDate((*ppBar)->time))<barEnd; ++ppBar )
			{
				var& bar = **ppBar;
				combined.high = std::max( combined.high, bar.high );
				combined.low = std::min( combined.low, bar.low );
				if( combined.open==0.0 )
					combined.open = bar.open;
				combined.close = bar.close;
				sum = ToDouble( bar.wap )*ToDouble( bar.volume );
				combined.volume += bar.volume;
				combined.count += bar.count;
			}
			if( combined.low!=std::numeric_limits<double>::max() )
			{
				combined.wap = ToDecimal( sum/ToDouble(combined.volume) );
				pToBars->push_back( ms<::Bar>(combined) );
			}
		}
		return pToBars;
	}
}