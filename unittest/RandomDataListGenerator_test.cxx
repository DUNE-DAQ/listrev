/**
 * @file RandomDataListGenerator_test.cxx RandomDataListGenerator plugin Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "RandomDataListGenerator.hpp"

#include "appfwk/ConfigurationManager.hpp"
#include "opmonlib/TestOpMonManager.hpp"
#include "serialization/Serialization.hpp"

#define BOOST_TEST_MODULE RandomDataListGenerator_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

BOOST_AUTO_TEST_SUITE(RandomDataListGenerator_test)

struct ConfigurationTestFixture
{
  ConfigurationTestFixture()
  {
    std::string sessionName = "lr-session";
    std::string appName = "listrev";
    std::string TEST_OKS_DB = "oksconflibs:config/lrSession-singleapp.data.xml";
    cfgmgr = std::make_shared<dunedaq::appfwk::ConfigurationManager>(TEST_OKS_DB, appName, sessionName);
    dunedaq::get_iomanager()->configure(
      sessionName, "localhost", cfgmgr->get_queues(), cfgmgr->get_networkconnections(), nullptr, opmgr);
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
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("RandomDataListGenerator", "rdlg0");
  opmgr.register_node("rdlg0", mod);
  BOOST_REQUIRE(mod->has_command("conf"));
  BOOST_REQUIRE(mod->has_command("start"));
  BOOST_REQUIRE(mod->has_command("stop"));
  BOOST_REQUIRE(mod->has_command("scrap"));
  BOOST_REQUIRE(mod->has_command("hello"));

  mod->init(cfgmgr);
  mod->execute_command("conf");
  mod->execute_command("start");
  mod->execute_command("hello");
  auto metrics = opmgr.collect();
  mod->execute_command("stop");
  mod->execute_command("scrap");

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.RandomListGeneratorInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);

  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_generated_numbers").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_lists_sent").uint8_value(), 0);
}

BOOST_FIXTURE_TEST_CASE(Lists, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("RandomDataListGenerator", "rdlg0");
  opmgr.register_node("rdlg0", mod);
  auto facility = opmgr.get_backend_facility();

  auto listReceiver = dunedaq::get_iomanager()->get_receiver<IntList>("lr0_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("conf");
  mod->execute_command("start");

  auto requestSender = dunedaq::get_iomanager()->get_sender<RequestList>("rdlg0_request_queue");
  auto createsSender = dunedaq::get_iomanager()->get_sender<CreateList>("creates_queue");

  RequestList requestOne(1, "lr0_list_queue");
  RequestList requestTwo(2, "lr0_list_queue");
  RequestList requestThree(3, "lr0_list_queue");
  CreateList createTwo(2, 1);
  CreateList createThree(3, 20);

  // Send a request before create message
  requestSender->send(std::move(requestOne), std::chrono::milliseconds(1000));
  BOOST_REQUIRE_EXCEPTION(listReceiver->receive(std::chrono::milliseconds(11000)),
                          dunedaq::iomanager::TimeoutExpired,
                          [&](dunedaq::iomanager::TimeoutExpired) { return true; });

  createsSender->send(std::move(createTwo), std::chrono::milliseconds(1000));
  createsSender->send(std::move(createThree), std::chrono::milliseconds(1000));

  // No request -> no response
  BOOST_REQUIRE_EXCEPTION(listReceiver->receive(std::chrono::milliseconds(1000)),
                          dunedaq::iomanager::TimeoutExpired,
                          [&](dunedaq::iomanager::TimeoutExpired) { return true; });

  auto metrics = opmgr.collect();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.RandomListGeneratorInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_generated_numbers").uint8_value(), 2);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_lists_sent").uint8_value(), 0);

  requestSender->send(std::move(requestTwo), std::chrono::milliseconds(1000));
  auto listTwo = listReceiver->receive(std::chrono::milliseconds(1000));
  BOOST_REQUIRE_EQUAL(listTwo.list_id, 2);
  BOOST_REQUIRE_EQUAL(listTwo.list.size(), 1);

  requestSender->send(std::move(requestThree), std::chrono::milliseconds(1000));
  auto listThree = listReceiver->receive(std::chrono::milliseconds(1000));
  BOOST_REQUIRE_EQUAL(listThree.list_id, 3);
  BOOST_REQUIRE_EQUAL(listThree.list.size(), 20);

  metrics = opmgr.collect();
  entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.RandomListGeneratorInfo"));
  mod->execute_command("stop");
  mod->execute_command("scrap");

  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_generated_numbers").uint8_value(), 0);
  BOOST_REQUIRE_EQUAL(entries.front().data().at("new_lists_sent").uint8_value(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
