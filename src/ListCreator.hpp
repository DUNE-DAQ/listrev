/**
 * @file ListCreator.hpp
 *
 * Helper methods for sending CreateList requests
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_SRC_LISTCREATOR_HPP_
#define LISTREV_SRC_LISTCREATOR_HPP_

#include "ListWrapper.hpp"

#include <random>
#include <string>

namespace dunedaq::listrev {

class ListCreator
{
public:
  ListCreator() = default; // NOLINT
  ListCreator(std::string conn, std::chrono::milliseconds tmo, size_t min_list_size, size_t max_list_size);

  // Methods
  size_t send_create(int id);

private:
  // Data
  std::mt19937 m_random_generator;
  std::uniform_int_distribution<> m_size_dist;

  // Configuration
  std::string m_create_connection{ "" };
  std::chrono::milliseconds m_send_timeout{ 0 };
};
} // namespace dunedaq::listrev

#endif // LISTREV_SRC_LISTCREATOR_HPP_
