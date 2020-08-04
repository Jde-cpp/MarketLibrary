#include "gtest/gtest.h"
#include "../../MarketLibrary/source/client/TwsClientCache.h"
#include "../../MarketLibrary/source/types/Exchanges.h"

#define var const auto
//UnitTests:
	// ask for week ago, then last 2 weeks, then last 3 weeks.
	//2x 1 load.
	//save to file.
	//make sure load from file.
	//combine into day,week,month.
		//if have minute bars, use that else just request.
	//during day cache then save.
	//some kind of cache cleanup.
namespace Jde::Markets
{
	class HistoricalDataCacheTest : public ::testing::Test
	{
	protected:
		HistoricalDataCacheTest() {/*You can do set-up work for each test here.*/}
	~HistoricalDataCacheTest() override{/*You can do clean-up work that doesn't throw exceptions here.*/}

	void SetUp() override {/*Code here will be called immediately after the constructor (right before each test).*/}
	void TearDown() override {/*Code here will be called immediately after each test (right before the destructor).*/}

	/* Class members declared here can be used by all tests in the test suite*/
	};


	TEST_F(HistoricalDataCacheTest, LoadTwice)
	{
		var prev = PreviousTradingDay();
		//TwsClientCache::Loa
		//ReqHistoricalData( TickerId reqId, const Contract& contract, DayIndex current, DayIndex dayCount, Proto::Requests::BarSize barSize, TwsDisplay::Enum display, bool useRth )
		const std::string input_filepath = "this/package/testdata/myinputfile.dat";
		const std::string output_filepath = "this/package/testdata/myoutputfile.dat";
	//	Foo f;
		EXPECT_NE( input_filepath, output_filepath );
	}
	/*
	TEST_F(HistoricalDataCacheTest, DoesXyz)
	{
	// Exercises the Xyz feature of Foo.
	}*/
}