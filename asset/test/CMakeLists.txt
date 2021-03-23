project(fty-asset-libng-test)

etn_test(fty-asset-libng-test
    SOURCES
        main.cpp
        db/insert.cpp
        db/names.cpp
        db/select.cpp

        test-utils.h
        read.cpp
        create.cpp
        import.cpp
        export.cpp
        delete.cpp
        usize.cpp
    CONFIGS
        conf/logger.conf
    USES
        fty-utils
        fty-asset-libng
        fty-asset-test-db
        Catch2::Catch2
        fty_common_db
        tntdb
        cxxtools
        yaml-cpp
        log4cplus
        fty_common_logging
        fty_common
)

#etn_coverage(asset-test)

