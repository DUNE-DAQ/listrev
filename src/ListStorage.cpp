/**
 * @file ListStorage.cpp ListStorage implementations
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ListStorage.hpp"
#include "CommonIssues.hpp"

bool
dunedaq::listrev::ListStorage::has_list(const int& id) const
{
  std::lock_guard<std::mutex> lk(m_lists_mutex);
  return m_lists.count(id);
}

dunedaq::listrev::IntList
dunedaq::listrev::ListStorage::get_list(const int& id) const
{
  std::lock_guard<std::mutex> lk(m_lists_mutex);
  if (!m_lists.count(id)) {
    throw ListNotFound(ERS_HERE, id);
  }

  return m_lists.at(id);
}

void
dunedaq::listrev::ListStorage::add_list(IntList list, bool ignoreDuplicates)
{
  std::lock_guard<std::mutex> lk(m_lists_mutex);
  if (m_lists.count(list.list_id) && !ignoreDuplicates) {
    throw ListExists(ERS_HERE, list.list_id);
  }
  m_lists[list.list_id] = list;

  while (m_lists.size() > m_capacity) {
    m_lists.erase(m_lists.begin());
  }
}

size_t
dunedaq::listrev::ListStorage::size() const
{
  std::lock_guard<std::mutex> lk(m_lists_mutex);
  return m_lists.size();
}

void
dunedaq::listrev::ListStorage::flush()
{
  std::lock_guard<std::mutex> lk(m_lists_mutex);
  m_lists.clear();
}