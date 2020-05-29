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

#pragma once

#include <cxxtools/serializationinfo.h>
#include <string>
// fwd declaration
namespace fty {
class Asset;
} // namespace fty

namespace fty { namespace conversion {
    std::string toJson(const Asset& asset);
    void        fromJson(const std::string& json, fty::Asset& asset);

    void operator<<=(cxxtools::SerializationInfo& si, const Asset& asset);
    void operator>>=(const cxxtools::SerializationInfo& si, Asset& asset);
}} // namespace fty::conversion
