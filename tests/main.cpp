#include "gtest/gtest.h"
#include "../source/wrapper/WrapperSync.h"
#include "../../Framework/source/Settings.h"
#define var const auto
namespace Jde::Markets
{
	shared_ptr<Settings::Container> SettingsPtr;
 	sp<WrapperSync> Startup( int argc, char **argv )noexcept
	{
		var sv2 = "Tests.MarketLibrary"sv;
		string appName{ sv2 };
		OSApp::Startup( argc, argv, appName );

/*		std::filesystem::path settingsPath{ fmt::format("{}.json", appName) };
		if( !fs::exists(settingsPath) )
			settingsPath = std::filesystem::path( fmt::format("../{}.json", appName) );
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		Cache::CreateInstance();
*/
		auto pInstance = make_shared<WrapperSync>();
		try
		{
			pInstance->CreateClient( Settings::Global().Get<uint>("twsClientId") );
		}
		catch( const Jde::Exception& e )
		{
			pInstance = sp<WrapperSync>{};
		}
		return pInstance;
	}
}
template<class T> using sp = std::shared_ptr<T>;
template<typename T>
constexpr auto ms = std::make_shared<T>;

int main(int argc, char **argv)
{
#ifdef _MSC_VER
	 _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
 //   _CrtSetBreakAlloc( 11626 );
	::testing::InitGoogleTest( &argc, argv );
#ifdef _MSC_VER
	auto x = new char[]{"aaaaaaaaaaaaaaaaaaaaaaaaaa"};
#endif
	auto p = Jde::Markets::Startup( argc, argv );
	auto result = EXIT_FAILURE;
	if( p )
	{
		//::testing::GTEST_FLAG(filter) = "OrderManagerTests.Adhoc";
		::testing::GTEST_FLAG(filter) = "NewsTest.Providers";//SaveToFile
	   result = RUN_ALL_TESTS();
		p->Shutdown();
		p = nullptr;
	}
	Jde::IApplication::Instance().Wait();
	Jde::IApplication::CleanUp();

	return result;
}