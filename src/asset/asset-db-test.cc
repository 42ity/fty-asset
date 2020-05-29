/*  =========================================================================
    asset_asset_db_test - asset/asset-db-test

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
    asset_asset_db_test - asset/asset-db-test
@discuss
@end
*/

#include "asset-db-test.h"

namespace fty {

AssetImpl::DBTest::DBTest()
{
    std::cout << "DBTest::DBTest()" << std::endl;
}

void AssetImpl::DBTest::loadAsset(const std::string& nameId, Asset& asset)
{
    std::cout << "DBTest::loadAsset" << std::endl;
    asset.setInternalName(nameId);
    asset.setAssetStatus(fty::AssetStatus::Nonactive);
    asset.setAssetType(fty::TYPE_DEVICE);
    asset.setAssetSubtype(fty::SUB_UPS);
    asset.setParentIname("abc123");
    asset.setPriority(4);
}

void AssetImpl::DBTest::loadExtMap(Asset& asset)
{
    std::cout << "DBTest::loadExtMap" << std::endl;
    asset.setExtEntry("uuid", "123-456-789", true);
    asset.setExtEntry("name", "My Asset", false);
}

void AssetImpl::DBTest::loadChildren(Asset& asset)
{
    std::cout << "DBTest::loadChildren" << std::endl;
    std::vector<std::string> children;

    children.push_back("child-1");
    children.push_back("child-2");

    asset.setChildren(children);
}

void AssetImpl::DBTest::loadLinkedAssets(Asset& asset)
{
    std::cout << "DBTest::loadLinkedAssets" << std::endl;
    std::vector<std::string> links;

    links.push_back("asset-1");
    links.push_back("asset-2");

    asset.setLinkedAssets(links);
}


bool AssetImpl::DBTest::isLastDataCenter(Asset& asset)
{
    std::cout << "DBTest::isLastDataCenter" << std::endl;
    return asset.getInternalName() == "dc-0";
}

void AssetImpl::DBTest::removeFromGroups(Asset& asset)
{
    std::cout << "DBTest::removeFromGroups" << std::endl;
}

void AssetImpl::DBTest::removeFromRelations(Asset& asset)
{
    std::cout << "DBTest::removeFromRelations" << std::endl;
}

void AssetImpl::DBTest::removeAsset(Asset& asset)
{
    std::cout << "DBTest::removeAsset" << std::endl;
}

void AssetImpl::DBTest::removeExtMap(Asset& asset)
{
    std::cout << "DBTest::removeExtMap" << std::endl;
}

void AssetImpl::DBTest::clearGroup(Asset& asset)
{
    std::cout << "DBTest::clearGroup" << std::endl;
}

bool AssetImpl::DBTest::hasLinkedAssets(const Asset& asset)
{
    std::cout << "DBTest::hasLinkedAssets" << std::endl;
    return true;
}

void AssetImpl::DBTest::link(Asset& src, Asset& dest)
{
    std::cout << "DBTest::link" << std::endl;
}

void AssetImpl::DBTest::unlink(Asset& src, Asset& dest)
{
    std::cout << "DBTest::unlink" << std::endl;
}

void AssetImpl::DBTest::unlinkAll(Asset& dest)
{
    std::cout << "DBTest::unlinkAll" << std::endl;
}

void AssetImpl::DBTest::beginTransaction()
{
    std::cout << "DBTest::beginTransaction" << std::endl;
}

void AssetImpl::DBTest::rollbackTransaction()
{
    std::cout << "DBTest::rollbackTransaction" << std::endl;
}

void AssetImpl::DBTest::commitTransaction()
{
    std::cout << "DBTest::commitTransaction" << std::endl;
}

void AssetImpl::DBTest::update(Asset& asset)
{
    std::cout << "DBTest::update" << std::endl;
}

void AssetImpl::DBTest::insert(Asset& asset)
{
    std::cout << "DBTest::insert" << std::endl;
}

std::string AssetImpl::DBTest::unameById(uint32_t id)
{
    std::cout << "DBTest::unameById" << std::endl;
    return "DC-1";
}

void AssetImpl::DBTest::saveLinkedAssets(Asset& asset)
{
    std::cout << "DBTest::saveLinkedAssets" << std::endl;
}

void AssetImpl::DBTest::saveExtMap(Asset& asset)
{
    std::cout << "DBTest::saveExtMap" << std::endl;
}

std::vector<std::string> AssetImpl::DBTest::listAllAssets()
{
    std::cout << "DBTest::listAllAssets" << std::endl;
    std::vector<std::string> assetList;

    assetList.push_back("asset-1");
    assetList.push_back("asset-2");
    assetList.push_back("asset-3");

    return assetList;
}

} // namespace fty
