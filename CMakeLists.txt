cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-astroberry-diy CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")
include(GNUInstallDirs)

set (VERSION_MAJOR 2)
set (VERSION_MINOR 4)

find_package(INDI REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_rpifocus.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_rpifocus.xml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_rpibrd.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_rpibrd.xml)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/../gpio_lib_c/wiringPi)

include(CMakeCommon)

set(GPIO_LIBRARIES "wiringPi")

################ Astroberry Focuser ################
set(indi_rpifocus_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/rpi_focus.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indi_rpifocus indi_rpifocus_SRCS 6 cpp)
ENDIF ()

add_executable(indi_rpifocus ${indi_rpifocus_SRCS})
target_link_libraries(indi_rpifocus ${INDI_DRIVER_LIBRARIES} ${GPIO_LIBRARIES})
install(TARGETS indi_rpifocus RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_rpifocus.xml DESTINATION ${INDI_DATA_DIR})

################ Astroberry Board ################
set(indi_rpibrd_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/rpi_brd.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indi_rpibrd indi_rpibrd_SRCS 6 cpp)
ENDIF ()

add_executable(indi_rpibrd ${indi_rpibrd_SRCS})
target_link_libraries(indi_rpibrd ${INDI_LIBRARIES} ${GPIO_LIBRARIES})
install(TARGETS indi_rpibrd RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_rpibrd.xml DESTINATION ${INDI_DATA_DIR})

