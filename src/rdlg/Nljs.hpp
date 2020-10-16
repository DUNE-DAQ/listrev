/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains functions struct and other type definitions for shema in 
 * namespace dunedaq::listrev::rdlg to be serialized via nlohmann::json.
 */
#ifndef DUNEDAQ_LISTREV_RDLG_NLJS_HPP
#define DUNEDAQ_LISTREV_RDLG_NLJS_HPP


#include "rdlg/Structs.hpp"


#include <nlohmann/json.hpp>

namespace dunedaq::listrev::rdlg {

    using data_t = nlohmann::json;


    
    inline void to_json(data_t& j, const Conf& obj) {
        j["nIntsPerList"] = obj.nIntsPerList;
        j["waitBetweenSendsMsec"] = obj.waitBetweenSendsMsec;
    }
    
    inline void from_json(const data_t& j, Conf& obj) {
        if (j.contains("nIntsPerList"))
            j.at("nIntsPerList").get_to(obj.nIntsPerList);    
        if (j.contains("waitBetweenSendsMsec"))
            j.at("waitBetweenSendsMsec").get_to(obj.waitBetweenSendsMsec);    
    }
    
    // fixme: add support for MessagePack serializers (at least)

} // namespace dunedaq::listrev::rdlg

#endif // DUNEDAQ_LISTREV_RDLG_NLJS_HPP