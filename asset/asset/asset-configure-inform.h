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
#include <fty_common_asset_types.h>
#include <string>
#include <vector>

namespace fty::asset {

Expected<void> sendConfigure(const std::vector<std::pair<db::AssetElement, persist::asset_operation>>& rows, const std::string& agentName);

Expected<void> sendConfigure(const db::AssetElement& row, persist::asset_operation actionType, const std::string& agentName);

std::string generateMlmClientId(const std::string& client_name);

} // namespace fty::asset
