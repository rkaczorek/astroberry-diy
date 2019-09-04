cmake_minimum_required(VERSION 2.4.7)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT(astroberry CXX C)
include(ExternalProject)

set (VERSION_MAJOR 2)
set (VERSION_MINOR 3)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

option(WITH_WIRINGPI "Use WiringPi as a low level hardware library" OFF)

find_package(INDI REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_rpifocus.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_rpifocus.xml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_rpibrd.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_rpibrd.xml)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})

#include_directories(${CMAKE_SOURCE_DIR}/libs/bcm2835-1.52/src)
#include_directories(${CMAKE_SOURCE_DIR}/../gpio_lib_c/wiringPi)

#add_library(bcm2835 STATIC IMPORTED)
#set_property(TARGET bcm2835 PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/bcm2835-1.52/src/libbcm2835.a)
#add_dependencies(bcm2835 libbcm2835 wiringPi)

if(WITH_WIRINGPI)
	include_directories(${CMAKE_SOURCE_DIR}/../gpio_lib_c/wiringPi)
	add_definitions(-DWIRINGPI)
	set (ASTROBERRY_LIBRARIES "wiringPi")
else()
	include_directories(${CMAKE_SOURCE_DIR}/libs/bcm2835-1.52/src)
	add_library(bcm2835 STATIC IMPORTED)
	set_property(TARGET bcm2835 PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/bcm2835-1.52/src/libbcm2835.a)
	add_dependencies(bcm2835 libbcm2835)
	set (ASTROBERRY_LIBRARIES "bcm2835")

	ExternalProject_Add(libbcm2835
		URL "http://www.airspayce.com/mikem/bcm2835/bcm2835-1.60.tar.gz"
		SOURCE_DIR ${CMAKE_SOURCE_DIR}/libs/bcm2835-1.52
		PREFIX ${CMAKE_CURRENT_BINARY_DIR}/bcm2835-1.52
		CONFIGURE_COMMAND ./configure
		BUILD_COMMAND ${MAKE}
		INSTALL_COMMAND ""
		BUILD_IN_SOURCE 1
	)
endif()

################ Astroberry Focuser ################

set(indi_rpifocus_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/rpi_focus.cpp
   )

add_executable(indi_rpifocus ${indi_rpifocus_SRCS})
#target_link_libraries(indi_rpifocus indidriver bcm2835 wiringPi crypt)
target_link_libraries(indi_rpifocus indidriver ${ASTROBERRY_LIBRARIES})
install(TARGETS indi_rpifocus RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_rpifocus.xml DESTINATION ${INDI_DATA_DIR})

################ Astroberry Board ################

set(indi_rpibrd_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/rpi_brd.cpp
   )

add_executable(indi_rpibrd ${indi_rpibrd_SRCS})
#target_link_libraries(indi_rpibrd indidriver bcm2835 wiringPi crypt)
target_link_libraries(indi_rpibrd indidriver ${ASTROBERRY_LIBRARIES})
install(TARGETS indi_rpibrd RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_rpibrd.xml DESTINATION ${INDI_DATA_DIR})
