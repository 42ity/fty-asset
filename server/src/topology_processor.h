/*  =========================================================================
    topology_processor - Topology requests processor

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

#pragma once

#include <fty/expected.h>
#include <string>


/// Retrieve the powerchains which powers a requested target asset
/// Implementation of REST /api/v1/topology/power?[from/to/filter_dc/filter_group] (see RFC11)
/// @param command is in {"from", "to", "filter_dc", "filter_group"} tokens set
/// @param assetName is the subject of the command
/// @param beautify beautify json or not
/// @return result (JSON payload) or error
fty::Expected<std::string> topologyPowerProcess(const std::string& command, const std::string& assetName, bool beautify = true);

/// Retrieve the closest powerchain which powers a requested target asset
/// implementation of REST /api/v1/topology/power?to (see RFC11) **filtered** on dst-id == assetName
/// assetName is the target asset
/// @param beautify beautify json or not
/// @return result (JSON payload) or error
fty::Expected<std::string> topologyPowerTo(const std::string& assetName, bool beautify = true);

/// Retrieve location topology for a requested target asset
/// Implementation of REST /api/v1/topology/location?[from/to] (see RFC11)
/// @param command is in {"to", "from"} tokens set
/// @param assetName is the subject of the command (can be "none" if command is "from")
/// @param options if 'to'  : must be empty (no option allowed)
///                if 'from': json payload as { "recursive": <true|false>, "filter": <element_kind>, "feed_by": <asset_id> }
///                where <element_kind> is in {"rooms" , "rows", "racks", "devices", "groups"}
///                if "feed_by" is specified, "filter" *must* be set to "devices", ASSETNAME *must* not be set to "none"
///                defaults are {"recursive": false, "filter": "", "feed_by": "" }
/// @param beautify beautify json or not
/// @return result (JSON payload) or error
fty::Expected<std::string> topologyLocationProcess(
    const std::string& command, const std::string& assetName, const std::string& options, bool beautify = true);

/// Retrieve input power chain topology for a requested target asset
/// Implementation of REST /api/v1/topology/input_power_chain (see RFC11)
/// @param assetName is an assetID (normaly a datacenter)
/// @param beautify beautify json or not
/// @return result (JSON payload) or error
fty::Expected<std::string> topologyInputPowerchainProcess(const std::string& assetName, bool beautify = true);
