/**
 * @file RandomDataListGenerator.cpp RandomDataListGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "listrev/dal/RandomDataListGenerator.hpp"
#include "listrev/randomdatalistgeneratorinfo/InfoNljs.hpp"

#include "CommonIssues.hpp"
#include "RandomDataListGenerator.hpp"

#include "appfwk/ModuleConfiguration.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include "coredal/Connection.hpp"

#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <set>
#include <string>
#include <thread>
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
{
  register_command("start", &RandomDataListGenerator::do_start, std::set<std::string>{ "CONFIGURED" });
  register_command("stop", &RandomDataListGenerator::do_stop, std::set<std::string>{ "TRIGGER_SOURCES_STOPPED" });
  register_command("scrap", &RandomDataListGenerator::do_unconfigure, std::set<std::string>{ "CONFIGURED" });
  register_command("hello", &RandomDataListGenerator::do_hello, std::set<std::string>{ "RUNNING", "READY" });
}

void
RandomDataListGenerator::init()
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto mdal = appfwk::ModuleConfiguration::get()
    ->module<dal::RandomDataListGenerator>(get_name());
  for (auto con : mdal->get_inputs()) {
    if (con->get_data_type() == "CreateList") {
      m_create_connection = con->UID();
    }
    if (con->get_data_type() == "RequestList") {
      m_request_connection = con->UID();
    }
  }

  // these are just tests to check if the connections are ok
  auto iom = iomanager::IOManager::get();
  iom->get_receiver<RequestList>(m_request_connection);
  iom->get_receiver<CreateList>(m_create_connection);

  m_send_timeout = std::chrono::milliseconds(mdal->get_send_timeout_ms());
  m_request_timeout = std::chrono::milliseconds(mdal->get_request_timeout_ms());
  m_generator_id = mdal->get_generator_id();
  m_list_mode = static_cast<ListMode>(m_generator_id % (static_cast<uint16_t>(ListMode::MAX) + 1));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
RandomDataListGenerator::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  randomdatalistgeneratorinfo::Info fcr;

  fcr.generated_numbers = m_generated_tot.load();
  fcr.new_generated_numbers = m_generated.exchange(0);
  fcr.sent_lists = m_sent_tot.load();
  fcr.new_sent_lists = m_sent.exchange(0);
  ci.add(fcr);
}


void
RandomDataListGenerator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";

  auto iom = iomanager::IOManager::get();
  iom->add_callback<RequestList>(
    m_request_connection, std::bind(&RandomDataListGenerator::process_request_list, this, std::placeholders::_1));
  iom->add_callback<CreateList>(m_create_connection,
                                std::bind(&RandomDataListGenerator::process_create_list, this, std::placeholders::_1));

  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
RandomDataListGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";

  auto iom = iomanager::IOManager::get();
  iom->remove_callback<RequestList>(m_request_connection);
  iom->remove_callback<CreateList>(m_create_connection);
  m_storage.flush();

  TLOG() << get_name() << " successfully stopped";

  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_stop() method, "
           << "generated " << m_generated_tot.load() << " lists, "
           << "and sent " << m_sent_tot.load() << " list messages";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
RandomDataListGenerator::do_unconfigure(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_unconfigure() method";

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
RandomDataListGenerator::process_create_list(const CreateList& create_request)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_create_list() method";
  std::vector<int> theList(create_request.list_size);

  TLOG_DEBUG(TLVL_LIST_GENERATION) << get_name() << ": Start of fill loop";
  for (size_t idx = 0; idx < create_request.list_size; ++idx) {
    switch (m_list_mode) {
      case ListMode::Random:
        theList[idx] = (rand() % 1000) + 1;
        break;
      case ListMode::Ascending:
        theList[idx] = create_request.list_id + idx;
        break;
      case ListMode::Evens:
        theList[idx] = (create_request.list_id % 2 == 0 ? 0 : 1) + create_request.list_id + idx * 2;
        break;
      case ListMode::Odds:
        theList[idx] = (create_request.list_id % 2 == 0 ? 1 : 0) + create_request.list_id + idx * 2;
        break;
      case ListMode::Descending:
        theList[idx] = create_request.list_id - idx;
        break;
    }
  }
  ++m_generated_tot;
  ++m_generated;
  std::ostringstream oss_prog;
  oss_prog << "Generated list #" << create_request.list_id << " with contents " << theList << " and size "
           << theList.size() << ". ";
  ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

  m_storage.add_list(IntList(create_request.list_id, m_generator_id, theList));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_create_list() method";
}

void
RandomDataListGenerator::process_request_list(const RequestList& request)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_request_list() method";
  auto start = std::chrono::steady_clock::now();
  IntList output;
  bool list_found = false;

  while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start) <
         m_request_timeout) {
    if (m_storage.has_list(request.list_id)) {
      output = m_storage.get_list(request.list_id);
      list_found = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  if (!list_found) {
    std::ostringstream oss_warn;
    oss_warn << "wait for list \"" << request.list_id << "\"";
    ers::warning(dunedaq::iomanager::TimeoutExpired(
      ERS_HERE,
      get_name(),
      oss_warn.str(),
      std::chrono::duration_cast<std::chrono::milliseconds>(m_send_timeout).count()));
    return;
  }

  try {
    dunedaq::get_iomanager()->get_sender<IntList>(request.destination)->send(std::move(output), m_send_timeout);

    ++m_sent;
    ++m_sent_tot;
  } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
    std::ostringstream oss_warn;
    oss_warn << "send to destination \"" << request.destination << "\"";
    ers::warning(dunedaq::iomanager::TimeoutExpired(
      ERS_HERE,
      get_name(),
      oss_warn.str(),
      std::chrono::duration_cast<std::chrono::milliseconds>(m_send_timeout).count()));
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_request_list() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::RandomDataListGenerator)

// Local Variables:
// c-basic-offset: 2
// End:
