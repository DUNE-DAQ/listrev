cmake_minimum_required(VERSION 3.12)
project(listrev VERSION 3.2.0)

find_package(daq-cmake REQUIRED )

daq_setup_environment()

find_package(rcif REQUIRED)
find_package(appfwk REQUIRED)
find_package(opmonlib REQUIRED)
find_package(logging REQUIRED)
find_package(daqconf REQUIRED)
find_package(ers REQUIRED)

daq_codegen( randomdatalistgenerator.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)
daq_codegen( randomdatalistgeneratorinfo.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )
daq_codegen( confgen.jsonnet DEP_PKGS daqconf TEMPLATES Structs.hpp.j2)

daq_add_plugin(ListReverser duneDAQModule	     LINK_LIBRARIES appfwk::appfwk)
daq_add_plugin(RandomDataListGenerator duneDAQModule LINK_LIBRARIES appfwk::appfwk)
daq_add_plugin(ReversedListValidator duneDAQModule   LINK_LIBRARIES appfwk::appfwk)

daq_install()
