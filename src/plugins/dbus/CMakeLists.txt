find_package(DBus)

if (DBUS_FOUND)
	add_plugin(dbus
		SOURCES
			dbus.h dbus.c
			sendmessage.c receivemessage.c
		INCLUDE_DIRECTORIES
			${DBUS_INCLUDE_DIR}
			${DBUS_ARCH_INCLUDE_DIR}
		LINK_LIBRARIES
			${DBUS_LIBRARIES}
		)

	include_directories(${DBUS_INCLUDE_DIR})
	include_directories(${DBUS_ARCH_INCLUDE_DIR})
	add_plugintest (dbus)
else (DBUS_FOUND)
	remove_plugin (dbus "dbus not found")
endif (DBUS_FOUND)
