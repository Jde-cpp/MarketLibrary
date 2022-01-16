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
			pInstance->CreateClient( Settings::Getɛ<uint>("tws/clientId") );
		}
		catch( const Jde::Exception& e )
		{
			pInstance = sp<WrapperSync>{};
			std::cout << "exiting:  " << e.what() << std::endl;
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
		ASSERT( Settings::Get<size_t>("workers/drive/threads") );//main thread busy with tests, linux has uint in global ns
		var pFilter=Settings::Get<string>( "testing/tests" );
		::testing::GTEST_FLAG( filter ) = pFilter ? *pFilter : "*";
	   result = RUN_ALL_TESTS();
		p->Shutdown();
	}
	IApplication::Shutdown();
	std::cout << "finished;" << endl;
	return result;
}