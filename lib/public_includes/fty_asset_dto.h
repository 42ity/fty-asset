/*  =========================================================================
    fty_asset_dto - Asset DTO: defines ASSET attributes for data exchange

    Copyright (C) 2016 - 2020 Eaton

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

#pragma once

#include "fty_common_asset.h"

#include <fty_common_asset_types.h>
#include <map>
#include <optional>
#include <string>

// fwd declaration
struct fty_proto_t;

namespace cxxtools {
    class SerializationInfo;
}

namespace fty {
// extended properties
static constexpr const char* EXT_UUID         = "uuid";
static constexpr const char* EXT_CREATE_TS    = "create_ts";
static constexpr const char* EXT_CREATE_USER  = "create_user";
static constexpr const char* EXT_UPDATE_TS    = "update_ts";
static constexpr const char* EXT_UPDATE_USER  = "update_user";
static constexpr const char* EXT_NAME         = "name"; // friendly name
static constexpr const char* EXT_MODEL        = "model";
static constexpr const char* EXT_MANUFACTURER = "manufacturer";
static constexpr const char* EXT_SERIAL_NO    = "serial_no";

/// List of valid asset statuses
enum class AssetStatus
{
    Unknown = 0,
    Active,
    Nonactive
};

const std::string assetStatusToString(AssetStatus status);
AssetStatus       stringToAssetStatus(const std::string& str);

class ExtMapElement
{
public:
    // constrcutors / destructors
    ExtMapElement(const std::string& val = "", bool readOnly = false, bool forceToFalse = false);
    ExtMapElement(const ExtMapElement& element);
    ExtMapElement(ExtMapElement&& element);
    ~ExtMapElement() = default;

    ExtMapElement& operator=(const ExtMapElement& element);
    ExtMapElement& operator=(ExtMapElement&& element);

    // getters
    const std::string& getValue() const;
    bool               isReadOnly() const;
    bool               wasUpdated() const;

    // setters
    void setValue(const std::string& val);
    void setReadOnly(bool readOnly);

    // overload equality and inequality check
    bool operator==(const ExtMapElement& element) const;
    bool operator!=(const ExtMapElement& element) const;

    // serialization / deserialization for cxxtools
    void serialize(cxxtools::SerializationInfo& si) const;
    void deserialize(const cxxtools::SerializationInfo& si);

private:
    std::string m_value;
    bool m_readOnly{false};
    bool m_wasUpdated{false};
};

void operator<<=(cxxtools::SerializationInfo& si, const ExtMapElement& e);
void operator>>=(const cxxtools::SerializationInfo& si, ExtMapElement& e);

class AssetLink
{
public:
    using ExtMap = std::map<std::string, ExtMapElement>;

    AssetLink() = default;
    AssetLink(const std::string& s, std::string o, std::string i, int t);

    const std::string&       sourceId() const;
    const std::string&       srcOut() const;
    const std::string&       destIn() const;
    int                      linkType() const;
    const AssetLink::ExtMap& ext() const;
    const std::string&       extEntry(const std::string& key) const;
    bool                     isReadOnly(const std::string& key) const;
    const std::string&       secondaryID() const;

    void setSourceId(const std::string& sourceId);
    void setSrcOut(const std::string& srcOut);
    void setDestIn(const std::string& destIn);
    void setLinkType(const int linkType);
    void setExt(const AssetLink::ExtMap& ext);
    void clearExtMap();

    void setExtEntry(const std::string& key, const std::string& value, bool readOnly = false,
        bool forceUpdatedFalse = false);

    void setSecondaryID(const std::string& secondaryID);

    void serialize(cxxtools::SerializationInfo& si) const;
    void deserialize(const cxxtools::SerializationInfo& si);

private:
    std::string m_sourceId;
    std::string m_srcOut;
    std::string m_destIn;
    int         m_linkType{0};
    ExtMap      m_ext;
    std::string m_secondaryID;
};

bool operator==(const AssetLink& l, const AssetLink& r);
void operator<<=(cxxtools::SerializationInfo& si, const AssetLink& l);
void operator>>=(const cxxtools::SerializationInfo& si, AssetLink& l);

// fwd declared
class FullAsset;

class Asset
{
public:
    using ExtMap = std::map<std::string, ExtMapElement>;

    virtual ~Asset() = default;

    // getters
    const std::string&   getInternalName() const;
    AssetStatus          getAssetStatus() const;
    const std::string&   getAssetType() const;
    const std::string&   getAssetSubtype() const;
    const std::string&   getParentIname() const;
    int                  getPriority() const;
    const std::string&   getAssetTag() const;
    const Asset::ExtMap& getExt() const;
    const std::string&   getSecondaryID() const;

    // ext param getters (return empty string if key not found)
    const std::string&              getExtEntry(const std::string& key) const;
    bool                            isExtEntryReadOnly(const std::string& key) const;
    const std::string&              getUuid() const;
    const std::string&              getManufacturer() const;
    const std::string&              getModel() const;
    const std::string&              getSerialNo() const;
    const std::vector<AssetLink>&   getLinkedAssets() const;
    bool                            hasParentsList() const;
    const std::vector<Asset>&       getParentsList() const;

    const std::string&              getFriendlyName() const;
    std::vector<std::string>        getAddresses() const;


    // setters
    void setInternalName(const std::string& internalName);
    void setAssetStatus(AssetStatus assetStatus);
    void setAssetType(const std::string& assetType);
    void setAssetSubtype(const std::string& assetSubtype);
    void setParentIname(const std::string& parentIname);
    void setPriority(int priority);
    void setAssetTag(const std::string& assetTag);
    void setExtMap(const ExtMap& map);
    void clearExtMap();
    void setExtEntry(const std::string& key, const std::string& value, bool readOnly = false,
        bool forceUpdatedFalse = false);
    void addLink(const std::string& sourceId, const std::string& scrOut, const std::string& destIn,
        int linkType, const AssetLink::ExtMap& attributes);
    void removeLink(
        const std::string& sourceId, const std::string& scrOut, const std::string& destIn, int linkType);
    void setLinkedAssets(const std::vector<AssetLink>& assets);
    void setSecondaryID(const std::string& secondaryID);
    void setFriendlyName(const std::string& friendlyName);

    //Wrapper for addresses => max 256
    using AddressMap = std::map<uint8_t, std::string>;

    AddressMap          getAddressMap() const;
    const std::string&  getAddress(uint8_t index) const;

    void setAddress(uint8_t index, const std::string & address);
    void removeAddress(uint8_t index);

    //Wrapper Endpoints => max 256
    using ProtocolMap = std::map<uint8_t, std::string>;

    ProtocolMap         getProtocolMap() const;

    const std::string&  getEndpointProtocol(uint8_t index) const;
    const std::string&  getEndpointPort(uint8_t index) const;
    const std::string&  getEndpointSubAddress(uint8_t index) const;
    const std::string&  getEndpointOperatingStatus(uint8_t index) const;
    const std::string&  getEndpointErrorMessage(uint8_t index) const;

    const std::string&  getEndpointProtocolAttribute(uint8_t index, const std::string & attributeName) const;

    void setEndpointProtocol(uint8_t index, const std::string & val);
    void setEndpointPort(uint8_t index, const std::string & val);
    void setEndpointSubAddress(uint8_t index, const std::string & val);
    void setEndpointOperatingStatus(uint8_t index, const std::string & val);
    void setEndpointErrorMessage(uint8_t index, const std::string & val);

    void setEndpointProtocolAttribute(uint8_t index, const std::string & attributeName, const std::string & val);

    void removeEndpoint(uint8_t index);

    //
    // dump
    void dump(std::ostream& os);

    // overload equality and inequality check
    bool operator==(const Asset& asset) const;
    bool operator!=(const Asset& asset) const;

    // serialization / deserialization for cxxtools
    void serialize(cxxtools::SerializationInfo& si) const;
    void deserialize(const cxxtools::SerializationInfo& si);

    // conversion from/to different DTO representations
    static void fromJson(const std::string& json, Asset& a);
    static void fromFtyProto(fty_proto_t* p, Asset& a, bool extAttributeReadOnly, bool test = false);

    static FullAsset toFullAsset(const Asset& a);
    static std::string toJson(const Asset& a);
    static fty_proto_t* toFtyProto(const Asset& a, const std::string& op, bool test = false);

protected:
    // internal name = <subtype>-<id>)
    std::string m_internalName;

    AssetStatus m_assetStatus  = AssetStatus::Unknown;
    std::string m_assetType    = TYPE_UNKNOWN;
    std::string m_assetSubtype = SUB_UNKNOWN;

    // direct parent iname
    std::string m_parentIname;
    // priority 1..5 (1 is most, 5 is least)
    int m_priority{5};
    // asset tag
    std::string m_assetTag;
    // secondary ID
    std::string m_secondaryID;
    // ext map storage (asset-specific values with readonly attribute)
    ExtMap                 m_ext;
    std::vector<AssetLink> m_linkedAssets;

    std::optional<std::vector<Asset>> m_parentsList;

    const std::string&  getEndpointData(uint8_t index, const std::string &field) const;
    void  setEndpointData(uint8_t index, const std::string &field, const std::string & val);

};

void operator<<=(cxxtools::SerializationInfo& si, const fty::Asset& asset);
void operator>>=(const cxxtools::SerializationInfo& si, fty::Asset& asset);

} // namespace fty
