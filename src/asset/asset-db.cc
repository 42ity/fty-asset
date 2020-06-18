/*  =========================================================================
    asset_db - asset_db

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

#include "asset-db.h"
#include "asset.h"
#include <cstdlib>
#include <fty_common_db_dbpath.h>
#include <tntdb.h>

namespace fty {

// static helpers

// generate asset name
static std::string createAssetName(const std::string& type, const std::string& subtype, long index)
{
    std::string assetName;

    if (type == fty::TYPE_DEVICE) {
        assetName = subtype + "-" + std::to_string(index);
    } else {
        assetName = type + "-" + std::to_string(index);
    }

    return assetName;
}

// DB
DB::DB(bool test)
{
    if (!test) {
        m_conn = tntdb::connectCached(DBConn::url);
    }
}

void DB::loadAsset(const std::string& nameId, Asset& asset)
{
    // clang-format off
    m_conn_lock.lock();
    tntdb::Row row = m_conn.prepareCached(R"(
        SELECT
            a.id_asset_element AS id,
            a.name             AS name,
            e.name             AS type,
            d.name             AS subType,
            p.name             AS parentName,
            a.status           AS status,
            a.priority         AS priority,
            a.asset_tag        AS tag
        FROM t_bios_asset_element AS a
            INNER JOIN t_bios_asset_device_type AS d
            INNER JOIN t_bios_asset_element_type AS e
            ON a.id_type = e.id_asset_element_type AND a.id_subtype = d.id_asset_device_type
            LEFT JOIN t_bios_asset_element AS p
            ON a.id_parent = p.id_asset_element
        WHERE a.name = :asset_name
    )")
    .set("asset_name", nameId)
    .selectRow();
    // clang-format on
    m_conn_lock.unlock();

    asset.setId(row.getInt("id"));
    asset.setInternalName(row.getString("name"));
    asset.setAssetType(row.getString("type"));
    asset.setAssetSubtype(row.getString("subType"));
    if (!row.isNull("parentName")) {
        asset.setParentIname(row.getString("parentName"));
    }
    asset.setAssetStatus(stringToAssetStatus(row.getString("status")));
    asset.setPriority(row.getInt("priority"));
    if (!row.isNull("tag")) {
        asset.setAssetTag(row.getString("tag"));
    }
}

void DB::loadExtMap(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn_lock.lock();
    auto ext = m_conn.prepareCached(R"(
        SELECT
            keytag,
            value,
            read_only
        FROM
            t_bios_asset_ext_attributes
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    for (const auto& row : ext) {
        asset.setExtEntry(row.getString("keytag"), row.getString("value"), row.getBool("read_only"));
    }
}

std::vector<std::string> DB::getChildren(const Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    auto result = m_conn.prepareCached(R"(
        SELECT
            name
        FROM
            t_bios_asset_element
        WHERE
            id_parent = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    std::vector<std::string> children;
    for (const auto& row : result) {
        children.push_back(row.getValue("name").getString());
    }

    return children;
}

void DB::loadLinkedAssets(Asset& asset)
{
    assert(asset.getId());

    // clang-format off
    m_conn_lock.lock();
    auto result = m_conn.prepareCached(R"(
        SELECT
            l.id_asset_element_src  AS id,
            e.name                  AS name
        FROM
            v_bios_asset_link AS l
        INNER JOIN
            t_bios_asset_element AS e ON l.id_asset_element_src = e.id_asset_element
        WHERE
            l.id_asset_element_dest = :asset_id
    )")
    .set("asset_id", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    std::vector<std::string> links;
    for (const auto& row : result) {
        links.push_back(row.getString("name"));
    }
    asset.setLinkedAssets(links);
}


bool DB::isLastDataCenter(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    int numDatacentersAfterDelete = m_conn.prepare(R"(
        SELECT
            COUNT(id_asset_element)
        FROM
            t_bios_asset_element
        WHERE
            id_type = (
                SELECT id_asset_element_type
                FROM   t_bios_asset_element_type
                WHERE  name = 'datacenter'
            )
            AND id_asset_element != :asset_id
    )")
    .set("asset_id", asset.getId())
    .selectValue()
    .getInt();
    // clang-format on
    m_conn_lock.unlock();

    return numDatacentersAfterDelete == 0;
}

void DB::removeFromGroups(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::removeFromRelations(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_monitor_asset_relation
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::removeAsset(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_element
        WHERE
            id_asset_element = :asset_id
    )")
    .set("asset_id", asset.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::removeExtMap(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_ext_attributes
        WHERE
            id_asset_element = :assetId
    )")
    .set("assetId", asset.getId())
    .execute();
    m_conn_lock.unlock();
    // clang-format on
}

void DB::clearGroup(Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_group_relation
        WHERE
            d_asset_group = :grp
    )")
    .set("grp", asset.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

bool DB::hasLinkedAssets(const Asset& asset)
{
    assert(asset.getId());

    m_conn_lock.lock();
    // clang-format off
    int linkedAssets = m_conn.prepare(R"(
        SELECT
            COUNT(id_link)
        FROM
            t_bios_asset_link
        WHERE
            id_asset_device_src = :src
    )")
    .set("src", asset.getId())
    .selectValue()
    .getInt();
    // clang-format on
    m_conn_lock.unlock();

    return linkedAssets != 0;
}

void DB::link(Asset& src, Asset& dest)
{
    assert(src.getId());
    assert(dest.getId());

    m_conn_lock.lock();
    // clang-format off
    auto res = m_conn.prepareCached(R"(
        SELECT
            e.id_asset_element AS srcId,
            e.name             AS srcName
        FROM t_bios_asset_link
        INNER JOIN
            t_bios_asset_element AS e
            ON e.id_asset_element = id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )")
    .set("assetId", dest.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    std::vector<std::string> existing;
    for (const auto& row : res) {
        existing.emplace_back(row.getString("srcName"));
    }

    const std::string& srcName = src.getInternalName();

    auto found = std::find(existing.begin(), existing.end(), srcName);

    if (found != existing.end()) {
        throw std::runtime_error("Link to asset " + src.getInternalName() + " already exists");
    }

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_link
            (id_asset_device_src, id_asset_device_dest, id_asset_link_type)
        VALUES (
            :src,
            :dest,
            1
        )
    )")
    .set("src", src.getId())
    .set("dest", dest.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::unlink(Asset& src, Asset& dest)
{
    assert(src.getId());
    assert(dest.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_link
        WHERE
            id_asset_device_src = :src
            AND
            id_asset_device_dest = :dest
    )")
    .set("src", src.getId())
    .set("dest", dest.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::unlinkAll(Asset& dest)
{
    assert(dest.getId());

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        DELETE FROM
            t_bios_asset_link
        WHERE
            id_asset_device_dest = :dest
    )")
    .set("dest", dest.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();
}

void DB::beginTransaction()
{
    m_conn_lock.lock();
    m_conn.beginTransaction();
    m_conn_lock.unlock();
}

void DB::rollbackTransaction()
{
    m_conn_lock.lock();
    m_conn.rollbackTransaction();
    m_conn_lock.unlock();
}

void DB::commitTransaction()
{
    m_conn_lock.lock();
    m_conn.commitTransaction();
    m_conn_lock.unlock();
}

void DB::update(Asset& asset)
{
    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        UPDATE
            t_bios_asset_element
        SET
           id_type = (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
           id_subtype = (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
           id_parent = (SELECT id_asset_element from (SELECT * FROM t_bios_asset_element) AS e where e.name = :parent),
            status = :status,
            priority = :priority,
            asset_tag = :assetTag
        WHERE
            id_asset_element = :assetId
    )")
   .set("type", asset.getAssetType())
   .set("subtype", asset.getAssetSubtype())
   .set("parent", asset.getParentIname())
   .set("status", assetStatusToString(asset.getAssetStatus()))
   .set("priority", asset.getPriority())
   .set("assetTag", asset.getAssetTag())
   .set("assetId", asset.getId())
   .execute();
   m_conn_lock.unlock();
    // clang-format on
}

void DB::insert(Asset& asset)
{
    // temporary asset name
    std::string assetName =
        (asset.getAssetType() == TYPE_DEVICE ? asset.getAssetSubtype() : asset.getAssetType()) + "-@@-" +
        std::to_string(rand());

    asset.setInternalName(assetName);

    m_conn_lock.lock();
    // clang-format off
    m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_element
            (name, id_type, id_subtype, id_parent, status, priority, asset_tag)
        VALUES (
            CONCAT(:name, "-", COALESCE((SELECT MAX(e.id_asset_element)+1 FROM t_bios_asset_element as e), '1')),
            (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
            (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
            (SELECT p.id_asset_element FROM t_bios_asset_element AS p WHERE p.name = :parent),
            :status,
            :priority,
            :asset_tag
        )
    )")
    .set("name", asset.getInternalName())
    .set("type", asset.getAssetType())
    .set("subtype", asset.getAssetSubtype())
    .set("parent", asset.getParentIname())
    .set("status", assetStatusToString(fty::AssetStatus::Nonactive))    // always insert as non active, update after activation
    .set("priority", asset.getPriority())
    .set("assetTag", asset.getAssetTag())
    .set("assetId", asset.getId())
    .execute();
    // clang-format on
    m_conn_lock.unlock();

    asset.setId(m_conn.lastInsertId());

    // generate new asset name
    asset.setInternalName(createAssetName(asset.getAssetType(), asset.getAssetSubtype(), asset.getId()));

    m_conn_lock.lock();
    // clang-format off
    int affected = m_conn.prepareCached(R"(
        UPDATE
            t_bios_asset_element
        SET
            name = :name
        WHERE
            id_asset_element = :assetId
    )")
   .set("name", asset.getInternalName())
   .set("assetId", asset.getId())
   .execute();
    // clang-format on
    m_conn_lock.unlock();

    if (affected == 0) {
        throw std::runtime_error(TRANSLATE_ME("Asset name update failed"));
    }
}

std::string DB::inameById(uint32_t id)
{
    m_conn_lock.lock();
    // clang-format off
    std::string result = m_conn.prepareCached(R"(
        SELECT name FROM t_bios_asset_element WHERE id_asset_element = :assetId
    )")
    .set("assetId", id)
    .selectRow()
    .getString("name");
    // clang-format on
    m_conn_lock.unlock();

    return result;
}

std::string DB::inameByUuid(const std::string& uuid)
{
    m_conn_lock.lock();
    // clang-format off
    std::string result = m_conn.prepareCached(R"(
        SELECT
            name
        FROM
            t_bios_asset_element
        WHERE
            id_asset_element = (
                SELECT
                    id_asset_element
                FROM
                    t_bios_asset_ext_attributes
                WHERE
                    keytag = "uuid" AND value = :uuid
            )
        )")
    .set("uuid", uuid)
    .selectRow()
    .getString("name");
    // clang-format on
    m_conn_lock.unlock();

    return result;
}

void DB::saveLinkedAssets(Asset& asset)
{
    m_conn_lock.lock();
    // clang-format off
    auto res = m_conn.prepareCached(R"(
        SELECT
            e.id_asset_element AS srcId,
            e.name             AS srcName
        FROM t_bios_asset_link
        INNER JOIN
            t_bios_asset_element AS e
            ON e.id_asset_element = id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )")
    .set("assetId", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    using Existing = std::pair<uint32_t, std::string>;

    std::vector<Existing> existing;
    for (const auto& row : res) {
        existing.emplace_back(row.getUnsigned32("srcId"), row.getString("srcName"));
    }

    for (const std::string& src : asset.getLinkedAssets()) {
        auto found = std::find_if(existing.begin(), existing.end(), [&](const Existing& e) {
            return e.second == src;
        });

        if (found == existing.end()) {
            // clang-format off
            m_conn_lock.lock();
            m_conn.prepareCached(R"(
                INSERT INTO t_bios_asset_link
                    (id_asset_device_src, id_asset_device_dest, id_asset_link_type)
                VALUES ((SELECT id_asset_element FROM t_bios_asset_element WHERE name = :src), :assetId, 1)
            )")
            .set("src", src)
            .set("assetId", asset.getId())
            .execute();
            // clang-format on
            m_conn_lock.unlock();
        } else {
            existing.erase(found);
        }
    }

    for (const auto& toRem : existing) {
        m_conn_lock.lock();
        // clang-format off
        m_conn.prepareCached(R"(
            DELETE FROM t_bios_asset_link
            WHERE id_asset_device_src := srcId
                AND id_asset_device_dest := assetId
        )")
        .set("srcId", toRem.first)
        .set("assetId", asset.getId())
        .execute();
        // clang-format on
        m_conn_lock.unlock();
    }
}

void DB::saveExtMap(Asset& asset)
{
    m_conn_lock.lock();
    // clang-format off
    auto res = m_conn.prepareCached(R"(
        SELECT
            id_asset_ext_attribute AS id,
            keytag                 AS akey,
            value                  AS avalue,
            read_only              AS readOnly
        FROM t_bios_asset_ext_attributes
        WHERE
             id_asset_element = : assetId
    )")
    .set("assetId", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    using Existing = std::tuple<uint32_t, std::string, std::string, bool>;

    std::vector<Existing> existing;
    for (const auto& row : res) {
        existing.emplace_back(
            row.getUnsigned32("id"), row.getString("akey"), row.getString("avalue"), row.getBool("readOnly"));
    }

    for (const auto& it : asset.getExt()) {
        auto found = std::find_if(existing.begin(), existing.end(), [&](const Existing& e) {
            return std::get<1>(e) == it.first;
        });

        if (found == existing.end()) {
            m_conn_lock.lock();
            // clang-format off
            m_conn.prepareCached(R"(
                INSERT INTO t_bios_asset_ext_attributes (keytag, value, id_asset_element, read_only)
                VALUES (:key, :value, :assetId, :readOnly)
            )")
            .set("key", it.first)
            .set("value", it.second.first)
            .set("readOnly", it.second.second)
            .set("assetId", asset.getId())
            .execute();
            // clang-format on
            m_conn_lock.unlock();
        } else {
            if (std::get<2>(*found) != it.second.first || std::get<3>(*found) != it.second.second) {
                m_conn_lock.lock();
                // clang-format off
                m_conn.prepareCached(R"(
                    UPDATE t_bios_asset_ext_attributes
                    SET
                        value = :value,
                        read_only = :readOnly
                    WHERE id_asset_ext_attribute = :extId
                )")
                .set("value", it.second.first)
                .set("readOnly", it.second.second)
                .set("extId", std::get<0>(*found))
                .execute();
                // clang-format on
                m_conn_lock.unlock();
            }
            existing.erase(found);
        }
    }

    for (const auto& toRem : existing) {
        m_conn_lock.lock();
        // clang-format off
        m_conn.prepareCached(R"(
            DELETE FROM t_bios_asset_ext_attributes
            WHERE id_asset_ext_attribute = :extId
        )")
        .set("extId", std::get<0>(toRem))
        .execute();
        // clang-format on
        m_conn_lock.unlock();
    }
}

std::vector<std::string> DB::listAllAssets()
{
    std::vector<std::string> assetList;

    m_conn_lock.lock();
    // clang-format off
    auto res = m_conn.prepareCached(R"(
        SELECT
            name          AS name
        FROM t_bios_asset_element
    )")
    .select();
    // clang-format on
    m_conn_lock.unlock();

    for (const auto& row : res) {
        const std::string& assetName = row.getString("name");
        // discard rackcontroller 0
        if (assetName != RC0) {
            assetList.emplace_back(assetName);
        }
    }

    return assetList;
}

} // namespace fty
