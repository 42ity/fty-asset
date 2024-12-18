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

void createAssetAttributes(fty::db::Connection& conn)
{
    conn.execute(R"(
        CREATE TABLE t_bios_asset_ext_attributes(
            id_asset_ext_attribute    INT UNSIGNED NOT NULL AUTO_INCREMENT,
            keytag                    VARCHAR(40)  NOT NULL,
            value                     VARCHAR(255) NOT NULL,
            id_asset_element          INT UNSIGNED NOT NULL,
            read_only                 TINYINT      NOT NULL DEFAULT 0,

            PRIMARY KEY (id_asset_ext_attribute),

            INDEX FK_ASSETEXTATTR_ELEMENT_idx (id_asset_element ASC),
            UNIQUE INDEX `UI_t_bios_asset_ext_attributes` (`keytag`, `id_asset_element` ASC),

            CONSTRAINT FK_ASSETEXTATTR_ELEMENT
            FOREIGN KEY (id_asset_element)
            REFERENCES t_bios_asset_element (id_asset_element)
            ON DELETE CASCADE
        );
    )");

    conn.execute(R"(
        CREATE VIEW v_bios_asset_ext_attributes AS
            SELECT * FROM t_bios_asset_ext_attributes ;
    )");
}

} // namespace fty
