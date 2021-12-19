#include "Accounts.h"
#include "../../../Framework/source/db/Database.h"
#include "../../../Framework/source/db/Syntax.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../wrapper/WrapperCo.h"
#include "../client/TwsClientCo.h"

#define var const auto
namespace Jde::Markets
{
	AccountAuthorizer _authorizer;
	flat_map<uint,Account> _accounts; shared_mutex _accountMutex; flat_set<uint> _deletedAccounts; flat_map<UserPK,UM::EAccess> _minimumAccess;
	α AccountAccess( UM::EAccess requested, UserPK userId, AccountPK accountId, bool allowDeleted=true )noexcept(false)->bool;

	struct GroupAuthorizer final: UM::IAuthorize
	{
		GroupAuthorizer()noexcept;

		α CanRead( uint pk, UserPK userId )noexcept->bool override{ return true; }
	};
	α AccountsAwait::await_ready()noexcept->bool{ return base::await_ready() || !_accounts.empty() || !WrapperPtr()->_accountHandle; }
	α AccountsAwait::await_suspend( HCoroutine h )noexcept->void//will never get called, accounts gets setup initially.
	{
		base::await_suspend( h );
		WrapperPtr()->_accountHandle = h;
		_pTws->reqManagedAccts();
	}
	α AccountsAwait::await_resume()noexcept->TaskResult
	{
		while( WrapperPtr()->_accountHandle )
			std::this_thread::yield();
		sl l{ _accountMutex };
		auto accounts = make_shared<vector<Account>>(); accounts->reserve( _accounts.size() );
		for( var& p : _accounts )
			accounts->push_back( p.second );
		return TaskResult{ accounts };
	}

#define db DB::DataSource()
	α LoadAccess()noexcept->void
	{
		unique_lock l{ _accountMutex };
		_deletedAccounts.clear();
		_accounts.clear();
		if( !db.TrySelect( format("select id, {}, name, target, description  from ib_accounts", DB::DefaultSyntax().DateTimeSelect("deleted")), [&]( const DB::IRow& row )
		{
			uint i=0;  var id = row.GetUInt( i ); var pDeleted = row.GetTimePointOpt( ++i );
			if( pDeleted )
				_deletedAccounts.emplace( id );
			else
				_accounts.emplace( id, Account{id, row.GetString(i+1), row.GetString(i+2), row.GetString(i+3)} );
		}) ) return;
		db.TrySelect( "select account_id, user_id, right_id  FROM ib_account_roles ib join um_group_roles gr on gr.role_id=ib.role_id join um_user_groups ug on gr.group_id=ug.group_id", [&]( const DB::IRow& row )
		{
			var accountId = row.GetUInt( 0 );
			var pAccount = std::find_if( _accounts.begin(), _accounts.end(), [&]( var& x ){ return x.second.Id==accountId; } );
			if( pAccount!=_accounts.end() )
				pAccount->second.Access.emplace( (UserPK)row.GetUInt(1), (UM::EAccess)row.GetUInt16(2) );
		} );
	}
	α LoadMinimumAccess()noexcept->void
	{
		unique_lock l{ _accountMutex };
		_minimumAccess.clear();
		db.TrySelect( "select user_id, right_id from um_permissions p join um_apis apis on p.api_id=apis.id and apis.name='TWS' and p.name is null join um_role_permissions rp on rp.permission_id=p.id join um_group_roles gr on gr.role_id=rp.role_id join um_user_groups ug on gr.group_id=ug.user_id", [&]( const DB::IRow& row )
		{
			_minimumAccess[(UserPK)row.GetUInt(0)] = (UM::EAccess)row.GetUInt16( 1 );
		} );
	}

	AccountAuthorizer::AccountAuthorizer()noexcept:
		IAuthorize{ "ib_accounts" }
	{
		DB::AddMutationListener( "ib", [](const DB::MutationQL& m, PK /*id*/)
		{
			if( m.JsonName.ends_with( "AccountRoles") || m.JsonName.ends_with("RoleAccounts") )
				LoadAccess();
		} );
		DB::AddMutationListener( "um", []( const DB::MutationQL& m, PK /*id*/ )
		{
			if( m.JsonName.ends_with("RolePermissions") || m.JsonName.ends_with("PermissionRoles") )
				LoadMinimumAccess();
		} );
	}

	α AccountAuthorizer::Initialize()noexcept->void
	{
		LoadAccess();
		LoadMinimumAccess();
	}

	α AccountAuthorizer::CanRead( uint pk, UserPK userId )noexcept->bool
	{
		try
		{
			return AccountAccess( UM::EAccess::Read, userId, (AccountPK)pk );
		}
		catch( IException& )
		{
			return false;
		}
	}

	α AccountAccess( UM::EAccess requested, UserPK userId, AccountPK accountId, bool allowDeleted )noexcept(false)->bool
	{
		shared_lock l{ _accountMutex };
		if( !allowDeleted && _deletedAccounts.find( accountId )!=_deletedAccounts.end() )
			return false;
		bool haveAccess = false;

		if( var pIdAccount = _accounts.find( accountId ); pIdAccount!=_accounts.end() )
		{
			var& account = pIdAccount->second;
			if( var pAccount = account.Access.find(userId); pAccount != account.Access.end() )
				haveAccess = (pAccount->second & requested)!=UM::EAccess::None;
			else if( var pAccount2 = account.Access.find(std::numeric_limits<UserPK>::max()); userId!=0 && pAccount2 != account.Access.end() )
				haveAccess = (pAccount2->second & requested)!=UM::EAccess::None;
		}
		if( var pUser = _minimumAccess.find(userId); !haveAccess && pUser!=_minimumAccess.end() )
			haveAccess = (pUser->second & requested)!=UM::EAccess::None;

		return haveAccess;
	}

	α Accounts::Set( const vector<string>& accounts )noexcept->void
	{
		for( auto&& ibName : accounts )
		{
			if( auto p = Accounts::Find( ibName ); !p )
				Accounts::TryInsert( ibName );
		}
	}

	α Accounts::Find( str ibName )noexcept->optional<Account>
	{
		ul l{ _accountMutex };
		auto p = std::find_if( _accounts.begin(), _accounts.end(), [&ibName](var& x)->bool{ return x.second.IbName==ibName; } );
		return p==_accounts.end() ? nullopt : optional{ p->second };
	}
	α Accounts::TryInsert( string ibName )noexcept->optional<Account>
	{
		optional<Account> y;
		try
		{
			DB::DataSource().ExecuteProc( "ib_account_insert( ?, 0, ?, ? )", {ibName,ibName,ibName} );
			INFO( "inserted account '{}'."sv, ibName );
			var id = *Future<uint>(DB::IdFromName("ib_accounts", ibName) ).get();
			y = Account{ id, move(ibName) };
			ul l{ _accountMutex };
			_accounts.emplace( id, y.value() );
		}
		catch( const IException& )
		{}
		return y;
	}

	α Accounts::CanRead( str ibName, UserPK userPK )noexcept->bool
	{
		return true;//TODO Fix this.
		//var pk = Find( ibName ).value_or( Account{} ).Id;
		//return pk && _authorizer.CanRead( userPK, pk );
	}
}