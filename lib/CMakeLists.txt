cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

etn_target(shared ${PROJECT_NAME}
    SOURCES
        src/*.cc
        src/conversion/*.cc
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_dto.h
        fty_common_asset.h
    USES_PRIVATE
        czmq
        cxxtools
        tntdb
        fty_common
        fty_common_db
        fty_common_logging
        fty_common_messagebus
        fty_common_mlm
        fty_proto
        fty-utils
)

#locate private headers
target_include_directories(${PROJECT_NAME} AFTER PRIVATE src/)

etn_test_target(${PROJECT_NAME}
    SOURCES
        test/*.cpp
    USES
        ${PROJECT_NAME}
    PREPROCESSOR
        -DCATCH_CONFIG_FAST_COMPILE
)

if(BUILD_TESTING)
#locate private headers (required for test)
target_include_directories(${PROJECT_NAME}-test AFTER PRIVATE src/)
endif()
