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
  bool list_reversed{ false };
  bool list_found{ false };
  std::vector<int> list;

  IntList() = default;
  explicit IntList(const int& id, std::vector<int> const& l, bool reversed = false, bool found = true)
    : list_id(id)
    , list_reversed(reversed)
    , list_found(found)
    , list(l.begin(), l.end())
  {
  }

  DUNE_DAQ_SERIALIZE(IntList, list_id, list_reversed, list_found, list);
};

struct CreateList
{
  int list_id;
  uint16_t list_size;
  uint16_t list_mode;

  enum class ListMode : uint16_t
  {
    Random = 0,
    Ascending = 1,
    Evens = 2,
    Odds = 3,
    Descending = 4,
  };

  CreateList() = default;
  CreateList(const int& id, const uint16_t& size, const uint16_t& mode = static_cast<uint16_t>(ListMode::Ascending))
    : list_id(id)
    , list_size(size)
    , list_mode(mode){}

  DUNE_DAQ_SERIALIZE(CreateList, list_id, list_size, list_mode);
};
struct RequestList
{
  int list_id;
  std::string destination;

  RequestList() = default;
  explicit RequestList(const int& id, const std::string& dest)
    : list_id(id), destination(dest) {}

  DUNE_DAQ_SERIALIZE(RequestList, list_id, destination);
};
} // namespace listrev

DUNE_DAQ_SERIALIZABLE(listrev::IntList, "IntList");
DUNE_DAQ_SERIALIZABLE(listrev::CreateList, "CreateList");
DUNE_DAQ_SERIALIZABLE(listrev::RequestList, "RequestList");
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTWRAPPER_HPP_
