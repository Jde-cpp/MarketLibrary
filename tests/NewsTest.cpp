#include "gtest/gtest.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
// #include "../../MarketLibrary/source/data/BarData.h"
// #include "../../MarketLibrary/source/types/Contract.h"
// #include "../../MarketLibrary/source/types/Exchanges.h"
#include <NewsProvider.h>

#define var const auto
#define _client TwsClientSync::Instance()
//UnitTests:
namespace Jde::Markets
{
	class NewsTest : public ::testing::Test
	{
	protected:
		NewsTest() {}
		~NewsTest() override{}

		void SetUp() override {}
		void TearDown() override {}
	};

	using namespace Chrono;

	// ask for 2 days ago, then last 2 days, then last 3 days.
	TEST_F(NewsTest, Providers)
	{
		ClearMemoryLog();
		auto pProviders = _client.RequestNewsProviders( 0 ).get();
		ASSERT_GT( pProviders->size(), 0 );
		ClearMemoryLog();
		pProviders = _client.RequestNewsProviders( 0 ).get();
		var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
		ASSERT_EQ( logs.size(), 0 );

		//var logs = FindMemoryLog( TwsClient::ReqHistoricalDataLogId );
		//ASSERT_EQ( logs.size(), 1 );

/*		var end = PreviousTradingDay();
		var start = PreviousTradingDay( end );
		req( start, 1 );
		req( end, 2 );
		req( end, 3, PreviousTradingDay(start) );*/
	}
}