#pragma once
#include <ostream>

#include "Exchanges.h"
//#include "../../../framework/io/Buffer.h"
#include "../TypeDefs.h"

namespace ibapi{ struct Contract; struct ContractDetails;}
namespace Jde::Markets
{
	namespace Proto{ class Contract; class ComboLeg; class DeltaNeutralContract; }
	namespace Proto::Results{  class ContractDetails; }
#pragma region DeltaNeutralContract
	struct DeltaNeutralContract
	{
		DeltaNeutralContract()noexcept{};
		DeltaNeutralContract( const Proto::DeltaNeutralContract& proto )noexcept;
		DeltaNeutralContract( IO::IncomingMessage& message );
		long Id{0};
		double Delta{0.0};
		double Price{0.0};

		sp<Proto::DeltaNeutralContract> ToProto( bool stupidPointer )const noexcept;
	};
#pragma endregion
#pragma region SecurityRight
	enum class SecurityRight : uint8
	{
		None=0,
		Call=1,
		Put=2
	};
	JDE_MARKETS_EXPORT SecurityRight ToSecurityRight( string_view name )noexcept;
#pragma endregion
#pragma region SecurityType
	enum class SecurityType : uint8
	{
		None=0,
		Stock=1,
		MutualFund=2,
		Etf=3,
		Future=4,
		Commodity=5,
		Bag=6,
		Cash=7,
		Fop=8,
		Index=9,
		Option=10,
		Warrant=11
	};

	JDE_MARKETS_EXPORT SecurityType ToSecurityType( string_view inputName )noexcept;
#pragma endregion
#pragma region ComboLeg
	struct ComboLeg
	{
		ComboLeg( IO::IncomingMessage& message, bool isOrder );
		ComboLeg( const Proto::ComboLeg& proto );

		ContractPK ConId{0};
		long Ratio{0};
		std::string Action; //BUY/SELL/SSHORT
		std::string Exchange;
		long OpenClose{0}; // LegOpenClose enum values

		// for stock legs when doing short sale
		long ShortSaleSlot{0}; // 1 = clearing broker, 2 = third party
		std::string	DesignatedLocation;
		int32 ExemptCode{-1};

		void SetProto( Proto::ComboLeg* pProto )const noexcept;
		std::ostream& to_stream( std::ostream& os, bool isOrder )const;
		bool operator==( const ComboLeg& other) const
		{
			return ConId == other.ConId && Ratio == other.Ratio && OpenClose == other.OpenClose 
				&& ShortSaleSlot == other.ShortSaleSlot && ExemptCode == other.ExemptCode && Action == other.Action 
				&& Exchange == other.Exchange &&  DesignatedLocation == other.DesignatedLocation;
		}
	};
	typedef sp<ComboLeg> ComboLegPtr_;
	std::ostream& operator<<( std::ostream& os, const ComboLeg& comboLeg );
#pragma endregion
#pragma region Contract
	struct JDE_MARKETS_EXPORT Contract
	{
		Contract()=default;
		Contract( IO::IncomingMessage& message, bool havePrimaryExchange=true )noexcept(false);
		explicit Contract( ContractPK id, string_view symbol="" );
		Contract( ContractPK id, string_view currency, string_view localSymbol, string_view multiplier, string_view name, Exchanges exchange, string_view symbol, string_view tradingClass, TimePoint issueDate=TimePoint::max() );
		Contract( const ibapi::Contract& contract );
		Contract( const Proto::Contract& contract );
		~Contract();
		bool operator <(const Contract &b) const{return Id<b.Id;}

		sp<ibapi::Contract> ToTws()const;
		sp<Proto::Contract> ToProto( bool stupidPointer=false )const noexcept;
		ContractPK Id{0};
		string Symbol;
		string SecType;
		string LastTradeDateOrContractMonth;
		double Strike{0.0};
		string Right;
		string Multiplier;
		string Exchange{"SMART"};
		Exchanges PrimaryExchange{Exchanges::Nyse}; // pick an actual (ie non-aggregate) exchange that the contract trades on.  DO NOT SET TO SMART.
		string Currency;//TODOEXT make int
		string LocalSymbol;
		string TradingClass;
		bool IncludeExpired{false};
		string SecIdType;		// CUSIP;SEDOL;ISIN;RIC
		string SecId;
		string ComboLegsDescrip; // received in open order 14 and up for all combos
		VectorPtr<ComboLegPtr_> ComboLegsPtr;
		DeltaNeutralContract DeltaNeutral;
		string Name;
		uint Flags{0};
		TimePoint IssueDate{ TimePoint::max() };

		ContractPK ShortContractId()const noexcept;
		PositionAmount LongShareCount( Amount price )const noexcept;
		PositionAmount ShortShareCount( Amount price )const noexcept;
		PositionAmount RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept;
		sp<DateTime> ExpirationTime()const noexcept;
		Amount RoundDownToMinTick( Amount price )const noexcept;

		std::ostream& to_stream( std::ostream& os, bool includePrimaryExchange=true )const;
	};
	typedef std::shared_ptr<const Contract> ContractPtr_;
	std::ostream& operator<<( std::ostream& os, const Contract& contract );
	JDE_MARKETS_EXPORT ContractPtr_ Find( const map<ContractPK, ContractPtr_>&, string_view symbol );

	//JDE_MARKETS_EXPORT sp<Proto::ContractDetails> ToProto( const ibapi::ContractDetails& details )noexcept;
	JDE_MARKETS_EXPORT Proto::Results::ContractDetails* ToProto( const ibapi::ContractDetails& details )noexcept;

	namespace Contracts
	{
		JDE_MARKETS_EXPORT extern const Contract Spy;
		extern const Contract SH;
		extern const Contract Qqq;
		extern const Contract Psq;
		JDE_MARKETS_EXPORT extern const Contract Tsla;
	}
#pragma endregion
}