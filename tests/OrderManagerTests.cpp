#include "gtest/gtest.h"
//#include "../../Framework/source/log/server/ServerSink.h"
#include "../../Framework/source/threading/Pool.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/types/IBException.h"
#include "../../MarketLibrary/source/TickManager.h"
#include "../../Tws/source/data/ContractData.h"

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

	Coroutine::Task<Tick> Run2()
	{
		var& contract = Contracts::Spy;
		var priceFields = Tick::PriceFields();
		Tick tick{ contract.Id };
		for( ;; )
		{
			Coroutine::Handle handle;
			const TickManager::TickParams params{ priceFields, tick };
			var newTick = co_await TickManager::Subscribe( params, handle );
			DBG( "bid={}"sv, newTick.Bid );
			tick = newTick;
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