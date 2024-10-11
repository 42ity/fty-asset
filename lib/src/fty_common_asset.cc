/*  =========================================================================
    fty_common_asset - General asset representation

    Copyright (C) 2019 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_common_asset - General asset representation
@discuss
@end
*/

#include "fty_common_asset.h"

#include <cxxtools/serializationinfo.h>
#include <fty_common_json.h>
#include <fty_common_mlm.h>
#include <fty_common_asset_types.h>
#include <fty_common_macros.h>
#include <fty_log.h>
#include <fty_proto.h>
#include <fty/convert.h>
#include <fty_common.h>

namespace fty {
    BasicAsset::BasicAsset ()
    {}

    BasicAsset::BasicAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype)
        : id_(id)
        , status_(stringToStatus (status))
        , type_subtype_(std::make_pair (stringToType (type), stringToSubtype (subtype)))
    {}

    BasicAsset::BasicAsset (fty_proto_t *msg)
    {
        if (!msg || fty_proto_id (msg) != FTY_PROTO_ASSET) {
            throw std::invalid_argument ("Wrong fty-proto type");
        }

        id_ = fty_proto_name (msg);

        std::string status_str = fty_proto_aux_string (msg, "status", "active");
        status_ = stringToStatus (status_str);

        std::string type_str = fty_proto_aux_string (msg, "type", "");
        std::string subtype_str = fty_proto_aux_string (msg, "subtype", "");
        type_subtype_ = std::make_pair (stringToType (type_str), stringToSubtype (subtype_str));
    }

    BasicAsset::BasicAsset (const cxxtools::SerializationInfo & si)
    {
        si >>= *this;
    }

    void BasicAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        std::string status_str;
        if (!si.findMember ("status")) {
            throw std::runtime_error("status");
        }
        si.getMember("status") >>= status_str;
        if (status_str.empty()) {
            throw std::runtime_error("status");
        }
        status_ = stringToStatus (status_str);

        if (!si.findMember ("type")) {
            throw std::runtime_error("type");
        }
        if (!si.findMember ("sub_type")) {
            throw std::runtime_error("sub_type");
        }
        std::string type_str, subtype_str;
        si.getMember("type") >>= type_str;
        if (type_str.empty()) {
            throw std::runtime_error("type");
        }
        si.getMember("sub_type") >>= subtype_str;
        type_subtype_ = std::make_pair (stringToType (type_str), stringToSubtype (subtype_str));

        if (si.findMember ("id")) {
            si.getMember("id") >>= id_;
        }
        else { // use type/subtype as preliminary ID, as is done elsewhere
            if (type_str == "device") {
                id_ = subtype_str;
            }
            else {
                id_ = type_str;
            }
        }
    }

    void BasicAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        si.addMember("id") <<= id_;

        std::string status_str = statusToString (status_);
        si.addMember("status") <<= status_str;

        std::string type_str = typeToString (type_subtype_.first);
        std::string subtype_str = subtypeToString (type_subtype_.second);
        si.addMember("type") <<= type_str;
        si.addMember("sub_type") <<= subtype_str;
    }

    bool BasicAsset::operator == (const BasicAsset &asset) const
    {
        return id_ == asset.id_
            && status_ == asset.status_
            && type_subtype_ == asset.type_subtype_;
    }

    bool BasicAsset::operator!= (const BasicAsset &asset) const
    {
        return !(*this == asset);
    }

    /// comparator for simplified equality check
    bool BasicAsset::compare (const BasicAsset &asset) const
    {
        return asset.id_ == id_;
    }

    // getters/setters
    std::string BasicAsset::getId () const
    {
        return id_;
    }

    BasicAsset::Status BasicAsset::getStatus () const
    {
        return status_;
    }

    std::string BasicAsset::getStatusString () const
    {
        return statusToString (status_);
    }

    uint16_t BasicAsset::getType () const
    {
        return type_subtype_.first;
    }

    std::string BasicAsset::getTypeString () const
    {
        return typeToString (type_subtype_.first);
    }

    uint16_t BasicAsset::getSubtype () const
    {
        return type_subtype_.second;
    }

    std::string BasicAsset::getSubtypeString () const
    {
        return subtypeToString (type_subtype_.second);
    }

    void BasicAsset::setStatus (const std::string & status)
    {
        status_ = stringToStatus (status);
    }

    void BasicAsset::setType (const std::string & type)
    {
        type_subtype_.first = stringToType (type);
    }

    void BasicAsset::setSubtype (const std::string & subtype)
    {
        type_subtype_.second = stringToSubtype (subtype);
    }

    std::string BasicAsset::typeToString (uint16_t type) const
    {
        return persist::typeid_to_type(type);
    }

    uint16_t BasicAsset::stringToType (const std::string & type) const
    {
        return persist::type_to_typeid(type);
    }

    std::string BasicAsset::subtypeToString (uint16_t subtype) const
    {
        return persist::subtypeid_to_subtype(subtype);
    }

    uint16_t BasicAsset::stringToSubtype (const std::string & subtype) const
    {
        return persist::subtype_to_subtypeid(subtype);
    }

    std::string BasicAsset::statusToString (BasicAsset::Status status) const
    {
        if (status == Status::Active) { return "active"; }
        if (status == Status::Nonactive) { return "nonactive"; }

        throw std::invalid_argument ("status is not known value");
    }

    BasicAsset::Status BasicAsset::stringToStatus (const std::string & status) const
    {
        if (status == "active") { return Status::Active; }
        if (status == "nonactive") { return Status::Nonactive; }

        throw std::invalid_argument (TRANSLATE_ME ("status is not known value"));
    }

    std::string BasicAsset::toJson() const
    {
        cxxtools::SerializationInfo si;
        si <<= *this;
        return JSON::writeToString(si, false);
    }

    bool BasicAsset::isPowerAsset () const
    {
        uint16_t type = type_subtype_.first;
        if (type != persist::asset_type::DEVICE) {
            return false;
        }

        uint16_t subtype = type_subtype_.second;
        return subtype == persist::asset_subtype::UPS
            || subtype == persist::asset_subtype::EPDU
            || subtype == persist::asset_subtype::STS
            || subtype == persist::asset_subtype::GENSET
            || subtype == persist::asset_subtype::PDU
        ;
    }

    ExtendedAsset::ExtendedAsset ()
    {}

    ExtendedAsset::ExtendedAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, int priority)
            : BasicAsset (id, status, type, subtype), name_(name), parent_id_(parent_id), priority_(fty::convert<uint8_t>(priority))
    {}

    ExtendedAsset::ExtendedAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, const std::string & priority)
            : BasicAsset (id, status, type, subtype), name_(name), parent_id_(parent_id)
    {
        setPriority (priority);
    }

    ExtendedAsset::ExtendedAsset (fty_proto_t *msg): BasicAsset (msg)
    {
        name_ = fty_proto_ext_string (msg, "name", fty_proto_name (msg));
        parent_id_ = fty_proto_aux_string (msg, "parent_name.1", "");
        priority_ = fty::convert<uint8_t>(fty_proto_aux_number (msg, "priority", 5));
    }

    ExtendedAsset::ExtendedAsset (const cxxtools::SerializationInfo & si)
    {
        si >>= *this;
    }

    void ExtendedAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        BasicAsset::deserialize (si);
        si.getMember("name") >>= name_;

        if (!si.findMember ("priority")) {
            throw std::runtime_error("priority");
        }
        std::string priority_str;
        si.getMember("priority") >>= priority_str;
        if (priority_str.empty()) {
            throw std::runtime_error("priority");
        }
        this->setPriority (priority_str);

        if (!si.findMember ("location")) {
            throw std::runtime_error("location");
        }
        si.getMember("location") >>= parent_id_;
    }

    void ExtendedAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        BasicAsset::serialize (si);
        si.addMember("name") <<= name_;

        std::string priority_str = this->getPriorityString();
        si.addMember("priority") <<= priority_str;

        si.addMember("location") <<= parent_id_;
    }

    std::string ExtendedAsset::toJson() const
    {
        cxxtools::SerializationInfo si;
        si <<= *this;
        return JSON::writeToString(si, false);
    }

    bool ExtendedAsset::operator == (const ExtendedAsset &asset) const
    {
        return BasicAsset::operator == (asset)
            && name_ == asset.name_
            && parent_id_ == asset.parent_id_
            && priority_ == asset.priority_;
    }

    bool ExtendedAsset::operator!= (const ExtendedAsset &asset) const
    {
        return !(*this == asset);
    }

    // getters/setters
    std::string ExtendedAsset::getName () const
    {
        return name_;
    }

    std::string ExtendedAsset::getParentId () const
    {
        return parent_id_;
    }

    std::string ExtendedAsset::getPriorityString () const
    {
        return std::string ("P") + std::to_string (priority_);
    }

    int ExtendedAsset::getPriority () const
    {
        return priority_;
    }

    void ExtendedAsset::setName (const std::string & name)
    {
        name_ = name;
    }

    void ExtendedAsset::setParentId (const std::string & parent_id)
    {
        parent_id_ = parent_id;
    }

    void ExtendedAsset::setPriority (int priority)
    {
        priority_ = fty::convert<uint8_t>(priority);
    }

    void ExtendedAsset::setPriority (const std::string & priority)
    {
        if (priority.find("P") == 0) {
            priority_ = fty::convert<uint8_t>(priority.substr(1));
        }
        else {
            priority_ = fty::convert<uint8_t>(priority);
        }
    }

    FullAsset::FullAsset()
    {}

    FullAsset::FullAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, int priority, HashMap aux, HashMap ext)
            : ExtendedAsset (id, status, type, subtype, name, parent_id, priority), aux_(aux), ext_(ext)
    {}

    FullAsset::FullAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, const std::string & priority, HashMap aux, HashMap ext)
            : ExtendedAsset (id, status, type, subtype, name, parent_id, priority), aux_(aux), ext_(ext)
    {}

    FullAsset::FullAsset (fty_proto_t *msg): ExtendedAsset (msg)
    {
        // get our own copies since zhash_to_map doesn't do copying
        zhash_t *aux = fty_proto_aux (msg);
        zhash_t *ext = fty_proto_ext (msg);
        aux_ = MlmUtils::zhash_to_map (aux);
        ext_ = MlmUtils::zhash_to_map (ext);
    }

    FullAsset::FullAsset (const cxxtools::SerializationInfo & si)
    {
       si >>= *this;
    }

    void FullAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        ExtendedAsset::deserialize (si);

        if (si.findMember ("aux")) {
            const cxxtools::SerializationInfo auxSi = si.getMember("aux");
            for (const auto& oneElement : auxSi) {
                auto key = oneElement.name();
                std::string value;
                oneElement >>= value;
                aux_[key] = value;
            }
        }

        if (si.findMember ("ext")) {
            const cxxtools::SerializationInfo extSi = si.getMember("ext");
            for (const auto& oneElement : extSi) {
                auto key = oneElement.name();
                // ext from UI behaves as an object of objects with empty 1st level keys
                if (key.empty()) {
                    for (const auto& innerElement : oneElement) {
                        auto innerKey = innerElement.name();
                        log_debug ("inner key = %s", innerKey.c_str ());
                        // only DB is interested in read_only attribute
                        if (innerKey != "read_only") {
                            std::string value;
                            innerElement >>= value;
                            ext_[innerKey] = value;
                        }
                    }
                }
                else {
                    std::string value;
                    oneElement >>= value;
                    log_debug ("key = %s, value = %s", key.c_str (), value.c_str ());
                    ext_[key] = value;
                }
            }
        }
    }

    void FullAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        ExtendedAsset::serialize (si);

        cxxtools::SerializationInfo &auxSi = si.addMember("aux");
        auxSi.setCategory (cxxtools::SerializationInfo::Object);
        for (const auto& keyValue : aux_) {
            auto key = keyValue.first;
            auto value = keyValue.second;
            cxxtools::SerializationInfo &keyValueObject = auxSi.addMember (key);
            keyValueObject.setCategory (cxxtools::SerializationInfo::Object);
            keyValueObject.setValue (value);
        }

        cxxtools::SerializationInfo &extSi = si.addMember("ext");
        extSi.setCategory (cxxtools::SerializationInfo::Object);
        for (const auto& keyValue : ext_) {
            auto key = keyValue.first;
            auto value = keyValue.second;
            cxxtools::SerializationInfo &keyValueObject = extSi.addMember (key);
            keyValueObject.setCategory (cxxtools::SerializationInfo::Object);
            keyValueObject.setValue (value);
        }
    }

    std::string FullAsset::toJson() const
    {
        cxxtools::SerializationInfo si;
        si <<= *this;
        return JSON::writeToString(si, false);
    }

    bool FullAsset::operator == (const FullAsset &asset) const
    {
        return ExtendedAsset::operator == (asset)
            && aux_ == asset.aux_
            && ext_ == asset.ext_;
    }

    bool FullAsset::operator!= (const FullAsset &asset) const
    {
        return !(*this == asset);
    }

    // getters/setters
    FullAsset::HashMap FullAsset::getAux () const
    {
        return aux_;
    }

    FullAsset::HashMap FullAsset::getExt () const
    {
        return ext_;
    }

    void FullAsset::setAux (FullAsset::HashMap aux)
    {
        aux_ = aux;
    }

    void FullAsset::setExt (FullAsset::HashMap ext)
    {
        ext_ = ext;
    }

    void FullAsset::setAuxItem (const std::string &key, const std::string &value)
    {
        aux_[key] = value;
    }

    void FullAsset::setExtItem (const std::string &key, const std::string &value)
    {
        ext_[key] = value;
    }

    std::string FullAsset::getAuxItem (const std::string &key) const
    {
        auto it = aux_.find (key);
        if (it != aux_.end ()) {
            return it->second;
        }
        return "";
    }

    std::string FullAsset::getExtItem (const std::string &key) const
    {
        auto it = ext_.find (key);
        if (it != ext_.end ()) {
            return it->second;
        }
        return "";
    }

    std::string FullAsset::getItem (const std::string &key) const
    {
        auto it = ext_.find (key);
        if (it != ext_.end ()) {
            return it->second;
        }

        auto it2 = aux_.find (key);
        if (it2 != aux_.end ()) {
            return it2->second;
        }

        return "";
    }

    std::unique_ptr<BasicAsset> getBasicAssetFromFtyProto (fty_proto_t *msg)
    {
        if (!msg || fty_proto_id (msg) != FTY_PROTO_ASSET) {
            throw std::invalid_argument ("Wrong fty-proto type");
        }

        return std::unique_ptr<BasicAsset>(new BasicAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", "")
        ));
    }

    std::unique_ptr<ExtendedAsset> getExtendedAssetFromFtyProto (fty_proto_t *msg)
    {
        if (!msg || fty_proto_id (msg) != FTY_PROTO_ASSET) {
            throw std::invalid_argument ("Wrong fty-proto type");
        }

        return std::unique_ptr<ExtendedAsset>(new ExtendedAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", ""),
            fty_proto_ext_string (msg, "name", fty_proto_name (msg)),
            fty_proto_aux_string (msg, "parent_name.1", ""),
            fty::convert<int>(fty_proto_aux_number (msg, "priority", 5))
        ));
    }

    std::unique_ptr<FullAsset> getFullAssetFromFtyProto (fty_proto_t *msg)
    {
        if (!msg || fty_proto_id (msg) != FTY_PROTO_ASSET) {
            throw std::invalid_argument ("Wrong fty-proto type");
        }

        zhash_t *aux = fty_proto_aux (msg);
        zhash_t *ext = fty_proto_ext (msg);

        return std::unique_ptr<FullAsset>(new FullAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", ""),
            fty_proto_ext_string (msg, "name", fty_proto_name (msg)),
            fty_proto_aux_string (msg, "parent_name.1", ""),
            fty::convert<uint8_t>(fty_proto_aux_number (msg, "priority", 5)),
            MlmUtils::zhash_to_map (aux),
            MlmUtils::zhash_to_map (ext)
        ));
    }

} // end namespace

    void operator>>= (const cxxtools::SerializationInfo & si, fty::BasicAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator>>= (const cxxtools::SerializationInfo & si, fty::ExtendedAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator>>= (const cxxtools::SerializationInfo & si, fty::FullAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::BasicAsset & asset)
    {
        asset.serialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::ExtendedAsset & asset)
    {
        asset.serialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::FullAsset & asset)
    {
        asset.serialize (si);
    }
