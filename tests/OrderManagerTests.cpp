#include "gtest/gtest.h"
#include "../../Framework/source/threading/Pool.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/types/IBException.h"
#include "../../MarketLibrary/source/TickManager.h"
#ifdef TWS_TESTS
	#include "../../Tws/source/data/ContractData.h"
#endif

#define var const auto
#define _client TwsClientSync::Instance()
//UnitTests:
namespace Jde::Markets
{
	class OrderManagerTests : public ::testing::Test
	{
	protected:
		OrderManagerTests() {}
		~OrderManagerTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	};

	using namespace Chrono;

	Coroutine::Task2 Run2()
	{
		var& contract = Contracts::Spy;
		var priceFields = Tick::PriceFields();
		Tick tick{ contract.Id };
		for( ;; )
		{
			Coroutine::Handle handle;
			const TickManager::TickParams params{ priceFields, tick };
			auto result = co_await TickManager::Subscribe( params, handle );
			DBG( "HasError = {}"sv, result.HasError() );
			try
			{
				auto pNewTick = result.Get<Tick>();
				DBG( "bid={}"sv, pNewTick->Bid );
				tick = *pNewTick;
			}
			catch( const Exception& e )
			{
				DBG( "Error has occured. - {}"sv, e.what() );
				break;
			}
		}
	}

	TEST_F( OrderManagerTests, Adhoc )
	{
		Run2();
		std::this_thread::sleep_for( 6min );
		/*var reqId = RequestId();

		placeOrder( const ::Contract& contract, const ::Order& order )noexcept;
		//TwsClient::reqMktData( reqId, , "", false, false, {} );//456=dividends - https://interactivebrokers.github.io/tws-api/tick_types.html

		_client.placeOrder( *Contracts::Spy.ToTws(),  )*/
	}
}