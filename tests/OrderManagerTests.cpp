#include "gtest/gtest.h"
//#include "../../Framework/source/log/server/ServerSink.h"
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

	Coroutine::TaskError<Jde::Markets::Tick> Run2()
	{
		var& contract = Contracts::Spy;
		var priceFields = Tick::PriceFields();
		Tick tick{ contract.Id };
		for( ;; )
		{
			Coroutine::Handle handle;
			const TickManager::TickParams params{ priceFields, tick };
//coroutine_handle<Jde::Coroutine::Task<std::__1::variant<Jde::Markets::Tick, std::exception_ptr>>::promise_type>
//coroutine_handle<Jde::Coroutine::CancelAwaitable<Jde::Coroutine::TaskError<Jde::Markets::Tick>>::TPromise>'
			auto result = co_await TickManager::Subscribe( params, handle );
			DBG( "Index = {}"sv, result.index() );
			if( result.index()==1 )
			{
				try
				{
					std::rethrow_exception( std::get<std::exception_ptr>(result) );
				}
				catch( const Exception& e )
				{
					DBG( "Error has occured. - {}"sv, e.what() );
				}
				break;
			}
			auto newTick = std::get<Jde::Markets::Tick>( result );
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