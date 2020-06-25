cmake_minimum_required(VERSION 3.12)
project(listrev)

##############################################################################
point_build_to( src )

add_library(listrev_ListReverser_duneDAQModule src/ListReverser.cpp)
target_link_libraries(listrev_ListReverser_duneDAQModule appfwk)

add_library(listrev_RandomDataListGenerator_duneDAQModule src/RandomDataListGenerator.cpp)
target_link_libraries(listrev_RandomDataListGenerator_duneDAQModule appfwk)

add_library(listrev_ReversedListValidator_duneDAQModule src/ReversedListValidator.cpp)
target_link_libraries(listrev_ReversedListValidator_duneDAQModule appfwk)

##############################################################################
point_build_to( test )

file(COPY test/list_reversal_app.json DESTINATION test)
