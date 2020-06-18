/*  =========================================================================
    asset_asset_utils - asset/asset-utils

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
#include <fty_common_messagebus.h>
#include <string>
#include <vector>

namespace fty { namespace assetutils {

    // create messages
    messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
        const std::string& from, const std::string& to, const std::string& status, const std::string& data);
    messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
        const std::string& from, const std::string& to, const std::string& status,
        const std::vector<std::string>& data);
    messagebus::Message createMessage(const std::string& subject, const std::string& correlationID,
        const std::string& from, const std::string& to, const std::string& status,
        const messagebus::UserData& data);

    // JSON serialization/deserialization
    std::string                 serialize(const cxxtools::SerializationInfo& si);
    cxxtools::SerializationInfo deserialize(const std::string& json);
}} // namespace fty::assetutils
