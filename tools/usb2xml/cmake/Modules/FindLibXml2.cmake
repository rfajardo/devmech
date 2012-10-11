include(LibFindMacros)

# Dependencies
#libfind_package(LibXml2 libc)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LIBXML2_PKGCONF libxml-2.0)

# Include dir
find_path(LIBXML2_INCLUDE_DIR
  NAMES libxml/xpath.h
  PATHS ${LIBXML2_PKGCONF_INCLUDE_DIRS} /usr/include/libxml2
)

# Finally the library itself
find_library(LIBXML2_LIBRARY
  NAMES xml2
  PATHS ${LIBXML2_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LibXml2_PROCESS_INCLUDES LIBXML2_INCLUDE_DIR)    #LIBXML2_INCLUDE_DIRS
set(LibXml2_PROCESS_LIBS LIBXML2_LIBRARY)            #LIBXML2_LIBRARIES
libfind_process(LibXml2)