#include "gtest/gtest.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/data/BarData.h"
#include "../../MarketLibrary/source/data/HistoricalDataCache.h"
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

	void SetUp() override {}
	void TearDown() override {}
	};

	using namespace Chrono;
	// ask for 2 days ago, then last 2 days, then last 3 days.
	TEST_F( HistoricalDataCacheTest, PartialCache )
	{
		auto check = []( DayIndex day )
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
		auto req = [check]( DayIndex day, DayIndex dayCount, DayIndex checkDay=0 )
		{
			auto pBars = Future<vector<::Bar>>( TwsClientCo::HistoricalData( make_shared<Contract>(Contracts::Spy), day, dayCount, EBarSize::Minute, EDisplay::Midpoint, true) ).get();
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

	TEST_F( HistoricalDataCacheTest, SaveToFile )
	{
		ClearMemoryLog();
		var& contract = Contracts::Aig;
		var testFrom = DaysSinceEpoch( DateTime{2020,1,1} );
		var dates = BarData::FindExisting( Contracts::Aig, testFrom );
		bool tested = false;
		var start = IsOpen( contract ) ? PreviousTradingDay() : CurrentTradingDay();
		for( auto day = start; !tested && day>testFrom; day=PreviousTradingDay(day) )
		{
			var file = BarData::File( contract, day );
			tested = !fs::exists( file );
			if( !tested )
				continue;
			auto pBars = Future<vector<::Bar>>( TwsClientCo::HistoricalData(make_shared<const Contract>(contract), day, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
			ASSERT_GT( pBars->size(), 0 );
			ASSERT_TRUE( fs::exists(file) );
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
			ASSERT_EQ( actual.volume, expected.volume );
		DBG( "actual.time={}, actual={}, expected={}, diff={}"sv, actual.time, actual.volume, expected.volume, actual.volume-expected.volume );
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
	TEST_F( HistoricalDataCacheTest, LoadFromFile )
	{
		var& contract = Contracts::Aig;
		//compare with non-cache.
		var dates = BarData::FindExisting( contract, PreviousTradingDay()-30 ); ASSERT_GT( dates.size(), 0 );
		var day = *dates.rbegin();
		ClearMemoryLog();
		auto pCache = Future<vector<::Bar>>( TwsClientCo::HistoricalData(make_shared<const Contract>(contract), day, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );

		var pNoCache = Future<vector<::Bar>>( TwsClientCo::HistoricalData(make_shared<const Contract>(contract), day, 1, EBarSize::Minute, EDisplay::Trades, true) ).get();
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 1 );

		CompareBars( *pCache, *pNoCache, false );
	}

	TEST_F( HistoricalDataCacheTest, DayBars )
	{
		var& contract = Contracts::SH;
		var day = PreviousTradingDay();
		auto pBars = Future<vector<::Bar>>( TwsClientCo::HistoricalData( make_shared<Contract>(contract), day, 1, EBarSize::Day, EDisplay::Trades, true) ).get();
		ASSERT_GT( pBars->size(), 0 );
		ClearMemoryLog();
		pBars = Future<vector<::Bar>>( TwsClientCo::HistoricalData( make_shared<Contract>(contract), day, 1, EBarSize::Day, EDisplay::Trades, true) ).get();
		var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
		ASSERT_EQ( logs.size(), 0 );
	}

//Test 3 days of SPY.
//Test a week of SPY.
	TEST_F(HistoricalDataCacheTest, StdDev)
	{
		var test = []( const HistoricalDataCache::StatCount& actual, const HistoricalDataCache::StatCount& expected )
		{
			var near2 = 1e-9;
		//	switch (0) case 0: default: if (const ::testing::AssertionResult gtest_ar = (::testing::internal::DoubleNearPredFormat("actual.Average", "expected.Average", "", actual.Average, expected.Average,))) ; else return ::testing::internal::AssertHelper(::testing::TestPartResult::kFatalFailure, "C:\\Users\\duffyj\\source\\repos\\jde\\MarketLibrary\\tests\\HistoricalDataCacheTest.cpp", 146, gtest_ar.failure_message()) = ::testing::Message() ;
			ASSERT_NEAR( actual.Average, expected.Average, near2 );
			ASSERT_NEAR( actual.Variance, expected.Variance, near2 );
			ASSERT_EQ( actual.Count, expected.Count );
			ASSERT_NEAR( actual.Min, expected.Min, near2 );
			ASSERT_NEAR( actual.Max, expected.Max, near2 );
		};
		var contract{ make_shared<const Contract>(Contracts::Spy) };
		auto pValues = Future<HistoricalDataCache::StatCount>( HistoricalDataCache::ReqStats(contract, 5, DaysSinceEpoch(DateTime{2019,1,2}.GetTimePoint()), DaysSinceEpoch(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{1.004734673, 0.0002306706838, 0.9463821274, 1.04559958}, 248} );

		pValues = Future<HistoricalDataCache::StatCount>( HistoricalDataCache::ReqStats(contract, 90/390.0, DaysSinceEpoch(DateTime{2019,1,2}.GetTimePoint()), DaysSinceEpoch(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{1.00012755, 0.000006148848795, 0.9905072578, 1.007764935}, 252} );

		pValues = Future<HistoricalDataCache::StatCount>( HistoricalDataCache::ReqStats(contract, 14, DaysSinceEpoch(DateTime{2019,1,2}.GetTimePoint()), DaysSinceEpoch(DateTime{2019,12,31}.GetTimePoint())) ).get();
		test( *pValues, {{1.008462279, .000361840218, .9486596698, 1.054081344}, 244} );
	}

	TEST_F(HistoricalDataCacheTest, CombineMinuteBars)
	{
		var contract = make_shared<const Contract>( Contracts::Aig );
		var dates = BarData::FindExisting( *contract, PreviousTradingDay()-30 ); ASSERT_GT( dates.size(), 0 );
		var day = *dates.rbegin();

		ClearMemoryLog(); //2020-10-02T13:30:00, 2020-10-02T14:00:00Z
		auto pCache = Future<vector<::Bar>>( TwsClientCo::HistoricalData( contract, day, 1, EBarSize::Hour, EDisplay::Trades, true) ).get();
		for( auto bar : *pCache )
			Dbg( DateTime{ConvertIBDate(bar.time)}.ToIsoString() );
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 0 );

		ClearMemoryLog();
		//auto pNoCache = _client.ReqHistoricalDataSync( contract, day, 1, EBarSize::Hour, EDisplay::Trades, true, false ).get();
		auto pNoCache = Future<vector<::Bar>>( TwsClientCo::HistoricalData( contract, day, 1, EBarSize::Hour, EDisplay::Trades, true) ).get();
		//for( auto bar : *pNoCache )
		//	DBG0( DateTime{ConvertIBDate(bar.time)}.ToIsoString() );
		ASSERT_EQ( FindMemoryLog(TwsClient::ReqHistoricalDataLogId).size(), 1 );

		CompareBars( *pCache, *pNoCache, false );//volume is off by a little
	}
	TEST_F(HistoricalDataCacheTest, LoadOptions)
	{
		auto check = []( DayIndex day )
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
		auto req = [check]( ContractPtr_ contract, DayIndex day, DayIndex dayCount, DayIndex checkDay=0 )
		{
			auto pBars = Future<vector<::Bar>>( TwsClientCo::HistoricalData( contract, day, dayCount, EBarSize::Hour, EDisplay::Trades, true) ).get();
			//ASSERT_GT( pBars->size(), 0 );
			check( checkDay ? checkDay : day );
		};
		Contract contract; contract.Symbol = "SPY"; contract.SecType=SecurityType::Option; contract.Right=SecurityRight::Call; contract.Strike = 330; contract.Expiration = 19377; contract.Currency = Proto::Currencies::UsDollar;
		var pDetails = Future<const Contract>( TwsClientCo::ContractDetails(contract.ToTws()) ).get();
		//var& ibContract = pDetails->contract;
		ClearMemoryLog();
		var end = PreviousTradingDay();
		var start = PreviousTradingDay( end );

		req( pDetails, start, 1 );
		req( pDetails, end, 2 );
		req( pDetails, end, 3, PreviousTradingDay(start) );
	}
}