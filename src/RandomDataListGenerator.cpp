/**
 * @file RandomDataListGenerator.cpp RandomDataListGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CommonIssues.hpp"
#include "RandomDataListGenerator.hpp"

#include <ers/ers.h>
#include <TRACE/trace.h>

#include <chrono>
#include <cstdlib>
#include <thread>

/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "RandomDataListGenerator" // NOLINT
#define TLVL_ENTER_EXIT_METHODS 10
#define TLVL_LIST_GENERATION 15

namespace dunedaq {
namespace listrev {

RandomDataListGenerator::RandomDataListGenerator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , thread_(std::bind(&RandomDataListGenerator::do_work, this, std::placeholders::_1))
  , outputQueues_()
  , queueTimeout_(100)
{
  register_command("configure", &RandomDataListGenerator::do_configure);
  register_command("start",  &RandomDataListGenerator::do_start);
  register_command("stop",  &RandomDataListGenerator::do_stop);
  register_command("unconfigure",  &RandomDataListGenerator::do_unconfigure);
}

void RandomDataListGenerator::init()
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  for (auto& output : get_config()["outputs"]) {
    try
    {
      outputQueues_.emplace_back(new dunedaq::appfwk::DAQSink<std::vector<int>>(output.get<std::string>()));
    }
    catch (const ers::Issue& excpt)
    {
      throw InvalidQueueFatalError(ERS_HERE, get_name(), output.get<std::string>(), excpt);
    }
  }
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
RandomDataListGenerator::do_configure(const std::vector<std::string>& /*args*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  nIntsPerList_ = get_config().value<size_t>("nIntsPerList", static_cast<size_t>(REASONABLE_DEFAULT_INTSPERLIST));
  waitBetweenSendsMsec_ = get_config().value<size_t>("waitBetweenSendsMsec", static_cast<size_t>(REASONABLE_DEFAULT_MSECBETWEENSENDS));
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
RandomDataListGenerator::do_start(const std::vector<std::string>& /*args*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  ERS_LOG(get_name() << " successfully started");
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
RandomDataListGenerator::do_stop(const std::vector<std::string>& /*args*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  ERS_LOG(get_name() << " successfully stopped");
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
RandomDataListGenerator::do_unconfigure(const std::vector<std::string>& /*args*/)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_unconfigure() method";
  nIntsPerList_ = REASONABLE_DEFAULT_INTSPERLIST;
  waitBetweenSendsMsec_ = REASONABLE_DEFAULT_MSECBETWEENSENDS;
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_unconfigure() method";
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
RandomDataListGenerator::do_work(std::atomic<bool>& running_flag)
{
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  size_t generatedCount = 0;
  size_t sentCount = 0;

  while (running_flag.load()) {
    TLOG(TLVL_LIST_GENERATION) << get_name() << ": Creating list of length " << nIntsPerList_;
    std::vector<int> theList(nIntsPerList_);

    TLOG(TLVL_LIST_GENERATION) << get_name() << ": Start of fill loop";
    for (size_t idx = 0; idx < nIntsPerList_; ++idx)
    {
      theList[idx] = (rand() % 1000) + 1;
    }
    generatedCount++;
    std::ostringstream oss_prog;
    oss_prog << "Generated list #" << generatedCount << " with contents " << theList
             << " and size " << theList.size() << ". ";
    ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

    TLOG(TLVL_LIST_GENERATION) << get_name() << ": Pushing list onto " << outputQueues_.size() << " outputQueues";
    for (auto& outQueue : outputQueues_)
    {
      std::string thisQueueName = outQueue->get_name();
      bool successfullyWasSent = false;
      while (!successfullyWasSent && running_flag.load())
      {
        TLOG(TLVL_LIST_GENERATION) << get_name() << ": Pushing the generated list onto queue " << thisQueueName;
        try
        {
          outQueue->push(theList, queueTimeout_);
          successfullyWasSent = true;
          ++sentCount;
        }
        catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
        {
          std::ostringstream oss_warn;
          oss_warn << "push to output queue \"" << thisQueueName << "\"";
          ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
                       std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
        }
      }
    }
    if (outputQueues_.size() == 0)
    {
      ers::warning(NoOutputQueuesAvailableWarning(ERS_HERE, get_name()));
    }

    TLOG(TLVL_LIST_GENERATION) << get_name() << ": Start of sleep between sends";
    std::this_thread::sleep_for(std::chrono::milliseconds(waitBetweenSendsMsec_));
    TLOG(TLVL_LIST_GENERATION) << get_name() << ": End of do_work loop";
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting the do_work() method, generated " << generatedCount
           << " lists and successfully sent " << sentCount << " copies. ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace listrev 
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::RandomDataListGenerator)
