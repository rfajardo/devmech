cmake_minimum_required(VERSION 2.6)

project(usbxml)

set(SRCS ${SRCS} devtree.c lsusb-t.c lsusb.c names.c usbmisc.c getusb.c 
dump.c std_dump.c classed_dump.c xml.c std_xml.c classed_xml.c)

if(${CMAKE_BUILD_TYPE} MATCHES [Dd][Ee][Bb][Uu][Gg])
    message("
    DEBUG MODE
    ----------")
elseif(${CMAKE_BUILD_TYPE} MATCHES [Rr][Ee][Ll][Ee][Aa][Ss][Ee])
    message("
    RELEASE MODE
    ------------")
endif()

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_DEBUG "-Wall ${CMAKE_C_FLAGS_DEBUG}")
endif()

set(REQUIRED_LIBRARIES ${REQUIRED_LIBRARIES} LibUSB1.0 LibXml2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
foreach(i ${REQUIRED_LIBRARIES})
    find_package(${i} REQUIRED)
    if (NOT ${i}_FOUND)
        message(FATAL_ERROR "Library ${i} is required but was not found!")
    else()
        include_directories(${${i}_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${${i}_LIBRARIES})
    endif()
endforeach()

add_executable(usbxml ${SRCS})

target_link_libraries(usbxml ${LIBS})