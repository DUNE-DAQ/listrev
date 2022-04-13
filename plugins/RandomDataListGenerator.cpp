/**
 * @file RandomDataListGenerator.cpp RandomDataListGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "listrev/randomdatalistgenerator/Nljs.hpp"
#include "listrev/randomdatalistgeneratorinfo/InfoNljs.hpp"

#include "CommonIssues.hpp"
#include "RandomDataListGenerator.hpp"

#include "appfwk/app/Nljs.hpp"

#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

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
  register_command("conf",  &RandomDataListGenerator::do_configure);
  register_command("start", &RandomDataListGenerator::do_start);
  register_command("stop",  &RandomDataListGenerator::do_stop);
  register_command("scrap", &RandomDataListGenerator::do_unconfigure);
  register_command("hello", &RandomDataListGenerator::do_hello);
}

void
RandomDataListGenerator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto ini = init_data.get<appfwk::app::ModInit>();
  iomanager::IOManager iom;
  for (const auto& cr : ini.conn_refs) {
    if (cr.dir != iomanager::connection::Direction::kOutput) {
      continue;                 // skip all but "output" direction
    }
    try
    {
      outputQueues_.emplace_back(iom.get_sender<std::vector<int>>(cr));
    }
    catch (const ers::Issue& excpt)
    {
      throw InvalidQueueFatalError(ERS_HERE, get_name(), cr.name, excpt);
    }
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}
void
RandomDataListGenerator::get_info(opmonlib::InfoCollector& ci, int /*level*/) {
  randomdatalistgeneratorinfo::Info fcr;

  fcr.generated_numbers = m_generated_tot.load();
  fcr.new_generated_numbers = m_generated.exchange(0);
  ci.add(fcr);
}

void
RandomDataListGenerator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  cfg_ = obj.get<randomdatalistgenerator::ConfParams>();
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
RandomDataListGenerator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  thread_.start_working_thread();
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
RandomDataListGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  thread_.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
RandomDataListGenerator::do_unconfigure(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_unconfigure() method";
  cfg_ = randomdatalistgenerator::ConfParams{};          // reset to defaults
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_unconfigure() method";
}

void
RandomDataListGenerator::do_hello(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering hello() method";
  TLOG() << "Hello my friend!";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_hello() method";
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
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_work() method";
  //size_t generatedCount = 0;
  size_t sentCount = 0;
  m_generated_tot = 0 ;
  m_generated = 0; 
  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Creating list of length " << cfg_.nIntsPerList;
    std::vector<int> theList(cfg_.nIntsPerList);

    TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Start of fill loop";
    for (size_t idx = 0; idx < cfg_.nIntsPerList; ++idx)
    {
      theList[idx] = (rand() % 1000) + 1;
    }
    ++m_generated_tot;
    ++m_generated;
    std::ostringstream oss_prog;
    oss_prog << "Generated list #" << m_generated_tot.load() << " with contents " << theList
             << " and size " << theList.size() << ". ";
    ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

    TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Pushing list onto " << outputQueues_.size() << " outputQueues";
    for (auto& outQueue : outputQueues_)
    {
      std::string thisQueueName = outQueue->get_name();
      bool successfullyWasSent = false;
      while (!successfullyWasSent && running_flag.load())
      {
        TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Pushing the generated list onto queue " << thisQueueName;
        try
        {
          outQueue->send(theList, queueTimeout_);
          successfullyWasSent = true;
          ++sentCount;
        } catch (const dunedaq::iomanager::QueueTimeoutExpired& excpt)
        {
          std::ostringstream oss_warn;
          oss_warn << "push to output queue \"" << thisQueueName << "\"";
          ers::warning(dunedaq::iomanager::QueueTimeoutExpired(
            ERS_HERE,
            get_name(),
            oss_warn.str(),
                       std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
        }
      }
    }
    if (outputQueues_.size() == 0)
    {
      ers::warning(NoOutputQueuesAvailableWarning(ERS_HERE, get_name()));
    }

    TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Start of sleep between sends";
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.waitBetweenSendsMsec));
    TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": End of do_work loop";
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting the do_work() method, generated " << m_generated_tot.load()
           << " lists and successfully sent " << sentCount << " copies. ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace listrev 
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::RandomDataListGenerator)

// Local Variables:
// c-basic-offset: 2
// End:
