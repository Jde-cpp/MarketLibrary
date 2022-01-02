#include "gtest/gtest.h"
#include "../../Framework/source/log/server/ServerSink.h"
#include "../../Framework/source/io/ProtoUtilities.h"
#include "../../Framework/source/collections/Collections.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"

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

	std::condition_variable_any cv;
	std::shared_mutex mtx;

	using namespace Chrono;
	auto TestProviders()->Task
	{
		ClearMemoryLog();
		auto pProviders = ( co_await Tws::NewsProviders() ).SP<map<string,string>>();
		ASSERT( FindMemoryLog( TwsClient::ReqNewsProvidersLogId ).size() );
		INFO( "[{}]={}"sv, pProviders->begin()->first, pProviders->begin()->second );
		ASSERT_DESC( pProviders->size(), "pProviders->size()" );
		ClearMemoryLog();
		pProviders = ( co_await Tws::NewsProviders() ).SP<map<string,string>>();
		INFO( "[{}]={}"sv, pProviders->rbegin()->first, pProviders->rbegin()->second );
		var logs = FindMemoryLog( TwsClient::ReqNewsProvidersLogId );
		ASSERT( !logs.size() );

		var providerCodes = Collections::Keys( *pProviders );
		auto wait = ( co_await Tws::ContractDetail(Contracts::Spy.Id) );// if( variant.index()==1 ) std::rethrow_exception( get<1>(variant) ); var pContract = move( get<0>(variant) );
		auto pContract = wait.SP<::ContractDetails>();
		auto pWait = Tws::HistoricalNews( pContract->contract.conId, {providerCodes->begin(), providerCodes->end()}, 10, Clock::now()-std::chrono::days(30), TimePoint{} );
		auto pHistorical = ( co_await pWait ).UP<Proto::Results::NewsCollection>();
		auto pResults = make_unique<Proto::Results::NewsCollection>( *pHistorical );

		std::shared_lock l{ mtx };
		cv.notify_one();
	}
	void TestProviders1()
	{
		TestProviders();
	}
	TEST_F(NewsTest, Providers)
	{
		TestProviders1();
		std::shared_lock l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( 1s );
	}
}