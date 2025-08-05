/**
 * @file ListStorage_test.cxx ListStorage class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "../src/ListStorage.hpp"
#include "../src/CommonIssues.hpp"

#define BOOST_TEST_MODULE ListStorage_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

BOOST_AUTO_TEST_SUITE(ListStorage_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  ListStorage stor;
  BOOST_REQUIRE_EQUAL(stor.size(), 0);
  BOOST_REQUIRE(!stor.has_list(0));
}

BOOST_AUTO_TEST_CASE(Methods)
{
  ListStorage stor;
  BOOST_REQUIRE_EQUAL(stor.size(), 0);

  IntList firstList(1, 1, { 3, 4, 5, 6 });
  IntList secondList(2, 1, { 7, 8 });
  IntList thirdList(3, 1, { 9, 10, 11, 12, 13 });
  IntList anotherSecondList(2, 2, { 14, 15, 16 });

  stor.set_capacity(2);
  BOOST_REQUIRE_EQUAL(stor.capacity(), 2);

  stor.add_list(firstList);
  BOOST_REQUIRE_EQUAL(stor.size(), 1);
  BOOST_REQUIRE(stor.has_list(1));
  auto retrieved = stor.get_list(1);
  BOOST_REQUIRE_EQUAL(retrieved.list_id, firstList.list_id);
  BOOST_REQUIRE_EQUAL(retrieved.generator_id, firstList.generator_id);
  BOOST_REQUIRE_EQUAL(retrieved.list.size(), firstList.list.size());

  stor.add_list(secondList);
  BOOST_REQUIRE_EQUAL(stor.size(), 2);
  BOOST_REQUIRE(stor.has_list(1));
  BOOST_REQUIRE(stor.has_list(2));

  stor.add_list(thirdList);
  BOOST_REQUIRE_EQUAL(stor.size(), 2);
  BOOST_REQUIRE(!stor.has_list(1));
  BOOST_REQUIRE(stor.has_list(2));
  BOOST_REQUIRE(stor.has_list(3));

  BOOST_REQUIRE_EXCEPTION(stor.get_list(1), ListNotFound, [&](ListNotFound) { return true; });
  BOOST_REQUIRE_EXCEPTION(stor.add_list(anotherSecondList), ListExists, [&](ListExists) { return true; });
  stor.add_list(anotherSecondList, true);
  BOOST_REQUIRE_EQUAL(stor.size(), 2);
  BOOST_REQUIRE(!stor.has_list(1));
  BOOST_REQUIRE(stor.has_list(2));
  BOOST_REQUIRE(stor.has_list(3));
  retrieved = stor.get_list(2);
  BOOST_REQUIRE_EQUAL(retrieved.list_id, anotherSecondList.list_id);
  BOOST_REQUIRE_EQUAL(retrieved.generator_id, anotherSecondList.generator_id);
  BOOST_REQUIRE_EQUAL(retrieved.list.size(), anotherSecondList.list.size());

  stor.flush();
  BOOST_REQUIRE_EQUAL(stor.size(), 0);
  BOOST_REQUIRE(!stor.has_list(1));
  BOOST_REQUIRE(!stor.has_list(2));
  BOOST_REQUIRE(!stor.has_list(3));
}

BOOST_AUTO_TEST_SUITE_END()
