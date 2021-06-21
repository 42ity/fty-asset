#include "asset/asset-manager.h"
#include <fty_common_db_connection.h>
#include <catch2/catch.hpp>
#include <fty/string-utils.h>
#include <test-db/sample-db.h>

using namespace fmt::literals;

static std::string csvTrim(const std::string& src)
{
    std::vector<std::string> ret;
    for (const std::string& s : fty::split(src, "\n")) {
        ret.push_back(fty::trimmed(s));
    }
    return fty::implode(ret, "\n");
}

TEST_CASE("Export asset / Simple")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
    )");

    auto exp = fty::asset::AssetManager::exportCsv();
    REQUIRE_EXP(exp);

    static std::string data = R"(
        name,type,sub_type,location,status,priority,asset_tag,description,ip.1,company,site_name,region,country,address,contact_name,contact_email,contact_phone,u_size,manufacturer,model,serial_no,runtime,installation_date,maintenance_date,maintenance_due,location_u_pos,location_w_pos,end_warranty_date,hostname.1,http_link.1,id
        Data Center,datacenter,,,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,datacenter
        Server,device,server,Data Center,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,srv
    )";

    CHECK(*exp == csvTrim(data));
}

TEST_CASE("Export asset / Wrong order")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
                  - type     : Server
                    name     : srv1
                    ext-name : Server 1
                  - type     : Rack
                    name     : rack
                    ext-name : Rack
    )");

    fty::db::Connection conn;
    auto ret = fty::asset::db::updateAssetElement(conn, db.idByName("srv1"), db.idByName("rack"), "active", 1, "tag");

    auto exp = fty::asset::AssetManager::exportCsv();
    REQUIRE_EXP(exp);

    static std::string data = R"(
        name,type,sub_type,location,status,priority,asset_tag,description,ip.1,company,site_name,region,country,address,contact_name,contact_email,contact_phone,u_size,manufacturer,model,serial_no,runtime,installation_date,maintenance_date,maintenance_due,location_u_pos,location_w_pos,end_warranty_date,hostname.1,http_link.1,id
        Data Center,datacenter,,,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,datacenter
        Server,device,server,Data Center,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,srv
        Rack,rack,,Data Center,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,rack
        Server 1,device,server,Rack,active,P1,tag,,,,,,,,,,,,,,,,,,,,,,,,srv1
    )";

    CHECK(*exp == csvTrim(data));
}

TEST_CASE("Export asset / Wrong power order")
{
    fty::SampleDb db(R"(
        items:
            - type     : Datacenter
              name     : datacenter
              ext-name : Data Center
              items :
                  - type     : Server
                    name     : srv
                    ext-name : Server
                  - type     : Server
                    name     : srv1
                    ext-name : Server 1
                  - type     : Feed
                    name     : feed
                    ext-name : Feed
        links:
            - dest : srv
              src  : feed
              type : power chain
    )");

    auto exp = fty::asset::AssetManager::exportCsv();
    REQUIRE_EXP(exp);

    static std::string data = R"(
        name,type,sub_type,location,status,priority,asset_tag,power_source.1,power_plug_src.1,power_input.1,description,ip.1,company,site_name,region,country,address,contact_name,contact_email,contact_phone,u_size,manufacturer,model,serial_no,runtime,installation_date,maintenance_date,maintenance_due,location_u_pos,location_w_pos,end_warranty_date,hostname.1,http_link.1,id
        Data Center,datacenter,,,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,,,,datacenter
        Feed,device,feed,Data Center,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,,,,feed
        Server,device,server,Data Center,active,P1,,Feed,,{},,,,,,,,,,,,,,,,,,,,,,,,srv
        Server 1,device,server,Data Center,active,P1,,,,,,,,,,,,,,,,,,,,,,,,,,,,srv1
    )"_format(db.idByName("srv"));

    CHECK(*exp == csvTrim(data));
}
