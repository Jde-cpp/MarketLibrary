#include "gtest/gtest.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/types/Contract.h"
#include "../../MarketLibrary/source/types/Exchanges.h"
#include <bar.h>

#define var const auto
#define _client TwsClientSync::Instance()
//UnitTests:

	//make sure load from file.
	//combine into day,week,month.
		//if have minute bars, use that else just request.
	//during day cache then save.
	//some kind of cache cleanup.
namespace Jde::Markets
{
	Jde::Markets::BarSize b;
	using Display2=Proto::Requests::Display;
	class HistoricalDataCacheTest : public ::testing::Test
	{
	protected:
		HistoricalDataCacheTest() {/*You can do set-up work for each test here.*/}
	~HistoricalDataCacheTest() override{/*You can do clean-up work that doesn't throw exceptions here.*/}

	void SetUp() override {/*Code here will be called immediately after the constructor (right before each test).*/}
	void TearDown() override {/*Code here will be called immediately after each test (right before the destructor).*/}

	/* Class members declared here can be used by all tests in the test suite*/
	};

	using namespace Chrono;
	// ask for 2 days ago, then last 2 days, then last 3 days.
	TEST_F(HistoricalDataCacheTest, PartialCache)
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
			VectorPtr<ibapi::Bar> pBars = _client.ReqHistoricalDataSync( Contracts::Spy, day, dayCount, EBarSize::Minute, EDisplay::Midpoint, true, true ).get();
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
	//save to file.
	TEST_F(HistoricalDataCacheTest, PartialCache)
	{
		ClearMemoryLog();
		var end = PreviousTradingDay();
		var file = BarData::File( Contracts::Aig );
		
		var start = PreviousTradingDay( end );
		req( start, 1 );
		req( end, 2 );
		req( end, 3, PreviousTradingDay(start) );
	}
	/*
	TEST_F(HistoricalDataCacheTest, DoesXyz)
	{
	// Exercises the Xyz feature of Foo.
	}*/
}