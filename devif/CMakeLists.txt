cmake_minimum_required(VERSION 2.6)

project(devif)

set(SRCS ${SRCS} adapt.c com.c comif.c delayedgetrules.c delayedsetrules.c delayedif.c delayedlist.c getrules.c helperif.c init.c modify.c quantify.c regop.c regutil.c setrules.c stdif.c userif.c listutil.c regutil.c)

option(SHARED "Build a shared library." OFF)


if(SHARED)
    set(mode SHARED)
else()
    set(mode STATIC)
endif()

set(DEVIF_TRACE "0x00")
set(VARIADIC_DECLARATION ...)
set(VARIABLE_ARGUMENT __VA_ARGS__)


if(${CMAKE_BUILD_TYPE} MATCHES [Dd][Ee][Bb][Uu][Gg])
    set(CONFIG_DEVIF_DEBUG true)
    message("
    DEBUG MODE
    ----------")
elseif(${CMAKE_BUILD_TYPE} MATCHES [Rr][Ee][Ll][Ee][Aa][Ss][Ee])
    unset(CONFIG_DEVIF_DEBUG)
    message("
    RELEASE MODE
    ------------")
endif()


if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS_DEBUG "-Wall ${CMAKE_C_FLAGS_DEBUG}")
    set(VARIADIC_DECLARATION args...)
    set(VARIABLE_ARGUMENT args)
endif()


if(CMAKE_CONFIGURATION_TYPES)
   foreach(config ${CMAKE_CONFIGURATION_TYPES})
       if(${config} MATCHES [Dd][Ee][Bb][Uu][Gg])
           set(CONFIG_DEVIF_DEBUG true)
       else()
           unset(CONFIG_DEVIF_DEBUG)
       endif()
       configure_file("${PROJECT_SOURCE_DIR}/devifdebug.h.in" "${PROJECT_BINARY_DIR}/${config}/devifdebug.h")
   endforeach(config)
else()
    configure_file("${PROJECT_SOURCE_DIR}/devifdebug.h.in" "${PROJECT_BINARY_DIR}/devifdebug.h")
endif()


include_directories("${PROJECT_SOURCE_DIR}/.." "${PROJECT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")    #CMAKE_CFG_INTDIR evaluates at runtime for Vistual Studio to define at build time


add_library(devif ${mode} ${SRCS})

set_target_properties(devif PROPERTIES COMPILE_DEFINITIONS DEVIF)
