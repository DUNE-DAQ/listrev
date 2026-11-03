/**
 * @file ListCreator_test.cxx ListCreator class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "../src/ListCreator.hpp"

#include "iomanager/IOManager.hpp"
#include "opmonlib/TestOpMonManager.hpp"

#define BOOST_TEST_MODULE ListCreator_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

const std::string TEST_OKS_DB = "config/lrSession.data.xml";

BOOST_AUTO_TEST_SUITE(ListCreator_test)

struct ConfigurationTestFixture
{
  ConfigurationTestFixture()
  {
    confdb = std::make_shared<dunedaq::conffwk::Configuration>("oksconflibs:" + TEST_OKS_DB);
    confdb->get<dunedaq::confmodel::Queue>(queues);
    confdb->get<dunedaq::confmodel::NetworkConnection>(connections);

    queue_id = dunedaq::iomanager::ConnectionId{ "creates_queue", "CreateList" };

    dunedaq::get_iomanager()->configure(
      "IOManager_t", queues, connections, nullptr, opmgr); // Not using connectivity service
  }
  ~ConfigurationTestFixture() { dunedaq::get_iomanager()->reset(); }

  ConfigurationTestFixture(ConfigurationTestFixture const&) = default;
  ConfigurationTestFixture(ConfigurationTestFixture&&) = default;
  ConfigurationTestFixture& operator=(ConfigurationTestFixture const&) = default;
  ConfigurationTestFixture& operator=(ConfigurationTestFixture&&) = default;

  dunedaq::iomanager::ConnectionId queue_id;

  std::shared_ptr<dunedaq::conffwk::Configuration> confdb;
  std::vector<const dunedaq::confmodel::Queue*> queues;
  std::vector<const dunedaq::confmodel::NetworkConnection*> connections;

  dunedaq::opmonlib::TestOpMonManager opmgr;
};

BOOST_AUTO_TEST_CASE(BasicTests)
{
  ListCreator create;
  BOOST_REQUIRE_EXCEPTION(create.send_create(0),
                          dunedaq::iomanager::TimeoutExpired,
                          [&](dunedaq::iomanager::TimeoutExpired) { return true; });
}

BOOST_FIXTURE_TEST_CASE(SendCreateQueue, ConfigurationTestFixture)
{
  auto receiver = dunedaq::get_iomanager()->get_receiver<CreateList>(queue_id);
  size_t min_size = 1;
  size_t max_size = 10;
  ListCreator create("creates_queue", std::chrono::milliseconds(10), min_size, max_size);

  auto sz = create.send_create(0);
  auto ret = receiver->receive(dunedaq::iomanager::Receiver::s_no_block);
  BOOST_REQUIRE_EQUAL(ret.list_id, 0);
  BOOST_REQUIRE(ret.list_size >= min_size);
  BOOST_REQUIRE(ret.list_size <= max_size);
  BOOST_REQUIRE_EQUAL(sz, ret.list_size);

  sz = create.send_create(1);
  ret = receiver->receive(dunedaq::iomanager::Receiver::s_no_block);
  BOOST_REQUIRE_EQUAL(ret.list_id, 1);
  BOOST_REQUIRE(ret.list_size >= min_size);
  BOOST_REQUIRE(ret.list_size <= max_size);
  BOOST_REQUIRE_EQUAL(sz, ret.list_size);
}

BOOST_FIXTURE_TEST_CASE(MaxAndMin, ConfigurationTestFixture)
{
  // Minimum list size is 1
  auto receiver = dunedaq::get_iomanager()->get_receiver<CreateList>(queue_id);
  ListCreator create("creates_queue", std::chrono::milliseconds(10), -2, 1);

  auto sz = create.send_create(0);
  auto ret = receiver->receive(dunedaq::iomanager::Receiver::s_no_block);
  BOOST_REQUIRE_EQUAL(ret.list_id, 0);
  BOOST_REQUIRE_EQUAL(ret.list_size, 1);
  BOOST_REQUIRE_EQUAL(sz, 1);

  // If max < min, then max = min
  ListCreator create2("creates_queue", std::chrono::milliseconds(10), 2, 1);

  sz = create2.send_create(0);
  ret = receiver->receive(dunedaq::iomanager::Receiver::s_no_block);
  BOOST_REQUIRE_EQUAL(ret.list_id, 0);
  BOOST_REQUIRE_EQUAL(ret.list_size, 2);
  BOOST_REQUIRE_EQUAL(sz, 2);
}

BOOST_AUTO_TEST_SUITE_END()
