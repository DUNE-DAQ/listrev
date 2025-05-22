/**
 * @file ListWrapper_test.cxx ListWrapper class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "../src/ListWrapper.hpp"

#include "serialization/Serialization.hpp"

#define BOOST_TEST_MODULE ListWrapper_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::listrev;

BOOST_AUTO_TEST_SUITE(ListWrapper_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  IntList intList;
  BOOST_REQUIRE_EQUAL(intList.list.size(), 0);
  ReversedList::Data reversedListData;
  BOOST_REQUIRE_EQUAL(reversedListData.original.list.size(), 0);
  BOOST_REQUIRE_EQUAL(reversedListData.reversed.list.size(), 0);
  ReversedList reversedList;
  BOOST_REQUIRE_EQUAL(reversedList.lists.size(), 0);
  CreateList createList;
  BOOST_REQUIRE_EQUAL(createList.list_size, 0);
  RequestList requestList;
  BOOST_REQUIRE_EQUAL(requestList.destination, "");
}

BOOST_AUTO_TEST_CASE(IntList_SerDes_MsgPack) {
  BOOST_REQUIRE(dunedaq::serialization::is_serializable<IntList>::value);
  BOOST_REQUIRE_EQUAL(dunedaq::datatype_to_string<IntList>(), "IntList");

  IntList intList;
  intList.list_id = 1;
  intList.generator_id = 2;
  intList.list = { 3, 4, 5, 6, 7 };

  IntList anotherList(8, 9, { 10, 11, 12 });
  
  auto bytes = dunedaq::serialization::serialize(intList, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << bytes.size() << " bytes";
  IntList intList_deserialized = dunedaq::serialization::deserialize<IntList>(bytes);

  BOOST_REQUIRE_EQUAL(intList.list_id, intList_deserialized.list_id);
  BOOST_REQUIRE_EQUAL(intList.generator_id, intList_deserialized.generator_id);
  BOOST_REQUIRE_EQUAL(intList.list.size(), intList_deserialized.list.size());
  BOOST_REQUIRE_EQUAL(intList.list[0], intList_deserialized.list[0]);

  auto more_bytes = dunedaq::serialization::serialize(anotherList, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << more_bytes.size() << " bytes";
  intList_deserialized = dunedaq::serialization::deserialize<IntList>(more_bytes);

  BOOST_REQUIRE_EQUAL(anotherList.list_id, intList_deserialized.list_id);
  BOOST_REQUIRE_EQUAL(anotherList.generator_id, intList_deserialized.generator_id);
  BOOST_REQUIRE_EQUAL(anotherList.list.size(), intList_deserialized.list.size());
  BOOST_REQUIRE_EQUAL(anotherList.list[0], intList_deserialized.list[0]);
}

BOOST_AUTO_TEST_CASE(ReversedListData_SerDes_MsgPack)
{
  BOOST_REQUIRE(dunedaq::serialization::is_serializable<ReversedList::Data>::value);
  BOOST_REQUIRE_EQUAL(dunedaq::datatype_to_string<ReversedList::Data>(), "ReversedListData");

  IntList intList(1, 2, {3,4,5,6});
  IntList reversed(1, 3, { 6, 5, 4, 3 });
  ReversedList::Data data;
  data.original = intList;
  data.reversed = reversed;

  auto bytes = dunedaq::serialization::serialize(data, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << bytes.size() << " bytes";
  ReversedList::Data data_deserialized = dunedaq::serialization::deserialize<ReversedList::Data>(bytes);

  BOOST_REQUIRE_EQUAL(intList.list_id, data_deserialized.original.list_id);
  BOOST_REQUIRE_EQUAL(intList.generator_id, data_deserialized.original.generator_id);
  BOOST_REQUIRE_EQUAL(intList.list.size(), data_deserialized.original.list.size());
  BOOST_REQUIRE_EQUAL(intList.list[0], data_deserialized.original.list[0]);

  BOOST_REQUIRE_EQUAL(reversed.list_id, data_deserialized.reversed.list_id);
  BOOST_REQUIRE_EQUAL(reversed.generator_id, data_deserialized.reversed.generator_id);
  BOOST_REQUIRE_EQUAL(reversed.list.size(), data_deserialized.reversed.list.size());
  BOOST_REQUIRE_EQUAL(reversed.list[0], data_deserialized.reversed.list[0]);
}

BOOST_AUTO_TEST_CASE(ReversedList_SerDes_MsgPack)
{
  BOOST_REQUIRE(dunedaq::serialization::is_serializable<ReversedList>::value);
  BOOST_REQUIRE_EQUAL(dunedaq::datatype_to_string<ReversedList>(), "ReversedList");

  IntList intList(1, 2, { 3, 4, 5, 6 });
  IntList reversed(1, 3, { 6, 5, 4, 3 });
  ReversedList::Data data;
  data.original = intList;
  data.reversed = reversed;
  ReversedList reversedList(1, 3, { data });

  auto bytes = dunedaq::serialization::serialize(reversedList, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << bytes.size() << " bytes";
  ReversedList reversedList_deserialized = dunedaq::serialization::deserialize<ReversedList>(bytes);

  BOOST_REQUIRE_EQUAL(reversedList.list_id, reversedList_deserialized.list_id);
  BOOST_REQUIRE_EQUAL(reversedList.reverser_id, reversedList_deserialized.reverser_id);
  BOOST_REQUIRE_EQUAL(reversedList.lists.size(), reversedList_deserialized.lists.size());

  auto data_deserialized = reversedList_deserialized.lists[0];

  BOOST_REQUIRE_EQUAL(intList.list_id, data_deserialized.original.list_id);
  BOOST_REQUIRE_EQUAL(intList.generator_id, data_deserialized.original.generator_id);
  BOOST_REQUIRE_EQUAL(intList.list.size(), data_deserialized.original.list.size());
  BOOST_REQUIRE_EQUAL(intList.list[0], data_deserialized.original.list[0]);

  BOOST_REQUIRE_EQUAL(reversed.list_id, data_deserialized.reversed.list_id);
  BOOST_REQUIRE_EQUAL(reversed.generator_id, data_deserialized.reversed.generator_id);
  BOOST_REQUIRE_EQUAL(reversed.list.size(), data_deserialized.reversed.list.size());
  BOOST_REQUIRE_EQUAL(reversed.list[0], data_deserialized.reversed.list[0]);
}

BOOST_AUTO_TEST_CASE(CreateList_SerDes_MsgPack)
{
  BOOST_REQUIRE(dunedaq::serialization::is_serializable<CreateList>::value);
  BOOST_REQUIRE_EQUAL(dunedaq::datatype_to_string<CreateList>(), "CreateList");

  CreateList create(2, 100);

  auto bytes = dunedaq::serialization::serialize(create, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << bytes.size() << " bytes";
  CreateList create_deserialized = dunedaq::serialization::deserialize<CreateList>(bytes);

  BOOST_REQUIRE_EQUAL(create.list_id, create_deserialized.list_id);
  BOOST_REQUIRE_EQUAL(create.list_size, create_deserialized.list_size);
}

BOOST_AUTO_TEST_CASE(RequestList_SerDes_MsgPack)
{
  BOOST_REQUIRE(dunedaq::serialization::is_serializable<RequestList>::value);
  BOOST_REQUIRE_EQUAL(dunedaq::datatype_to_string<RequestList>(), "RequestList");

  RequestList request(1, "test");

  auto bytes = dunedaq::serialization::serialize(request, dunedaq::serialization::kMsgPack);
  TLOG(TLVL_INFO) << "MsgPack message size: " << bytes.size() << " bytes";
  RequestList request_deserialized = dunedaq::serialization::deserialize<RequestList>(bytes);

  BOOST_REQUIRE_EQUAL(request.list_id, request_deserialized.list_id);
  BOOST_REQUIRE_EQUAL(request.destination, request_deserialized.destination);
}

BOOST_AUTO_TEST_SUITE_END()
