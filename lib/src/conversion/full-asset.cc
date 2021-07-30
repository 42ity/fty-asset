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

struct fty_proto_t;

namespace fty { namespace conversion {

    fty::FullAsset toFullAsset(const fty::Asset& asset)
    {
        fty::FullAsset::HashMap auxMap; // does not exist in new Asset implementation
        fty::FullAsset::HashMap extMap;

        for (const auto& element : asset.getExt()) {
            // FullAsset hash map has no readOnly parameter
            extMap[element.first] = element.second.getValue();
        }

        fty::FullAsset fa(asset.getInternalName(), fty::assetStatusToString(asset.getAssetStatus()),
            asset.getAssetType(), asset.getAssetSubtype(),
            asset.getExtEntry(fty::EXT_NAME), // asset name is stored in ext structure
            asset.getParentIname(), asset.getPriority(), auxMap, extMap);

        return fa;
    }

}} // namespace fty::conversion
