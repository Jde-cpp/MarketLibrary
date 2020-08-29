#pragma once
#include "../Exports.h"

namespace Jde::Markets
{
	namespace Proto{ enum Currencies : int; }
	constexpr std::array<std::string_view,53> CurrencyStrings={	"NoCurrency", "ARS", "BRL", "CAD",	"CLP",	"COP",	"EcuadorUsDollar",	"MXN",	"PEN",	"UYU",	"VEB",	"AUD",	"CNY",	"HKD",	"INR",	"IDR",	"JPY",	"MYR",	"NZD",	"PKR",	"PHP",	"SGD",	"KRW",	"TWD",	"THB",	"CZK",	"DKK",	"EUR",	"HUF",	"MTL",	"NOK",	"PLN",	"RUB",	"SKK",	"SEK",	"CHF",	"TRY",	"GBP",	"BHD",	"EGP",	"ILS",	"JOD",	"KWD",	"LBP",	"SAR",	"ZAR",	"AED",	"Sdr",	"USD",	"VEB",	"VND",	"RON",	"KES"  };

	JDE_MARKETS_EXPORT string_view ToString( Proto::Currencies x )noexcept;
	JDE_MARKETS_EXPORT Proto::Currencies ToCurrency( string_view x )noexcept;
}