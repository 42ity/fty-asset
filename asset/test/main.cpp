#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>
#include <filesystem>
#include <thread>
#include <fty_log.h>
#include "test-db/test-db.h"


int main(int argc, char* argv[])
{
    Catch::Session session;

    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }

    Catch::ConfigData data = session.configData();
    if (data.listReporters || data.listTestNamesOnly) {
        return session.run();
    }

    ManageFtyLog::setInstanceFtylog("asset-test", "conf/logger.conf");
    int result = session.run(argc, argv);
    fty::TestDb::destroy();
    return result;
}
