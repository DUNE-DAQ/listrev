/**
 * @file ReversedListValidator.hpp
 *
 * ReversedListValidator is a DAQModule implementation that reads lists
 * of integers from two queues and verifies that the order of the elements
 * in the lists from the first queue are opposite from the order of the
 * elements in the lists from the second queue.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_SRC_REVERSEDLISTVALIDATOR_HPP_
#define LISTREV_SRC_REVERSEDLISTVALIDATOR_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace listrev {

/**
 * @brief ReversedListValidator reads lists of integers from two queues
 * and verifies that the lists have the same data, but stored in reverse order.
 */
class ReversedListValidator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief ReversedListValidator Constructor
   * @param name Instance name for this ReversedListValidator instance
   */
  explicit ReversedListValidator(const std::string& name);

  ReversedListValidator(const ReversedListValidator&) =
    delete; ///< ReversedListValidator is not copy-constructible
  ReversedListValidator& operator=(const ReversedListValidator&) =
    delete; ///< ReversedListValidator is not copy-assignable
  ReversedListValidator(ReversedListValidator&&) =
    delete; ///< ReversedListValidator is not move-constructible
  ReversedListValidator& operator=(ReversedListValidator&&) =
    delete; ///< ReversedListValidator is not move-assignable

  void init(const nlohmann::json& obj) override;

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  // Threading
  dunedaq::appfwk::ThreadHelper thread_;
  void do_work(std::atomic<bool>&);

  // Configuration
  using source_t = dunedaq::appfwk::DAQSource<std::vector<int>>;
  std::unique_ptr<source_t> reversedDataQueue_;
  std::unique_ptr<source_t> originalDataQueue_;
  std::chrono::milliseconds queueTimeout_;
};
} // namespace listrev

ERS_DECLARE_ISSUE_BASE(listrev,
                       DataMismatchError,
                       appfwk::GeneralDAQModuleIssue,
                       "Data mismatch when validating lists: doubly-reversed list contents = " << revContents << ", original list contents = " << origContents,
                       ((std::string)name),
                       ((std::string)revContents)((std::string)origContents))

} // namespace dunedaq

#endif // LISTREV_SRC_REVERSEDLISTVALIDATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
