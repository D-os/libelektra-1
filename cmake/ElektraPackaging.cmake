set (PACKAGE_NAME "elektra")
set (CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
set (PACKAGE_URL "http://www.libelektra.org/")
set (PACKAGE_BUGREPORT "http://bugs.libelektra.org/")


set (PROJECT_VERSION "${KDB_VERSION}")
set (CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set (CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")

set (PACKAGE_DESCRIPTION "Elektra provides a universal and secure framework to store configuration parameters in a global, hierarchical key database. The core is a small library implemented in C. The plugin-based framework fulfills many configuration-related tasks to avoid any unnecessary code duplication across applications while it still allows the core to stay without any external dependency.  Elektra abstracts from cross-platform-related issues with an consistent API, and allows applications to be aware of other applications' configurations, leveraging easy application integration.")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
set (CPACK_PACKAGE_CONTACT "${PACKAGE_URL}")
set (CPACK_SOURCE_IGNORE_FILES
	"/.cvsignore"
	"/.gitignore"
	"/songs/"
	"/build/"
	"/.svn/"
	"/.git/"
	"/osx-utils/"
	"/portage-overlay/"
)
set (CPACK_PACKAGE_EXECUTABLES kdb)
set (CPACK_SOURCE_GENERATOR "TBZ2")
set (CPACK_GENERATOR "TBZ2")

# needed because otherwise files would be written to
# system during creating the package
SET (CPACK_SET_DESTDIR "ON")
#package is not relocatable:
unset(CPACK_RPM_PACKAGE_RELOCATABLE)
unset(CPACK_RPM_PACKAGE_RELOCATABLE CACHE)


if ("${CMAKE_BUILD_TYPE}" MATCHES "Release")
	set (CPACK_STRIP_FILES TRUE)
endif ("${CMAKE_BUILD_TYPE}" MATCHES "Release")

if (APPLE)
	set (CPACK_GENERATOR "PACKAGEMAKER;OSXX11")
endif (APPLE)
if (UNIX)
	# Try to find architecture
	execute_process (COMMAND uname -m OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE)
	string (STRIP "${CPACK_PACKAGE_ARCHITECTURE}" CPACK_PACKAGE_ARCHITECTURE)

	# Try to find distro name and distro-specific arch
	execute_process (COMMAND lsb_release -is OUTPUT_VARIABLE LSB_ID)
	execute_process (COMMAND lsb_release -rs OUTPUT_VARIABLE LSB_RELEASE)
	string (STRIP "${LSB_ID}" LSB_ID)
	string (STRIP "${LSB_RELEASE}" LSB_RELEASE)
	set (LSB_DISTRIB "${LSB_ID}${LSB_RELEASE}")
	if (NOT LSB_DISTRIB)
		set (LSB_DISTRIB "unix")
	endif (NOT LSB_DISTRIB)

	# For Debian-based distros we want to create DEB packages.
	if ("${LSB_DISTRIB}" MATCHES "Ubuntu|Debian")
		set (CPACK_GENERATOR "DEB")
		set (CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
		set (CPACK_DEBIAN_PACKAGE_SECTION "libs")
		set (CPACK_DEBIAN_PACKAGE_RECOMMENDS "")

		# We need to alter the architecture names as per distro rules
		if ("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
			set (CPACK_PACKAGE_ARCHITECTURE i386)
		endif ("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
		if ("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
			set (CPACK_PACKAGE_ARCHITECTURE amd64)
		endif ("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")

		# Set the dependencies based on the distro version
		# thus only one package is build you must list here *any* depending libraries,
		# even if they are only used in one module
		if ("${LSB_DISTRIB}" MATCHES "Debian5.*")
			set (CPACK_DEBIAN_PACKAGE_DEPENDS " ")
		endif ("${LSB_DISTRIB}" MATCHES "Debian5.*")
		if ("${LSB_DISTRIB}" MATCHES "Debian6.*")
			set (CPACK_DEBIAN_PACKAGE_DEPENDS " ")
		endif ("${LSB_DISTRIB}" MATCHES "Debian6.*")
		if ("${LSB_DISTRIB}" MATCHES "Debian7.*")
			set (CPACK_DEBIAN_PACKAGE_DEPENDS " ") # if build with no plugins
		endif ("${LSB_DISTRIB}" MATCHES "Debian7.*")
		if (NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
			message ("WARNING: ${LSB_DISTRIB} not supported yet.\nPlease set deps in cmake/ElektraPackaging.cmake before packaging.")
		endif (NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
	endif ("${LSB_DISTRIB}" MATCHES "Ubuntu|Debian")
	set (CPACK_SYSTEM_NAME "${LSB_DISTRIB}-${CPACK_PACKAGE_ARCHITECTURE}")
	message (STATUS "Detected ${CPACK_SYSTEM_NAME}. Use make package to build packages (${CPACK_GENERATOR}).")
endif (UNIX)


set(CPACK_RPM_SPEC_MORE_DEFINE "%define ignore \#") 

set(CPACK_RPM_USER_FILELIST
	"%ignore /etc/profile.d"
	"%ignore /etc"
	"%ignore /etc/bash_completion.d"
	"%ignore /usr"
	"%ignore /usr/local"
	"%ignore /usr/local/bin"
	"%ignore /usr/local/include"
	"%ignore /usr/local/lib"
	"%ignore /usr/local/share"
	"%ignore /usr/local/share/man"
	"%ignore /etc"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /etc/bash_completion.d"
	"%ignore /usr"
	"%ignore /usr/bin"
	"%ignore /usr/include"
	"%ignore /usr/lib"
	"%ignore /usr/share"
	"%ignore /usr/share/doc"
	"%ignore /usr/share/man"
	)

include (CPack)
