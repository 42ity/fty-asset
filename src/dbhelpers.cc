/*  =========================================================================
    dbhelpers - Helpers functions for database

    Copyright (C) 2014 - 2020 Eaton

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
    dbhelpers - Helpers functions for database
@discuss
@end
*/


#include "fty_asset_classes.h"
#include <cxxtools/jsonserializer.h>

#define INPUT_POWER_CHAIN     1
#define AGENT_ASSET_ACTIVATOR "etn-licensing-credits"

// for test purposes
std::map<std::string, std::string> test_map_asset_state;

/**
 *  \brief Wrapper for select_assets_by_container_cb
 *
 *  \param[in] container_name - iname of container
 *  \param[in] filter - types and subtypes we are interested in
 *  \param[out] assets - inames of assets in this container
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_assets_by_container(const std::string& container_name, const std::set<std::string>& filter,
    std::vector<std::string>& assets, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int rv = DBAssets::select_assets_by_container_name_filter(conn, container_name, filter, assets);
    return rv;
}

/**
 *  \brief Selects basic asset info
 *
 *  \param[in] element_id - iname of asset
 *  \param[in] cb - function to call on each row
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_asset_element_basic(
    const std::string& asset_name, std::function<void(const tntdb::Row&)> cb, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int               rv   = DBAssets::select_asset_element_basic_cb(conn, asset_name, cb);
    return rv;
}

/**
 *  \brief Selects ext attributes for given asset
 *
 *  \param[in] element_id - iname of asset
 *  \param[in] cb - function to call on each row
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_ext_attributes(uint32_t asset_id, std::function<void(const tntdb::Row&)> cb, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int               rv   = DBAssets::select_ext_attributes_cb(conn, asset_id, cb);
    return rv;
}

/**
 *  \brief Selects all parents of given asset
 *
 *  \param[in] id - iname of asset
 *  \param[in] cb - function to call on each row
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_asset_element_super_parent(uint32_t id, std::function<void(const tntdb::Row&)>& cb, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int               rv   = DBAssets::select_asset_element_super_parent(conn, id, cb);
    return rv;
}

/**
 * Wrapper for select_assets_by_filter_cb
 *
 * \param[in] filter - types/subtypes we are interested in
 * \param[out] assets - inames of returned assets
 */
int select_assets_by_filter(const std::set<std::string>& filter, std::vector<std::string>& assets, bool test)
{
    if (test)
        return 0;

    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int               rv   = DBAssets::select_assets_by_filter(conn, filter, assets);
    return rv;
}

/**
 *  \brief Selects basic asset info for all assets in the DB
 *
 *  \param[in] cb - function to call on each row
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_assets(std::function<void(const tntdb::Row&)>& cb, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    int               rv   = DBAssets::select_assets_cb(conn, cb);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////

#define SQL_EXT_ATT_INVENTORY                                                                                \
    " INSERT INTO"                                                                                           \
    "   t_bios_asset_ext_attributes"                                                                         \
    "   (keytag, value, id_asset_element, read_only)"                                                        \
    " VALUES"                                                                                                \
    "  ( :keytag, :value, (SELECT id_asset_element FROM t_bios_asset_element WHERE name=:device_name), "     \
    ":readonly)"                                                                                             \
    " ON DUPLICATE KEY"                                                                                      \
    "   UPDATE "                                                                                             \
    "       value = VALUES (value),"                                                                         \
    "       read_only = :readonly,"                                                                          \
    "       id_asset_ext_attribute = LAST_INSERT_ID(id_asset_ext_attribute)"
/**
 *  \brief Inserts ext attributes from inventory message into DB
 *
 *  \param[in] device_name - iname of assets
 *  \param[in] ext_attributes - recent ext attributes for this asset
 *  \param[in] read_only - whether to insert ext attributes as readonly
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int process_insert_inventory(
    const std::string& device_name, zhash_t* ext_attributes, bool readonly, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached(DBConn::url);
    } catch (const std::exception& e) {
        log_error("DB: cannot connect, %s", e.what());
        return -1;
    }

    tntdb::Transaction trans(conn);
    tntdb::Statement   st = conn.prepareCached(SQL_EXT_ATT_INVENTORY);

    for (void* it = zhash_first(ext_attributes); it != NULL; it = zhash_next(ext_attributes)) {

        const char* value     = (const char*)it;
        const char* keytag    = (const char*)zhash_cursor(ext_attributes);
        bool        readonlyV = readonly;

        if (strcmp(keytag, "name") == 0 || strcmp(keytag, "description") == 0 ||
            strcmp(keytag, "ip.1") == 0) {
            readonlyV = false;
        }
        if (strcmp(keytag, "uuid") == 0) {
            readonlyV = true;
        }
        try {
            st.set("keytag", keytag)
                .set("value", value)
                .set("device_name", device_name)
                .set("readonly", readonlyV)
                .execute();
        } catch (const std::exception& e) {
            log_warning("%s:\texception on updating %s {%s, %s}\n\t%s", "", device_name.c_str(), keytag,
                value, e.what());
            continue;
        }
    }

    trans.commit();
    return 0;
}

/**
 *  \brief Inserts ext attributes from inventory message into DB only
 *         if the new value is different from the cache map
 *         This method is not thread safe.
 *
 *  \param[in] device_name - iname of assets
 *  \param[in] ext_attributes - recent ext attributes for this asset
 *  \param[in] read_only - whether to insert ext attributes as readonly
 *  \param[in] test - unit tests indicator
 *  \param[in] map_cache -  cache map
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int process_insert_inventory(const std::string& device_name, zhash_t* ext_attributes, bool readonly,
    std::unordered_map<std::string, std::string>& map_cache, bool test)
{
    if (test)
        return 0;
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached(DBConn::url);
    } catch (const std::exception& e) {
        log_error("DB: cannot connect, %s", e.what());
        return -1;
    }

    tntdb::Transaction trans(conn);
    tntdb::Statement   st = conn.prepareCached(SQL_EXT_ATT_INVENTORY);

    for (void* it = zhash_first(ext_attributes); it != NULL; it = zhash_next(ext_attributes)) {
        const char* value     = (const char*)it;
        const char* keytag    = (const char*)zhash_cursor(ext_attributes);
        bool        readonlyV = readonly;
        if (strcmp(keytag, "name") == 0 || strcmp(keytag, "description") == 0)
            readonlyV = false;

        std::string cache_key = device_name;
        cache_key.append(":").append(keytag).append(readonlyV ? "1" : "0");
        auto el = map_cache.find(cache_key);
        if (el != map_cache.end() && el->second == value)
            continue;
        try {
            st.set("keytag", keytag)
                .set("value", value)
                .set("device_name", device_name)
                .set("readonly", readonlyV)
                .execute();
            map_cache[cache_key] = value;
        } catch (const std::exception& e) {
            log_warning("%s:\texception on updating %s {%s, %s}\n\t%s", "", device_name.c_str(), keytag,
                value, e.what());
            continue;
        }
    }

    trans.commit();
    return 0;
}
/**
 *  \brief Selects user-friendly name for given asset name
 *
 *  \param[in] iname - asset iname
 *  \param[out] ename - user-friendly asset name
 *  \param[in] test - unit tests indicator
 *
 *  \return  0 - in case of success
 *          -1 - in case of some unexpected error
 */
int select_ename_from_iname(std::string& iname, std::string& ename, bool test)
{
    if (test) {
        if (iname == TEST_INAME) {
            ename = TEST_ENAME;
            return 0;
        } else
            return -1;
    }
    try {

        tntdb::Connection conn = tntdb::connectCached(DBConn::url);
        tntdb::Statement  st   = conn.prepareCached(
            "SELECT e.value FROM  t_bios_asset_ext_attributes AS e "
            "INNER JOIN t_bios_asset_element AS a "
            "ON a.id_asset_element = e.id_asset_element "
            "WHERE keytag = 'name' and a.name = :iname; "

        );

        tntdb::Row row = st.set("iname", iname).selectRow();
        log_debug("[s_handle_subject_ename_from_iname]: were selected %" PRIu32 " rows", 1);

        row[0].get(ename);
    } catch (const std::exception& e) {
        log_error("exception caught %s for element '%s'", e.what(), ename.c_str());
        return -1;
    }

    return 0;
}


/**
 *  \brief Get number of active power devices
 *
 *  \return  X - number of active power devices
 */

int get_active_power_devices(bool test)
{
    int count = 0;
    if (test) {
        log_debug("[get_active_power_devices]: runs in test mode");
        for (auto const& as : test_map_asset_state) {
            if ("active" == as.second) {
                ++count;
            }
        }
        return count;
    }
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    return DBAssets::get_active_power_devices(conn);
}

bool should_activate(std::string operation, std::string current_status, std::string new_status)
{
    bool rv = (((operation == FTY_PROTO_ASSET_OP_CREATE) || (operation == "create-force")) &&
               (new_status == "active"));
    rv |= (operation == FTY_PROTO_ASSET_OP_UPDATE && current_status == "nonactive" && new_status == "active");

    return rv;
}

bool should_deactivate(std::string operation, std::string current_status, std::string new_status)
{
    bool rv =
        (operation == FTY_PROTO_ASSET_OP_UPDATE && current_status == "active" && new_status == "nonactive");
    rv |= (operation == FTY_PROTO_ASSET_OP_DELETE);

    return rv;
}

//////////////////////////////////////////////////////////////////////////////////

long insertAssetToDB(const fty::Asset& asset)
{
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    tntdb::Statement  statement;

    // if parent iname is empty, set parent id to NULL
    const char* query_parent_null =
        "INSERT INTO t_bios_asset_element "
        "(name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
        "VALUES ("
        ":name, "
        "(SELECT id_asset_element_type from t_bios_asset_element_type where name = :type), "
        "(SELECT id_asset_device_type from t_bios_asset_device_type where name = :subtype), "
        "NULL, "
        ":status, "
        ":priority, "
        ":asset_tag)";

    const char* query_with_parent =
        "INSERT INTO t_bios_asset_element "
        "(name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
        "VALUES ("
        ":name, "
        "(SELECT id_asset_element_type from t_bios_asset_element_type where name = :type), "
        "(SELECT id_asset_device_type from t_bios_asset_device_type where name = :subtype), "
        "(SELECT id_asset_element from (SELECT * FROM t_bios_asset_element) AS e where e.name = :parent), "
        ":status, "
        ":priority, "
        ":asset_tag)";

    if (asset.getParentIname().empty()) {
        statement = conn.prepareCached(query_parent_null);
    } else {
        statement = conn.prepareCached(query_with_parent);
        statement.set("parent", asset.getParentIname());
    }
    statement.set("name", asset.getInternalName());
    statement.set("type", asset.getAssetType());
    statement.set("subtype", asset.getAssetSubtype());
    statement.set("status", fty::assetStatusToString(asset.getAssetStatus()));
    statement.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? statement.setNull("asset_tag")
                                : statement.set("asset_tag", asset.getAssetTag());

    statement.execute();

    // TODO get id of last inserted element (race condition may occur, use a select statement instead)
    long assetIndex = conn.lastInsertId();
    log_debug("[dbhelpers]: inserted asset with ID %ld", assetIndex);

    return assetIndex;
}

long updateAssetToDB(const fty::Asset& asset)
{
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    tntdb::Statement  statement;

    // if parent iname is empty, set parent id to NULL
    const char* query_parent_null =
        "UPDATE t_bios_asset_element SET "
        "id_type = (SELECT id_asset_element_type from t_bios_asset_element_type where name = :type), "
        "id_subtype = (SELECT id_asset_device_type from t_bios_asset_device_type where name = :subtype), "
        "id_parent = NULL, "
        "status = :status, "
        "priority = :priority, "
        "asset_tag = :asset_tag "
        "WHERE name = :name";

    const char* query_with_parent =
        "UPDATE t_bios_asset_element SET "
        "id_type = (SELECT id_asset_element_type from t_bios_asset_element_type where name = :type), "
        "id_subtype = (SELECT id_asset_device_type from t_bios_asset_device_type where name = :subtype), "
        "id_parent = (SELECT id_asset_element from (SELECT * FROM t_bios_asset_element) AS e where e.name = "
        ":parent), "
        "status = :status, "
        "priority = :priority, "
        "asset_tag = :asset_tag "
        "WHERE name = :name";

    if (asset.getParentIname().empty()) {
        statement = conn.prepareCached(query_parent_null);
    } else {
        statement = conn.prepareCached(query_with_parent);
        statement.set("parent", asset.getParentIname());
    }
    statement.set("name", asset.getInternalName());
    statement.set("type", asset.getAssetType());
    statement.set("subtype", asset.getAssetSubtype());
    statement.set("status", fty::assetStatusToString(asset.getAssetStatus()));
    statement.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? statement.setNull("asset_tag")
                                : statement.set("asset_tag", asset.getAssetTag());

    statement.execute();

    long assetIndex = selectAssetProperty<long>("id_asset_element", "name", asset.getInternalName());
    log_debug("[dbhelpers]: updated asset with ID %ld", assetIndex);

    return assetIndex;
}

void updateAssetExtProperties(const fty::Asset& asset)
{
    long assetIndex = selectAssetProperty<long>("id_asset_element", "name", asset.getInternalName());

    // WARNING the update query works until (keytag, id_asset_element) are declared as UNIQUE INDEX
    constexpr const char* insertExtProperties =
        " INSERT INTO t_bios_asset_ext_attributes "
        " (keytag, value, id_asset_element, read_only) "
        " VALUES "
        " ( :keytag, :value, :asset_id, :readonly )"
        " ON DUPLICATE KEY"
        " UPDATE "
        " value = VALUES (value),"
        " read_only = :readonly,"
        " id_asset_ext_attribute = LAST_INSERT_ID(id_asset_ext_attribute)";

    tntdb::Connection conn;
    conn = tntdb::connectCached(DBConn::url);

    tntdb::Transaction trans(conn);

    tntdb::Statement st = conn.prepareCached(insertExtProperties);

    for (const auto& property : asset.getExt()) {
        bool readOnly = (property.first == "uuid" ? true : property.second.second);

        st.set("keytag", property.first)
            .set("value", property.second.first)
            .set("asset_id", assetIndex)
            .set("readonly", readOnly)
            .execute();
    }

    trans.commit();
}

static fty::Asset readAssetFromRow(tntdb::Connection& conn, const tntdb::Row& row)
{
    tntdb::Statement statement;

    fty::Asset asset;

    int         assetId;
    std::string assetName;
    std::string assetType;
    std::string assetSubtype;
    int         parentId;
    bool        parentIdNotNull;
    std::string parentIname;
    std::string assetStatus;
    int         assetPriority;
    std::string assetTag;

    row.reader()
        .get(assetId)
        .get(assetName)
        .get(assetType)
        .get(assetSubtype)
        .get(parentId, parentIdNotNull)
        .get(assetStatus)
        .get(assetPriority)
        .get(assetTag);

    if (parentIdNotNull) {
        parentIname = selectAssetProperty<std::string>("name", "id_asset_element", parentId);
    }

    asset.setId(assetId);
    asset.setInternalName(assetName);
    asset.setAssetType(assetType);
    asset.setAssetSubtype(assetSubtype);
    asset.setParentIname(parentIname);
    asset.setAssetStatus(fty::stringToAssetStatus(assetStatus));
    asset.setPriority(assetPriority);
    asset.setAssetTag(assetTag);

    constexpr const char* selectExtProperties =
        " SELECT ext.keytag, ext.value, ext.read_only "
        " FROM t_bios_asset_ext_attributes as ext"
        " WHERE ext.id_asset_element = :asset_id ";

    statement = conn.prepareCached(selectExtProperties);

    tntdb::Result result = statement.set("asset_id", assetId).select();

    for (const auto& row : result) {
        std::string keytag;
        std::string value;
        bool        readOnly = false;

        row.reader().get(keytag).get(value).get(readOnly);

        asset.setExtEntry(keytag, value, readOnly);
    }

    // Linked assets
    {
        auto sql = conn.prepareCached(
            " SELECT l.id_asset_element_dest, e.name "
            " FROM v_bios_asset_link as l"
            "   INNER JOIN t_bios_asset_element as e ON l.id_asset_element_dest = e.id_asset_element"
            " WHERE l.id_asset_element_src = :asset_id ");
        tntdb::Result res = sql.set("asset_id", assetId).select();

        std::vector<std::string> links;
        for (const auto& row : result) {
            links.push_back(row.getValue("name").getString());
        }

        asset.setLinkedAssets(links);
    }

    return asset;
}

fty::Asset getAssetFromDB(const std::string& assetInternalName)
{
    tntdb::Connection conn;
    conn = tntdb::connectCached(DBConn::url);

    constexpr const char* selectAsset =
        " SELECT a.id_asset_element, a.name, e.name, d.name, a.id_parent, a.status, a.priority, a.asset_tag "
        " FROM t_bios_asset_element as a INNER JOIN t_bios_asset_device_type as d INNER JOIN "
        "t_bios_asset_element_type as e"
        " ON a.id_type = e.id_asset_element_type AND a.id_subtype = d.id_asset_device_type "
        " WHERE a.name = :asset_name ";

    tntdb::Statement st = conn.prepareCached(selectAsset);

    tntdb::Row row = st.set("asset_name", assetInternalName).selectRow();

    fty::Asset asset = readAssetFromRow(conn, row);

    return asset;
}

std::vector<fty::Asset> getAllAssetsFromDB()
{
    constexpr const char* selectAll =
        " SELECT a.id_asset_element, a.name, e.name, d.name, a.id_parent, a.status, a.priority, a.asset_tag "
        " FROM t_bios_asset_element as a INNER JOIN t_bios_asset_device_type as d INNER JOIN "
        "t_bios_asset_element_type as e"
        " ON a.id_type = e.id_asset_element_type AND a.id_subtype = d.id_asset_device_type ";

    std::vector<fty::Asset> assetVector;

    tntdb::Connection conn = tntdb::connectCached(DBConn::url);

    tntdb::Statement statement = conn.prepareCached(selectAll);
    tntdb::Result    result    = statement.select();
    for (const auto& row : result) {
        fty::Asset asset = readAssetFromRow(conn, row);
        assetVector.push_back(asset);
    }

    return assetVector;
}
