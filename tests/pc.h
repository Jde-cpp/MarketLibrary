#ifdef _MSC_VER
	#define NTDDI_VERSION NTDDI_WIN10_RS1 // work around linker failure MapViewOfFileNuma2@36
#endif

#pragma push_macro("assert")
#undef assert
#pragma warning(push)
#pragma warning( disable : 4267 )
#include <platformspecific.h>
#pragma pop_macro("assert")

#include <jde/TypeDefs.h>