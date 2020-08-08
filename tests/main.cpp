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
		pInstance->CreateClient( Settings::Global().Get<uint>("twsClientId") );
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

	std::map<uint,std::shared_ptr<std::string_view>> x;
	std::string_view test = "Test String";
	std::string fooString{ "Test String" };
	std::shared_ptr<std::string_view> test2{};
	//std::optional<uint> x2{5};
	//var has = x2.has_value();
	std::shared_ptr<std::string_view> xxx = std::make_shared<std::string_view>( test );
	//x.emplace( 1, xxx );
	//var z = x.begin();
	//var& first = z->first;
	//var& second = z->second;
	//DBG( "x.size=", x.size() );

   var result = RUN_ALL_TESTS();
	p = nullptr;
	return result;
}