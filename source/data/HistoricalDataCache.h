﻿#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#pragma warning( disable : 4244 )
#include <jde/markets/types/proto/requests.pb.h>
#pragma warning( default : 4244 )

#define Φ ΓM auto
struct Bar;
namespace Jde::Markets
{
	struct Contract;
	using EBarSize=Proto::Requests::BarSize;
	using EDisplay=Proto::Requests::Display;
}
namespace Jde::Markets::HistoryCache
{
	Φ Get( const Contract& contract, Day end, Day tradingDays, EBarSize barSize, EDisplay display, bool useRth )noexcept->flat_map<Day,VectorPtr<sp<::Bar>>>;
	Φ Set( const Contract& contract, EDisplay display, EBarSize barSize, bool useRth, const vector<::Bar>& bars, Day end=0, Day subDayCount=0 )noexcept->void;
	α SetDay( const Contract& contract, bool useRth, const vector<sp<::Bar>>& bars )noexcept->void;
	Φ Clear( ContractPK contractId, EDisplay display, EBarSize barSize=EBarSize::Minute )noexcept->void;
}
#undef Φ