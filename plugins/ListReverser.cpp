/**
 * @file ListReverser.cpp ListReverser class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ListReverser.hpp"
#include "CommonIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "ListReverser" // NOLINT
#define TLVL_ENTER_EXIT_METHODS 10
#define TLVL_LIST_REVERSAL 15

namespace dunedaq {
namespace listrev {

ListReverser::ListReverser(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&ListReverser::do_work, this, std::placeholders::_1))
  , inputQueue_(nullptr)
  , outputQueue_(nullptr)
  , queueTimeout_(100)
{
  register_command("start", &ListReverser::do_start);
  register_command("stop", &ListReverser::do_stop);
}

void
ListReverser::init(const nlohmann::json& iniobj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto qi = appfwk::connection_index(iniobj, { "input", "output" });
  try {
    inputQueue_ = get_iom_receiver<IntList>(qi["input"]);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
  }
  try {
    outputQueue_ = get_iom_sender<IntList>(qi["output"]);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "output", excpt);
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
ListReverser::do_start(const nlohmann::json& /*startobj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ListReverser::do_stop(const nlohmann::json& /*stopobj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
ListReverser::do_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  int receivedCount = 0;
  int sentCount = 0;
  std::vector<int> workingVector;

  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Going to receive data from input queue";
    try {
      workingVector = inputQueue_->receive(queueTimeout_).list;
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      // it is perfectly reasonable that there might be no data in the queue
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }

    ++receivedCount;
    TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Received list #" << receivedCount << ". It has size "
                                   << workingVector.size() << ". Reversing its contents";
    std::reverse(workingVector.begin(), workingVector.end());

    TLOG_DEBUG() << ProgressUpdate(ERS_HERE, get_name(), static_cast<std::ostringstream&>\
				   (lval<std::ostringstream>().getlval()
				    << "Reversed list #" << receivedCount << ", new contents " << workingVector
				    << " and size " << workingVector.size() << ".").str());

    bool successfullyWasSent = false;
    while (!successfullyWasSent && running_flag.load()) {
      TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Pushing the reversed list onto the output queue";
      try {
        IntList wrapped(workingVector);
        outputQueue_->send(std::move(wrapped), queueTimeout_);
        successfullyWasSent = true;
        ++sentCount;
      } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
        ers::warning(dunedaq::iomanager::TimeoutExpired(ERS_HERE,get_name(),static_cast<std::ostringstream&>\
							(lval<std::ostringstream>().getlval()
							 << "push to output queue \"" << outputQueue_->get_name()
							 << "\"").str(),\
				std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
      }
    }
    TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": End of do_work loop";
  }

  ers::info(ProgressUpdate(ERS_HERE, get_name(), static_cast<std::ostringstream&>\
			   (lval<std::ostringstream>().getlval()
			    << ": Exiting do_work() method, received " << receivedCount
			    << " lists and successfully sent " << sentCount << ".").str()));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ListReverser)

// Local Variables:
// c-basic-offset: 2
// End:
