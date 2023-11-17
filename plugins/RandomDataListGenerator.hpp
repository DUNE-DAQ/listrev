/**
 * @file RandomDataListGenerator.hpp
 *
 * RandomDataListGenerator is a simple DAQModule implementation that
 * periodically generates a list of random integers.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_
#define LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_

#include "ListWrapper.hpp"
#include "ListStorage.hpp"

#include "listrev/randomdatalistgenerator/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "iomanager/Sender.hpp"
#include "utilities/WorkerThread.hpp"

#include <ers/Issue.hpp>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace listrev {

/**
 * @brief RandomDataListGenerator creates vectors of ints and writes
 * them to the configured output queues.
 */
class RandomDataListGenerator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief RandomDataListGenerator Constructor
   * @param name Instance name for this RandomDataListGenerator instance
   */
  explicit RandomDataListGenerator(const std::string& name);

  RandomDataListGenerator(const RandomDataListGenerator&) =
    delete; ///< RandomDataListGenerator is not copy-constructible
  RandomDataListGenerator& operator=(const RandomDataListGenerator&) =
    delete;                                                    ///< RandomDataListGenerator is not copy-assignable
  RandomDataListGenerator(RandomDataListGenerator&&) = delete; ///< RandomDataListGenerator is not move-constructible
  RandomDataListGenerator& operator=(RandomDataListGenerator&&) =
    delete; ///< RandomDataListGenerator is not move-assignable

  void init() override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_unconfigure(const nlohmann::json& obj);
  void do_hello(const nlohmann::json& obj);

  // Callbacks
  void process_create_list(const CreateList& create_request);
  void process_request_list(const RequestList& request_list);

  // Init
  std::string m_request_connection;
  std::string m_create_connection;

  // Configuration

  enum class ListMode : uint16_t
  {
    Random = 0,
    Ascending = 1,
    Evens = 2,
    Odds = 3,
    Descending = 4,
    MAX = Descending,
  };
  ListMode m_list_mode{ ListMode::Random };
  std::chrono::milliseconds m_send_timeout{ 100 };
  std::chrono::milliseconds m_request_timeout{ 100 };
  size_t m_generator_id{ 0 };

  // Data
  ListStorage m_storage;

  // Monitoring
  std::atomic<uint64_t> m_generated{ 0 };     // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_generated_tot{ 0 }; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_sent{ 0 };
  std::atomic<uint64_t> m_sent_tot {0};
};
} // namespace listrev

} // namespace dunedaq

#endif // LISTREV_PLUGINS_RANDOMDATALISTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
