cmake_minimum_required(VERSION 3.12)
project(listrev VERSION 3.3.0)

find_package(daq-cmake REQUIRED )

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(opmonlib REQUIRED)

find_package(genconfig REQUIRED)
find_package(coredal REQUIRED)
include_directories(${CMAKE_BINARY_DIR}/coredal ${CMAKE_BINARY_DIR}/listrev)
daq_oks_codegen(listrev.schema.xml NAMESPACE dunedaq::listrev::dal
  DALDIR listrev/dal DEP_PKGS coredal)


daq_codegen( listreverser.jsonnet randomdatalistgenerator.jsonnet reversedlistvalidator.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_codegen( listreverserinfo.jsonnet randomdatalistgeneratorinfo.jsonnet reversedlistvalidatorinfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

daq_add_library(ListCreator.cpp ListStorage.cpp LINK_LIBRARIES  appfwk::appfwk)

daq_add_plugin(ListReverser            duneDAQModule LINK_LIBRARIES listrev)
daq_add_plugin(RandomDataListGenerator duneDAQModule LINK_LIBRARIES listrev)
daq_add_plugin(ReversedListValidator   duneDAQModule LINK_LIBRARIES listrev)

daq_install()
