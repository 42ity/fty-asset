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

#include <cxxtools/serializationinfo.h>
#include <fty_common.h>
#include <map>
#include <string>

namespace fty {
/// List of valid asset statuses
enum class AssetStatus
{
    Unknown = 0,
    Active,
    Nonactive
};

const std::string assetStatusToString(AssetStatus status);
AssetStatus       stringToAssetStatus(const std::string& str);

// extended properties
static constexpr const char* EXT_UUID         = "uuid";
static constexpr const char* EXT_CREATE_TS    = "create_ts";
static constexpr const char* EXT_CREATE_USER  = "create_user";
static constexpr const char* EXT_NAME         = "name";
static constexpr const char* EXT_MODEL        = "model";
static constexpr const char* EXT_MANUFACTURER = "manufacturer";
static constexpr const char* EXT_SERIAL_NO    = "serial_no";

// WARNING keep consistent with DB table t_bios_asset_element_type
static constexpr const char* TYPE_UNKNOWN         = "unknown";         // 0
static constexpr const char* TYPE_GROUP           = "group";           // 1
static constexpr const char* TYPE_DATACENTER      = "datacenter";      // 2
static constexpr const char* TYPE_ROOM            = "room";            // 3
static constexpr const char* TYPE_ROW             = "row";             // 4
static constexpr const char* TYPE_RACK            = "rack";            // 5
static constexpr const char* TYPE_DEVICE          = "device";          // 6
static constexpr const char* TYPE_INFRA_SERVICE   = "infra-service";   // 7
static constexpr const char* TYPE_CLUSTER         = "cluster";         // 8
static constexpr const char* TYPE_HYPERVISOR      = "hypervisor";      // 9
static constexpr const char* TYPE_VIRTUAL_MACHINE = "virtual-machine"; // 10
static constexpr const char* TYPE_STORAGE_SERVICE = "storage-service"; // 11
static constexpr const char* TYPE_VAPP            = "vapp";            // 12
static constexpr const char* TYPE_CONNECTOR       = "connector";       // 13
static constexpr const char* TYPE_SERVER          = "server";          // 15
static constexpr const char* TYPE_PLANNER         = "planner";         // 16
static constexpr const char* TYPE_PLAN            = "plan";            // 17

// WARNING keep consistent with DB table t_bios_asset_device_type
static constexpr const char* SUB_UNKNOWN                          = "unknown";                          // 0
static constexpr const char* SUB_UPS                              = "ups";                              // 1
static constexpr const char* SUB_GENSET                           = "genset";                           // 2
static constexpr const char* SUB_EPDU                             = "epdu";                             // 3
static constexpr const char* SUB_PDU                              = "pdu";                              // 4
static constexpr const char* SUB_SERVER                           = "server";                           // 5
static constexpr const char* SUB_FEED                             = "feed";                             // 6
static constexpr const char* SUB_STS                              = "sts";                              // 7
static constexpr const char* SUB_SWITCH                           = "switch";                           // 8
static constexpr const char* SUB_STORAGE                          = "storage";                          // 9
static constexpr const char* SUB_VM                               = "vm";                               // 10
static constexpr const char* SUB_N_A                              = "N_A";                              // 11
static constexpr const char* SUB_ROUTER                           = "router";                           // 12
static constexpr const char* SUB_RACK_CONTROLLER                  = "rack controller";                  // 13
static constexpr const char* SUB_SENSOR                           = "sensor";                           // 14
static constexpr const char* SUB_APPLIANCE                        = "appliance";                        // 15
static constexpr const char* SUB_CHASSIS                          = "chassis";                          // 16
static constexpr const char* SUB_PATCH_PANEL                      = "patch panel";                      // 17
static constexpr const char* SUB_OTHER                            = "other";                            // 18
static constexpr const char* SUB_SENSORGPIO                       = "sensorgpio";                       // 19
static constexpr const char* SUB_GPO                              = "gpo";                              // 20
static constexpr const char* SUB_NETAPP_ONTAP_NODE                = "netapp.ontap.node";                // 21
static constexpr const char* SUB_IPMINFRA_SERVER                  = "ipminfra.server";                  // 22
static constexpr const char* SUB_IPMINFRA_SERVICE                 = "ipminfra.service";                 // 23
static constexpr const char* SUB_VMWARE_VCENTER                   = "vmware.vcenter";                   // 24
static constexpr const char* SUB_CITRIX_POOL                      = "citrix.pool";                      // 25
static constexpr const char* SUB_VMWARE_CLUSTER                   = "vmware.cluster";                   // 26
static constexpr const char* SUB_VMWARE_ESXI                      = "vmware.esxi";                      // 27
static constexpr const char* SUB_MICROSOFT_HYPERV                 = "microsoft.hyperv";                 // 28
static constexpr const char* SUB_VMWARE_VM                        = "vmware.vm";                        // 29
static constexpr const char* SUB_MICROSOFT_VM                     = "microsoft.vm";                     // 30
static constexpr const char* SUB_CITRIX_VM                        = "citrix.vm";                        // 31
static constexpr const char* SUB_NETAPP_NODE                      = "netapp.node";                      // 32
static constexpr const char* SUB_VMWARE_STANDALONE_ESXI           = "vmware.standalone.esxi";           // 33
static constexpr const char* SUB_VMWARE_TASK                      = "vmware.task";                      // 34
static constexpr const char* SUB_VMWARE_VAPP                      = "vmware.vapp";                      // 35
static constexpr const char* SUB_CITRIX_XENSERVER                 = "citrix.xenserver";                 // 36
static constexpr const char* SUB_CITRIX_VAPP                      = "citrix.vapp";                      // 37
static constexpr const char* SUB_CITRIX_TASK                      = "citrix.task";                      // 38
static constexpr const char* SUB_MICROSOFT_VIRTUALIZATION_MACHINE = "microsoft.virtualization.machine"; // 39
static constexpr const char* SUB_MICROSOFT_TASK                   = "microsoft.task";                   // 40
static constexpr const char* SUB_MICROSOFT_SERVER_CONNECTOR       = "microsoft.server.connector";       // 41
static constexpr const char* SUB_MICROSOFT_SERVER                 = "microsoft.server";                 // 42
static constexpr const char* SUB_MICROSOFT_CLUSTER                = "microsoft.cluster";                // 43
static constexpr const char* SUB_HP_ONEVIEW_CONNECTOR             = "hp.oneview.connector";             // 44
static constexpr const char* SUB_HP_ONEVIEW                       = "hp.oneview";                       // 45
static constexpr const char* SUB_HP_IT_SERVER                     = "hp.it.server";                     // 46
static constexpr const char* SUB_HP_IT_RACK                       = "hp.it.rack";                       // 47
static constexpr const char* SUB_NETAPP_SERVER                    = "netapp.server";                    // 48
static constexpr const char* SUB_NETAPP_ONTAP_CONNECTOR           = "netapp.ontap.connector";           // 49
static constexpr const char* SUB_NETAPP_ONTAP_CLUSTER             = "netapp.ontap.cluster";             // 50
static constexpr const char* SUB_NUTANIX_VM                       = "nutanix.vm";                       // 51
static constexpr const char* SUB_NUTANIX_PRISM_GATEWAY            = "nutanix.prism.gateway";            // 52
static constexpr const char* SUB_NUTANIX_NODE                     = "nutanix.node";                     // 53
static constexpr const char* SUB_NUTANIX_CLUSTER                  = "nutanix.cluster";                  // 54
static constexpr const char* SUB_NUTANIX_PRISM_CONNECTOR          = "nutanix.prism.connector";          // 55
static constexpr const char* SUB_VMWARE_VCENTER_CONNECTOR         = "vmware.vcenter.connector";         // 60
static constexpr const char* SUB_VMWARE_STANDALONE_ESXI_CONNECTOR = "vmware.standalone.esxi.connector"; // 61
static constexpr const char* SUB_NETAPP_ONTAP                     = "netapp.ontap";                     // 62
static constexpr const char* SUB_VMWARE_SRM                       = "vmware.srm";                       // 65
static constexpr const char* SUB_VMWARE_SRM_PLAN                  = "vmware.srm.plan";                  // 66

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

class AssetLink
{
public:
    AssetLink() = default;
    AssetLink(const std::string& s, std::string o, std::string i, int t);

    std::string sourceId;
    std::string srcOut;
    std::string destIn;
    int         linkType = 0;
};

bool operator==(const AssetLink& l, const AssetLink& r);
void operator<<=(cxxtools::SerializationInfo& si, const AssetLink& l);
void operator>>=(const cxxtools::SerializationInfo& si, AssetLink& l);

class Asset
{
public:
    using ExtMap = std::map<std::string, std::pair<std::string, bool>>;

    virtual ~Asset() = default;

    // getters
    uint32_t             getId() const;
    const std::string&   getInternalName() const;
    AssetStatus          getAssetStatus() const;
    const std::string&   getAssetType() const;
    const std::string&   getAssetSubtype() const;
    const std::string&   getParentIname() const;
    int                  getPriority() const;
    const std::string&   getAssetTag() const;
    const Asset::ExtMap& getExt() const;
    // ext param getters (return empty string if key not found)
    const std::string&            getExtEntry(const std::string& key) const;
    bool                          isExtEntryReadOnly(const std::string& key) const;
    const std::string&            getUuid() const;
    const std::string&            getManufacturer() const;
    const std::string&            getModel() const;
    const std::string&            getSerialNo() const;
    const std::vector<AssetLink>& getLinkedAssets() const;

    // setters
    void setId(uint32_t id);
    void setInternalName(const std::string& internalName);
    void setAssetStatus(AssetStatus assetStatus);
    void setAssetType(const std::string& assetType);
    void setAssetSubtype(const std::string& assetSubtype);
    void setParentIname(const std::string& parentIname);
    void setPriority(int priority);
    void setAssetTag(const std::string& assetTag);
    void setExt(const Asset::ExtMap& map);
    void setExtEntry(const std::string& key, const std::string& value, bool readOnly = false);
    void setLinkedAssets(const std::vector<AssetLink>& assets);
    // dump
    void dump(std::ostream& os);

    // overload equality and inequality check
    bool operator==(const Asset& asset) const;
    bool operator!=(const Asset& asset) const;

private:
    uint32_t m_id = 0;
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
    // ext map storage (asset-specific values with readonly attribute)
    ExtMap                 m_ext;
    std::vector<AssetLink> m_linkedAssets;
};

} // namespace fty

//  Self test of this class
void fty_asset_dto_test(bool verbose);
