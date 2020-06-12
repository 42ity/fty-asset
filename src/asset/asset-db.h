/*  =========================================================================
    asset_db - asset_db

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
    =========================================================================
*/

#pragma once
#include "asset.h"
#include <memory>
#include <mutex>
#include <tntdb.h>

namespace fty {

class AssetImpl::DB
{
public:
    static DB& getInstance()
    {
        static DB m_instance;
        return m_instance;
    }

    virtual void loadAsset(const std::string& nameId, Asset& asset);

    virtual void                     loadExtMap(Asset& asset);
    virtual void                     loadLinkedAssets(Asset& asset);
    virtual std::vector<std::string> getChildren(const Asset& asset);

    virtual bool hasLinkedAssets(const Asset& asset);
    virtual void link(Asset& src, Asset& dest);
    virtual void unlink(Asset& src, Asset& dest);
    virtual void unlinkAll(Asset& dest);
    virtual void clearGroup(Asset& asset);
    virtual void removeAsset(Asset& asset);
    virtual void removeFromRelations(Asset& asset);
    virtual void removeFromGroups(Asset& asset);
    virtual void removeExtMap(Asset& asset);
    virtual bool isLastDataCenter(Asset& asset);

    virtual void beginTransaction();
    virtual void rollbackTransaction();
    virtual void commitTransaction();

    virtual void update(Asset& asset);
    virtual void insert(Asset& asset);

    virtual void        saveLinkedAssets(Asset& asset);
    virtual void        saveExtMap(Asset& asset);
    virtual std::string unameById(uint32_t id);

    virtual std::vector<std::string> listAllAssets();

protected:
    DB(bool test = false);

private:
    std::mutex                m_conn_lock;
    mutable tntdb::Connection m_conn;
};

} // namespace fty
