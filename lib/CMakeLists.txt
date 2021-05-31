cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

etn_target(shared ${PROJECT_NAME}
    SOURCES
        src/fty_asset_dto.cc
        src/fty_common_asset.cc
        src/conversion/full-asset.cc
        src/conversion/json.cc
        src/conversion/proto.cc
    INCLUDE_DIRS
        include
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_dto.h
        fty_common_asset.h
    USES_PRIVATE
        czmq
        cxxtools  # cxxtools cannot be use as public because we do not have the cmake package yet
        fty_common
        fty_common_db
        fty_common_logging
        fty_common_messagebus
        fty_common_mlm
        fty_proto
        fty-utils
        tntdb
)


##############################################################################################################

if(BUILD_TESTING)
    etn_test_target(${PROJECT_NAME}
        SOURCES
            test/*.cpp
        USES
            Catch2::Catch2
            cxxtools
    )

    ## manual set of include dirs, can't be set in the etn_target_test macro
    get_target_property(INCLUDE_DIRS_TARGET ${PROJECT_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(${PROJECT_NAME}-test PRIVATE ${INCLUDE_DIRS_TARGET})
    target_include_directories(${PROJECT_NAME}-coverage PRIVATE ${INCLUDE_DIRS_TARGET})
endif()
