#include "crashdetect.h"
#include "amx/amx.h"

/// Gets called before amx_Exec() on error but before AMX stack/heap is reset
extern "C" int AMXAPI amx_Error(AMX *amx, cell index, int error) {
	if (error != AMX_ERR_NONE) {
		crashdetect::RuntimeError(amx, index, error);		
	}
	return AMX_ERR_NONE;
}

