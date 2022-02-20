#pragma once
#include <jde/markets/Exports.h>

namespace Jde::Markets
{
	namespace Proto{ enum Currencies : int; }
	constexpr std::array<sv,53> CurrencyStrings={ "", "ARS", "BRL", "CAD", "CLP", "COP", "EcuadorUsDollar", "MXN", "PEN", "UYU", "VEB", "AUD", "CNY", "HKD", "INR", "IDR", "JPY", "MYR", "NZD", "PKR", "PHP", "SGD", "KRW", "TWD", "THB", "CZK", "DKK", "EUR", "HUF", "MTL", "NOK", "PLN", "RUB", "SKK", "SEK", "CHF", "TRY", "GBP", "BHD", "EGP", "ILS", "JOD", "KWD", "LBP", "SAR", "ZAR", "AED", "Sdr", "USD", "VEB", "VND", "RON", "KES"  };

	ΓM α ToString( Proto::Currencies x )noexcept->sv;
	ΓM α ToCurrency( sv x )noexcept->Proto::Currencies;
}