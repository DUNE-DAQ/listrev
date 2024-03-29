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

#include "appfwk/DAQModule.hpp"
#include "iomanager/Receiver.hpp"
#include "iomanager/Sender.hpp"
#include "utilities/WorkerThread.hpp"

#include <ers/Issue.hpp>

#include <memory>
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

  void init(const nlohmann::json& iniobj) override;

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);

  // Threading
  dunedaq::utilities::WorkerThread thread_;
  void do_work(std::atomic<bool>&);

  // Configuration
  using source_t = dunedaq::iomanager::ReceiverConcept<IntList>;
  std::shared_ptr<source_t> inputQueue_;
  using sink_t = dunedaq::iomanager::SenderConcept<IntList>;
  std::shared_ptr<sink_t> outputQueue_;
  std::chrono::milliseconds queueTimeout_;
};
} // namespace listrev
} // namespace dunedaq

#endif // LISTREV_PLUGINS_LISTREVERSER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
