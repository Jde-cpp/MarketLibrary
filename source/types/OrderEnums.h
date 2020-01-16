#pragma once
#include "proto/ib.pb.h"
#include "proto/results.pb.h"

namespace Jde
{
	using Markets::Proto::Results::EOrderStatus;
	// enum class EOrderType : uint8
	// {
	// 	Market=0,
	// 	Limit=1
	// };
namespace Markets
{
	constexpr std::array<std::string_view,22> EOrderTypeStrings={ "MKT", "LMT", "MTL", "MIT", "MOC", "PEG MKT", "PEG STK", "REL", "BOX TOP", "LIT", "LOC", "PASSV REL", "PEG MID", "STP", "STP LMT", "STP PRT", "TRAIL", "TRAIL LIMIT", "REL + LMT", "REL + MKT", "VOL", "PEG BENCH"  };
	inline std::string_view ToOrderTypeString( Proto::EOrderType orderType )noexcept{ return StringUtilities::FromEnum( EOrderTypeStrings, orderType ); }
	inline Proto::EOrderType ToOrderType( string_view value )noexcept{ return StringUtilities::ToEnum( EOrderTypeStrings, value, Proto::EOrderType::Limit ); }

	constexpr std::array<std::string_view,7> ETifStrings={ "DAY", "GTC", "IOC", "GTD", "OPG", "FOK", "DTC" };
	constexpr std::array<std::string_view,9> EOrderStatusStrings={ "PendingSubmit","PendingCancel","PreSubmitted","Submitted","ApiCancelled","Cancelled","Filled","Inactive","UnknownStatus" };

	enum class EAction : uint8
	{
		Buy=0,
		Sell=1/*,
				Short=2*/
				//BUY/SELL/SSHORT
	};
	constexpr std::string_view EActionStrings[]={ "BUY", "SELL" /*"SSHORT"*/ };
	constexpr std::string_view ToActionString( EAction action ){ return EActionStrings[(uint)action]; }

	inline EOrderStatus operator& (EOrderStatus a, EOrderStatus b){ return (EOrderStatus)( (uint8)a & (uint8)b ); }
	inline bool operator!( EOrderStatus a ){ return a==EOrderStatus::None; }
	inline bool IsComplete( EOrderStatus a ){ return a==EOrderStatus::Filled || a==EOrderStatus::Cancelled; }
	inline std::string_view ToString( EOrderStatus order )noexcept;
	inline EOrderStatus ToOrderStatus( string_view statusText )noexcept;
	inline std::ostream& operator<<( std::ostream& os, const EOrderStatus& status )noexcept{ os << ToString(status); return os; }
// 	JDE_TWS_EXPORT std::string ToString( const ETickType display )noexcept;
}
	inline std::string_view Markets::ToString( EOrderStatus orderStatus )noexcept
	{
		std::string_view result = "None";
		switch( orderStatus )
		{
			case EOrderStatus::PendingSubmit: result = "PendingSubmit"; break;
			case EOrderStatus::PendingCancel: result = "PendingCancel"; break;
			case EOrderStatus::PreSubmitted: result = "PreSubmitted"; break;
			case EOrderStatus::Submitted: result = "Submitted"; break;
			case EOrderStatus::ApiCancelled: result = "ApiCancelled"; break;
			case EOrderStatus::Cancelled: result = "Cancelled"; break;
			case EOrderStatus::Filled: result = "Filled"; break;
			case EOrderStatus::Inactive: result = "Inactive"; break;
			default:
				GetDefaultLogger()->critical( "Could not parse Order Status {}", (uint8)orderStatus );
		}
		return result;
	}
	inline EOrderStatus Markets::ToOrderStatus( string_view statusText )noexcept
	{
		EOrderStatus orderStatus = EOrderStatus::None;
		if( statusText=="PendingSubmit" ) orderStatus = EOrderStatus::PendingSubmit;
		else if( statusText=="PendingCancel" ) orderStatus = EOrderStatus::PendingCancel;
		else if( statusText=="PreSubmitted" ) orderStatus = EOrderStatus::PreSubmitted;
		else if( statusText=="Submitted" ) orderStatus = EOrderStatus::Submitted;
		else if( statusText=="ApiCancelled" ) orderStatus = EOrderStatus::ApiCancelled;
		else if( statusText=="Cancelled" ) orderStatus = EOrderStatus::Cancelled;
		else if( statusText=="Filled" ) orderStatus = EOrderStatus::Filled;
		else if( statusText=="Inactive" ) orderStatus = EOrderStatus::Inactive;
		else
			GetDefaultLogger()->critical( "Could not parse Order Status {}", statusText );
		return orderStatus;
	}
}