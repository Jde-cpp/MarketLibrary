#include <jde/Log.h>
#include <jde/Assert.h>
#include <jde/markets/Exports.h>
#include "../../../Framework/source/um/UM.h"

namespace Jde::Markets
{
	struct Account{ PK Id{0}; string IbName; string target; string description; flat_map<UserPK,UM::EAccess> Access; };

	struct AccountAuthorizer final: UM::IAuthorize
	{
		AccountAuthorizer()noexcept;

		Ω Initialize()noexcept->void;
		α CanRead( uint pk, UserPK userId )noexcept->bool override;
		//Ω Instance()noexcept->AccountAuthorizer*{ return (AccountAuthorizer*)_pInstance; }
		//static AccountAuthorizer* _pInstance;
	};
}
namespace Jde::Markets::Accounts
{
	α Find( str ibName )noexcept->optional<Account>;
	α Insert( string ibName )noexcept(false)->Account;
	α CanRead( str ibName, UserPK userId )noexcept->bool;
}
