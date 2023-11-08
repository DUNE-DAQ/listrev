/**
 * @file ListStorage.hpp
 *
 * ListStorage defines the data storage class used by the listrev modules
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_LISTSTORAGE_HPP_
#define LISTREV_PLUGINS_LISTSTORAGE_HPP_

#include "ListWrapper.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace dunedaq {
namespace listrev {

	class ListStorage
	{
        public:
          ListStorage() {}

          bool has_list(const int& id) const;
          IntList get_list(const int& id) const;
          void add_list(IntList list, bool ignoreDuplicates = false);

          size_t size() const;
          void set_capacity(const size_t& capacity) { m_capacity = capacity; }
          size_t capacity() const { return m_capacity; }
          void flush();

        private:
          std::map<int, IntList> m_lists;
          mutable std::mutex m_lists_mutex;
          size_t m_capacity{ 1000 };
	};
} // namespace listrev
} // namespace duneadq

#endif // LISTREV_PLUGINS_LISTSTORAGE_HPP_