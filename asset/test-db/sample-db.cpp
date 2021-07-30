#include "sample-db.h"
#include "test-db/test-db.h"
#include <asset/asset-db.h>
#include <fty_common_asset_types.h>
#include <fty_common_db_connection.h>
#include <pack/pack.h>

namespace fty {

using namespace fmt::literals;

// =====================================================================================================================

struct DBData : pack::Node
{
    enum class Types
    {
        Unknown,
        Datacenter,
        Server,
        Connector,
        InfraService,
        Hypervisor,
        VirtualMachine,
        Feed,
        Rack,
        Ups,
        Row,
        Room
    };

    struct Item : pack::Node
    {
        pack::Enum<Types>      type    = FIELD("type");
        pack::String           name    = FIELD("name");
        pack::String           extName = FIELD("ext-name");
        pack::ObjectList<Item> items   = FIELD("items");
        pack::StringMap        attrs   = FIELD("attrs");

        using pack::Node::Node;
        META(Item, type, name, extName, items, attrs);
    };

    struct Link : public pack::Node
    {
        pack::String dest = FIELD("dest");
        pack::String src  = FIELD("src");
        pack::String type = FIELD("type");

        using pack::Node::Node;
        META(Link, dest, src, type);
    };

    pack::ObjectList<Item> items = FIELD("items");
    pack::ObjectList<Link> links = FIELD("links");

    using pack::Node::Node;
    META(DBData, items, links);
};

std::istream& operator>>(std::istream& ss, DBData::Types& value)
{
    std::string strval;
    ss >> strval;
    if (strval == "Datacenter") {
        value = DBData::Types::Datacenter;
    } else if (strval == "Server") {
        value = DBData::Types::Server;
    } else if (strval == "Connector") {
        value = DBData::Types::Connector;
    } else if (strval == "InfraService") {
        value = DBData::Types::InfraService;
    } else if (strval == "Hypervisor") {
        value = DBData::Types::Hypervisor;
    } else if (strval == "VirtualMachine") {
        value = DBData::Types::VirtualMachine;
    } else if (strval == "Feed") {
        value = DBData::Types::Feed;
    } else if (strval == "Rack") {
        value = DBData::Types::Rack;
    } else if (strval == "Ups") {
        value = DBData::Types::Ups;
    } else if (strval == "Row") {
        value = DBData::Types::Row;
    } else if (strval == "Room") {
        value = DBData::Types::Room;
    } else {
        value = DBData::Types::Unknown;
    }
    return ss;
}

std::ostream& operator<<(std::ostream& ss, DBData::Types /*value*/)
{
    return ss;
}


// =====================================================================================================================

static void createItem(fty::db::Connection& conn, const DBData::Item& item, std::vector<uint32_t>& ids,
    std::map<std::string, uint32_t>& mapping, std::uint32_t parentId = 0)
{
    auto type = [&]() {
        switch (item.type) {
            case DBData::Types::Datacenter:
                return persist::DATACENTER;
            case DBData::Types::Server:
                return persist::DEVICE;
            case DBData::Types::Connector:
                return persist::CONNECTOR;
            case DBData::Types::InfraService:
                return persist::INFRA_SERVICE;
            case DBData::Types::Hypervisor:
                return persist::HYPERVISOR;
            case DBData::Types::VirtualMachine:
                return persist::VIRTUAL_MACHINE;
            case DBData::Types::Feed:
                return persist::DEVICE;
            case DBData::Types::Rack:
                return persist::RACK;
            case DBData::Types::Row:
                return persist::ROW;
            case DBData::Types::Room:
                return persist::ROOM;
            case DBData::Types::Ups:
                return persist::DEVICE;
            case DBData::Types::Unknown:
                return persist::TUNKNOWN;
        }
        return persist::TUNKNOWN;
    };

    auto subType = [&]() {
        switch (item.type) {
            case DBData::Types::Datacenter:
                return persist::SUNKNOWN;
            case DBData::Types::Server:
                return persist::SERVER;
            case DBData::Types::Connector:
                return persist::VMWARE_VCENTER_CONNECTOR;
            case DBData::Types::InfraService:
                return persist::VMWARE_VCENTER;
            case DBData::Types::Hypervisor:
                return persist::VMWARE_ESXI;
            case DBData::Types::VirtualMachine:
                return persist::VMWARE_VM;
            case DBData::Types::Feed:
                return persist::FEED;
            case DBData::Types::Rack:
                return persist::SUNKNOWN;
            case DBData::Types::Row:
                return persist::SUNKNOWN;
            case DBData::Types::Room:
                return persist::SUNKNOWN;
            case DBData::Types::Ups:
                return persist::UPS;
            case DBData::Types::Unknown:
                return persist::SUNKNOWN;
        }
        return persist::SUNKNOWN;
    };

    fty::asset::db::AssetElement el;
    el.name      = item.name;
    el.status    = "active";
    el.priority  = 1;
    el.parentId  = parentId;
    el.typeId    = type();
    el.subtypeId = subType();

    uint32_t id;
    if (auto ret = fty::asset::db::insertIntoAssetElement(conn, el, true); !ret) {
        throw std::runtime_error(ret.error());
    } else {
        id = *ret;
    }

    if (item.extName.hasValue()) {
        if (auto ret = fty::asset::db::insertIntoAssetExtAttributes(conn, id, {{"name", item.extName}}, true); !ret) {
            throw std::runtime_error(ret.error());
        }
    }

    for (const auto& attr : item.attrs) {
        if (auto ret = fty::asset::db::insertIntoAssetExtAttributes(conn, id, {{attr.first, attr.second}}, true);
            !ret) {
            throw std::runtime_error(ret.error());
        }
    }

    for (const auto& ch : item.items) {
        createItem(conn, ch, ids, mapping, id);
    }

    ids.push_back(id);
    mapping.emplace(item.name, id);
}

// =====================================================================================================================

uint16_t linkId(const std::string& link)
{
    static std::map<std::string, uint16_t> lnks = []() {
        fty::db::Connection             conn;
        std::map<std::string, uint16_t> ret;

        for (const auto& row : conn.select("select * from t_bios_asset_link_type")) {
            ret.emplace(row.get("name"), row.get<uint16_t>("id_asset_link_type"));
        }

        return ret;
    }();

    if (lnks.count(link)) {
        return lnks[link];
    }
    return 0;
}

// =====================================================================================================================

SampleDb::SampleDb(const std::string& data)
{
    TestDb::init();

    DBData sample;
    if (auto ret = pack::yaml::deserialize(data, sample); !ret) {
        throw std::runtime_error(ret.error());
    }

    fty::db::Connection conn;

    for (const auto& item : sample.items) {
        createItem(conn, item, m_ids, m_mapping);
    }

    for (const auto& link : sample.links) {
        asset::db::AssetLink lnk;
        lnk.dest = idByName(link.dest);
        lnk.src  = idByName(link.src);
        lnk.type = linkId(link.type);
        if (auto ret = asset::db::insertIntoAssetLink(conn, lnk)) {
            m_links.push_back(*ret);
        }
    }
}

SampleDb::~SampleDb()
{
    fty::db::Connection conn;

    for (int64_t id : m_links) {
        conn.execute("delete from t_bios_asset_link where id_link = :id", "id"_p = id);
    }

    for (uint32_t id : m_ids) {
        fty::asset::db::deleteAssetExtAttributesWithRo(conn, id, true);
        fty::asset::db::deleteAssetExtAttributesWithRo(conn, id, false);
        fty::asset::db::deleteAssetElement(conn, id);
    }
}

uint32_t SampleDb::idByName(const std::string& name)
{
    if (m_mapping.count(name)) {
        return m_mapping[name];
    }
    throw std::runtime_error("{} not in sample db"_format(name));
}

// =====================================================================================================================

} // namespace fty
