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

void createElementType(fty::db::Connection& conn)
{
    conn.execute(R"(
        CREATE TABLE t_bios_asset_element_type (
            id_asset_element_type TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
            name                  VARCHAR(50)      NOT NULL,
            PRIMARY KEY (id_asset_element_type),
            UNIQUE INDEX `UI_t_bios_asset_element_type` (`name` ASC)
        ) AUTO_INCREMENT = 1;
    )");

    conn.execute(R"(
        CREATE VIEW v_bios_asset_element_type AS
            SELECT
                t1.id_asset_element_type AS id,
                t1.name
            FROM
                t_bios_asset_element_type t1;
    )");

    conn.execute(R"(
        INSERT INTO t_bios_asset_element_type (id_asset_element_type, name)
        VALUES  (1,  "group"),
                (2,  "datacenter"),
                (3,  "room"),
                (4,  "row"),
                (5,  "rack"),
                (6,  "device"),
                (7,  "infra-service"),
                (8,  "cluster"),
                (9,  "hypervisor"),
                (10, "virtual-machine"),
                (11, "storage-service"),
                (12, "vapp"),
                (13, "connector"),
                (15, "server"),
                (16, "planner"),
                (17, "plan"),
                (18, "cops"),
                (19, "operating-system"),
                (20, "host-group"),
                (21, "container-cluster"),
                (22, "container-node");
    )");
}

}
