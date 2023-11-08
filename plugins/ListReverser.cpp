/**
 * @file ListReverser.cpp ListReverser class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "listrev/listreverser/Nljs.hpp"
#include "listrev/listreverserinfo/InfoNljs.hpp"

#include "CommonIssues.hpp"
#include "ListReverser.hpp"

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
#define TLVL_REQUEST_SENDING 16
#define TLVL_CONFIGURE 17

namespace dunedaq {
namespace listrev {

ListReverser::ListReverser(const std::string& name)
  : DAQModule(name)
{
  register_command("conf", &ListReverser::do_configure, std::set<std::string>{ "INITIAL" });
  register_command("start", &ListReverser::do_start, std::set<std::string>{ "CONFIGURED" });
  register_command("stop", &ListReverser::do_stop, std::set<std::string>{ "TRIGGER_SOURCES_STOPPED" });
}

void
ListReverser::init(const nlohmann::json& iniobj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto qi = appfwk::connection_index(iniobj, { "request_input", "list_input" });
  m_requests = qi["request_input"];
  m_list_connection = qi["list_input"];

  try {
    get_iom_receiver<IntList>(m_list_connection);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
  }
  try {
    get_iom_receiver<RequestList>(m_requests);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "output", excpt);
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
ListReverser::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  listreverserinfo::Info fcr;

  fcr.requests_received = m_requests_received.exchange(0);
  fcr.requests_sent = m_requests_sent.exchange(0);
  fcr.lists_received = m_lists_received.exchange(0);
  fcr.lists_sent = m_lists_sent.exchange(0);
  fcr.total_requests_received = m_total_requests_received.load();
  fcr.total_requests_sent = m_total_requests_sent.load();
  fcr.total_lists_received = m_total_lists_received.load();
  fcr.total_lists_sent = m_total_lists_sent.load();
  ci.add(fcr);
}

void
ListReverser::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  auto parsed_conf = obj.get<listreverser::ConfParams>();
  m_send_timeout = std::chrono::milliseconds(parsed_conf.send_timeout_ms);
  m_request_timeout = std::chrono::milliseconds(parsed_conf.request_timeout_ms);
  m_num_generators = parsed_conf.num_generators;
  m_reverser_id = parsed_conf.reverser_id;

  TLOG_DEBUG(TLVL_CONFIGURE) << "ListReverser " << m_reverser_id << " configured with "
                             << "send timeout " << parsed_conf.send_timeout_ms << " ms,"
                             << " request timeout " << parsed_conf.request_timeout_ms << "ms, "
                             << " and num_generators " << parsed_conf.num_generators;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
ListReverser::do_start(const nlohmann::json& /*startobj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  get_iomanager()->add_callback<IntList>(m_list_connection,
                                         std::bind(&ListReverser::process_list, this, std::placeholders::_1));
  get_iomanager()->add_callback<RequestList>(
    m_requests, std::bind(&ListReverser::process_list_request, this, std::placeholders::_1));

  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ListReverser::do_stop(const nlohmann::json& /*stopobj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  get_iomanager()->remove_callback<RequestList>(m_requests);
  get_iomanager()->remove_callback<IntList>(m_list_connection);
  TLOG() << get_name() << " successfully stopped";

  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_stop() method, received " << m_total_requests_received.load() << " request messages, "
           << "sent " << m_total_requests_sent.load() << ", received " << m_total_lists_received.load()
           << " lists, and sent " << m_total_lists_sent.load() << " reversed list messages";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
ListReverser::process_list_request(const RequestList& request)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_list_request() method";
  {
    std::lock_guard<std::mutex> lk(m_map_mutex);
    if (!m_pending_lists.count(request.list_id)) {
      m_pending_lists[request.list_id] = PendingList(request.destination, request.list_id, m_reverser_id);
      ++m_requests_received;
      ++m_total_requests_received;
    }
  }

  for (size_t gen_idx = 0; gen_idx < m_num_generators; ++gen_idx) {
    TLOG_DEBUG(TLVL_REQUEST_SENDING) << "Sending request for " << request.list_id << " with destination "
                                     << m_list_connection << " to rdlg" << gen_idx
                                     << "_request_connection (num_generators=" << m_num_generators << ")";
    RequestList req(request.list_id, m_list_connection);
    get_iomanager()
      ->get_sender<RequestList>("rdlg" + std::to_string(gen_idx) + "_request_connection")
      ->send(std::move(req), m_send_timeout);
    ++m_requests_sent;
    ++m_total_requests_sent;
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_list_request() method";
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
ListReverser::process_list(const IntList& list)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_list() method";

  std::lock_guard<std::mutex> lk(m_map_mutex);
  ++m_lists_received;
  ++m_total_lists_received;
  TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Received list #" << list.list_id << " from " << list.generator_id
                                 << ". It has size " << list.list.size() << ". Reversing its contents";

  if (m_pending_lists.count(list.list_id) == 0) {

    std::ostringstream oss_warn;
    oss_warn << "send " << list.list_id << " to \"" << m_pending_lists[list.list_id].requestor
             << "\" (late list receive)";
    ers::warning(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_send_timeout.count()));
    return;
  }

  auto workingVector = list.list;
  std::reverse(workingVector.begin(), workingVector.end());
  IntList wrapped(list.list_id, m_reverser_id, workingVector);

  ReversedList::Data this_data;
  this_data.original = list;
  this_data.reversed = wrapped;

  m_pending_lists[list.list_id].list.lists.push_back(this_data);

  std::ostringstream oss_prog;
  oss_prog << "Reversed list #" << list.list_id << " from " << list.generator_id << ", new contents " << workingVector
           << " and size " << workingVector.size() << ". ";
  ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

  if (m_pending_lists[list.list_id].list.lists.size() >= m_num_generators ||
      std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - m_pending_lists[list.list_id].start_time) > m_request_timeout) {

    bool successfullyWasSent = false;
    int failCount = 0;
    while (!successfullyWasSent && failCount < 100) {
      TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Sending the reversed lists " << list.list_id;
      try {
        get_iomanager()
          ->get_sender<ReversedList>(m_pending_lists[list.list_id].requestor)
          ->send(std::move(m_pending_lists[list.list_id].list), m_send_timeout);
        successfullyWasSent = true;
        ++m_lists_sent;
        ++m_total_lists_sent;
        m_pending_lists.erase(list.list_id);
      } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
        std::ostringstream oss_warn;
        oss_warn << "send " << list.list_id << " to \"" << m_pending_lists[list.list_id].requestor << "\"";
        ers::warning(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_send_timeout.count()));
        ++failCount;
      }
    }
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_list() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ListReverser)

// Local Variables:
// c-basic-offset: 2
// End:
