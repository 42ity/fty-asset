/*  =========================================================================
    fty_asset_accessor

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

#include <fty_asset_dto.h>
#include <fty/expected.h>
#include <string>

namespace fty
{
    class AssetAccessor
    {
    public:
        static fty::Expected<uint32_t> assetInameToID(const std::string& iname);
        static fty::Expected<fty::Asset> getAsset(const std::string& iname);
        static void notifyStatusUpdate(const std::string& iname, const std::string& oldStatus, const std::string& newStatus);
        static void notifyAssetUpdate(const Asset& before, const Asset& after);
    };

} // namespace fty
