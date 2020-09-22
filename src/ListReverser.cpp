/**
 * @file ListReverser.cpp ListReverser class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CommonIssues.hpp"
#include "ListReverser.hpp"

#include "appfwk/DAQModuleHelper.hpp"

#include <ers/ers.h>
#include "TRACE/trace.h"

#include <chrono>
#include <thread>

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
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto qi = appfwk::qindex(iniobj, {"input","output"});
  try
  {
    inputQueue_.reset(new source_t(qi["input"].inst));
  }
  catch (const ers::Issue& excpt)
  {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
  }
  try
  {
    outputQueue_.reset(new sink_t(qi["output"].inst));
  }
  catch (const ers::Issue& excpt)
  {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "output", excpt);
  }
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
ListReverser::do_start(const nlohmann::json& /*startobj*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  ERS_LOG(get_name() << " successfully started");
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ListReverser::do_stop(const nlohmann::json& /*stopobj*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  ERS_LOG(get_name() << " successfully stopped");
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
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
ListReverser::do_work(std::atomic<bool>& running_flag)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  int receivedCount = 0;
  int sentCount = 0;
  std::vector<int> workingVector;

  while (running_flag.load()) {
    TLOG(TLVL_LIST_REVERSAL) << get_name() << ": Going to receive data from input queue";
    try
    {
      inputQueue_->pop(workingVector, queueTimeout_);
    }
    catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
    {
      // it is perfectly reasonable that there might be no data in the queue 
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }

    ++receivedCount;
    TLOG(TLVL_LIST_REVERSAL) << get_name() << ": Received list #" << receivedCount
                             << ". It has size " << workingVector.size() << ". Reversing its contents";
    std::reverse(workingVector.begin(), workingVector.end());

    std::ostringstream oss_prog;
    oss_prog << "Reversed list #" << receivedCount << ", new contents " << workingVector
             << " and size " << workingVector.size() << ". ";
    ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

    bool successfullyWasSent = false;
    while (!successfullyWasSent && running_flag.load())
    {
      TLOG(TLVL_LIST_REVERSAL) << get_name() << ": Pushing the reversed list onto the output queue";
      try
      {
        outputQueue_->push(workingVector, queueTimeout_);
        successfullyWasSent = true;
        ++sentCount;
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
      {
        std::ostringstream oss_warn;
        oss_warn << "push to output queue \"" << outputQueue_->get_name() << "\"";
        ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
                     std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
      }
    }
    TLOG(TLVL_LIST_REVERSAL) << get_name() << ": End of do_work loop";
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_work() method, received " << receivedCount
           << " lists and successfully sent " << sentCount << ". ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ListReverser)

// Local Variables:
// c-basic-offset: 2
// End:
