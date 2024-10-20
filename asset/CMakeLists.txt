cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

etn_target(shared ${PROJECT_NAME}-libng
    PUBLIC_INCLUDE_DIR
        include/
    PUBLIC
        asset/*.h
        asset/db/*.h
    INCLUDE_DIR
        src/
    SOURCES
        src/*.cpp
        src/messagebus/*.cpp
        src/manager/*.cpp
        src/db/*.cpp
    USES
        mlm
        czmq
        tntdb
        cxxtools
        uuid
        crypto
        fty-asset
        fty_shm
        fty_proto
        fty_security_wallet
        fty_common_dto
    USES_PUBLIC
        fty_common
        fty_common_logging
        fty_common_mlm
        fty_common_messagebus
        fty_common_db
        fty_common_socket
        fty-pack
        fty-utils
)

########################################################################################################################

# libfty-asset-test-db.so (tntdb/mysql wrapper for unit test)
add_subdirectory(test-db)

########################################################################################################################

etn_test_target(${PROJECT_NAME}-libng
    SOURCES
        test/main.cpp
        test/sampledb.cpp
        test/db/insert.cpp
        test/db/names.cpp
        test/db/select.cpp

        test/test-utils.h
        test/read.cpp
        test/create.cpp
        test/import.cpp
        test/export.cpp
        test/delete.cpp
        test/usize.cpp
        test/norm-name.cpp

        test/db2/misc.cpp
        test/db2/select.cpp
    CONFIGS
        test/conf/logger.conf
    USES
        ${PROJECT_NAME}-test-db
        yaml-cpp
#    SUBDIR
#        test
)
