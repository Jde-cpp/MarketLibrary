#include "gtest/gtest.h"
#include "../source/wrapper/WrapperSync.h"
#include "../../Framework/source/Settings.h"
#define var const auto
namespace Jde::Markets
{
	shared_ptr<Settings::Container> SettingsPtr;
 	sp<WrapperSync> Startup()noexcept
	{
		var sv2 = "Tests.MarketLibrary"sv;
		string appName{ sv2 };
		std::filesystem::path settingsPath{ fmt::format("{}.json", appName) };
		if( !fs::exists(settingsPath) )
			settingsPath = std::filesystem::path( fmt::format("../{}.json", appName) );
		Settings::SetGlobal( std::make_shared<Jde::Settings::Container>(settingsPath) );
		InitializeLogger( appName );
		Cache::CreateInstance();

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
	::testing::InitGoogleTest( &argc, argv );
	auto p = Jde::Markets::Startup();
	auto result = EXIT_FAILURE;
	if( p )
	{
		//::testing::GTEST_FLAG(filter) = "NewsTest.*";
	   result = RUN_ALL_TESTS();
		p = nullptr;
	}
	return result;
}