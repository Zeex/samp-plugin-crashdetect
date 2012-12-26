configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc"
	@ONLY
)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/version.h"
	@ONLY
)
