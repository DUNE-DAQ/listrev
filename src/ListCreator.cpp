/**
 * @file ListCreator.cpp
 *
 * Helper methods for sending CreateList requests (implementation)
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ListCreator.hpp"

#include "iomanager/IOManager.hpp"
#include "iomanager/Sender.hpp"

#include <string>
#include <utility>

dunedaq::listrev::ListCreator::ListCreator(std::string conn,
                                           std::chrono::milliseconds tmo,
                                           size_t min_list_size,
                                           size_t max_list_size)
  : m_random_generator(std::random_device()())
  , m_create_connection(conn)
  , m_send_timeout(tmo)
{
  int min = static_cast<int>(min_list_size);
  int max = static_cast<int>(max_list_size);
  if (min < 0) {
    min = 1;
  }
  if (max < min) {
    max = min;
  }
  m_size_dist = std::uniform_int_distribution<>{ min, max };

  get_iomanager()->get_sender<CreateList>(m_create_connection);
}

size_t
dunedaq::listrev::ListCreator::send_create(int id)
{
  CreateList req;
  req.list_id = id;
  req.list_size = m_size_dist(m_random_generator);
  size_t output = req.list_size; // Save list_size since std::move may invalidate object
  get_iomanager()->get_sender<CreateList>(m_create_connection)->send(std::move(req), m_send_timeout); // NOLINT
  return output;
}
