/**
 * @file ListReverser.hpp
 *
 * ListReverser is a simple DAQModule implementation that reads a list
 * of integers from one queue, reverses their order in the list, and pushes
 * the reversed list onto another queue.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_LISTREVERSER_HPP_
#define LISTREV_PLUGINS_LISTREVERSER_HPP_

#include "ListWrapper.hpp"
#include "ListStorage.hpp"

#include "appfwk/DAQModule.hpp"
#include "iomanager/Receiver.hpp"
#include "iomanager/Sender.hpp"
#include "utilities/WorkerThread.hpp"

#include <ers/Issue.hpp>

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace listrev {

/**
 * @brief ListReverser reads lists of integers from one queue,
 * reverses the order of the list, and writes out the reversed list.
 */
class ListReverser : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief ListReverser Constructor
   * @param name Instance name for this ListReverser instance
   */
  explicit ListReverser(const std::string& name);

  ListReverser(const ListReverser&) = delete;            ///< ListReverser is not copy-constructible
  ListReverser& operator=(const ListReverser&) = delete; ///< ListReverser is not copy-assignable
  ListReverser(ListReverser&&) = delete;                 ///< ListReverser is not move-constructible
  ListReverser& operator=(ListReverser&&) = delete;      ///< ListReverser is not move-assignable

  void init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg) override;

protected:
  void generate_opmon_data() override;

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  // Callbacks
  void process_list_request(const RequestList& request);
  void process_list(const IntList& list);

  // Data
  struct PendingList
  {
    std::string requestor;
    std::chrono::steady_clock::time_point start_time;
    ReversedList list;

    PendingList() = default;
    explicit PendingList(std::string req, int list_id, int rev_id)
      : requestor(req)
      , start_time(std::chrono::steady_clock::now())
    {
      list.list_id = list_id;
      list.reverser_id = rev_id;
    }
  };
  std::map<int, PendingList> m_pending_lists;
  mutable std::mutex m_map_mutex;

  // Init
  std::string m_requests;
  std::string m_list_connection;

  // Configuration
  std::chrono::milliseconds m_send_timeout{ 100 };
  std::chrono::milliseconds m_request_timeout{ 1000 };
  size_t m_reverser_id{ 0 };

  std::vector<uint32_t> m_generatorIds;

  // Monitoring
  std::atomic<uint64_t> m_requests_received{ 0 };
  std::atomic<uint64_t> m_requests_sent{ 0 };
  std::atomic<uint64_t> m_lists_received{ 0 };
  std::atomic<uint64_t> m_lists_sent{ 0 };
  std::atomic<uint64_t> m_total_requests_received{ 0 };
  std::atomic<uint64_t> m_total_requests_sent{ 0 };
  std::atomic<uint64_t> m_total_lists_received{ 0 };
  std::atomic<uint64_t> m_total_lists_sent{ 0 };
};
} // namespace listrev
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTREVERSER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
