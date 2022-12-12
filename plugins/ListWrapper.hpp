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
  std::vector<int> list;

  IntList() = default;
  explicit IntList(std::vector<int> const& l)
    : list(l.begin(), l.end())
  {
  }

  DUNE_DAQ_SERIALIZE(IntList, list);
};
} // namespace listrev
DUNE_DAQ_SERIALIZABLE(listrev::IntList, "IntList");
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTWRAPPER_HPP_
