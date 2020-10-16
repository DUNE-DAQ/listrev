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

#ifndef LISTREV_SRC_RANDOMDATALISTGENERATOR_HPP_
#define LISTREV_SRC_RANDOMDATALISTGENERATOR_HPP_

#include "rdlg/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.h>

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
    delete; ///< RandomDataListGenerator is not copy-assignable
  RandomDataListGenerator(RandomDataListGenerator&&) =
    delete; ///< RandomDataListGenerator is not move-constructible
  RandomDataListGenerator& operator=(RandomDataListGenerator&&) =
    delete; ///< RandomDataListGenerator is not move-assignable

  void init(const nlohmann::json& obj) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_unconfigure(const nlohmann::json& obj);

  // Threading
  dunedaq::appfwk::ThreadHelper thread_;
  void do_work(std::atomic<bool>&);

  // Configuration
  using sink_t = dunedaq::appfwk::DAQSink<std::vector<int>>;
  std::vector<std::unique_ptr<sink_t>> outputQueues_;
  std::chrono::milliseconds queueTimeout_;
  rdlg::Conf cfg_;
};
} // namespace listrev

ERS_DECLARE_ISSUE_BASE(listrev,
                       NoOutputQueuesAvailableWarning,
                       appfwk::GeneralDAQModuleIssue,
                       "No output queues were available, so the generated list of integers will be dropped. Has initialization been successfully completed?",
                       ((std::string)name),
                       ERS_EMPTY)

} // namespace dunedaq

#endif // LISTREV_SRC_RANDOMDATALISTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
