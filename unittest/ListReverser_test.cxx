/**
 * @file ListReverser_test.cxx ListReverser plugin Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ListReverser.hpp"

#include "appfwk/ConfigurationManager.hpp"
#include "opmonlib/TestOpMonManager.hpp"
#include "serialization/Serialization.hpp"

#define BOOST_TEST_MODULE ListReverser_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

BOOST_AUTO_TEST_SUITE(ListReverser_test)

struct ConfigurationTestFixture
{
  ConfigurationTestFixture()
  {
    std::string sessionName = "lr-session";
    std::string appName = "listrev";
    std::string TEST_OKS_DB = "oksconflibs:config/lrSession-singleapp.data.xml";
    cfgmgr = std::make_shared<dunedaq::appfwk::ConfigurationManager>(TEST_OKS_DB, appName, sessionName);
    dunedaq::get_iomanager()->configure(sessionName, cfgmgr->queues(), cfgmgr->networkconnections(), nullptr, opmgr);
  }
  ~ConfigurationTestFixture()
  {
    dunedaq::get_iomanager()->reset();
    cfgmgr = nullptr;
  }

  ConfigurationTestFixture(ConfigurationTestFixture const&) = default;
  ConfigurationTestFixture(ConfigurationTestFixture&&) = default;
  ConfigurationTestFixture& operator=(ConfigurationTestFixture const&) = default;
  ConfigurationTestFixture& operator=(ConfigurationTestFixture&&) = default;

  dunedaq::opmonlib::TestOpMonManager opmgr;
  std::shared_ptr<dunedaq::appfwk::ConfigurationManager> cfgmgr;
};

BOOST_FIXTURE_TEST_CASE(Commands, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ListReverser", "lr0");
  opmgr.register_node("lr0", mod);
  BOOST_REQUIRE(mod->has_command("start"));
  BOOST_REQUIRE(mod->has_command("stop"));

  mod->init(cfgmgr);
  mod->execute_command("start");
  auto metrics = opmgr.collect();
  mod->execute_command("stop");

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ListReverserInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);

  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_received").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_sent").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_received").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_sent").uint8_value(), 0);
}

BOOST_FIXTURE_TEST_CASE(Requests, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ListReverser", "lr0");
  opmgr.register_node("lr0", mod);

  auto requestReceiver = dunedaq::get_iomanager()->get_receiver<RequestList>("rdlg0_request_queue");
  auto listReceiver = dunedaq::get_iomanager()->get_receiver<ReversedList>("validator_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("start");

  auto requestSender = dunedaq::get_iomanager()->get_sender<RequestList>("lr0_request_queue");

  // ListReverser receives request and forwards to data list generator
  RequestList theRequest(1, "validator_list_queue");
  requestSender->send(std::move(theRequest), std::chrono::milliseconds(1000));
  auto received = requestReceiver->receive(std::chrono::milliseconds(1000));
  BOOST_REQUIRE_EQUAL(received.list_id, 1);
  BOOST_REQUIRE_EQUAL(received.destination, "lr0_list_queue");

  auto metrics = opmgr.collect();
  mod->execute_command("stop");

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ListReverserInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);

  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_received").uint8_value(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_sent").uint8_value(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_received").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_sent").uint8_value(), 0);
}

BOOST_FIXTURE_TEST_CASE(Lists, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ListReverser", "lr0");
  opmgr.register_node("lr0", mod);

  auto requestReceiver = dunedaq::get_iomanager()->get_receiver<RequestList>("rdlg0_request_queue");
  auto listReceiver = dunedaq::get_iomanager()->get_receiver<ReversedList>("validator_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("start");

  auto requestSender = dunedaq::get_iomanager()->get_sender<RequestList>("lr0_request_queue");
  auto listSender = dunedaq::get_iomanager()->get_sender<IntList>("lr0_list_queue");

  RequestList theRequest(2, "validator_list_queue");
  IntList theList(2, 0, { 3, 4, 5, 6 });

  requestSender->send(std::move(theRequest), std::chrono::milliseconds(1000));
  auto receivedRequest = requestReceiver->receive(std::chrono::milliseconds(1000));
  BOOST_REQUIRE_EQUAL(receivedRequest.list_id, 2);
  BOOST_REQUIRE_EQUAL(receivedRequest.destination, "lr0_list_queue");

  listSender->send(std::move(theList), std::chrono::milliseconds(1000));
  auto receivedList = listReceiver->receive(std::chrono::milliseconds(1000));

  BOOST_REQUIRE_EQUAL(receivedList.list_id, 2);
  BOOST_REQUIRE_EQUAL(receivedList.reverser_id, 0);
  BOOST_REQUIRE_EQUAL(receivedList.lists.size(), 1);
  BOOST_REQUIRE_EQUAL(receivedList.lists[0].original.list[0], 3);
  BOOST_REQUIRE_EQUAL(receivedList.lists[0].reversed.list[0], 6);

  auto metrics = opmgr.collect();
  mod->execute_command("stop");

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ListReverserInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);

  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_received").uint8_value(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("requests_sent").uint8_value(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_received").uint8_value(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("lists_sent").uint8_value(), 1);
}

BOOST_AUTO_TEST_SUITE_END()
