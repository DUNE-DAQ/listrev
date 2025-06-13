cmake_minimum_required(VERSION 3.12)
project(listrev VERSION 5.0.1)

find_package(daq-cmake REQUIRED )

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(opmonlib REQUIRED)

find_package(oksdalgen REQUIRED)
find_package(confmodel REQUIRED)

daq_add_dal_library(listrev.schema.xml NAMESPACE dunedaq::listrev::dal DALDIR dal DEP_PKGS confmodel)

daq_codegen( listreverser.jsonnet randomdatalistgenerator.jsonnet reversedlistvalidator.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_protobuf_codegen( opmon/*.proto )

daq_add_library(ListCreator.cpp ListStorage.cpp LINK_LIBRARIES  appfwk::appfwk confmodel::confmodel)

daq_add_plugin(ListReverser            duneDAQModule LINK_LIBRARIES listrev listrev_dal)
daq_add_plugin(RandomDataListGenerator duneDAQModule LINK_LIBRARIES listrev listrev_dal)
daq_add_plugin(ReversedListValidator   duneDAQModule LINK_LIBRARIES listrev listrev_dal)

daq_install()
