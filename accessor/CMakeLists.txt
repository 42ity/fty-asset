cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

##############################################################################################################

set(ACCESSOR_NAME ${PROJECT_NAME}-accessor)

etn_target(shared ${ACCESSOR_NAME}
    SOURCES
        src/fty_asset_accessor.cc
    PUBLIC_INCLUDE_DIR
        public_includes
    PUBLIC
        fty_asset_accessor.h
    USES_PUBLIC
        fty-asset
        fty-utils
    USES_PRIVATE
        cxxtools
        fty_common
        fty_common_logging
        fty_common_messagebus
        czmq
        mlm
)

# these tests require asset-agent-ng and db access
set(ACCESSOR_TEST_ENABLED "Off")
##set(ACCESSOR_TEST_ENABLED "On")
if(ACCESSOR_TEST_ENABLED AND BUILD_TESTING)
    etn_test_target(${ACCESSOR_NAME}
        SOURCES
            test/*.cpp
        USES
            Catch2::Catch2
            cxxtools
    )

    ## manual set of include dirs, can't be set in the etn_target_test macro
    get_target_property(INCLUDE_DIRS_TARGET ${ACCESSOR_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(${ACCESSOR_NAME}-test PRIVATE ${INCLUDE_DIRS_TARGET})
    target_include_directories(${ACCESSOR_NAME}-coverage PRIVATE ${INCLUDE_DIRS_TARGET})
endif()
