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

#include "test-db.h"

#include "structure/bios_asset_device_type.h"
#include "structure/bios_asset_element.h"
#include "structure/bios_asset_element_type.h"
#include "structure/bios_asset_ext_attributes.h"
#include "structure/bios_asset_group_relation.h"
#include "structure/bios_asset_link.h"
#include "structure/bios_asset_link_type.h"
#include "structure/bios_device_type.h"
#include "structure/bios_discovered_device.h"
#include "structure/bios_monitor_asset_relation.h"
#include "structure/bios_tag.h"

#include <fty_common_db_connection.h>

#include <filesystem>
#include <mariadb/mysql.h>
#include <thread>
#include <iostream>
#include <unistd.h>

namespace fty {

// =====================================================================================================================

// create database
static void createDB(const std::string& dbName)
{
    fty::db::Connection conn;
    conn.execute("CREATE DATABASE IF NOT EXISTS " + dbName + " character set utf8 collate utf8_general_ci;");
    conn.execute("USE " + dbName);

    createElementType(conn);
    createAssetDeviceType(conn);
    createAssetLinkType(conn);
    createAssetElement(conn);
    createDeviceType(conn);
    createAssetAttributes(conn);
    createGroupRelations(conn);
    createAssetLink(conn);
    createDiscoveryDevice(conn);
    createMonitorAssetRelation(conn);
    createTag(conn);
}

Expected<std::string> TestDb::create()
{
    //std::cout << "== TestDb::createDB started " << std::endl;

    std::stringstream ss;
    ss << getpid();
    std::string pid = ss.str();

    m_path = "/tmp/mysql_" + pid;
    std::string sock = m_path + ".sock";
    std::string dbName = "box_utf8"; // *must be*

    // create DB directory
    if (!std::filesystem::create_directory(m_path)) {
        return unexpected("cannot create db dir {}", m_path);
    }

    // mysql init. DB dir
    char cmd[255]; snprintf(cmd, sizeof(cmd), "/usr/bin/mysql_install_db --datadir=%s", m_path.c_str());
    int r = std::system(cmd);
    if (r != 0) {
        return unexpected("cannot initialize db dir {}", m_path);
    }

    // start mysql client on the DB directory
    char dbname[255]; snprintf(dbname, sizeof(dbname), "%s", dbName.c_str());
    char datadir[255]; snprintf(datadir, sizeof(datadir), "--datadir=%s", m_path.c_str());
    char socket[255]; snprintf(socket, sizeof(socket), "--socket=%s", sock.c_str());
    char group1[255]; snprintf(group1, sizeof(group1), "%s", "libmysqld_server");
    char group2[255]; snprintf(group2, sizeof(group2), "%s", "libmysqld_client");
    int argc = 3;
    char* argv[] = { dbname, datadir, socket, nullptr };
    char* groups[] = { group1, group2, nullptr };

    mysql_library_init(argc, argv, groups);

    std::string dburl = "mysql:unix_socket=" + sock;
    setenv("DBURL", dburl.c_str(), 1);

    // create database and tables
    try {
        //std::cout << "== TestDb::createDB: " << dbName << std::endl;
        createDB(dbName);
    } catch (const std::exception& e) {
        std::cerr << "== createDB exception: " << e.what() << std::endl;
        return unexpected(e.what());
    }

    dburl = "mysql:unix_socket=" + sock + ";db=" + dbName;
    setenv("DBURL", dburl.c_str(), 1);

    //std::cout << "== TestDb::createDB ended: dburl: " << dburl << std::endl;

    return dburl;
}

//static
void TestDb::destroy()
{
    //std::cout << "== TestDb::destroy started" << std::endl;

    if (instance().m_inited) {

        instance().m_inited = false;
        std::filesystem::path path = instance().m_path;
        //std::cout << "== TestDb::destroy path: " << path << std::endl;

        // stop mysql client
        try {
            //std::cout << "== TestDb::destroy DB shutdown " << std::endl;
            fty::db::shutdown();
            // DB client must breath a while to shutdown properly (may crash if not)
            sleep(1);
        }
        catch (const std::exception& e) {
            std::cerr << "== TestDb::destroy shutdown exception: " << e.what() << std::endl;
        }

        // remove DB files and directory
        std::filesystem::remove_all(path);
    }

    //std::cout << "== TestDb::destroy ended " << std::endl;
}

//static
Expected<void> TestDb::init()
{
    if (!instance().m_inited) {
        if (auto res = instance().create(); !res) {
            std::cerr << res.error() << std::endl;
            return fty::unexpected(res.error());
        }
        instance().m_inited = true;
    }
    return {};
}

//static
TestDb& TestDb::instance()
{
    static TestDb db;
    return db;
}


} // namespace fty
