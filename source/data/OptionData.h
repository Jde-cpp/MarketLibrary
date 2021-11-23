#pragma once
#include <jde/markets/TypeDefs.h>
#include <jde/markets/Exports.h>
#include "../../../Framework/source/collections/Collections.h"
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
	using OptionPtr=sp<const Option>;
	using OptionSet=flat_set<OptionPtr,SPCompare<const Option>>;
	using OptionSetPtr=sp<OptionSet>;
	namespace OptionData
	{
		ΓM OptionSetPtr SyncContracts( ContractPtr_ pContract, const vector<ContractDetails>& pDetails )noexcept(false);
		ΓM OptionSetPtr Load( ContractPK underlyingId, Day earliestDay=0 )noexcept(false);
		α LoadFiles( const Contract& contract )noexcept->flat_map<Day,sp<Proto::UnderlyingOIValues>>;
		ΓM Proto::Results::OptionValues* LoadDiff( const Contract& contract, bool isCall, Day from, Day to, bool includeExpired=false, bool noFromDayOk=false )noexcept(false);
		ΓM Day LoadDiff( const Contract& underlying, const vector<ContractDetails>& options, Proto::Results::OptionValues& results )noexcept(false);
		α Insert( const Option& value )noexcept(false)->void;
		Φ OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept->fs::path;
	}
}
#undef Φ