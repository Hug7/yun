#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "wx_route::route_domain" for configuration "Debug"
set_property(TARGET wx_route::route_domain APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(wx_route::route_domain PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/route_domain.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS wx_route::route_domain )
list(APPEND _IMPORT_CHECK_FILES_FOR_wx_route::route_domain "${_IMPORT_PREFIX}/lib/route_domain.lib" )

# Import target "wx_route::route_tsp" for configuration "Debug"
set_property(TARGET wx_route::route_tsp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(wx_route::route_tsp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/route_tsp.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS wx_route::route_tsp )
list(APPEND _IMPORT_CHECK_FILES_FOR_wx_route::route_tsp "${_IMPORT_PREFIX}/lib/route_tsp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
