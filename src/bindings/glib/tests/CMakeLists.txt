include_directories (${CMAKE_SOURCE_DIR}/tests)

set (TEST_FILES $<TARGET_OBJECTS:cframework>)
add_testheaders(TEST_FILES)

macro (do_glib_test file)
	get_filename_component (name ${file} NAME_WE)
	set (SOURCES ${file} ${GELEKTRA_HDR_FILES} ${TEST_FILES})

	add_executable (${name} ${SOURCES})

	set_target_properties (
		${name} PROPERTIES
		COMPILE_DEFINITIONS HAVE_KDBCONFIG_H
	)

	set (GLIBS ${GELEKTRA_LIBRARY} ${GOBJECT_LIBRARIES})
	tool_link_elektra (${name})
	target_link_libraries (${name} ${GLIBS})

	add_test (
		NAME ${name}
		COMMAND "${CMAKE_BINARY_DIR}/bin/${name}" "${CMAKE_CURRENT_SOURCE_DIR}"
	)

	set_property (
		TEST ${name}
		PROPERTY LABELS memleak bindings
	)

	if (${name} MATCHES ".*_kdb")
		set_property (
			TEST ${name}
			APPEND PROPERTY LABELS kdbtests
		)
	endif ()
endmacro (do_glib_test)

file (GLOB TESTS testglib_*.c)
foreach (file ${TESTS})
	do_glib_test (${file})
endforeach (file ${TESTS})
