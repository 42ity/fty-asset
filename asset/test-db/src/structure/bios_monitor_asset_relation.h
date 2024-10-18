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

#include <fty_common_db_connection.h>

namespace fty {

void createMonitorAssetRelation(fty::db::Connection& conn)
{
    conn.execute(R"(
        CREATE TABLE t_bios_monitor_asset_relation(
            id_ma_relation        INT UNSIGNED      NOT NULL AUTO_INCREMENT,
            id_discovered_device  SMALLINT UNSIGNED NOT NULL,
            id_asset_element      INT UNSIGNED      NOT NULL,

            PRIMARY KEY(id_ma_relation),

            INDEX(id_discovered_device),
            INDEX(id_asset_element),

            FOREIGN KEY (id_discovered_device)
                REFERENCEs t_bios_discovered_device(id_discovered_device)
                ON DELETE RESTRICT,

            FOREIGN KEY (id_asset_element)
                REFERENCEs t_bios_asset_element(id_asset_element)
                ON DELETE RESTRICT
        );
    )");

    conn.execute(R"(
        create view v_bios_monitor_asset_relation as select * from t_bios_monitor_asset_relation;
    )");
}

} // namespace fty
