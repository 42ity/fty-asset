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

#include "asset/asset-dto.h"
#include <fty/expected.h>
#include <pack/pack.h>

namespace fty::asset {

Expected<void> sendStreamNotification(const std::string& stream, const std::string& subject, const std::string& payload);

namespace notification::created {
    struct Topic
    {
        static constexpr const char* Full  = "FTY.T.ASSET.CREATED";
        static constexpr const char* Light = "FTY.T.ASSET_LIGHT.CREATED";
    };

    struct Subject
    {
        static constexpr const char* Full  = "CREATED";
        static constexpr const char* Light = "CREATED_LIGHT";
    };

    using PayloadFull  = Dto;
    using PayloadLight = pack::String;
} // namespace notification::created

namespace notification::updated {
    struct Topic
    {
        static constexpr const char* Full  = "FTY.T.ASSET.UPDATED";
        static constexpr const char* Light = "FTY.T.ASSET_LIGHT.UPDATED";
    };

    struct Subject
    {
        static constexpr const char* Full  = "UPDATED";
        static constexpr const char* Light = "UPDATED_LIGHT";
    };

    struct PayloadFull : public pack::Node
    {
        Dto before = FIELD("before");
        Dto after  = FIELD("after");

        using pack::Node::Node;

        META(PayloadFull, before, after);
    };
    using PayloadLight = pack::String;
} // namespace notification::updated

namespace notification::deleted {
    struct Topic
    {
        static constexpr const char* Full  = "FTY.T.ASSET.DELETED";
        static constexpr const char* Light = "FTY.T.ASSET_LIGHT.DELETED";
    };

    struct Subject
    {
        static constexpr const char* Full  = "DELETED";
        static constexpr const char* Light = "DELETED_LIGHT";
    };

    using PayloadFull  = Dto;
    using PayloadLight = pack::String;
} // namespace notification::deleted

} // namespace fty::asset
