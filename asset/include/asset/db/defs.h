/*
 *
 * Copyright (C) 2021 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#pragma once

#include <functional>
#include <map>
#include <string>

namespace fty::db {
class Row;
} // namespace fty::db

namespace fty::db::asset {

struct ExtAttrValue
{
    std::string value;
    bool        readOnly{false};
};

using Attributes = std::map<std::string, ExtAttrValue>;

struct AssetItem
{
public:
    uint32_t    id{0};
    std::string name;
    std::string status;
    uint32_t    parentId{0};
    uint16_t    priority{0};
    uint16_t    typeId{0};
    uint16_t    subtypeId{0};
    std::string assetTag;
};

struct AssetItemExt : public AssetItem
{
    std::string extName;
    std::string typeName;
    uint16_t    parentTypeId{0};
    std::string subtypeName;
    std::string parentName;
};

struct AssetLink
{
    uint32_t    srcId{0};
    uint32_t    destId{0};
    std::string srcName;
    std::string srcSocket;
    std::string destSocket;
};


} // namespace fty::db::asset
