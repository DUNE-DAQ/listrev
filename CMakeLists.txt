cmake_minimum_required(VERSION 3.12)
project(listrev VERSION 1.1.0)

find_package(daq-cmake REQUIRED )

daq_setup_environment()

find_package(appfwk REQUIRED)

daq_add_plugin(ListReverser duneDAQModule	     LINK_LIBRARIES appfwk::appfwk)
daq_add_plugin(RandomDataListGenerator duneDAQModule SCHEMA LINK_LIBRARIES appfwk::appfwk)
daq_add_plugin(ReversedListValidator duneDAQModule   LINK_LIBRARIES appfwk::appfwk)

daq_install()
