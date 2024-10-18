cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

etn_target(shared ${PROJECT_NAME}-test-db
    PUBLIC_INCLUDE_DIR
        include/
    PUBLIC
        *.h
    SOURCES
        src/structure/*.h
        src/*.cpp
    USES
        ${PROJECT_NAME}-libng
        mysqld
        stdc++fs
        tntdb
        cxxtools
    USES_PUBLIC
        fty-utils
        fty_common_db
        fty_common
        fty-pack
)
