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

/*
@header
    asset_asset_utils - asset/asset-utils
@discuss
@end
*/

#include "asset-utils.h"

namespace fty {
namespace assetutils {

    // create message (user data is a single string)
    messagebus::Message createMessage(
        const std::string& subject,
        const std::string& correlationID,
        const std::string& from,
        const std::string& to,
        const std::string& status,
        const std::string& data)
    {
        return createMessage(subject, correlationID, from, to, status, messagebus::UserData{{data}});
    }

    // create message
    messagebus::Message createMessage(
        const std::string& subject,
        const std::string& correlationID,
        const std::string& from,
        const std::string& to,
        const std::string& status,
        const messagebus::UserData& userdata)
    {
        messagebus::MetaData metadata;

        auto metaAdd = [&metadata](const std::string& key, const std::string& value) {
            if (!value.empty()) {
                metadata.emplace(key, value);
            }
        };

        metaAdd(messagebus::Message::SUBJECT, subject);
        metaAdd(messagebus::Message::CORRELATION_ID, correlationID);
        metaAdd(messagebus::Message::FROM, from);
        metaAdd(messagebus::Message::TO, to);
        metaAdd(messagebus::Message::STATUS, status);

        return messagebus::Message{metadata, userdata};
    }
}} // namespace fty::assetutils
