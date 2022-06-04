#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#include <jde/markets/types/proto/ib.pb.h>
#include "../../../Framework/source/collections/Collections.h"
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "../../../Framework/source/Settings.h"
#pragma warning( disable : 4244 )
#include "../types/proto/OptionOI.pb.h"

#pragma warning( default : 4244 )

#define Φ ΓM auto
struct ContractDetails;
namespace Jde::Markets
{
	namespace Proto::Results{class OptionValues;}
	struct Contract;
	using ContractPtr_=sp<const Contract>;
	struct Option
	{
		Option()=default;
		Option( const ContractDetails& ib );
		Option( Day expirationDate, Amount Strike, bool isCall, ContractPK UnderlyingId=0 );
		ContractPK Id{0};
		Day ExpirationDay;
		bool IsCall{false};
		Amount Strike;
		ContractPK UnderlyingId;
		ContractPtr_ ContractPtr;
		bool operator<( const Option& op2 )const noexcept;
	};
	using OptionSet=flat_set<sp<const Markets::Option>,SPCompare<const Markets::Option>>;
	namespace OptionData
	{
		using SecurityRight = Proto::SecurityRight;
		using Proto::Results::OptionValues;
		Φ Load( sp<::ContractDetails> pUnderlying, Day startExp, Day endExp, SecurityRight right, double startStrike, double endStrike, Proto::Results::OptionValues* pResults )->AsyncAwait;
		Φ Load( ContractPK underlyingId, Day earliestDay=0 )ε->sp<OptionSet>;
		Φ SyncContracts( ContractPtr_ pContract, const vector<ContractDetails>& pDetails )ε->sp<OptionSet>;
		α LoadFiles( const Contract& contract )noexcept->flat_map<Day,sp<Proto::UnderlyingOIValues>>;
		Φ LoadDiff( const Contract& contract, bool isCall, Day from, Day to, bool includeExpired=false, bool noFromDayOk=false )ε->OptionValues*;
		Φ LoadDiff( const Contract& underlying, const vector<sp<ContractDetails>>& options, OptionValues& results )ε->Day;
		α Insert( const Markets::Option& value )ε->void;
		Φ OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept->fs::path;
	}
}
#undef Φ