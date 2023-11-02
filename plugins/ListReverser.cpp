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
#define TLVL_PROCESS_REQUEST 17

namespace dunedaq {
namespace listrev {

ListReverser::ListReverser(const std::string& name)
  : DAQModule(name)
  , m_request_thread(std::bind(&ListReverser::send_requests, this, std::placeholders::_1))
{
  register_command("start", &ListReverser::do_start);
  register_command("stop", &ListReverser::do_stop);
}

void
ListReverser::init(const nlohmann::json& iniobj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto qi = appfwk::connection_index(iniobj, { "requests_input", "lists_input" });
  m_requests = qi["requests_input"];
  m_list_connection = qi["lists_input"];

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
  ci.add(fcr);
}

void
ListReverser::do_start(const nlohmann::json& /*startobj*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_request_thread.start_working_thread();
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
  m_request_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
ListReverser::process_list_request(const RequestList& request)
{
  TLOG_DEBUG(TLVL_PROCESS_REQUEST) << get_name() << "Checking if I should process this request";
  if (request.list_id % m_num_reversers != m_my_index) {
    return;
  }

  std::lock_guard<std::mutex> lk(m_map_mutex);
  if (!m_pending_requests.count(request.list_id)) {
    m_pending_requests[request.list_id] = request.destination;
    ++m_requests_received;
  }

  if (!m_pending_lists.count(request.list_id)) {
    auto generator = get_next_generator();
    m_pending_lists[request.list_id] = generator;
    send_create(request.list_id, generator.create_connection);
  }
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
  TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Received list #" << list.list_id << ". It has size "
                                 << list.list.size() << ". Reversing its contents";

  auto workingVector = list.list;
  std::reverse(workingVector.begin(), workingVector.end());

  std::ostringstream oss_prog;
  oss_prog << "Reversed list #" << list.list_id << ", new contents " << workingVector << " and size "
           << workingVector.size() << ". ";
  ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

  bool successfullyWasSent = false;
  int failCount = 0;
  while (!successfullyWasSent && failCount < 100) {
    TLOG_DEBUG(TLVL_LIST_REVERSAL) << get_name() << ": Sending the reversed list " << list.list_id;
    try {
      IntList wrapped(list.list_id, workingVector);
      get_iomanager()->get_sender<IntList>(m_pending_requests[list.list_id])->send(std::move(wrapped), m_send_timeout);
      successfullyWasSent = true;
      ++m_lists_sent;
      m_pending_lists.erase(list.list_id);
      m_pending_requests.erase(list.list_id);
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      std::ostringstream oss_warn;
      oss_warn << "send " << list.list_id << " to \"" << m_pending_requests[list.list_id] << "\"";
      ers::warning(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_send_timeout.count()));
      ++failCount;
    }
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_list() method";
}

void
ListReverser::send_requests(std::atomic<bool>& running)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering send_requests() method";
  while (running.load()) {
    TLOG_DEBUG(TLVL_REQUEST_SENDING) << get_name() << ": Start of send_requests loop, waiting "
                                     << m_request_send_interval.count() << " ms before sending requests";
    std::this_thread::sleep_for(m_request_send_interval);
    std::lock_guard<std::mutex> lk(m_map_mutex);
    TLOG_DEBUG(TLVL_REQUEST_SENDING) << get_name() << ": Sending " << m_pending_lists.size() << " requests";
    for (auto& pending_list_pair : m_pending_lists) {
      RequestList req(pending_list_pair.first, m_list_connection);
      get_iomanager()
        ->get_sender<RequestList>(pending_list_pair.second.request_connection)
        ->send(std::move(req), m_send_timeout);
    }
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting send_requests() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ListReverser)

// Local Variables:
// c-basic-offset: 2
// End:
