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

#include <cxxtools/serializationinfo.h>
#include <fty_common_asset_types.h>
#include <map>
#include <optional>
#include <string>

// fwd declaration
struct fty_proto_t;

namespace fty {
// extended properties
static constexpr const char* EXT_UUID         = "uuid";
static constexpr const char* EXT_CREATE_TS    = "create_ts";
static constexpr const char* EXT_CREATE_USER  = "create_user";
static constexpr const char* EXT_UPDATE_TS    = "update_ts";
static constexpr const char* EXT_UPDATE_USER  = "update_user";
static constexpr const char* EXT_NAME         = "name";
static constexpr const char* EXT_MODEL        = "model";
static constexpr const char* EXT_MANUFACTURER = "manufacturer";
static constexpr const char* EXT_SERIAL_NO    = "serial_no";

// WARNING keep consistent with DB table t_bios_asset_link_type
// clang-format off
static constexpr const char* LINK_POWER_CHAIN                         = "power chain";                      //  1
static constexpr const char* LINK_VMWARE_VCENTER_MONITORS_ESXI        = "vmware.vcenter.monitors.esxi";     //  2
static constexpr const char* LINK_VMWARE_CLUSTER_CONTAINS_ESXI        = "vmware.cluster.contains.esxi";     //  3
static constexpr const char* LINK_VMWARE_ESXI_HOSTS_VM                = "vmware.esxi.hosts.vm";             //  4
static constexpr const char* LINK_VMWARE_STANDALONE_ESXI_HOSTS_VM     = "vmware.standalone.esxi.hosts.vm";  //  5
static constexpr const char* LINK_VMWARE_VCENTER_MONITORS_CLUSTER     = "vmware.vcenter.monitors.cluster";  //  6
static constexpr const char* LINK_VMWARE_VCENTER_MONITORS_VAPP        = "vmware.vcenter.monitors.vapp";     //  7
static constexpr const char* LINK_CITRIX_ZENSERVER_HOSTS_VM           = "citrix.zenserver.hosts.vm";        //  8
static constexpr const char* LINK_CITRIX_POOL_MONITORS_XENSERVER      = "citrix.pool.monitors.xenserver";   //  9
static constexpr const char* LINK_CITRIX_POOL_MONITORS_VAPP           = "citrix.pool.monitors.vapp";        // 10
static constexpr const char* LINK_CITRIX_POOL_MONITORS_TASK           = "citrix.pool.monitors.task";        // 11
static constexpr const char* LINK_CITRIX_POOL_MONITORS_HALTED_VM      = "citrix.pool.monitors.halted.vm";   // 12
static constexpr const char* LINK_HP_IT_RACK_LINK_SERVER              = "hp.it.rack.link.server";           // 13
static constexpr const char* LINK_HP_IT_MANAGER_MONITOR_SERVER        = "hp.it.manager.monitor.server";     // 14
static constexpr const char* LINK_HP_IT_MANAGER_MONITOR_RACK          = "hp.it.manager.monitor.rack";       // 15
static constexpr const char* LINK_MICROSOFT_HYPERV_HOSTS_VM           = "microsoft.hyperv.hosts.vm";        // 16
static constexpr const char* LINK_NETAPP_CLUSTER_CONTAINS_NODE        = "netapp.cluster.contains.node";     // 17
static constexpr const char* LINK_NUTANIX_CLUSTER_CONTAINS_NODE       = "nutanix.cluster.contains.node";    // 18
static constexpr const char* LINK_NUTANIX_NODE_HOSTS_VM               = "nutanix.node.hosts.vm";            // 19
static constexpr const char* LINK_NUTANIX_PROXY_MONITORS_CLUSTER      = "nutanix.proxy.monitors.cluster";   // 20
static constexpr const char* LINK_NUTANIX_PROXY_MONITORS_NODE         = "nutanix.proxy.monitors.node";      // 21
static constexpr const char* LINK_NUTANIX_PROXY_MONITORS_VM           = "nutanix.proxy.monitors.vm";        // 22
static constexpr const char* LINK_IPMINFRA_SERVER_HOSTS_HYPERVISOR    = "ipminfra.server.hosts.hypervisor"; // 23
static constexpr const char* LINK_NUTANIX_CONNECTED_TO_PRISM          = "nutanix.connected.to.prism";       // 24
static constexpr const char* LINK_MICROSOFT_CONNECTED_TO_SERVER       = "microsoft.connected.to.server";    // 25
static constexpr const char* LINK_HP_IT_CONNECTED_TO_ONEVIEW          = "hp.it.connected.to.oneview";       // 26
static constexpr const char* LINK_VMWARE_CONNECTED_TO_VCENTER         = "vmware.connected.to.vcenter";      // 27
static constexpr const char* LINK_VMWARE_CONNECTED_TO_ESXI            = "vmware.connected.to.esxi";         // 28
static constexpr const char* LINK_NETAPP_CONNECTED_TO_ONTAP           = "netapp.connected.to.ontap";        // 29
static constexpr const char* LINK_NETAPP_ONTAP_MONITOR_CLUSTER        = "netapp.ontap.monitor.cluster";     // 30
static constexpr const char* LINK_VMWARE_VCENTER_MANAGES_SRM          = "vmware.vcenter.manages.srm";       // 31
static constexpr const char* LINK_VMWARE_SRM_HAS_PLAN                 = "vmware.srm.has.plan";              // 32
static constexpr const char* LINK_SERVER_HOSTS_HYPERVISOR             = "server.hosts.hypervisor";          // 33
static constexpr const char* LINK_DELL_VXRAIL_CONNECTED_TO_MANAGER    = "dell.vxrail.connected.to.manager"; // 34
static constexpr const char* LINK_MICROSOFT_SERVER_HAS_HYPERV_SERVICE = "microsoft.server.has.hyperv.service"; // 35
// clang-format on

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
    bool        m_readOnly   = false;
    bool        m_wasUpdated = false;
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
    int         m_linkType = 0;
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
    int m_priority = 5;
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


class UIAsset : public Asset
{
public:
    explicit UIAsset(const Asset& a = Asset());

    // serialization / deserialization for cxxtools
    void serializeUI(cxxtools::SerializationInfo& si) const;
    void deserializeUI(const cxxtools::SerializationInfo& si);
};

} // namespace fty

//  Self test of this class
void fty_asset_dto_test(bool verbose);
