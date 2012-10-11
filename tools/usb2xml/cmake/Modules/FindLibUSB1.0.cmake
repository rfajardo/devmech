include(LibFindMacros)

# Dependencies
#libfind_package(LibUSB1.0 libc)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LIBUSB10_PKGCONF libusb-1.0)

# Include dir
find_path(LIBUSB10_INCLUDE_DIR
  NAMES libusb.h
  PATHS ${LIBUSB10_PKGCONF_INCLUDE_DIRS} 
  HINTS /usr/include/libusb-1.0
)

# Finally the library itself
find_library(LIBUSB10_LIBRARY
  NAMES usb-1.0
  PATHS ${LIBUSB10_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LibUSB1.0_PROCESS_INCLUDES LIBUSB10_INCLUDE_DIR)	#LIBUSB10_INCLUDE_DIRS
set(LibUSB1.0_PROCESS_LIBS LIBUSB10_LIBRARY)	        #LIBUSB10_LIBRARIES
libfind_process(LibUSB1.0)