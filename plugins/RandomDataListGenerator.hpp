/**
 * @file RandomDataListGenerator.hpp
 *
 * RandomDataListGenerator is a simple DAQModule implementation that
 * periodically generates a list of random integers.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_
#define LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_

#include "ListWrapper.hpp"

#include "listrev/randomdatalistgenerator/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "iomanager/Sender.hpp"
#include "utilities/WorkerThread.hpp"

#include <ers/Issue.hpp>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace listrev {

/**
 * @brief RandomDataListGenerator creates vectors of ints and writes
 * them to the configured output queues.
 */
class RandomDataListGenerator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief RandomDataListGenerator Constructor
   * @param name Instance name for this RandomDataListGenerator instance
   */
  explicit RandomDataListGenerator(const std::string& name);

  RandomDataListGenerator(const RandomDataListGenerator&) =
    delete; ///< RandomDataListGenerator is not copy-constructible
  RandomDataListGenerator& operator=(const RandomDataListGenerator&) =
    delete;                                                    ///< RandomDataListGenerator is not copy-assignable
  RandomDataListGenerator(RandomDataListGenerator&&) = delete; ///< RandomDataListGenerator is not move-constructible
  RandomDataListGenerator& operator=(RandomDataListGenerator&&) =
    delete; ///< RandomDataListGenerator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_unconfigure(const nlohmann::json& obj);
  void do_hello(const nlohmann::json& obj);

  // Threading
  dunedaq::utilities::WorkerThread thread_;
  void do_work(std::atomic<bool>&);

  // Configuration
  using sink_t = dunedaq::iomanager::SenderConcept<IntList>;
  std::vector<std::shared_ptr<sink_t>> outputQueues_;
  std::chrono::milliseconds queueTimeout_;
  randomdatalistgenerator::ConfParams cfg_;

  // Statistic counters
  std::atomic<uint64_t> m_generated{ 0 };     // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_generated_tot{ 0 }; // NOLINT(build/unsigned)
};
} // namespace listrev

// Disable coverage collection LCOV_EXCL_START
ERS_DECLARE_ISSUE_BASE(listrev,
                       NoOutputQueuesAvailableWarning,
                       appfwk::GeneralDAQModuleIssue,
                       "No output queues were available, so the generated list of integers will be dropped. Has "
                       "initialization been successfully completed?",
                       ((std::string)name),
                       ERS_EMPTY)
// Re-enable coverage collection LCOV_EXCL_STOP

} // namespace dunedaq

#endif // LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
