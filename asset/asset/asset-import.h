/*  ====================================================================================================================
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
    ====================================================================================================================
*/

#pragma once

#include "asset-db.h"
#include "error.h"
#include <fty_common_asset_types.h>
#include <map>
#include <set>

namespace tntdb {
class Connection;
}

namespace fty::asset {
class CsvMap;

namespace db {
    struct AssetLink;
}

class Import
{
public:
    using ImportResMap = std::map<size_t, Expected<db::AssetElement>>;

    Import(const CsvMap& cm);
    AssetExpected<void>      process(bool checkLic);
    const ImportResMap&      items() const;
    persist::asset_operation operation() const;

private:
    std::string                        mandatoryMissing() const;
    std::map<std::string, std::string> sanitizeRowExtNames(size_t row, bool sanitize) const;
    AssetExpected<db::AssetElement>    processRow(size_t row, const std::set<uint32_t>& ids, bool sanitize, bool checkLic);
    uint16_t                           getPriority(const std::string& s) const;
    bool                               isDate(const std::string& key) const;
    std::string                        matchExtAttr(const std::string& value, const std::string& key) const;
    bool                               checkUSize(const std::string& s) const;

    AssetExpected<void> updateDcRoomRowRackGroup(fty::db::Connection& conn, uint32_t elementId,
        const std::string& elementName, uint32_t parentId, const std::map<std::string, std::string>& extattributes,
        const std::string& status, uint16_t priority, const std::set<uint32_t>& groups, const std::string& assetTag,
        const std::map<std::string, std::string>& extattributesRO) const;

    AssetExpected<void> updateDevice(fty::db::Connection& conn, uint32_t elementId, const std::string& elementName,
        uint32_t parentId, const std::map<std::string, std::string>& extattributes, const std::string& status,
        uint16_t priority, const std::set<uint32_t>& groups, const std::vector<db::AssetLink>& links,
        const std::string& assetTag, const std::map<std::string, std::string>& extattributesRO) const;

    Expected<uint32_t> insertDcRoomRowRackGroup(fty::db::Connection& conn, const std::string& elementName,
        uint16_t elementTypeId, uint32_t parentId, const std::map<std::string, std::string>& extattributes,
        const std::string& status, uint16_t priority, const std::set<uint32_t>& groups, const std::string& assetTag,
        const std::map<std::string, std::string>& extattributesRO) const;

    Expected<uint32_t> insertDevice(fty::db::Connection& conn, const std::vector<db::AssetLink>& links,
        const std::set<uint32_t>& groups, const std::string& elementName, uint32_t parentId,
        const std::map<std::string, std::string>& extattributes, uint16_t assetDeviceTypeId, const std::string& status,
        uint16_t priority, const std::string& assetTag,
        const std::map<std::string, std::string>& extattributesRO) const;

private:
    const CsvMap&            m_cm;
    ImportResMap             m_el;
    persist::asset_operation m_operation{persist::asset_operation::GET};
};

} // namespace fty::asset
