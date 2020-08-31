/*  =========================================================================
    fty_asset_dto - Asset DTO: defines ASSET attributes for data exchange

    Copyright (C) 2016 - 2020 Eaton

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
    =========================================================================
*/

/*
@header
    fty_asset_dto - Asset DTO: defines ASSET attributes for data exchange
@discuss
@end
*/

#include "include/fty_asset_dto.h"
#include <algorithm>
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>
#include <sstream>

namespace fty {

AssetLink::AssetLink(const std::string& s, std::string o, std::string i, int t)
    : sourceId(s)
    , srcOut(o)
    , destIn(i)
    , linkType(t)
{
}

bool operator==(const AssetLink& l, const AssetLink& r)
{
    return (
        l.sourceId == r.sourceId && l.srcOut == r.srcOut && l.destIn == r.destIn && r.linkType == l.linkType);
}

void operator<<=(cxxtools::SerializationInfo& si, const AssetLink& l)
{
    si.addMember("source") <<= l.sourceId;
    si.addMember("link_type") <<= l.linkType;
    if (!l.srcOut.empty()) {
        si.addMember("src_out") <<= l.srcOut;
    }
    if (!l.destIn.empty()) {
        si.addMember("dest_in") <<= l.destIn;
    }
}

void operator>>=(const cxxtools::SerializationInfo& si, AssetLink& l)
{
    si.getMember("source") >>= l.sourceId;
    si.getMember("link_type") >>= l.linkType;

    std::string tmpStr;
    if (si.getMember("src_out", tmpStr)) {
        l.srcOut = tmpStr;
    }

    if (si.getMember("dest_in", tmpStr)) {
        l.destIn = tmpStr;
    }
} // namespace fty


const std::string assetStatusToString(AssetStatus status)
{
    std::string retVal;

    switch (status) {
        case AssetStatus::Active:
            retVal = "active";
            break;
        case AssetStatus::Nonactive:
            retVal = "nonactive";
            break;
        case AssetStatus::Unknown:
        default:
            retVal = "unknown";
            break;
    }

    return retVal;
}

AssetStatus stringToAssetStatus(const std::string& str)
{
    AssetStatus retVal = AssetStatus::Unknown;

    if (str == "active") {
        retVal = AssetStatus::Active;
    } else if (str == "nonactive") {
        retVal = AssetStatus::Nonactive;
    }

    return retVal;
}

// getters

const std::string& Asset::getInternalName() const
{
    return m_internalName;
}

AssetStatus Asset::getAssetStatus() const
{
    return m_assetStatus;
}

const std::string& Asset::getAssetType() const
{
    return m_assetType;
}

const std::string& Asset::getAssetSubtype() const
{
    return m_assetSubtype;
}

const std::string& Asset::getParentIname() const
{
    return m_parentIname;
}

int Asset::getPriority() const
{
    return m_priority;
}

const std::string& Asset::getAssetTag() const
{
    return m_assetTag;
}

const Asset::ExtMap& Asset::getExt() const
{
    return m_ext;
}

const std::string& Asset::getSecondaryID() const
{
    return m_secondaryID;
}

const std::string& Asset::getExtEntry(const std::string& key) const
{
    static const std::string extNotFound;

    auto search = m_ext.find(key);

    if (search != m_ext.end()) {
        return search->second.getValue();
    }

    return extNotFound;
}

bool Asset::isExtEntryReadOnly(const std::string& key) const
{
    auto search = m_ext.find(key);

    if (search != m_ext.end()) {
        return search->second.isReadOnly();
    }

    return false;
}

// ext param getters
const std::string& Asset::getUuid() const
{
    return getExtEntry(EXT_UUID);
}

const std::string& Asset::getManufacturer() const
{
    return getExtEntry(EXT_MANUFACTURER);
}

const std::string& Asset::getModel() const
{
    return getExtEntry(EXT_MODEL);
}

const std::string& Asset::getSerialNo() const
{
    return getExtEntry(EXT_SERIAL_NO);
}

const std::vector<AssetLink>& Asset::getLinkedAssets() const
{
    return m_linkedAssets;
}

const std::optional<std::vector<Asset>> Asset::getParentsList() const
{
    return m_parentsList;
}

// setters

void Asset::setInternalName(const std::string& internalName)
{
    m_internalName = internalName;
}

void Asset::setAssetStatus(AssetStatus assetStatus)
{
    m_assetStatus = assetStatus;
}

void Asset::setAssetType(const std::string& assetType)
{
    m_assetType = assetType;
}

void Asset::setAssetSubtype(const std::string& assetSubtype)
{
    m_assetSubtype = assetSubtype;
}

void Asset::setParentIname(const std::string& parentIname)
{
    m_parentIname = parentIname;
}

void Asset::setPriority(int priority)
{
    m_priority = priority;
}

void Asset::setAssetTag(const std::string& assetTag)
{
    m_assetTag = assetTag;
}

void Asset::setExtMap(const ExtMap& map)
{
    m_ext = map;
}

void Asset::clearExtMap()
{
    m_ext.clear();
}

void Asset::setExtEntry(const std::string& key, const std::string& value, bool readOnly, bool wasUpdated)
{
    ExtMapElement element(value, readOnly, wasUpdated);

    m_ext[key] = element;
}

void Asset::setLinkedAssets(const std::vector<AssetLink>& assets)
{
    m_linkedAssets = assets;
}

void Asset::setSecondaryID(const std::string& secondaryID)
{
    m_secondaryID = secondaryID;
}

void Asset::dump(std::ostream& os)
{
    os << "iname       : " << m_internalName << std::endl;
    os << "status      : " << assetStatusToString(m_assetStatus) << std::endl;
    os << "type        : " << m_assetType << std::endl;
    os << "subtype     : " << m_assetSubtype << std::endl;
    os << "parent      : " << m_parentIname << std::endl;
    os << "priority    : " << m_priority << std::endl;
    os << "tag         : " << m_assetTag << std::endl;
    os << "secondaryID : " << m_secondaryID << std::endl;

    for (const auto& e : m_ext) {
        os << "- key: " << e.first << " - value: " << e.second.getValue()
           << (e.second.isReadOnly() ? " [ReadOny]" : "") << (e.second.wasUpdated() ? " [Updated]" : "")
           << std::endl;
    }

    for (const auto& l : m_linkedAssets) {
        os << "- linked to: " << l.sourceId << " on port: " << l.srcOut << " from port " << l.destIn
           << std::endl;
    }
}

bool Asset::operator==(const Asset& asset) const
{
    return (m_internalName == asset.m_internalName && m_assetStatus == asset.m_assetStatus &&
            m_assetType == asset.m_assetType && m_assetSubtype == asset.m_assetSubtype &&
            m_parentIname == asset.m_parentIname && m_priority == asset.m_priority &&
            m_assetTag == asset.m_assetTag && m_ext == asset.m_ext && m_linkedAssets == asset.m_linkedAssets);
}

bool Asset::operator!=(const Asset& asset) const
{
    return !(*this == asset);
}

static constexpr const char* SI_STATUS       = "status";
static constexpr const char* SI_TYPE         = "type";
static constexpr const char* SI_SUB_TYPE     = "sub_type";
static constexpr const char* SI_NAME         = "name";
static constexpr const char* SI_PRIORITY     = "priority";
static constexpr const char* SI_PARENT       = "parent";
static constexpr const char* SI_EXT          = "ext";
static constexpr const char* SI_LINKED       = "linked";
static constexpr const char* SI_PARENTS_LIST = "parents_list";

void Asset::serialize(cxxtools::SerializationInfo& si) const
{
    si.addMember(SI_STATUS) <<= int(m_assetStatus);
    si.addMember(SI_TYPE) <<= m_assetType;
    si.addMember(SI_SUB_TYPE) <<= m_assetSubtype;
    si.addMember(SI_NAME) <<= m_internalName;
    si.addMember(SI_PRIORITY) <<= m_priority;
    si.addMember(SI_PARENT) <<= m_parentIname;
    si.addMember(SI_LINKED) <<= m_linkedAssets;
    // ext
    cxxtools::SerializationInfo& ext = si.addMember("");

    cxxtools::SerializationInfo data;
    for (const auto& e : m_ext) {
        cxxtools::SerializationInfo& entry = data.addMember(e.first);
        entry <<= e.second;
    }
    data.setCategory(cxxtools::SerializationInfo::Category::Object);
    ext = data;
    ext.setName(SI_EXT);

    if (m_parentsList.has_value()) {
        si.addMember(SI_PARENTS_LIST) <<= m_parentsList.value();
    }
}

void Asset::deserialize(const cxxtools::SerializationInfo& si)
{
    int tmpInt = 0;

    si.getMember(SI_STATUS) >>= tmpInt;
    m_assetStatus = AssetStatus(tmpInt);

    si.getMember(SI_TYPE) >>= m_assetType;
    si.getMember(SI_SUB_TYPE) >>= m_assetSubtype;
    si.getMember(SI_NAME) >>= m_internalName;
    si.getMember(SI_PRIORITY) >>= m_priority;
    si.getMember(SI_PARENT) >>= m_parentIname;
    si.getMember(SI_LINKED) >>= m_linkedAssets;

    // ext map
    const cxxtools::SerializationInfo ext = si.getMember(SI_EXT);
    for (const auto& si : ext) {
        std::string key = si.name();
        ExtMapElement element;
        si >>= element;
        m_ext[key] = element;
    }

    if (si.findMember(SI_PARENTS_LIST) != nullptr) {
        std::vector<Asset> parentsList;
        si.getMember(SI_PARENTS_LIST) >>= parentsList;
        m_parentsList = parentsList;
    }
}

void operator<<=(cxxtools::SerializationInfo& si, const fty::Asset& asset)
{
    asset.serialize(si);
}

void operator>>=(const cxxtools::SerializationInfo& si, fty::Asset& asset)
{
    asset.deserialize(si);
}

ExtMapElement::ExtMapElement(const std::string& val, bool readOnly, bool wasUpdated)
{
    m_value      = val;
    m_readOnly   = readOnly;
    m_wasUpdated = wasUpdated;
}

ExtMapElement::ExtMapElement(const ExtMapElement& element)
{
    m_value      = element.m_value;
    m_readOnly   = element.m_readOnly;
    m_wasUpdated = element.m_wasUpdated;
}

ExtMapElement::ExtMapElement(ExtMapElement&& element)
{
    m_value      = element.m_value;
    m_readOnly   = element.m_readOnly;
    m_wasUpdated = element.m_wasUpdated;

    element.m_value      = std::string();
    element.m_readOnly   = false;
    element.m_wasUpdated = false;
}

ExtMapElement& ExtMapElement::operator=(const ExtMapElement& element)
{
    m_value      = element.m_value;
    m_readOnly   = element.m_readOnly;
    m_wasUpdated = element.m_wasUpdated;
    return *this;
}

ExtMapElement& ExtMapElement::operator=(ExtMapElement&& element)
{
    m_value      = element.m_value;
    m_readOnly   = element.m_readOnly;
    m_wasUpdated = element.m_wasUpdated;
    return *this;
}

const std::string& ExtMapElement::getValue() const
{
    return m_value;
}

bool ExtMapElement::isReadOnly() const
{
    return m_readOnly;
}

bool ExtMapElement::wasUpdated() const
{
    return m_wasUpdated;
}

void ExtMapElement::setValue(const std::string& val)
{
    m_value = val;
}

void ExtMapElement::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

void ExtMapElement::setWasUpdated(bool wasUpdated)
{
    m_wasUpdated = wasUpdated;
}

bool ExtMapElement::operator==(const ExtMapElement& element) const
{
    return (m_value == element.m_value && m_readOnly == element.m_readOnly);
}

bool ExtMapElement::operator!=(const ExtMapElement& element) const
{
    return !(*this == element);
}

static constexpr const char* SI_EXT_MAP_ELEMENT_VALUE    = "value";
static constexpr const char* SI_EXT_MAP_ELEMENT_READONLY = "readOnly";
static constexpr const char* SI_EXT_MAP_ELEMENT_UPDATED  = "update";

// serialization / deserialization for cxxtools
void ExtMapElement::serialize(cxxtools::SerializationInfo& si) const
{
    si.addMember(SI_EXT_MAP_ELEMENT_VALUE) <<= m_value;
    si.addMember(SI_EXT_MAP_ELEMENT_READONLY) <<= m_readOnly;
    si.addMember(SI_EXT_MAP_ELEMENT_UPDATED) <<= m_wasUpdated;
}

void ExtMapElement::deserialize(const cxxtools::SerializationInfo& si)
{
    si.getMember(SI_EXT_MAP_ELEMENT_VALUE) >>= m_value;
    si.getMember(SI_EXT_MAP_ELEMENT_READONLY) >>= m_readOnly;
    si.getMember(SI_EXT_MAP_ELEMENT_UPDATED) >>= m_wasUpdated;
}

void operator<<=(cxxtools::SerializationInfo& si, const ExtMapElement& e)
{
    e.serialize(si);
}

void operator>>=(const cxxtools::SerializationInfo& si, ExtMapElement& e)
{
    e.deserialize(si);
}


UIAsset::UIAsset(const Asset& a)
    : Asset(a)
{
}

void UIAsset::serializeUI(cxxtools::SerializationInfo& /*si*/) const
{
}

void UIAsset::deserializeUI(const cxxtools::SerializationInfo& /*si*/)
{
}


} // namespace fty

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

// color output definition for test function
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

void fty_asset_dto_test(bool verbose)
{
    printf(" * fty_asset_dto: ");

    std::vector<std::pair<std::string, bool>> testsResults;

    std::string testNumber;
    std::string testName;

    // Next test
    testNumber = "1.1";
    testName   = "Create fty::Asset object";
    printf(
        "\n----------------------------------------------------------------"
        "-------\n");
    {
        printf(" *=>  Test #%s %s\n", testNumber.c_str(), testName.c_str());

        try {
            using namespace fty;

            Asset asset;

            printf(" *<=  Test #%s > OK\n", testNumber.c_str());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, true);
        } catch (const std::exception& e) {
            printf(" *<=  Test #%s > Failed\n", testNumber.c_str());
            printf("Error: %s\n", e.what());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, false);
        }
    }

    // Next test
    testNumber = "2.1";
    testName   = "Equality check - good case";
    printf(
        "\n----------------------------------------------------------------"
        "-------\n");
    {
        printf(" *=>  Test #%s %s\n", testNumber.c_str(), testName.c_str());

        try {
            using namespace fty;

            Asset asset;
            asset.setInternalName("dc-0");
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            asset.setParentIname("abc123");
            asset.setExtEntry("testKey", "testValue");
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            if (!(asset == asset2)) {
                throw std::runtime_error("Equality check failed");
            }

            printf(" *<=  Test #%s > OK\n", testNumber.c_str());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, true);
        } catch (const std::exception& e) {
            printf(" *<=  Test #%s > Failed\n", testNumber.c_str());
            printf("Error: %s\n", e.what());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, false);
        }
    }

    // Next test
    testNumber = "2.2";
    testName   = "Equality check - bad case";
    printf(
        "\n----------------------------------------------------------------"
        "-------\n");
    {
        printf(" *=>  Test #%s %s\n", testNumber.c_str(), testName.c_str());

        try {
            using namespace fty;

            Asset asset;
            asset.setInternalName("dc-0");
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            asset.setParentIname("abc123");
            asset.setExtEntry("testKey", "testValue");
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            asset2.setParentIname("wrong-name");

            if (asset == asset2) {
                throw std::runtime_error("Equality check failed");
            }

            printf(" *<=  Test #%s > OK\n", testNumber.c_str());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, true);
        } catch (const std::exception& e) {
            printf(" *<=  Test #%s > Failed\n", testNumber.c_str());
            printf("Error: %s\n", e.what());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, false);
        }
    }

    // Next test
    testNumber = "2.3";
    testName   = "Inequality check - good case";
    printf(
        "\n----------------------------------------------------------------"
        "-------\n");
    {
        printf(" *=>  Test #%s %s\n", testNumber.c_str(), testName.c_str());

        try {
            using namespace fty;

            Asset asset;
            asset.setInternalName("dc-0");
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            asset.setParentIname("abc123");
            asset.setExtEntry("testKey", "testValue");
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;
            asset2.setAssetType(SUB_RACK_CONTROLLER);

            if (!(asset != asset2)) {
                throw std::runtime_error("Inequality check failed");
            }

            printf(" *<=  Test #%s > OK\n", testNumber.c_str());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, true);
        } catch (const std::exception& e) {
            printf(" *<=  Test #%s > Failed\n", testNumber.c_str());
            printf("Error: %s\n", e.what());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, false);
        }
    }

    // Next test
    testNumber = "2.4";
    testName   = "Inequality check - bad case";
    printf(
        "\n----------------------------------------------------------------"
        "-------\n");
    {
        printf(" *=>  Test #%s %s\n", testNumber.c_str(), testName.c_str());

        try {
            using namespace fty;

            Asset asset;
            asset.setInternalName("dc-0");
            asset.setAssetStatus(AssetStatus::Nonactive);
            asset.setAssetType(TYPE_DEVICE);
            asset.setAssetSubtype(SUB_UPS);
            asset.setParentIname("abc123");
            asset.setExtEntry("testKey", "testValue");
            asset.setPriority(4);

            Asset asset2;
            asset2 = asset;

            if (asset != asset2) {
                throw std::runtime_error("Inequality check failed");
            }

            printf(" *<=  Test #%s > OK\n", testNumber.c_str());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, true);
        } catch (const std::exception& e) {
            printf(" *<=  Test #%s > Failed\n", testNumber.c_str());
            printf("Error: %s\n", e.what());
            testsResults.emplace_back(" Test #" + testNumber + " " + testName, false);
        }
    }

    // collect results

    printf("\n-----------------------------------------------------------------------\n");

    uint32_t testsPassed = 0;
    uint32_t testsFailed = 0;


    printf("\tSummary tests from fty_asset_dto\n");
    for (const auto& result : testsResults) {
        if (result.second) {
            printf(ANSI_COLOR_GREEN "\tOK " ANSI_COLOR_RESET "\t%s\n", result.first.c_str());
            testsPassed++;
        } else {
            printf(ANSI_COLOR_RED "\tNOK" ANSI_COLOR_RESET "\t%s\n", result.first.c_str());
            testsFailed++;
        }
    }

    printf("\n-----------------------------------------------------------------------\n");

    if (testsFailed == 0) {
        printf(ANSI_COLOR_GREEN "\n %i tests passed, everything is ok\n" ANSI_COLOR_RESET "\n", testsPassed);
    } else {
        printf(ANSI_COLOR_RED "\n!!!!!!!! %i/%i tests did not pass !!!!!!!! \n" ANSI_COLOR_RESET "\n",
            testsFailed, (testsPassed + testsFailed));

        assert(false);
    }
}
