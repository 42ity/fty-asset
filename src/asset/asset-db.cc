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
#include <sstream>
#include <tntdb.h>

namespace fty {

// static helpers

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
        children.push_back(row.getString("name"));
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
            e.name                  AS name,
            l.src_out               AS srcOut,
            l.dest_in               AS destIn,
            l.id_asset_link_type    AS linkType
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

    std::vector<AssetLink> links;
    for (const auto& row : result) {
        std::string srcOut, destIn;
        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", srcOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", destIn);
        }

        links.push_back(AssetLink(row.getString("name"), srcOut, destIn, row.getInt("linkType")));
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

void DB::link(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType)
{
    assert(src.getId());
    assert(dest.getId());

    m_conn_lock.lock();
    // clang-format off
    auto res = m_conn.prepareCached(R"(
        SELECT
            e.id_asset_element   AS srcId,
            e.name               AS srcName,
            l.src_out            AS srcOut,
            l.dest_in            AS destIn,
            l.id_asset_link_type AS linkType
        FROM
            t_bios_asset_element AS e
        INNER JOIN
            t_bios_asset_link AS l
        ON
            e.id_asset_element = l.id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )")
    .set("assetId", dest.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    std::vector<AssetLink> existing;
    for (const auto& row : res) {
        std::string tmpOut, tmpIn;

        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", tmpOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", tmpIn);
        }

        existing.push_back(AssetLink(row.getString("srcName"), tmpOut, tmpIn, row.getInt("linkType")));
    }

    // new link to insert
    const AssetLink l(src.getInternalName(), srcOut, destIn, linkType);

    auto found = std::find(existing.begin(), existing.end(), l);

    if (found != existing.end()) {
        throw std::runtime_error("Link to asset " + src.getInternalName() + " already exists");
    }

    // clang-format off
    auto q = m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_link
            (id_asset_device_src, src_out, id_asset_device_dest, dest_in, id_asset_link_type)
        VALUES (
            :src,
            :srcOut,
            :dest,
            :destIn,
            :linkType
        )
    )");

    // clang-format on

    q.set("src", src.getId());
    q.set("dest", dest.getId());

    srcOut.empty() ? q.setNull("srcOut") : q.set("srcOut", srcOut);
    destIn.empty() ? q.setNull("destIn") : q.set("destIn", destIn);

    q.set("linkType", linkType);

    m_conn_lock.lock();
    q.execute();
    m_conn_lock.unlock();
}

void DB::unlink(Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType)
{
    assert(src.getId());
    assert(dest.getId());

    tntdb::Statement q;

    std::stringstream qs;

    // clang-format off
    qs << 
        " DELETE FROM "                     \
        "    t_bios_asset_link"             \
        " WHERE"                            \
        "    id_asset_device_src = :src"    \
        "    AND"                           \
        "    id_asset_device_dest = :dest";
    if(!srcOut.empty()) {
        qs <<
        "    AND"                   \
        "    src_out = :srcOut";
    }
    if(!destIn.empty()) {
        qs <<
        "    AND"                   \
        "    dest_in = :destIn";
    }
    qs <<
        "    AND" \
        "    id_asset_link_type = :linkType";
    qs << ")";
    // clang-format on

    q = m_conn.prepareCached(qs.str().c_str());

    q.set("src", src.getId());
    q.set("dest", dest.getId());

    if (!srcOut.empty())
        q.set("srcOut", srcOut);
    if (!destIn.empty())
        q.set("destIn", destIn);

    q.set("linkType", linkType);

    m_conn_lock.lock();
    q.execute();
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
    // clang-format off
    auto q = m_conn.prepareCached(R"(
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
    )");
    // clang-format on

    q.set("type", asset.getAssetType());
    q.set("subtype", asset.getAssetSubtype());
    // name field can't be null, parent id is set to NULL if parentIname is empty
    asset.getParentIname().empty() ? q.setNull("parent") : q.set("parent", asset.getParentIname());
    q.set("status", assetStatusToString(asset.getAssetStatus()));
    q.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? q.setNull("assetTag") : q.set("assetTag", asset.getAssetTag());

    m_conn_lock.lock();
    q.execute();
    m_conn_lock.unlock();
}

void DB::insert(Asset& asset)
{
    // clang-format off
    auto q = m_conn.prepareCached(R"(
        INSERT INTO
            t_bios_asset_element
            (name, id_type, id_subtype, id_parent, status, priority, asset_tag)
        VALUES (
            :name,
            (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = :type),
            (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = :subtype),
            (SELECT p.id_asset_element FROM t_bios_asset_element AS p WHERE p.name = :parent),
            :status,
            :priority,
            :asset_tag
        )
    )");
    // clang-format on
    q.set("name", asset.getInternalName());
    q.set("type", asset.getAssetType());
    q.set("subtype", asset.getAssetSubtype());
    // name field can't be null, parent id is set to NULL if parentIname is empty
    asset.getParentIname().empty() ? q.setNull("parent") : q.set("parent", asset.getParentIname());
    // always insert as non active, update after activation
    q.set("status", assetStatusToString(fty::AssetStatus::Nonactive));
    q.set("priority", asset.getPriority());
    asset.getAssetTag().empty() ? q.setNull("assetTag") : q.set("assetTag", asset.getAssetTag());

    m_conn_lock.lock();
    q.execute();
    m_conn_lock.unlock();

    // update ID after insertion
    asset.setId(m_conn.lastInsertId());
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
            e.id_asset_element   AS srcId,
            e.name               AS srcName,
            l.src_out            AS srcOut,
            l.dest_in            AS destIn,
            l.id_asset_link_type AS linkType
        FROM t_bios_asset_link   AS l
        INNER JOIN
            t_bios_asset_element AS e
            ON e.id_asset_element = l.id_asset_device_src
        WHERE
             id_asset_device_dest = :assetId
    )")
    .set("assetId", asset.getId())
    .select();
    // clang-format on
    m_conn_lock.unlock();

    using Existing = std::pair<uint32_t, AssetLink>;

    std::vector<Existing> existing;
    for (const auto& row : res) {
        std::string tmpOut, tmpIn;

        // may be NULL
        if (!row.isNull("srcOut")) {
            row.getString("srcOut", tmpOut);
        }
        if (!row.isNull("destIn")) {
            row.getString("destIn", tmpIn);
        }

        existing.emplace_back(row.getUnsigned32("srcId"),
            AssetLink(row.getString("srcName"), tmpOut, tmpIn, row.getInt("linkType")));
    }

    // add new links
    for (const AssetLink l : asset.getLinkedAssets()) {

        try {
            AssetImpl src(l.sourceId);
            link(src, l.srcOut, asset, l.destIn, l.linkType);
        } catch (std::runtime_error) {
            auto found = std::find_if(existing.begin(), existing.end(), [&](const Existing& e) {
                return e.second == l;
            });

            existing.erase(found);
        }
    }

    // remove links not present in DTO
    for (const auto& toRem : existing) {
        const AssetLink& l = toRem.second;

        AssetImpl src(l.sourceId);
        unlink(src, l.srcOut, asset, l.destIn, l.linkType);
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
