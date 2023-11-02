/**
 * @file ReversedListValidator.cpp ReversedListValidator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "listrev/reversedlistvalidator/Nljs.hpp"
#include "listrev/reversedlistvalidatorinfo/InfoNljs.hpp"

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
#define TLVL_REQUEST_SENDING 16
#define TLVL_PROCESS_LIST 17

namespace dunedaq {
namespace listrev {

ReversedListValidator::ReversedListValidator(const std::string& name)
  : DAQModule(name)
  , m_work_thread(std::bind(&ReversedListValidator::do_work, this, std::placeholders::_1))
  , m_request_thread(std::bind(&ReversedListValidator::send_requests, this, std::placeholders::_1))
{
  register_command("conf", &ReversedListValidator::do_configure, std::set<std::string>{ "INITIAL" });
  register_command("start", &ReversedListValidator::do_start, std::set<std::string>{ "CONFIGURED" });
  register_command("stop", &ReversedListValidator::do_stop, std::set<std::string>{ "TRIGGER_SOURCES_STOPPED" });
}

void
ReversedListValidator::init(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto mandatory_connections = appfwk::connection_index(obj, { "list_input", "requests_out" });

  m_list_connection = mandatory_connections["list_input"];

  // these are just tests to check if the connections are ok
  auto iom = iomanager::IOManager::get();
  iom->get_receiver<IntList>(m_list_connection);
  m_requests = iom->get_sender<RequestList>(mandatory_connections["requests_out"]);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
ReversedListValidator::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  reversedlistvalidatorinfo::Info fcr;

  fcr.requests_total = m_requests_total.load();
  fcr.new_requests = m_new_requests.exchange(0);
  fcr.total_lists = m_total_lists.load();
  fcr.new_lists = m_new_lists.exchange(0);
  fcr.total_reversed = m_total_reversed.load();
  fcr.new_reversed = m_new_reversed.exchange(0);
  fcr.total_valid_pairs = m_total_valid_pairs.load();
  fcr.valid_list_pairs = m_valid_list_pairs.exchange(0);
  fcr.total_invalid_pairs = m_total_invalid_pairs.load();
  fcr.invalid_list_pairs = m_invalid_list_pairs.exchange(0);
  ci.add(fcr);
}

void
ReversedListValidator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  auto parsed_conf = obj.get<reversedlistvalidator::ConfParams>();
  m_send_timeout = iomanager::Sender::timeout_t(parsed_conf.send_timeout_ms);
  m_max_outstanding_requests = parsed_conf.max_outstanding_requests;
  m_request_send_interval = std::chrono::milliseconds(parsed_conf.request_send_interval);
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
ReversedListValidator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_next_id = 0;
  m_work_thread.start_working_thread();
  m_request_thread.start_working_thread();
  get_iomanager()->add_callback<IntList>(m_list_connection,
                                         std::bind(&ReversedListValidator::process_list, this, std::placeholders::_1));
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ReversedListValidator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  get_iomanager()->remove_callback<IntList>(m_list_connection);
  m_work_thread.stop_working_thread();
  m_request_thread.stop_working_thread();
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

  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Locking out id list";
    std::lock_guard<std::mutex> lk(m_outstanding_id_mutex);

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Sending new requests";
    while (m_outstanding_ids.size() < m_max_outstanding_requests) {
      m_outstanding_ids.insert(++m_next_id);
      ++m_requests_total;
      ++m_new_requests;
    }

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Checking for received list pairs";
    for (auto iter = m_outstanding_ids.begin(); iter != m_outstanding_ids.end();) {
      if (m_lists.has_list(*iter) && m_reversed.has_list(*iter)) {
        IntList original = m_lists.get_list(*iter);
        IntList reversed = m_reversed.get_list(*iter);

        std::ostringstream oss_prog;
        oss_prog << "Validating list #" << *iter << ", original contents " << original.list
                 << " and reversed contents " << reversed.list << ". ";
        ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

        TLOG_DEBUG(TLVL_LIST_VALIDATION)
          << get_name() << ": Re-reversing the reversed list so that it can be compared to the original list";
        std::reverse(reversed.list.begin(), reversed.list.end());

        TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Comparing the doubly-reversed list with the original list";
        if (reversed.list != original.list) {
          std::ostringstream oss_rev;
          oss_rev << reversed.list;
          std::ostringstream oss_orig;
          oss_orig << original.list;
          ers::error(DataMismatchError(ERS_HERE, get_name(),*iter, oss_rev.str(), oss_orig.str()));
          ++m_invalid_list_pairs;
          ++m_total_invalid_pairs;
        } else {
          ++m_valid_list_pairs;
          ++m_total_valid_pairs;
        }

        iter = m_outstanding_ids.erase(iter);
      } else {
        ++iter;
      }
    }
    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": End of do_work loop";
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_work() method, received " << m_total_reversed.load() << " reversed lists, "
           << "compared " << m_total_valid_pairs.load() + m_total_invalid_pairs.load()
           << " of them to their original data, and found " << m_total_invalid_pairs.load()
           << " mismatches. ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

void
ReversedListValidator::send_requests(std::atomic<bool>& running)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering send_requests() method";
  while (running.load()) {
    TLOG_DEBUG(TLVL_REQUEST_SENDING) << get_name() << ": Start of send_requests loop, waiting "
                                     << m_request_send_interval.count() << " ms before sending requests";
    std::this_thread::sleep_for(m_request_send_interval);
    std::lock_guard<std::mutex> lk(m_outstanding_id_mutex);
    TLOG_DEBUG(TLVL_REQUEST_SENDING) << get_name() << ": Sending " << m_outstanding_ids.size() << " requests";
    for (auto& id : m_outstanding_ids) {
      RequestList req(id, m_list_connection);
      m_requests->send(std::move(req), m_send_timeout);
    }
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting send_requests() method";
}

void
ReversedListValidator::process_list(const IntList& list)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_list() method";
  if (list.list_found) {
    if (list.list_reversed && !m_reversed.has_list(list.list_id)) {
      TLOG_DEBUG(TLVL_PROCESS_LIST) << get_name() << ": List is reversed, adding to reversed list storage";
      m_reversed.add_list(list);
      ++m_total_reversed;
      ++m_new_reversed;
    } else if(!m_lists.has_list(list.list_id)) {
      TLOG_DEBUG(TLVL_PROCESS_LIST) << get_name() << ": List is not reversed, adding to list storage";
      m_lists.add_list(list);
      ++m_total_lists;
      ++m_new_lists;
    }
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_list() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ReversedListValidator)

// Local Variables:
// c-basic-offset: 2
// End:
