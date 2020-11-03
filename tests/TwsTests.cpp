#include "gtest/gtest.h"
//#include "../../Framework/source/log/server/ServerSink.h"
#include "../../Framework/source/threading/Pool.h"
#include "../../MarketLibrary/source/client/TwsClientSync.h"
#include "../../MarketLibrary/source/types/IBException.h"
#include "../../Tws/source/data/ContractData.h"

#define var const auto
#define _client TwsClientSync::Instance()
//UnitTests:
namespace Jde::Markets
{
	class TwsTests : public ::testing::Test
	{
	protected:
		TwsTests() {}
		~TwsTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	};

	using namespace Chrono;

	// ask for 2 days ago, then last 2 days, then last 3 days.
	struct ReqContractPool : public Threading::TypePool<const Contract>
	{
		typedef Threading::TypePool<const Contract> Base;
		ReqContractPool():Base{8, "ReqContractPool"}{}
		void Execute( sp<const Contract> p )noexcept override
		{
			try
			{
				_client.ReqContractDetails( *(p->ToTws()) ).get();
			}
			catch( const IBException& e )
			{
				ERR( "{} - {}"sv, p->Symbol, e.what() );
			}
		}
	};
	TEST_F(TwsTests, ReqContractDetails)
	{
		var pContracts = Data::ContractData::Load();
		vector<ContractPtr_> contracts;
		for( var& [id,pContract] : *pContracts )
			contracts.push_back( pContract );
		constexpr uint count = 200;
		srand( time(nullptr) );
		ReqContractPool pool;
		for( uint i=0; i<count; ++i )
			pool.Push( contracts[rand() % contracts.size()] );
		pool.Shutdown();
	}
}