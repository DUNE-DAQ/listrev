/**
 * @file ReversedListValidator.cpp ReversedListValidator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include <string>

#include "listrev/reversedlistvalidatorinfo/InfoNljs.hpp"
#include "listrev/dal/ReversedListValidator.hpp"
#include "listrev/dal/RandomDataListGenerator.hpp"
#include "listrev/dal/RandomListGeneratorSet.hpp"

#include "ReversedListValidator.hpp"
#include "CommonIssues.hpp"

#include "appfwk/ModuleConfiguration.hpp"
#include "confmodel/Connection.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <functional>
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
{
  register_command("start", &ReversedListValidator::do_start, std::set<std::string>{ "CONFIGURED" });
  register_command("stop", &ReversedListValidator::do_stop, std::set<std::string>{ "TRIGGER_SOURCES_STOPPED" });
}

void
ReversedListValidator::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  auto mdal = mcfg
    ->module<dal::ReversedListValidator>(get_name());
  for (auto con : mdal->get_inputs()) {
    if (con->get_data_type() == datatype_to_string<ReversedList>()) {
      m_list_connection = con->UID();
      break;
    }
  }
  for (auto con : mdal->get_outputs()) {
    if (con->get_data_type() == datatype_to_string<CreateList>()) {
      m_create_connection = con->UID();
    }
    if (con->get_data_type() == datatype_to_string<RequestList>()) {
      m_num_reversers++;
      m_reveserIds.push_back(con->UID());
    }
  }

  for (auto gen : mdal->get_generatorSet()->get_generators()) {
    m_generatorIds.push_back(gen->get_generator_id());
  }
  m_num_generators = m_generatorIds.size();

  // these are just tests to check if the connections are ok
  auto iom = iomanager::IOManager::get();
  iom->get_receiver<ReversedList>(m_list_connection);
  iom->get_sender<CreateList>(m_create_connection);

  m_send_timeout = std::chrono::milliseconds(mdal->get_send_timeout_ms());
  m_request_timeout = std::chrono::milliseconds(mdal->get_request_timeout_ms());
  m_max_outstanding_requests = mdal->get_max_outstanding_requests();

  m_list_creator =
    ListCreator(m_create_connection,
                m_send_timeout,
                mdal->get_min_list_size(),
                mdal->get_max_list_size());

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
  fcr.total_valid_pairs = m_total_valid_pairs.load();
  fcr.valid_list_pairs = m_valid_list_pairs.exchange(0);
  fcr.total_invalid_pairs = m_total_invalid_pairs.load();
  fcr.invalid_list_pairs = m_invalid_list_pairs.exchange(0);
  ci.add(fcr);
}


void
ReversedListValidator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_next_id = 0;
  m_work_thread.start_working_thread();
  get_iomanager()->add_callback<ReversedList>(
    m_list_connection,
    std::bind(&ReversedListValidator::process_list, this, std::placeholders::_1));
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
ReversedListValidator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_work_thread.stop_working_thread();

  std::chrono::milliseconds stop_timeout(10000);
  auto stop_wait = std::chrono::steady_clock::now();
  size_t outstanding_wait = 1;
  while (outstanding_wait > 0 && std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now() - stop_wait) < stop_timeout) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::lock_guard<std::mutex> lk(m_outstanding_id_mutex);
    outstanding_wait = m_outstanding_ids.size();
  }

  TLOG() << get_name() << " Removing callback, there are " << outstanding_wait << " requests left outstanding.";

  get_iomanager()->remove_callback<ReversedList>(m_list_connection);
  TLOG() << get_name() << " successfully stopped";

  
  std::ostringstream oss_summ;
  oss_summ << ": Exiting do_stop() method, received " << m_total_lists.load() << " reversed list messages, "
           << "compared " << m_total_valid_pairs.load() + m_total_invalid_pairs.load()
           << " reversed lists to their original data, and found " << m_total_invalid_pairs.load() << " mismatches. ";
  ers::info(ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));

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
  m_request_start = std::chrono::steady_clock::now();

  while (running_flag.load()) {
    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Locking out id list";
    std::lock_guard<std::mutex> lk(m_outstanding_id_mutex);

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Sending new requests";
    auto next_req_time = [&]() { 
        auto ms = 1000.0 / m_request_rate_hz;
      auto off = ms * m_next_id;
      return m_request_start + std::chrono::milliseconds(static_cast<int>(off));
    };

    while (m_outstanding_ids.size() < m_max_outstanding_requests && std::chrono::steady_clock::now() > next_req_time()) {
      m_list_creator.send_create(++m_next_id);
      m_outstanding_ids[m_next_id] = std::chrono::steady_clock::now();
      send_request(m_next_id);
      ++m_requests_total;
      ++m_new_requests;
    }

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": End of do_work loop";
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

void
ReversedListValidator::process_list(const ReversedList& list)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering process_list() method";

  ++m_total_lists;
  ++m_new_lists;

  std::ostringstream oss_prog;
  oss_prog << "Validating list set #" << list.list_id << " from reverser " << list.reverser_id << ". ";
  ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

  if (list.lists.size() != m_num_generators) {
    ers::error(MissingListError(ERS_HERE, get_name(), list.list_id, m_num_generators, list.lists.size()));  
  }

  for (auto& list_data : list.lists) {

    std::ostringstream oss_prog;
    oss_prog << "Validating list #" << list.list_id << " from generator " << list_data.original.generator_id << ", original contents " << list_data.original.list
             << " and reversed contents " << list_data.reversed.list << ". ";
    ers::debug(ProgressUpdate(ERS_HERE, get_name(), oss_prog.str()));

    TLOG_DEBUG(TLVL_LIST_VALIDATION)
      << get_name() << ": Re-reversing the reversed list so that it can be compared to the original list";
    auto reversed = list_data.reversed.list;
    std::reverse(reversed.begin(), reversed.end());

    TLOG_DEBUG(TLVL_LIST_VALIDATION) << get_name() << ": Comparing the doubly-reversed list with the original list";
    if (reversed != list_data.original.list) {
      std::ostringstream oss_rev;
      oss_rev << reversed;
      std::ostringstream oss_orig;
      oss_orig << list_data.original.list;
      ers::error(DataMismatchError(ERS_HERE, get_name(), list.list_id, oss_rev.str(), oss_orig.str()));
      ++m_invalid_list_pairs;
      ++m_total_invalid_pairs;
    } else {
      ++m_valid_list_pairs;
      ++m_total_valid_pairs;
    }
  }

  std::lock_guard<std::mutex> lk(m_outstanding_id_mutex);
  m_outstanding_ids.erase(list.list_id);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting process_list() method";
}

void
ReversedListValidator::send_request(int id)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering send_request() method";

  auto reverser_id = id % m_num_reversers;

  RequestList req;
  req.list_id = id;
  req.destination = m_list_connection;

  get_iomanager()
    ->get_sender<RequestList>(m_reveserIds[reverser_id])
    ->send(std::move(req), m_send_timeout);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting send_request() method";
}

} // namespace listrev
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::listrev::ReversedListValidator)

// Local Variables:
// c-basic-offset: 2
// End:
