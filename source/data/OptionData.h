#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#include "../../../Framework/source/collections/Collections.h"
#include "../../../Framework/source/Settings.h"
#pragma warning( disable : 4244 )
#include "../types/proto/OptionOI.pb.h"
#pragma warning( default : 4244 )

struct ContractDetails;
namespace Jde::Markets
{
	namespace Proto::Results{class OptionValues;}
	struct Contract;
	typedef std::shared_ptr<const Contract> ContractPtr_;
	struct Option
	{
		Option()=default;
		Option( const ContractDetails& ib );
		Option( DayIndex expirationDate, Amount Strike, bool isCall, ContractPK UnderlyingId=0 );
		ContractPK Id{0};
		DayIndex ExpirationDay;
		bool IsCall{false};
		Amount Strike;
		ContractPK UnderlyingId;
		ContractPtr_ ContractPtr;
		bool operator<( const Option& op2 )const noexcept;
	};
	typedef sp<const Option> OptionPtr;
	typedef set<OptionPtr,SPCompare<const Option>> OptionSet;
	typedef sp<OptionSet> OptionSetPtr;
	namespace OptionData
	{
		JDE_MARKETS_EXPORT OptionSetPtr SyncContracts( ContractPtr_ pContract, const vector<ContractDetails>& pDetails )noexcept(false);
		JDE_MARKETS_EXPORT OptionSetPtr Load( ContractPK underlyingId, DayIndex earliestDay=0 )noexcept(false);
		map<DayIndex,sp<Proto::UnderlyingOIValues>> LoadFiles( const Contract& contract )noexcept;
		JDE_MARKETS_EXPORT Proto::Results::OptionValues* LoadDiff( const Contract& contract, bool isCall, DayIndex from, DayIndex to, bool includeExpired=false, bool noFromDayOk=false )noexcept(false);
		JDE_MARKETS_EXPORT DayIndex LoadDiff( const Contract& underlying, const vector<ContractDetails>& options, Proto::Results::OptionValues& results )noexcept(false);
		void Insert( const Option& value )noexcept(false);
		JDE_MARKETS_EXPORT fs::path OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept;
	}
}