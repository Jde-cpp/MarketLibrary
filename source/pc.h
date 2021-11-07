#include <jde/TypeDefs.h>
#ifdef _MSC_VER
	#include <WinSock2.h>
	#define TWSAPIDLLEXP __declspec( dllimport )
#pragma push_macro("assert")
#undef assert
#pragma warning(push)
#pragma warning( disable : 4267 )
#include <platformspecific.h>
#pragma pop_macro("assert")
#else
	#define TWSAPIDLLEXP
#endif

#include <Contract.h>
#include <Execution.h>
#include <jde/markets/TypeDefs.h>
#pragma warning(pop)
#include "../../Framework/source/DateTime.h"
