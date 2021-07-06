#include <jde/TypeDefs.h>
#ifdef _MSC_VER
	#include <WinSock2.h>
	#define TWSAPIDLLEXP __declspec( dllimport )
#pragma push_macro("assert")
#undef assert
#include <platformspecific.h>
#pragma pop_macro("assert")
#else
	#define TWSAPIDLLEXP
#endif

#include <Contract.h>
#include <Execution.h>
#include "../../Framework/source/DateTime.h"
