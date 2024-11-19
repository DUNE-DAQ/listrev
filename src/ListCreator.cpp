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

dunedaq::listrev::ListCreator::ListCreator(std::string conn,
                                                  std::chrono::milliseconds tmo,
                                                  int min_list_size,
                                                  int max_list_size)
  : m_create_connection(conn)
  , m_send_timeout(tmo)
{
  std::random_device seed;
  m_random_generator = std::mt19937(seed());

  if (min_list_size < 0) {
    min_list_size = 1;
  }
  if (max_list_size < min_list_size) {
    max_list_size = min_list_size;
  }
  m_size_dist = std::uniform_int_distribution<>{ min_list_size, max_list_size };

  get_iomanager()->get_sender<CreateList>(m_create_connection);
}

void
dunedaq::listrev::ListCreator::send_create(int id)
{
  CreateList req;
  req.list_id = id;
  req.list_size = m_size_dist(m_random_generator);

  get_iomanager()->get_sender<CreateList>(m_create_connection)->send(std::move(req), m_send_timeout);
}
