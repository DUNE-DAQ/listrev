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
#include "ListCreator.hpp"

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

  void init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg) override;
  void generate_opmon_data() override;

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  // Threading
  dunedaq::utilities::WorkerThread m_work_thread;
  void do_work(std::atomic<bool>&);

  // Callbacks
  void process_list(const ReversedList& list);

  // Methods
  void send_request(int id);

  // Data
  std::map<int,std::chrono::steady_clock::time_point> m_outstanding_ids;
  int m_next_id{ 0 };
  std::chrono::steady_clock::time_point m_request_start;
  mutable std::mutex m_outstanding_id_mutex;
  ListCreator m_list_creator;

  // Init
  std::string m_list_connection;
  std::string m_create_connection;

  // Configuration
  std::chrono::milliseconds m_send_timeout{ 100 };
  std::chrono::milliseconds m_request_timeout{ 1000 };
  size_t m_max_outstanding_requests{ 100 };
  size_t m_num_generators{ 0 };
  size_t m_num_reversers{ 0 };
  size_t m_request_rate_hz{ 100 };

  std::vector<uint32_t> m_generatorIds;
  std::vector<std::string> m_reveserIds;

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
                       MissingListError,
                       appfwk::GeneralDAQModuleIssue,
                       "Missing lists detected, for list set " << id << " expected " << n_gen << " lists, but received only " << n_lists,
                       ((std::string)name),
                       ((int)id)((int)n_gen)((int)n_lists))

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
