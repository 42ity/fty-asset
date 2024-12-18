/*  =========================================================================
    asset_conversion_full_asset - asset/conversion/full-asset

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

#include "conversion/full-asset.h"

#include <fty_asset_dto.h>
#include <fty_common_asset.h>

namespace fty {
namespace conversion {

    fty::FullAsset toFullAsset(const Asset& asset)
    {
        // FullAsset ext map has no readOnly parameter
        fty::FullAsset::HashMap extMap;
        for (const auto& element : asset.getExt()) {
            extMap[element.first] = element.second.getValue();
        }

        return fty::FullAsset{
            asset.getInternalName(),
            fty::assetStatusToString(asset.getAssetStatus()),
            asset.getAssetType(),
            asset.getAssetSubtype(),
            asset.getExtEntry(fty::EXT_NAME), // asset name is stored in ext structure
            asset.getParentIname(),
            asset.getPriority(),
            fty::FullAsset::HashMap{}, // no aux map in new Asset implementation
            extMap};
    }

}} // namespace fty::conversion
