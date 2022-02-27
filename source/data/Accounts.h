#pragma once
#include <jde/markets/Exports.h>
#include "../client/awaitables/TwsAwaitable.h"
#include "../../../Framework/source/um/UM.h"

namespace Jde::UM{ enum class EAccess : uint8; }
namespace Jde::Markets
{
	struct Account{ PK Id{0}; string Display/*Name*/; string IbName/*Target*/; string Description; bool Connected{false}; flat_map<UserPK, UM::EAccess> Access; };//TODO
	struct ΓM AccountsAwait final:ITwsAwaitShared//sp<vector<Account>>
	{
		using base=ITwsAwaitShared;
		α await_ready()noexcept->bool override;
		α await_suspend( HCoroutine h )noexcept->void override;
		α await_resume()noexcept->AwaitResult override;
	};

	struct ΓM AccountAuthorizer final: UM::IAuthorize
	{
		AccountAuthorizer()noexcept;

		Ω Initialize()noexcept->void;
		α CanRead( uint pk, UserPK userId )noexcept->bool override;
		α Test( DB::EMutationQL ql, UserPK userId, SRCE )noexcept(false)->void override;
	};
}
#define Φ ΓM α
namespace Jde::Markets::Accounts
{
	α Set( const vector<string>& accounts )noexcept->void;
	Φ Find( str ibName )noexcept->optional<Account>;
	Φ TryInsert( string ibName, shared_mutex* pAccountMutex )noexcept->optional<Account>;
	Φ CanRead( str ibName, UserPK userId )noexcept->bool;
}
#undef Φ