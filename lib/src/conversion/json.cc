/*  =========================================================================
    asset_conversion_json - asset/conversion/json

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

#include "conversion/json.h"

#include <cxxtools/serializationinfo.h>
#include <fty_common_json.h>
#include <fty_asset_dto.h>

namespace fty { namespace conversion {

    std::string toJson(const Asset& asset)
    {
        cxxtools::SerializationInfo si;
        si <<= asset;
        return JSON::writeToString(si, false);
    }

    void fromJson(const std::string& json, fty::Asset& asset)
    {
        cxxtools::SerializationInfo si;
        JSON::readFromString(json, si);
        si >>= asset;
    }

}} // namespace fty::conversion
