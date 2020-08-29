/*  =========================================================================
    asset_asset_storage - asset/asset-storage

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
#include "fty_asset_dto.h"
#include <map>
#include <string>
#include <vector>

namespace fty {

class Asset;

class AssetStorage
{
public:
    enum class StorageType
    {
        StorageDB,
        StorageDBTest
    };

    virtual void loadAsset(const std::string& nameId, Asset& asset) = 0;

    virtual fty::Asset::ExtMap       getExtMap(const std::string& iname) = 0;
    virtual void                     loadLinkedAssets(Asset& asset)      = 0;
    virtual std::vector<std::string> getChildren(const Asset& asset)     = 0;

    virtual uint32_t getID(const std::string& internalName)   = 0;
    virtual uint32_t getTypeID(const std::string& type)       = 0;
    virtual uint32_t getSubtypeID(const std::string& subtype) = 0;

    virtual bool hasLinkedAssets(const Asset& asset) = 0;
    virtual void link(
        Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType) = 0;
    virtual void unlink(
        Asset& src, const std::string& srcOut, Asset& dest, const std::string& destIn, int linkType) = 0;
    virtual void unlinkAll(Asset& dest)                                                              = 0;
    virtual void clearGroup(Asset& asset)                                                            = 0;
    virtual void removeAsset(Asset& asset)                                                           = 0;
    virtual void removeFromRelations(Asset& asset)                                                   = 0;
    virtual void removeFromGroups(Asset& asset)                                                      = 0;
    virtual void removeExtMap(Asset& asset)                                                          = 0;
    virtual bool isLastDataCenter(Asset& asset)                                                      = 0;

    virtual void beginTransaction()    = 0;
    virtual void rollbackTransaction() = 0;
    virtual void commitTransaction()   = 0;

    virtual void update(Asset& asset) = 0;
    virtual void insert(Asset& asset) = 0;

    virtual void        saveLinkedAssets(Asset& asset)       = 0;
    virtual void        saveExtMap(Asset& asset)             = 0;
    virtual std::string inameById(uint32_t id)               = 0;
    virtual std::string inameByUuid(const std::string& uuid) = 0;

    virtual std::vector<std::string> listAssets(std::map<std::string, std::vector<std::string>> filters) = 0;
    virtual std::vector<std::string> listAllAssets()                                                     = 0;
};

} // namespace fty
