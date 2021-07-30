#pragma once
#include <fty/expected.h>
#include <map>
#include <vector>

#if defined(CATCH_VERSION_MAJOR)
#define REQUIRE_EXP(expr)\
    if (!expr) {\
        FAIL(expr.error().message()); \
    }
#endif

namespace fty {

// =====================================================================================================================

struct SampleDb
{
    SampleDb(const std::string& data);
    ~SampleDb();

    uint32_t idByName(const std::string& name);

private:
    std::vector<uint32_t>           m_ids;
    std::map<std::string, uint32_t> m_mapping;
    std::vector<int64_t>            m_links;
};

// =====================================================================================================================

} // namespace fty
