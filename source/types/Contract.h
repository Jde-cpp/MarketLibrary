#pragma once
#include <ostream>
#include "../Exports.h"
#include "./proto/ib.pb.h"
//#include "Exchanges.h"
//#include "../../../framework/io/Buffer.h"
#include "../TypeDefs.h"
struct ContractDetails;
struct Contract;

namespace Jde::Markets
{
	typedef std::chrono::system_clock::time_point TimePoint;
//	namespace Proto{ class Contract; class ComboLeg; class DeltaNeutralContract; enum Currencies : int; enum SecurityRight : int; enum SecurityType : int; enum Exchanges : int; }
	namespace Proto::Results{ class ContractDetail; class ContractHours; }
	using Proto::Exchanges;
	typedef std::string str;
	typedef std::string_view sv;
	typedef uint_fast16_t DayIndex;
	typedef double PositionAmount;
	typedef double Amount;
#pragma region DeltaNeutralContract
	struct DeltaNeutralContract
	{
		DeltaNeutralContract()noexcept{};
		DeltaNeutralContract( const Proto::DeltaNeutralContract& proto )noexcept;
		//DeltaNeutralContract( IO::IncomingMessage& message );
		long Id{0};
		double Delta{0.0};
		double Price{0.0};

		sp<Proto::DeltaNeutralContract> ToProto( bool stupidPointer )const noexcept;
	};
#pragma endregion
#pragma region SecurityRight
	using SecurityRight = Proto::SecurityRight;
	JDE_MARKETS_EXPORT SecurityRight ToSecurityRight( sv name )noexcept;
	JDE_MARKETS_EXPORT sv ToString( SecurityRight right )noexcept;
#pragma endregion
#pragma region SecurityType
	using SecurityType=Proto::SecurityType;
	JDE_MARKETS_EXPORT SecurityType ToSecurityType( sv inputName )noexcept;
	sv ToString( SecurityType type )noexcept;
#pragma endregion
#pragma region ComboLeg
	struct ComboLeg
	{
		//ComboLeg( IO::IncomingMessage& message, bool isOrder )noexcept;
		ComboLeg( const Proto::ComboLeg& proto )noexcept;

		ContractPK ConId{0};
		long Ratio{0};
		std::string Action; //BUY/SELL/SSHORT
		std::string Exchange;
		long OpenClose{0}; // LegOpenClose enum values

		// for stock legs when doing short sale
		long ShortSaleSlot{0}; // 1 = clearing broker, 2 = third party
		std::string	DesignatedLocation;
		int_fast32_t ExemptCode{-1};

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
		//Contract( IO::IncomingMessage& message, bool havePrimaryExchange=true )noexcept(false);
		explicit Contract( ContractPK id, sv symbol="" )noexcept;
		Contract( ContractPK id, Proto::Currencies currency, sv localSymbol, uint_fast32_t multiplier, sv name, Exchanges exchange, sv symbol, sv tradingClass, TimePoint issueDate=TimePoint::max() )noexcept;
		Contract( const ::Contract& contract )noexcept;
		Contract( const ContractDetails& details )noexcept;
		Contract( const Proto::Contract& contract )noexcept;
		~Contract();
		bool operator <(const Contract &b)const noexcept{return Id<b.Id;}

		sp<::Contract> ToTws()const noexcept;
		sp<Proto::Contract> ToProto( bool stupidPointer=false )const noexcept;
		ContractPK Id{0};
		str Symbol;
		SecurityType SecType{SecurityType::Stock};//"STK", "OPT"
		DayIndex Expiration{0};
		double Strike{0.0};
		SecurityRight Right{SecurityRight::None};
		uint_fast32_t Multiplier{0};
		Exchanges Exchange{ Exchanges::Smart };
		Exchanges PrimaryExchange{Exchanges::Smart}; // pick an actual (ie non-aggregate) exchange that the contract trades on.  DO NOT SET TO SMART.
		Proto::Currencies Currency{Proto::Currencies::NoCurrency};
		str LocalSymbol;
		str TradingClass;
		bool IncludeExpired{false};
		str SecIdType;		// CUSIP;SEDOL;ISIN;RIC
		str SecId;
		str ComboLegsDescrip; // received in open order 14 and up for all combos
		std::shared_ptr<std::vector<ComboLegPtr_>> ComboLegsPtr;
		DeltaNeutralContract DeltaNeutral;
		str Name;
		uint Flags{0};
		TimePoint IssueDate{ TimePoint::max() };
		ContractPK UnderlyingId{0};

		ContractPK ShortContractId()const noexcept;
		PositionAmount LongShareCount( Amount price )const noexcept;
		PositionAmount ShortShareCount( Amount price )const noexcept;
		PositionAmount RoundShares( PositionAmount amount, PositionAmount roundAmount )const noexcept;
		//sp<DateTime> ExpirationTime()const noexcept;
		Amount RoundDownToMinTick( Amount price )const noexcept;
		static DayIndex ToDay( const str& str )noexcept;
		sp<std::vector<Proto::Results::ContractHours>> TradingHoursPtr;//TODO a const vector.
		sp<std::vector<Proto::Results::ContractHours>> LiquidHoursPtr;

		std::ostream& to_stream( std::ostream& os, bool includePrimaryExchange=true )const noexcept;
	};
	typedef std::shared_ptr<const Contract> ContractPtr_;
	std::ostream& operator<<( std::ostream& os, const Contract& contract )noexcept;
	JDE_MARKETS_EXPORT ContractPtr_ Find( const std::map<ContractPK, ContractPtr_>&, sv symbol )noexcept;

	//JDE_MARKETS_EXPORT sp<Proto::ContractDetails> ToProto( const ::ContractDetails& details )noexcept;
	JDE_MARKETS_EXPORT void ToProto( const ContractDetails& details, Proto::Results::ContractDetail& proto )noexcept;
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