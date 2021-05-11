cmake_minimum_required(VERSION 3.13)

########################################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
########################################################################################################################

etn_target(shared ${PROJECT_NAME}-libng
    PUBLIC
        asset/json.h
        asset/asset-manager.h
        asset/asset-helpers.h
        asset/asset-computed.h
        asset/asset-db.h
        asset/asset-licensing.h
        asset/asset-import.h
        asset/asset-configure-inform.h
        asset/csv.h
        asset/error.h
        asset/logger.h
        asset/db.h

    SOURCES
        src/json.cpp
        src/asset-cam.cpp
        src/asset-manager.cpp
        src/asset-computed.cpp
        src/asset-helpers.cpp
        src/asset-db.cpp
        src/asset-licensing.cpp
        src/asset-import.cpp
        src/asset-configure-inform.cpp
        src/csv.cpp

        src/manager/read.cpp
        src/manager/delete.cpp
        src/manager/create.cpp
        src/manager/import.cpp
        src/manager/export.cpp
    INCLUDE_DIRS src
    USES
        mlm
        fty-asset
        fty_shm
        fty_common_rest
        czmq
        fty_proto
        fty_security_wallet
        fty_common_dto
    USES_PUBLIC
        fty_common_logging
        fty_common_mlm
        fty_common_db
        fty_common
        fty_common_socket
        fty-utils
)

etn_target(shared ${PROJECT_NAME}-test-db
    PUBLIC
        test-db/sample-db.h
        test-db/test-db.h
    SOURCES
        test-db/structure/bios_asset_element_type.h
        test-db/structure/bios_asset_device_type.h
        test-db/structure/bios_asset_element.h
        test-db/structure/bios_asset_link_type.h
        test-db/structure/bios_asset_ext_attributes.h
        test-db/structure/bios_asset_group_relation.h
        test-db/structure/bios_asset_link.h
        test-db/structure/bios_device_type.h
        test-db/structure/bios_discovered_device.h
        test-db/structure/bios_monitor_asset_relation.h
        test-db/sample-db.cpp
        test-db/test-db.cpp
    USES
        fty-utils
        ${PROJECT_NAME}-libng
        fty_common_db
        fty_common
        fty-pack
        mysqld
        stdc++fs
        tntdb
)

if (BUILD_TESTING)
    add_subdirectory(test)
endif()

