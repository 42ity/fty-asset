#define CATCH_CONFIG_RUNNER

#include <catch2/catch.hpp>
#include <fty_log.h>
#include "test-db/test-db.h"

int main(int argc, char* argv[])
{
    Catch::Session session;

    ManageFtyLog::setInstanceFtylog("asset-test", "conf/logger.conf");
    int result = session.run(argc, argv);

    fty::TestDb::destroy();

    return result;
}
