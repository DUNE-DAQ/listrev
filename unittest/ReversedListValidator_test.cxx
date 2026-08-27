/**
 * @file ReversedListValidator_test.cxx ReversedListValidator plugin Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ReversedListValidator.hpp"

#include "appfwk/ConfigurationManager.hpp"
#include "opmonlib/TestOpMonManager.hpp"
#include "serialization/Serialization.hpp"

#define BOOST_TEST_MODULE ReversedListValidator_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

BOOST_AUTO_TEST_SUITE(ReversedListValidator_test)

struct ConfigurationTestFixture
{
  ConfigurationTestFixture()
  {
    std::string sessionName = "lr-session";
    std::string appName = "listrev";
    std::string TEST_OKS_DB = "oksconflibs:config/lrSession-singleapp.data.xml";
    cfgmgr = std::make_shared<dunedaq::appfwk::ConfigurationManager>(TEST_OKS_DB, appName, sessionName);
    dunedaq::get_iomanager()->configure(sessionName,
                                        cfgmgr->get_queues(),
                                        cfgmgr->get_networkconnections(),
                                        cfgmgr->get_connection_overrides(),
                                        nullptr,
                                        opmgr);
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
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ReversedListValidator", "lrv");
  opmgr.register_node("lrv", mod);
  BOOST_REQUIRE(mod->has_command("start"));
  BOOST_REQUIRE(mod->has_command("stop"));

  mod->init(cfgmgr);
  mod->execute_command("start");

  auto metrics = opmgr.collect();

  mod->execute_command("stop");

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ReversedListValidatorInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("valid_list_pairs").uint8_value(), 0);
  BOOST_CHECK_EQUAL(entries.front().data().at("invalid_list_pairs").uint8_value(), 0);
  BOOST_CHECK_EQUAL(entries.front().data().at("total_lists").uint8_value(), 0);
}

BOOST_FIXTURE_TEST_CASE(ProcessList, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ReversedListValidator", "lrv");
  opmgr.register_node("lrv", mod);

  std::vector<CreateList> creates_received;
  auto create_callback = [&](CreateList c) { creates_received.push_back(c); };
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->add_callback(create_callback);

  std::vector<RequestList> requests_received;
  auto request_callback = [&](RequestList r) { requests_received.push_back(r); };
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->add_callback(request_callback);

  auto list_sender = dunedaq::get_iomanager()->get_sender<ReversedList>("validator_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("start");

  usleep(100000);

  BOOST_REQUIRE(creates_received.size() > 0);
  BOOST_REQUIRE(requests_received.size() > 0);
  BOOST_REQUIRE_EQUAL(creates_received[0].list_id, 1);
  BOOST_REQUIRE_EQUAL(requests_received[0].list_id, 1);

  size_t ii = 0;
  auto g = [&]() { return ++ii; };
  auto r = [&]() { return ii--; };

  ReversedList testList(1, 0, {});
  IntList original(1, 0, {});
  IntList reversed(1, 0, {});
  std::generate_n(std::back_inserter(original.list), creates_received[0].list_size, g);
  std::generate_n(std::back_inserter(reversed.list), creates_received[0].list_size, r);
  ReversedList::Data data;
  data.original = original;
  data.reversed = reversed;
  testList.lists.push_back(data);
  list_sender->send(std::move(testList), std::chrono::milliseconds(1000));

  usleep(10000);

  auto metrics = opmgr.collect();
  mod->execute_command("stop");
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->remove_callback();
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->remove_callback();

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ReversedListValidatorInfo"));
  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("valid_list_pairs").uint8_value(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("invalid_list_pairs").uint8_value(), 0);
  BOOST_CHECK_EQUAL(entries.front().data().at("total_lists").uint8_value(), 1);
}

BOOST_FIXTURE_TEST_CASE(InvalidList, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ReversedListValidator", "lrv");
  opmgr.register_node("lrv", mod);

  std::vector<CreateList> creates_received;
  auto create_callback = [&](CreateList c) { creates_received.push_back(c); };
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->add_callback(create_callback);

  std::vector<RequestList> requests_received;
  auto request_callback = [&](RequestList r) { requests_received.push_back(r); };
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->add_callback(request_callback);

  auto list_sender = dunedaq::get_iomanager()->get_sender<ReversedList>("validator_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("start");

  usleep(100000);

  BOOST_REQUIRE(creates_received.size() > 0);
  BOOST_REQUIRE(requests_received.size() > 0);
  BOOST_REQUIRE_EQUAL(creates_received[0].list_id, 1);
  BOOST_REQUIRE_EQUAL(requests_received[0].list_id, 1);

  size_t ii = 0;
  auto g = [&]() { return ++ii; };
  auto r = [&]() { return ii--; };

  ReversedList testList(1, 0, {});
  IntList original(1, 0, {});
  IntList reversed(1, 0, {});
  std::generate_n(std::back_inserter(original.list), creates_received[0].list_size, g);
  std::generate_n(std::back_inserter(reversed.list), creates_received[0].list_size, r);
  reversed.list[0]++; // Invalidate the list
  ReversedList::Data data;
  data.original = original;
  data.reversed = reversed;
  testList.lists.push_back(data);
  list_sender->send(std::move(testList), std::chrono::milliseconds(1000));

  usleep(10000);

  auto metrics = opmgr.collect();
  mod->execute_command("stop");
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->remove_callback();
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->remove_callback();

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ReversedListValidatorInfo"));

  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("valid_list_pairs").uint8_value(), 0);
  BOOST_CHECK_EQUAL(entries.front().data().at("invalid_list_pairs").uint8_value(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("total_lists").uint8_value(), 1);
}

BOOST_FIXTURE_TEST_CASE(WrongSizeList, ConfigurationTestFixture)
{
  std::shared_ptr<dunedaq::appfwk::DAQModule> mod = dunedaq::appfwk::make_module("ReversedListValidator", "lrv");
  opmgr.register_node("lrv", mod);

  std::vector<CreateList> creates_received;
  auto create_callback = [&](CreateList c) { creates_received.push_back(c); };
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->add_callback(create_callback);

  std::vector<RequestList> requests_received;
  auto request_callback = [&](RequestList r) { requests_received.push_back(r); };
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->add_callback(request_callback);

  auto list_sender = dunedaq::get_iomanager()->get_sender<ReversedList>("validator_list_queue");

  mod->init(cfgmgr);
  mod->execute_command("start");

  usleep(100000);

  BOOST_REQUIRE(creates_received.size() > 0);
  BOOST_REQUIRE(requests_received.size() > 0);
  BOOST_REQUIRE_EQUAL(creates_received[0].list_id, 1);
  BOOST_REQUIRE_EQUAL(requests_received[0].list_id, 1);

  size_t ii = 0;
  auto g = [&]() { return ++ii; };
  auto r = [&]() { return ii--; };

  ReversedList testList(1, 0, {});
  IntList original(1, 0, {});
  IntList reversed(1, 0, {});
  std::generate_n(std::back_inserter(original.list), creates_received[0].list_size + 1, g);
  std::generate_n(std::back_inserter(reversed.list), creates_received[0].list_size + 1, r);
  ReversedList::Data data;
  data.original = original;
  data.reversed = reversed;
  testList.lists.push_back(data);
  list_sender->send(std::move(testList), std::chrono::milliseconds(1000));

  usleep(10000);

  auto metrics = opmgr.collect();
  mod->execute_command("stop");
  dunedaq::get_iomanager()->get_receiver<CreateList>("creates_queue")->remove_callback();
  dunedaq::get_iomanager()->get_receiver<RequestList>("lr0_request_queue")->remove_callback();

  auto facility = opmgr.get_backend_facility();
  auto entries = facility->get_entries(std::regex("dunedaq.listrev.opmon.ReversedListValidatorInfo"));

  BOOST_REQUIRE_EQUAL(entries.size(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("valid_list_pairs").uint8_value(), 0);
  BOOST_CHECK_EQUAL(entries.front().data().at("invalid_list_pairs").uint8_value(), 1);
  BOOST_CHECK_EQUAL(entries.front().data().at("total_lists").uint8_value(), 1);
}
BOOST_AUTO_TEST_SUITE_END()
