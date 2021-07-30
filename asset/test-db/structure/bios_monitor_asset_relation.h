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
