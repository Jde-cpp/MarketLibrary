#include "gtest/gtest.h"
#include "../source/wrapper/WrapperSync.h"
#include "../../Framework/source/Settings.h"
#define var const auto

#ifndef _MSC_VER
namespace Jde{  string OSApp::CompanyName()noexcept{ return "Jde-Cpp"; } }
#endif

namespace Jde::Markets
{
 	α Startup( int argc, char **argv )noexcept->sp<WrapperSync>
	{
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, "Tests.MarketLibrary", "Test program" );
		auto pInstance = make_shared<WrapperSync>();
		try
		{
			pInstance->CreateClient( Settings::Get<uint>("tws/clientId") );
		}
		catch( const Jde::Exception& e )
		{
			pInstance = sp<WrapperSync>{};
		}
		return pInstance;
	}
}

α main( int argc, char **argv )->int
{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	auto result = EXIT_FAILURE;
	if( auto p = Markets::Startup(argc, argv); p )
	{
		ASSERT( Settings::TryGet<uint>("workers/drive/threads") );//main thread busy with tests
		var pFilter=Settings::TryGet<string>( "testing/tests" );
		::testing::GTEST_FLAG( filter ) = pFilter ? *pFilter : "*";
	   result = RUN_ALL_TESTS();
		p->Shutdown();
	}
	IApplication::CleanUp();

	return result;
}