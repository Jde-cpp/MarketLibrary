﻿#pragma once
#include <jde/markets/types/proto/ib.pb.h>
#include <jde/markets/types/proto/results.pb.h>
#include <jde/Str.h>

namespace Jde::Markets
{
	using Proto::EOrderStatus;
	constexpr std::array<sv,22> EOrderTypeStrings={ "LMT", "MKT", "MTL", "MIT", "MOC", "PEG MKT", "PEG STK", "REL", "BOX TOP", "LIT", "LOC", "PASSV REL", "PEG MID", "STP", "STP LMT", "STP PRT", "TRAIL", "TRAIL LIMIT", "REL + LMT", "REL + MKT", "VOL", "PEG BENCH"  };
	Ξ ToOrderTypeString( Proto::EOrderType orderType )noexcept{ return Str::FromEnum( EOrderTypeStrings, orderType ); }
	Ξ ToOrderType( sv value )noexcept{ return Str::ToEnum<Proto::EOrderType>( EOrderTypeStrings, value).value_or(Proto::EOrderType::Limit ); }

	constexpr std::array<sv,7> ETifStrings={ "DAY", "GTC", "IOC", "GTD", "OPG", "FOK", "DTC" };
	constexpr std::array<sv,9> EOrderStatusStrings={ "PendingSubmit","PendingCancel","PreSubmitted","Submitted","ApiCancelled","Cancelled","Filled","Inactive","UnknownStatus" };

	enum class EAction : uint8{ Buy=0, Sell=1 /*,Short=2 //BUY/SELL/SSHORT*/ };
	constexpr sv EActionStrings[]={ "BUY", "SELL" /*"SSHORT"*/ };
	constexpr sv ToActionString( EAction action ){ return EActionStrings[(uint)action]; }

	Ξ operator& (EOrderStatus a, EOrderStatus b){ return (EOrderStatus)( (uint8)a & (uint8)b ); }
	Ξ operator!( EOrderStatus a ){ return a==EOrderStatus::NoStatus; }
	Ξ IsComplete( EOrderStatus a ){ return a==EOrderStatus::Filled || a==EOrderStatus::Cancelled; }
	Ξ ToString( EOrderStatus order )noexcept;
	Ξ ToOrderStatus( sv statusText )noexcept;
}
namespace Jde
{
	Ξ Markets::ToString( EOrderStatus orderStatus )noexcept
	{
		sv result = "None";
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
				CRITICAL( "Could not parse Order Status {}"sv, (uint8)orderStatus );
		}
		return result;
	}
	Ξ Markets::ToOrderStatus( sv statusText )noexcept
	{
		EOrderStatus orderStatus = EOrderStatus::NoStatus;
		if( statusText=="PendingSubmit" ) orderStatus = EOrderStatus::PendingSubmit;
		else if( statusText=="PendingCancel" ) orderStatus = EOrderStatus::PendingCancel;
		else if( statusText=="PreSubmitted" ) orderStatus = EOrderStatus::PreSubmitted;
		else if( statusText=="Submitted" ) orderStatus = EOrderStatus::Submitted;
		else if( statusText=="ApiCancelled" ) orderStatus = EOrderStatus::ApiCancelled;
		else if( statusText=="Cancelled" ) orderStatus = EOrderStatus::Cancelled;
		else if( statusText=="Filled" ) orderStatus = EOrderStatus::Filled;
		else if( statusText=="Inactive" ) orderStatus = EOrderStatus::Inactive;
		else
			CRITICAL( "Could not parse Order Status {}"sv, statusText );
		return orderStatus;
	}
}