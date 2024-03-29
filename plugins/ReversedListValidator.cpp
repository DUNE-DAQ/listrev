/**
 * @file ReversedListValidator.cpp ReversedListValidator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ReversedListValidator.hpp"
#include "CommonIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "ReversedListValidator" // NOLINT
#define TLVL_ENTER_EXIT_METHODS 10
#define TLVL_LIST_VALIDATION 15

namespace dunedaq {
namespace listrev {

ReversedListValidator::ReversedListValidator(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&ReversedListValidator::do_work, this, std::placeholders::_1))
  , reversedDataQueue_(nullptr)
  , originalDataQueue_(nullptr)
  , queueTimeout_(100)
{
  register_command("start", &ReversedListValidator::do_start);
  register_command("stop", &ReversedListValidator::do_stop);
}

void
ReversedListValidator::init(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto qi = appfwk::connection_index(obj, { "reversed_data_input", "original_data_input" });
  try {
    reversedDataQueue_ = get_iom_receiver<IntList>(qi["reversed_data_input"]);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "reversed data input", excpt);
  }

  try {
    originalDataQueue_ = get_iom_receiver<IntList>(qi["original_data_input"]);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "original data input", excpt);
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
ReversedListValidator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ReversedListValidator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

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

void
ReversedListValidator::do_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  int reversedCount = 0;
  int comparisonCount = 0;
  int failureCount = 0;
  std::vector<int> reversedData;
  std::vector<int> originalData;

  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Going to receive data from the reversed list queue";
    try {
      reversedData = reversedDataQueue_->receive(queueTimeout_).list;
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      // it is perfectly reasonable that there might be no reversed data in the queue
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }
    ++reversedCount;

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Received reversed list #" << reversedCount << ". It has size "
                                     << reversedData.size()
                                     << ". Now going to receive data from the original data queue.";
    bool originalWasSuccessfullyReceived = false;
    while (!originalWasSuccessfullyReceived && running_flag.load()) {
      TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Popping the next element off the original data queue";
      try {
        originalData = originalDataQueue_->receive(queueTimeout_).list;
        originalWasSuccessfullyReceived = true;
        ++comparisonCount;
      } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
        std::ostringstream oss_warn;
        oss_warn << "pop from original data queue";
        ers::warning(dunedaq::iomanager::TimeoutExpired(
          ERS_HERE,
          get_name(),
          oss_warn.str(),
          std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
      }
    }

    if (originalWasSuccessfullyReceived) {
      std::ostringstream oss_prog;
      oss_prog << "Validating list #" << reversedCount << ", original contents " << originalData
               << " and reversed contents " << reversedData << ". ";
      ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

      TLOG_DEBUG(TLVL_LIST_VALIDATION)
        << get_name() << ": Re-reversing the reversed list so that it can be compared to the original list";
      std::reverse(reversedData.begin(), reversedData.end());

      TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Comparing the doubly-reversed list with the original list";
      if (reversedData != originalData) {
        std::ostringstream oss_rev;
        oss_rev << reversedData;
        std::ostringstream oss_orig;
        oss_orig << originalData;
        ers::error(DataMismatchError(ERS_HERE, get_name(), oss_rev.str(), oss_orig.str()));
        ++failureCount;
      }
    }
    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": End of do_work loop";
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_work() method, received " << reversedCount << " reversed lists, "
           << "compared " << comparisonCount << " of them to their original data, and found " << failureCount
           << " mismatches. ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ReversedListValidator)

// Local Variables:
// c-basic-offset: 2
// End:
