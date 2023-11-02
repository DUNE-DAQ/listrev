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

#ifndef LISTREV_PLUGINS_REVERSEDLISTVALIDATOR_HPP_
#define LISTREV_PLUGINS_REVERSEDLISTVALIDATOR_HPP_

#include "ListWrapper.hpp"
#include "ListStorage.hpp"

#include "appfwk/DAQModule.hpp"
#include "iomanager/Receiver.hpp"
#include "iomanager/Sender.hpp"
#include "utilities/WorkerThread.hpp"

#include <ers/Issue.hpp>

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

  ReversedListValidator(const ReversedListValidator&) = delete; ///< ReversedListValidator is not copy-constructible
  ReversedListValidator& operator=(const ReversedListValidator&) =
    delete;                                                ///< ReversedListValidator is not copy-assignable
  ReversedListValidator(ReversedListValidator&&) = delete; ///< ReversedListValidator is not move-constructible
  ReversedListValidator& operator=(ReversedListValidator&&) = delete; ///< ReversedListValidator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  // Threading
  dunedaq::utilities::WorkerThread m_work_thread;
  dunedaq::utilities::WorkerThread m_request_thread;
  void do_work(std::atomic<bool>&);
  void send_requests(std::atomic<bool>&);

  // Callbacks
  void process_list(const IntList& list);

  // Data
  ListStorage m_lists;
  ListStorage m_reversed;
  std::set<int> m_outstanding_ids;
  int m_next_id{ 0 };
  mutable std::mutex m_outstanding_id_mutex;

  // Init
  std::string m_list_connection;
  std::shared_ptr<iomanager::SenderConcept<RequestList>> m_requests;

  // Configuration
  iomanager::Sender::timeout_t m_send_timeout{ 100 };
  size_t m_max_outstanding_requests{ 100 };
  std::chrono::milliseconds m_request_send_interval{ 1000 };

  // Monitoring
  std::atomic<uint64_t> m_requests_total{ 0 };
  std::atomic<uint64_t> m_new_requests{ 0 };
  std::atomic<uint64_t> m_total_lists{ 0 };
  std::atomic<uint64_t> m_new_lists{ 0 };
  std::atomic<uint64_t> m_total_reversed{ 0 };
  std::atomic<uint64_t> m_new_reversed{ 0 };
  std::atomic<uint64_t> m_total_valid_pairs{ 0 };
  std::atomic<uint64_t> m_valid_list_pairs{ 0 };
  std::atomic<uint64_t> m_total_invalid_pairs{ 0 };
  std::atomic<uint64_t> m_invalid_list_pairs{ 0 };
};
} // namespace listrev

// Disable coverage collection LCOV_EXCL_START
ERS_DECLARE_ISSUE_BASE(listrev,
                       DataMismatchError,
                       appfwk::GeneralDAQModuleIssue,
                       "Data mismatch when validating list" << id << ": doubly-reversed list contents = "
                         << revContents << ", original list contents = " << origContents,
                       ((std::string)name),
                       ((int)id)((std::string)revContents)((std::string)origContents))
// Re-enable coverage collection LCOV_EXCL_STOP

} // namespace dunedaq

#endif // LISTREV_PLUGINS_REVERSEDLISTVALIDATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
