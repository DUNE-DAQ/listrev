cmake_minimum_required(VERSION 3.12)
project(listrev VERSION 1.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../daq-buildtools/cmake ${CMAKE_MODULE_PATH})
find_package(daq-buildtools)

daq_setup_environment()

find_package(appfwk REQUIRED)

##############################################################################
daq_point_build_to( src )

add_library(listrev_ListReverser_duneDAQModule src/ListReverser.cpp)
target_link_libraries(listrev_ListReverser_duneDAQModule appfwk::appfwk)

add_library(listrev_RandomDataListGenerator_duneDAQModule src/RandomDataListGenerator.cpp)
target_link_libraries(listrev_RandomDataListGenerator_duneDAQModule appfwk::appfwk)

add_library(listrev_ReversedListValidator_duneDAQModule src/ReversedListValidator.cpp)
target_link_libraries(listrev_ReversedListValidator_duneDAQModule appfwk::appfwk)

##############################################################################
daq_point_build_to( test )

file(COPY test/list_reversal_app.json DESTINATION test)

daq_install(TARGETS listrev_ReversedListValidator_duneDAQModule listrev_RandomDataListGenerator_duneDAQModule listrev_ListReverser_duneDAQModule )

target_include_directories(listrev_RandomDataListGenerator_duneDAQModule PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> )
