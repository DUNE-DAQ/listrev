/**
 * @file ListCreator.hpp
 *
 * Helper methods for sending CreateList requests
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_LISTCREATOR_HPP_
#define LISTREV_PLUGINS_LISTCREATOR_HPP_

#include "ListWrapper.hpp"

#include <random>

namespace dunedaq {
namespace listrev {

class ListCreator
{
public:
  ListCreator() = default;
  ListCreator(std::string conn, std::chrono::milliseconds tmo, int min_list_size, int max_list_size);

  // Methods
  void send_create(int id);

private:
  // Data
  std::mt19937 m_random_generator;
  std::uniform_int_distribution<> m_size_dist;

  // Configuration
  std::string m_create_connection;
  std::chrono::milliseconds m_send_timeout;
};
} // namespace listrev
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTCREATOR_HPP_