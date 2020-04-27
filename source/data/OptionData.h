#pragma once
#include "../TypeDefs.h"
#include "../Exports.h"
#include "../../../Framework/source/SmartPointer.h"
#include "../../../Framework/source/Settings.h"
#include "../types/proto/OptionOI.pb.h"

namespace ibapi{ struct ContractDetails;}
namespace Jde::Markets
{
	namespace Proto::Results{class OptionValues;}
	struct Contract;
	typedef std::shared_ptr<const Contract> ContractPtr_;
	struct Option
	{
		Option()=default;
		Option( const ibapi::ContractDetails& ib );
		Option( TimePoint expirationDate, Amount Strike, bool isCall );
		ContractPK Id{0};
		TimePoint ExpirationDate;
		bool IsCall{false};
		Amount Strike;
		ContractPK UnderlyingId;
		ContractPtr_ ContractPtr;
		uint16 DaysSinceEpoch()const noexcept{return std::chrono::duration_cast<std::chrono::hours>(ExpirationDate-DateTime(1970,1,1).GetTimePoint()).count()/24; }
		bool operator<( const Option& op2 )const noexcept;
	};
	typedef sp<const Option> OptionPtr;
	typedef set<OptionPtr,SPCompare<const Option>> OptionSet;
	typedef sp<OptionSet> OptionSetPtr;
	namespace OptionData
	{
		JDE_MARKETS_EXPORT OptionSetPtr SyncContracts( ContractPtr_ pContract, const list<ibapi::ContractDetails>& pDetails )noexcept(false);
		JDE_MARKETS_EXPORT OptionSetPtr Load( ContractPK underlyingId )noexcept(false);
		map<DayIndex,sp<Proto::UnderlyingOIValues>> LoadFiles( const Contract& contract )noexcept;
		JDE_MARKETS_EXPORT Proto::Results::OptionValues* LoadDiff( const Contract& contract, bool isCall, DayIndex from, DayIndex to, bool includeExpired=false )noexcept(false);
		void Insert( const Option& value )noexcept(false);
		JDE_MARKETS_EXPORT fs::path OptionFile( const Contract& contract, uint16 year, uint8 month, uint8 day )noexcept;
		//JDE_MARKETS_EXPORT TimePoint LastOptionDate( const Contract& contract )noexcept;
	}

	inline fs::path RootMinuteBar()noexcept(false){ return Settings::Global().Get<fs::path>("rootMinuteBar"); }
}