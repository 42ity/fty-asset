/*  =========================================================================
    total_power - Calculation of total power for one asset

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

#include <string>
#include <vector>
#include <fty/expected.h>

/// For the specified asset finds out the devices that are used for total power computation
///
/// @param assetName - name of the asset, for which we need to determine devices for total power calculation
/// @param test - use for testing only
/// @return list of devices for the specified asset used for total power computation or error.
fty::Expected<std::vector<std::string>> selectDevicesTotalPower(const std::string& assetName, bool test);

