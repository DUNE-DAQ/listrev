/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace dunedaq::listrev::rdlg.
 */
#ifndef DUNEDAQ_LISTREV_RDLG_STRUCTS_HPP
#define DUNEDAQ_LISTREV_RDLG_STRUCTS_HPP

#include <cstdint>


namespace dunedaq::listrev::rdlg {

    // @brief A count of very many things
    using Size = uint64_t;

    // @brief A count of not too many things
    using Count = int32_t;

    // @brief RandomDataListGenerator configuration
    struct Conf {

        // @brief Number of numbers
        Size nIntsPerList;

        // @brief Millisecs to wait between sending
        Count waitBetweenSendsMsec;
    };

} // namespace dunedaq::listrev::rdlg

#endif // DUNEDAQ_LISTREV_RDLG_STRUCTS_HPP