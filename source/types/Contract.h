#pragma once
#include <ostream>

#include "Exchanges.h"
//#include "../../../framework/io/Buffer.h"
#include "../TypeDefs.h"

namespace ibapi{ struct Contract; struct ContractDetails;}
namespace Jde::Markets
{
	namespace Proto{ class Contract; class ComboLeg; class DeltaNeutralContract; enum Currencies : int; }
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
	using SecurityRight = Proto::SecurityRight;
	JDE_MARKETS_EXPORT SecurityRight ToSecurityRight( string_view name )noexcept;
	JDE_MARKETS_EXPORT string_view ToString( SecurityRight right )noexcept;
#pragma endregion
#pragma region SecurityType
	using SecurityType=Proto::SecurityType;
/*	enum class SecurityType : uint8
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
	};*/

	JDE_MARKETS_EXPORT SecurityType ToSecurityType( string_view inputName )noexcept;
	string_view ToString( SecurityType type )noexcept;
#pragma endregion
#pragma region ComboLeg
	struct ComboLeg
	{
		ComboLeg( IO::IncomingMessage& message, bool isOrder )noexcept;
		ComboLeg( const Proto::ComboLeg& proto )noexcept;

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
		std::ostream& to_stream( std::ostream& os, bool isOrder )const noexcept;
		bool operator==( const ComboLeg& other) const noexcept
		{
			return ConId == other.ConId && Ratio == other.Ratio && OpenClose == other.OpenClose
				&& ShortSaleSlot == other.ShortSaleSlot && ExemptCode == other.ExemptCode && Action == other.Action
				&& Exchange == other.Exchange &&  DesignatedLocation == other.DesignatedLocation;
		}
	};
	typedef sp<ComboLeg> ComboLegPtr_;
	std::ostream& operator<<( std::ostream& os, const ComboLeg& comboLeg )noexcept;
#pragma endregion
#pragma region Contract
	struct JDE_MARKETS_EXPORT Contract
	{
		Contract()=default;
		Contract( IO::IncomingMessage& message, bool havePrimaryExchange=true )noexcept(false);
		explicit Contract( ContractPK id, string_view symbol="" )noexcept;
		Contract( ContractPK id, Proto::Currencies currency, string_view localSymbol, uint multiplier, string_view name, Exchanges exchange, string_view symbol, string_view tradingClass, TimePoint issueDate=TimePoint::max() )noexcept;
		Contract( const ibapi::Contract& contract )noexcept;
		Contract( const ibapi::ContractDetails& details )noexcept;
		Contract( const Proto::Contract& contract )noexcept;
		~Contract();
		bool operator <(const Contract &b)const noexcept{return Id<b.Id;}

		sp<ibapi::Contract> ToTws()const noexcept;
		sp<Proto::Contract> ToProto( bool stupidPointer=false )const noexcept;
		ContractPK Id{0};
		string Symbol;
		SecurityType SecType{SecurityType::Stock};//"STK", "OPT"
		DayIndex Expiration{0};
		double Strike{0.0};
		SecurityRight Right{SecurityRight::None};
		uint32 Multiplier{0};
		Exchanges Exchange{ Exchanges::Smart };
		Exchanges PrimaryExchange{Exchanges::Smart}; // pick an actual (ie non-aggregate) exchange that the contract trades on.  DO NOT SET TO SMART.
		Proto::Currencies Currency{Proto::Currencies::NoCurrency};
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
		ContractPK UnderlyingId{0};

		ContractPK ShortContractId()const noexcept;
		PositionAmount LongShareCount( Amount price )const noexcept;
		PositionAmount ShortShareCount( Amount price )const noexcept;
		PositionAmount RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept;
		//sp<DateTime> ExpirationTime()const noexcept;
		Amount RoundDownToMinTick( Amount price )const noexcept;
		static DayIndex ToDay( const string& str )noexcept;

		std::ostream& to_stream( std::ostream& os, bool includePrimaryExchange=true )const noexcept;
	};
	typedef std::shared_ptr<const Contract> ContractPtr_;
	std::ostream& operator<<( std::ostream& os, const Contract& contract )noexcept;
	JDE_MARKETS_EXPORT ContractPtr_ Find( const map<ContractPK, ContractPtr_>&, string_view symbol )noexcept;

	//JDE_MARKETS_EXPORT sp<Proto::ContractDetails> ToProto( const ibapi::ContractDetails& details )noexcept;
	JDE_MARKETS_EXPORT Proto::Results::ContractDetails* ToProto( const ibapi::ContractDetails& details )noexcept;

	namespace Contracts
	{
		JDE_MARKETS_EXPORT extern const Contract Spy;
		extern const Contract SH;
		extern const Contract Qqq;
		extern const Contract Psq;
		JDE_MARKETS_EXPORT extern const Contract Tsla;
		JDE_MARKETS_EXPORT extern const Contract Aig;
	}
#pragma endregion
}