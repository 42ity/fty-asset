/*  ========================================================================
    Copyright (C) 2020 Eaton
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    ========================================================================
*/

#define CATCH_CONFIG_RUNNER //own main()
#include <catch2/catch.hpp>

//#include <fty_log.h>
#include "asset/asset.h"

static int runCatch2Tests(int argc, char* argv[])
{
    Catch::Session session;

    int r = session.applyCommandLine(argc, argv);
    if (r != 0) {
        return r; // command line error
    }

    return session.run();
}

int main(int argc, char* argv[])
{
    //bool verbose = true;
    //ManageFtyLog::setInstanceFtylog("fty-asset-test", FTY_COMMON_LOGGING_DEFAULT_CFG);
    //if (verbose) {
    //    ManageFtyLog::getInstanceFtylog()->setVerboseMode();
    //}

    g_testMode = true;

    return runCatch2Tests(argc, argv);
}
