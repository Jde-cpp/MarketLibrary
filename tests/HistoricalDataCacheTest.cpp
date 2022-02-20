#include "gtest/gtest.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../Framework/source/math/MathUtilities.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/data/BarData.h"
#include "../../MarketLibrary/source/data/HistoricalDataCache.h"
#include "../../MarketLibrary/source/data/StatAwait.h"
#include <jde/markets/types/Contract.h>
#include "../../MarketLibrary/source/types/Exchanges.h"
#include <bar.h>

#define var const auto
//#define _client TwsClientSync::Instance()
//UnitTests:
	//during day cache then save.
	//some kind of cache cleanup.
	//test no data, like future date.
	//1 year gold, day bars
	//test options.
namespace Jde::Markets
{
	Proto::Requests::BarSize b;
	using Display2=Proto::Requests::Display;
	class HistoricalDataCacheTest : public ::testing::Test
	{
	protected:
		HistoricalDataCacheTest() {}
		~HistoricalDataCacheTest() override{}

		α SetUp()noexcept->void override{Logging::SetTag( "tws-requests" ); }
		α TearDown()noexcept->void override {}
	};

	using namespace Chrono;
	// ask for 2 days ago, then last 2 days, then last 3 days.
	TEST_F( HistoricalDataCacheTest, PartialCache )
	{
		sp<::ContractDetails> pContract = SFuture<::ContractDetails>( Tws::ContractDetail(Contracts::Spy.Id) ).get();
		auto check = []( Day day )
		{
			var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
			ASSERT_EQ( logs.size(), 1 );
			var& log = logs[0];
			var date = DateTime{ FromDays(day) };
			ASSERT_EQ( log.Variables[2], format("{}{:0>2}{:0>2} 23:59:59 GMT", date.Year(), date.Month(), date.Day()) );
			ASSERT_EQ( log.Variables[3], "1 D" );
			ASSERT_EQ( log.Variables[4], "1 min" );
			ClearMemoryLog();
		};
		auto req = [check, pContract]( Day day, Day dayCount, Day checkDay=0 )
		{
			auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<Contract>(*pContract), day, dayCount, EBarSize::Minute, EDisplay::Midpoint, true) ).get();
			ASSERT_GT( pBars->size(), 0 );
			check( checkDay ? checkDay : day );
		};
		ClearMemoryLog();
		var end = PreviousTradingDay();
		var start = PreviousTradingDay( end );
		req( start, 1 );
		req( end, 2 );
		req( end, 3, PreviousTradingDay(start) );
	}

	α True( bool result, str msg, SRCE )->void
	{
		if( !result )
			Dbg( msg, sl );
		ASSERT_TRUE( result );
	}

	TEST_F( HistoricalDataCacheTest, SaveToFile )
	{
		ClearMemoryLog();
		var pContract = SFuture<::ContractDetails>( Tws::ContractDetail(Contracts::Xom.Id) ).get();
		const Contract contract{ *pContract };
		var testFrom = ToDays( DateTime{2020,1,1} );
		var dates = BarData::FindExisting( Contracts::Aig, testFrom );
		bool tested = false;
		var start = IsOpen( contract ) ? PreviousTradingDay() : CurrentTradingDay();
		var month = DateTime{ FromDays(start) }.Month();
		for( auto day = start; !tested && day>testFrom; day=PreviousTradingDay(day) )
		{
			var file = BarData::File( contract, day );
			if( fs::exists(file) )
				continue;
			var testMonth = DateTime{ FromDays(day) }.Month();
			if( testMonth<month )
			{
				fs::remove( BarData::File(contract, start) );
				day = NextTradingDay( start );
				continue;
			}
			tested = true;
			auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), day, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
			ASSERT_GT( pBars->size(), 0 );
			True( fs::exists(file), file.string() );//needs to work on prior month
		}
		EXPECT_TRUE( tested );
	}

	static bool _testCompareBar=true;
	void CompareBar( const ::Bar& actual, const ::Bar& expected, bool compareVolume=true )noexcept
	{
		_testCompareBar = false;
		ASSERT_EQ( actual.time, expected.time );
		ASSERT_NEAR( actual.high, expected.high, .0001 );
		ASSERT_NEAR( actual.low, expected.low, .0001 );
		ASSERT_NEAR( actual.open, expected.open, .0001 );
		ASSERT_NEAR( actual.close, expected.close, .0001 );
		//ASSERT_NEAR( actual.wap, expected.wap, .0001 );
		if( compareVolume )
			ASSERT_EQ( Round(ToDouble(actual.volume)), Round(ToDouble(expected.volume)) );
		//DBG( "actual.time={}, actual={}, expected={}, diff={}"sv, actual.time, ToDouble(actual.volume), ToDouble(expected.volume), ToDouble(Subtract(actual.volume,expected.volume)) );
		//ASSERT_EQ( actual.count, expected.count );
		_testCompareBar = true;
	}

	void CompareBars( const vector<::Bar>& actual, const vector<::Bar>& expected, bool compareVolume=true )noexcept
	{
		ASSERT_EQ( actual.size(), expected.size() ); ASSERT_GT( actual.size(), 0 );
		_testCompareBar = true;
		for( uint i=0; i<actual.size() && _testCompareBar; ++i )
			CompareBar( actual[i], expected[i], compareVolume );
	}
	struct NoCache
	{
		NoCache(){ Settings::Set("marketHistorian/barPath", _tempPath, false); }
		~NoCache(){ Settings::Set( "marketHistorian/barPath", _original, false ); fs::remove_all( _tempPath ); }
		fs::path _original{ Settings::Getɛ<fs::path>("marketHistorian/barPath") };
		fs::path _tempPath{ fs::temp_directory_path()/"unitTests" };
	};

	TEST_F( HistoricalDataCacheTest, LoadFromFile )
	{
		var pContract = SFuture<::ContractDetails>( Tws::ContractDetail(Contracts::Aig.Id) ).get();
		var& contract = *pContract;
		constexpr auto Display = EDisplay::Trades;
		var prev = PreviousTradingDay();
		auto dates = BarData::FindExisting( contract, prev-30 );
		if( dates.size()==0 )
		{
			auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), prev, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
			dates = BarData::FindExisting( contract, prev-30 ); ASSERT_GT( dates.size(), 0 );
		}
		var day = *dates.rbegin();
		ClearMemoryLog();
		auto pFileCache = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), day, 1, EBarSize::Minute, Display, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );
		HistoryCache::Clear( contract.contract.conId, Display );
		NoCache nc;
		var pNoCache = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), day, 1, EBarSize::Minute, Display, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 1 );
		CompareBars( *pFileCache, *pNoCache, false );
	}

	TEST_F( HistoricalDataCacheTest, LoadFromFile2 )
	{
		var pContract = SFuture<::ContractDetails>( Tws::ContractDetail(Contracts::Tsla.Id) ).get();
		var& contract = *pContract;
		constexpr auto Display = EDisplay::Trades;
		var prev = PreviousTradingDay();
		auto dates = BarData::FindExisting( contract, prev-30 );
		if( dates.size()==0 )
		{
			auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), prev, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
			dates = BarData::FindExisting( contract, prev-30 ); ASSERT_GT( dates.size(), 0 );
		}
		var day = *dates.rbegin();
		ClearMemoryLog();
		auto pFileCache = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), day, 1, EBarSize::Minute, Display, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );
		HistoryCache::Clear( contract.contract.conId, Display );
		NoCache nc;
		var pNoCache = SFuture<vector<::Bar>>( Tws::HistoricalData(ms<const Contract>(contract), day, 1, EBarSize::Minute, Display, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 1 );
		CompareBars( *pFileCache, *pNoCache, false );
	}
	TEST_F( HistoricalDataCacheTest, DayBars )
	{
		var pContract = SFuture<::ContractDetails>( Tws::ContractDetail(Contracts::SH.Id) ).get();
		var& contract = *pContract;
		var day = PreviousTradingDay();
		auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData( ms<Contract>(contract), day, 1, EBarSize::Day, EDisplay::Trades, true) ).get();
		ASSERT_GT( pBars->size(), 0 );
		ClearMemoryLog();
		pBars = SFuture<vector<::Bar>>( Tws::HistoricalData( ms<Contract>(contract), day, 1, EBarSize::Day, EDisplay::Trades, true) ).get();
		var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
		ASSERT_EQ( logs.size(), 0 );
	}

	//Test 3 days of SPY.
	//Test a week of SPY.
	α Near( double actual, double expected, double value, SRCE )->void
	{
		if( abs(actual-expected)>value )
			ERR( "x={}", abs(actual-expected) );
		ASSERT_NEAR( actual, expected, value );
	}
	TEST_F(HistoricalDataCacheTest, StdDev)
	{
		var test = []( const StatCount& actual, const StatCount& expected )
		{
			var near2 = 1e-6;
		//	switch (0) case 0: default: if (const ::testing::AssertionResult gtest_ar = (::testing::internal::DoubleNearPredFormat("actual.Average", "expected.Average", "", actual.Average, expected.Average,))) ; else return ::testing::internal::AssertHelper(::testing::TestPartResult::kFatalFailure, "C:\\Users\\duffyj\\source\\repos\\jde\\MarketLibrary\\tests\\HistoricalDataCacheTest.cpp", 146, gtest_ar.failure_message()) = ::testing::Message() ;
			Near( actual.Average, expected.Average, near2 );
			Near( actual.Variance, expected.Variance, near2 );
			ASSERT_EQ( actual.Count, expected.Count );
			Near( actual.Min, expected.Min, near2 );
			Near( actual.Max, expected.Max, near2 );
		};
		var pContract{ SFuture<::ContractDetails>(Tws::ContractDetail(Contracts::Spy.Id)).get() };
		var contract{ ms<const Contract>(*pContract) };
		//_pTws->reqHistoricalData( id, *_contractPtr->ToTws(), endTimeString, format("{} D", _dayCount), string{BarSize::ToString(_barSize)}, string{TwsDisplay::ToString(_display)}, _useRth ? 1 : 0, 2, false, TagValueListSPtr{} );
/*		var tempPath = fs::temp_directory_path()/"unitTests";
		Settings::Set( "marketHistorian/barPath", tempPath );
		SFuture<vector<::Bar>>( Tws::HistoricalData(contract, ToDays(DateTime{2019,1,8}), 1, EBarSize::Day, EDisplay::Trades, true) ).get();
		var pNoCache = SFuture<vector<::Bar>>( Tws::HistoricalData(contract, ToDays(DateTime{2019,1,8}), 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
		var bar = pNoCache->back();
		//DBG( "2019,1,9 '{}' - count: '{}', volume: '{}', wap: '{}', open: '{:.2f}', close: '{:.2f}', high: '{:.2f}', low: '{:.2f}'", Chrono::Display(ConvertIBDate(bar.time)), bar.count, ToDouble(bar.volume), ToDouble(bar.wap), bar.open, bar.close, bar.high, bar.low );
		fs::remove_all( tempPath );
*/
		bool cached = true;
		auto pValues = SFuture<StatCount>( ReqStats(contract, 5, ToDays(DateTime{2019,1,2}.GetTimePoint()), ToDays(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{cached ? 1.0047580 : 1.004734673, cached ? .0002294 : .0002306706838, cached ? .94711575 : .9463821274, cached ? 1.0459227 : 1.04559958}, 248} );

		pValues = SFuture<StatCount>( ReqStats(contract, 90/390.0, ToDays(DateTime{2019,1,2}.GetTimePoint()), ToDays(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{1.00012755, 0.000006148848795, 0.9905072578, 1.007764935}, 252} );

		pValues = SFuture<StatCount>( ReqStats(contract, 14, ToDays(DateTime{2019,1,2}.GetTimePoint()), ToDays(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{ cached ? 1.0080295 : 1.008462279, cached ? 0.0003629049 : .000361840218, cached ? .946934704 : .9486596698, cached ? 1.05569695 : 1.054081344}, 244} );
	}


	TEST_F(HistoricalDataCacheTest, CombineMinuteBars)
	{
		sp<const ::ContractDetails> pContract{ SFuture<::ContractDetails>(Tws::ContractDetail(Contracts::Aig.Id)).get() };
		var contract{ ms<const Contract>(*pContract) };
		var dates = BarData::FindExisting( *contract, PreviousTradingDay()-30 ); ASSERT_GT( dates.size(), 0 );
		ASSERT_GT( dates.size(), 0 );
		var day = *dates.rbegin();

		ClearMemoryLog(); //2020-10-02T13:30:00, 2020-10-02T14:00:00Z
		auto pFileCache = SFuture<vector<::Bar>>( Tws::HistoricalData(contract, day, 1, EBarSize::Hour, EDisplay::Trades, true) ).get();
//		for( auto bar : *pCache )
//			Dbg( DateTime{ConvertIBDate(bar.time)}.ToIsoString() );
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );

		NoCache nc;
		ClearMemoryLog();
		//auto pNoCache = _client.ReqHistoricalDataSync( contract, day, 1, EBarSize::Hour, EDisplay::Trades, true, false ).get();
		auto pMemoryCache = SFuture<vector<::Bar>>( Tws::HistoricalData(contract, day, 1, EBarSize::Hour, EDisplay::Trades, true) ).get();
		//for( auto bar : *pNoCache )
		//	DBG0( DateTime{ConvertIBDate(bar.time)}.ToIsoString() );
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );

		CompareBars( *pFileCache, *pMemoryCache, false );//volume is off by a little
	}
	TEST_F(HistoricalDataCacheTest, LoadOptions)
	{
		auto check = []( Day day )
		{
			var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
			ASSERT_EQ( logs.size(), 1 );
			var& log = logs[0];
			var date = DateTime{ FromDays(day) };
			ASSERT_EQ( log.Variables[2], format("{}{:0>2}{:0>2} 23:59:59 GMT", date.Year(), date.Month(), date.Day()) );
			ASSERT_EQ( log.Variables[3], "1 D" );
			ASSERT_EQ( log.Variables[4], "1 hour" );
			ClearMemoryLog();
		};
		auto req = [check]( ContractPtr_ contract, Day day, Day dayCount, Day checkDay=0 )
		{
			auto pBars = SFuture<vector<::Bar>>( Tws::HistoricalData( contract, day, dayCount, EBarSize::Hour, EDisplay::Trades, true) ).get();
			//ASSERT_GT( pBars->size(), 0 );
			check( checkDay ? checkDay : day );
		};
		Contract contract; contract.Symbol = "SPY"; contract.SecType=SecurityType::Option; contract.Right=SecurityRight::Call; contract.Strike = 430; contract.Expiration = 19377; contract.Currency = Proto::Currencies::UsDollar;
		var pDetails = SFuture<::ContractDetails>( Tws::ContractDetail(*contract.ToTws()) ).get();
		//var& ibContract = pDetails->contract;
		ClearMemoryLog();
		var end = PreviousTradingDay();
		var start = PreviousTradingDay( end );
		auto pContract = ms<Contract>(*pDetails);
		req( pContract, start, 1 );
		req( pContract, end, 2 );
		req( pContract, end, 3, PreviousTradingDay(start) );
	}
}