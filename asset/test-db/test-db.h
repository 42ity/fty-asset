#pragma once
#include <fty/expected.h>

namespace fty {

// =====================================================================================================================

class TestDb
{
public:
    static Expected<void> init();
    static TestDb&        instance();

    Expected<std::string> create();
    static void           destroy();

private:
    std::string m_path;
    bool        m_inited = false;
};


} // namespace fty
