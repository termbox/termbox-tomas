cmake_minimum_required(VERSION 2.8)
project(libtermbox C)

option(WITH_TRUECOLOR "Enable true-color support" 0)
option(BUILD_DEMOS "Build demos" 1)
option(BUILD_SHARED_LIBS "Build Shared Library (OFF for static-only)" ON)

include(cmake/add_cflag_if_supported.cmake)

add_cflag_if_supported("-std=gnu99")
add_cflag_if_supported("-Wall")
add_cflag_if_supported("-Wextra")

# Must nog have errors
add_cflag_if_supported("-Werror")

# Source fortification
add_cflag_if_supported("-D_FORTIFY_SOURCE=2")
add_cflag_if_supported("-fstrict-aliasing=1")
add_cflag_if_supported("-Wl,-z,relro")
add_cflag_if_supported("-Wl,-z,now")
add_cflag_if_supported("-fPIC")

# Extra warnings
add_cflag_if_supported("-Wshadow")
add_cflag_if_supported("-Wformat")
add_cflag_if_supported("-Wformat-security")
add_cflag_if_supported("-Wno-sign-conversion")
add_cflag_if_supported("-Wno-type-limits")
add_cflag_if_supported("-Wno-format-truncation")
#add_cflag_if_supported("-Wconversion")
add_cflag_if_supported("-Wmisleading-indentation")
add_cflag_if_supported("-Wno-float-conversion")
add_cflag_if_supported("-Wmaybe-uninitialized")
#add_cflag_if_supported("-Wmissing-variable-declarations")
#add_cflag_if_supported("-Wshorten-64-to-32")
add_cflag_if_supported("-Wimplicit-function-declaration")
add_cflag_if_supported("-Wredundant-decls")
add_cflag_if_supported("-Wundef")
add_cflag_if_supported("-Wswitch")
add_cflag_if_supported("-Wfloat-equal")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_cflag_if_supported("-Og")
	add_cflag_if_supported("-g")
else()
	add_cflag_if_supported("-O3")
endif()

add_definitions(-D_XOPEN_SOURCE)

set(SRC src/termbox.c src/utf8.c)
#include_directories(src)

add_library(${PROJECT_NAME}-static STATIC ${SRC})
set_target_properties(${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PREFIX "")

if (BUILD_DEMOS)
  file(GLOB DEMOS demos/*.c)
  foreach(DEMO ${DEMOS})
  	MESSAGE("----> Adding demo ${DEMO}")
  	get_filename_component(DEMOEXE ${DEMO} NAME_WE)
  	add_executable(${DEMOEXE} ${DEMO})
  	add_dependencies(${DEMOEXE} ${PROJECT_NAME}-static)
  	target_link_libraries(${DEMOEXE} ${PROJECT_NAME}-static rt)
  	if(${DEMO} MATCHES "dirmenu.c") # this demo requires pthread
  		target_link_libraries(${DEMOEXE} pthread)
  	endif()
  endforeach()
endif()

if (BUILD_SHARED_LIBS)
	add_library(${PROJECT_NAME}-shared SHARED ${SRC})
	set_target_properties(${PROJECT_NAME}-shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PREFIX "")

	install(TARGETS ${PROJECT_NAME}-shared
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
endif()

install(TARGETS ${PROJECT_NAME}-static
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

install(FILES src/termbox.h
	DESTINATION include
)
