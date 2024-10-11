/*
 *
 * Copyright (C) 2021 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#pragma once

#include <fty/expected.h>

namespace fty::db {
class Connection;
class Row;
} // namespace fty::db

namespace fty::db::asset {

// =========================================================================================================================================

/// Converts asset internal name to database id
/// @param assetName internal name of the asset
/// @return asset is or error
Expected<uint32_t> nameToAssetId(const std::string& assetName);

/// Converts asset internal name to database id
/// @param conn database established connection
/// @param assetName internal name of the asset
/// @return asset is or error
Expected<uint32_t> nameToAssetId(Connection& conn, const std::string& assetName);

// =========================================================================================================================================

struct Names
{
    std::string name;    //!< internal name
    std::string extName; //!< external name
};

/// Converts database id to internal name and extended (unicode) name
/// @param assetId asset id
/// @return struct of name and extended name or error
Expected<Names> idToNameExtName(uint32_t assetId);

/// Converts database id to internal name and extended (unicode) name
/// @param conn database established connection
/// @param assetId asset id
/// @return struct of name and extended name or error
Expected<Names> idToNameExtName(Connection& conn, uint32_t assetId);

// =========================================================================================================================================

/// Converts asset's extended name to its internal name
/// @param assetExtName asset external name
/// @return internal name or error
Expected<std::string> extNameToAssetName(const std::string& assetExtName);

/// Converts asset's extended name to its internal name
/// @param conn database established connection
/// @param assetExtName asset external name
/// @return internal name or error
Expected<std::string> extNameToAssetName(Connection& conn, const std::string& assetExtName);

// =========================================================================================================================================

/// Converts internal name to extended name
/// @param assetName asset internal name
/// @return external name or error
Expected<std::string> nameToExtName(const std::string& assetName);

/// Converts internal name to extended name
/// @param conn database established connection
/// @param assetName asset internal name
/// @return external name or error
Expected<std::string> nameToExtName(Connection& conn, const std::string& assetName);

// =========================================================================================================================================

/// Selects maximum number of power sources for device in the system
/// @return  number of power sources or error
Expected<uint32_t> maxNumberOfPowerLinks();

/// Selects maximum number of power sources for device in the system
/// @param conn database established connection
/// @return  number of power sources or error
Expected<uint32_t> maxNumberOfPowerLinks(Connection& conn);

// =========================================================================================================================================

/// Selects maximal number of groups in the system
/// @return number of groups or error
Expected<uint32_t> maxNumberOfAssetGroups();

/// Selects maximal number of groups in the system
/// @param conn database established connection
/// @return number of groups or error
Expected<uint32_t> maxNumberOfAssetGroups(Connection& conn);

// =========================================================================================================================================

/// Returns how many times is gived a couple keytag/value in t_bios_asset_ext_attributes
/// @param keytag keytag
/// @param value asset name
/// @param elementId asset element id
/// @return count or error
Expected<int32_t> countKeytag(const std::string& keytag, const std::string& value, uint32_t elementId = 0);

/// Returns how many times is gived a couple keytag/value in t_bios_asset_ext_attributes
/// @param keytag keytag
/// @param value asset name
/// @param elementId asset element id
/// @return count or error
Expected<int32_t> countKeytag(Connection& conn, const std::string& keytag, const std::string& value, uint32_t elementId = 0);

// =========================================================================================================================================

} // namespace fty::db::asset
