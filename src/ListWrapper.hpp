/**
 * @file ListWrapper.hpp
 *
 * ListWrapper wraps a std::vector<int> so that it can be transmitted over the network using the Unified Communications
 * API (iomanager)
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_LISTWRAPPER_HPP_
#define LISTREV_PLUGINS_LISTWRAPPER_HPP_

#include "serialization/Serialization.hpp"

#include <vector>

namespace dunedaq {
namespace listrev {
struct IntList
{
  int list_id;
  int generator_id;
  std::vector<int> list;

  IntList() = default;
  explicit IntList(const int& id, const int& gid, std::vector<int> const& l)
    : list_id(id)
    , generator_id(gid)
    , list(l.begin(), l.end())
  {
  }

  DUNE_DAQ_SERIALIZE(IntList, list_id, generator_id, list);
};

struct ReversedList
{
  struct Data
  {
    IntList original;
    IntList reversed;

    DUNE_DAQ_SERIALIZE(Data, original, reversed);
  };
  int list_id;
  int reverser_id;
  std::vector<Data> lists;

  ReversedList() = default;
  ReversedList(const int& id, const int& rid, std::vector<Data> const& ls)
    : list_id(id)
    , reverser_id(rid)
    , lists(ls.begin(), ls.end())
  {
  }

  DUNE_DAQ_SERIALIZE(ReversedList, list_id, reverser_id, lists);
};

struct CreateList
{
  int list_id;
  uint16_t list_size;

  CreateList() = default;
  CreateList(const int& id, const uint16_t& size)
    : list_id(id)
    , list_size(size)
  {
  }

  DUNE_DAQ_SERIALIZE(CreateList, list_id, list_size);
};
struct RequestList
{
  int list_id;
  std::string destination;

  RequestList() = default;
  explicit RequestList(const int& id, const std::string& dest)
    : list_id(id)
    , destination(dest)
  {
  }

  DUNE_DAQ_SERIALIZE(RequestList, list_id, destination);
};
} // namespace listrev

DUNE_DAQ_SERIALIZABLE(listrev::IntList, "IntList");
DUNE_DAQ_SERIALIZABLE(listrev::ReversedList::Data, "ReversedListData");
DUNE_DAQ_SERIALIZABLE(listrev::ReversedList, "ReversedList");
DUNE_DAQ_SERIALIZABLE(listrev::CreateList, "CreateList");
DUNE_DAQ_SERIALIZABLE(listrev::RequestList, "RequestList");
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTWRAPPER_HPP_
