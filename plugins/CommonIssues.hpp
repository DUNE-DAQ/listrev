/**
 * @file CommonIssues.hpp
 *
 * This file contains the definitions of ERS Issues that are common
 * to two or more of the DAQModules in this package.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_COMMONISSUES_HPP_
#define LISTREV_PLUGINS_COMMONISSUES_HPP_

#include "logging/Logging.hpp"    // include this BEFORE ERS_DECLARE_ISSUE* to allow TLOG()<<issue;
#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"

#include <string>

namespace dunedaq {

// Disable coverage collection LCOV_EXCL_START
ERS_DECLARE_ISSUE_BASE(listrev,
                       ProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))

ERS_DECLARE_ISSUE_BASE(listrev,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))
// Re-enable coverage collection LCOV_EXCL_STOP

namespace listrev {
/**
 * @brief Format a std::vector<int> to a stream
 * @param t ostream Instance
 * @param ints Vector to format
 * @return ostream Instance
 */
std::ostream&
operator<<(std::ostream& t, std::vector<int> ints)
{
  t << "{";
  bool first = true;
  for (auto& i : ints) {
    if (!first)
      t << ", ";
    first = false;
    t << i;
  }
  return t << "}";
}
/**
 * Helper for ostringstream belong to allow all stringstream stuff to be within Issue.
 */
template<typename T> struct lval { T t; T &getlval() { return t; } };

} // namespace listrev
} // namespace dunedaq

#endif // LISTREV_PLUGINS_COMMONISSUES_HPP_
