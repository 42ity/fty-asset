/*  =========================================================================
    topology_processor - Topology requests processor

    Copyright (C) 2014 - 2020 Eaton

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

#include "topology_processor.h"
#include "topology_input_powerchain.h"
#include "topology_location.h"
#include "topology_power.h"
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>
#include <fty_common.h>
#include <fty_log.h>
#include <map>

//==========================================================================================================================================

static fty::Expected<std::string> siToString(const cxxtools::SerializationInfo& si, bool beautify)
{
    try {
        std::ostringstream       output;
        cxxtools::JsonSerializer js;
        js.beautify(beautify);
        js.begin(output);
        js.serialize(si);
        js.finish();
        return output.str();
    } catch (...) {
        log_error("Exception reached");
    }
    return fty::unexpected("Cannot serialize info into json string");
}

static fty::Expected<cxxtools::SerializationInfo> stringToSi(const std::string& str)
{
    try {
        cxxtools::SerializationInfo si;
        std::istringstream          input(str);
        cxxtools::JsonDeserializer  deserializer(input);
        deserializer.deserialize(si);
        return si; // ok
    } catch (...) {
        log_error("Exception reached");
    }
    return fty::unexpected("Cannot deserialize json string");
}

static fty::Expected<std::string> memberValue(const cxxtools::SerializationInfo& si, const std::string& member)
{
    const cxxtools::SerializationInfo* m = si.findMember(member);
    if (!m || m->isNull())
        return fty::unexpected("not found");

    std::string value;
    m->getValue(value);
    return value;
}

static fty::Expected<std::string> jsonStringBeautify(const std::string& str)
{
    if (auto si = stringToSi(str)) {
        if (auto beauty = siToString(*si, true)) {
            return *beauty;
        } else {
            return fty::unexpected(beauty.error());
        }
    } else {
        return fty::unexpected(si.error());
    }
}

//==========================================================================================================================================

fty::Expected<std::string> topologyPowerProcess(const std::string& command, const std::string& assetName, bool beautify)
{
    std::string result;

    std::map<std::string, std::string> param;
    param[command] = assetName;

    int r = topology_power(param, result);
    if (r != 0) {
        logError("topology_power() failed, error: {}, command: {}, assetName: {}", r, command, assetName);
        return fty::unexpected(param["error"]);
    }

    // beautify result, optional
    if (beautify) {
        if (auto ret = jsonStringBeautify(result)) {
            result = *ret;
        } else {
            logError("beautification failed, error: {}, result: \n{}", ret.error(), result);
            return fty::unexpected(TRANSLATE_ME("JSON beautification failed"));
        }
    }

    logDebug("topology_power_process() success, command: {}, assetName: {}, result:\n{}", command, assetName, result);

    return result;
}

//==========================================================================================================================================

fty::Expected<std::string> topologyPowerTo(const std::string& assetName, bool beautify)
{
    std::string result;

    if (auto ret = topologyPowerProcess("to", assetName, beautify)) {
        result = *ret;
    } else {
        logError("topology_power_process 'to' failed, error: {}", ret.error());
        return fty::unexpected(ret.error());
    }

    cxxtools::SerializationInfo* si_powerchains = nullptr;
    // deserialize result to si
    if (auto si = stringToSi(result)) {
        si_powerchains = si->findMember("powerchains");
        if (si_powerchains == 0) {
            logError("powerchains member not defined, payload:\n{}", result);
            return fty::unexpected(TRANSLATE_ME("Internal error"));
        }

        if (si_powerchains->category() != cxxtools::SerializationInfo::Category::Array) {
            logError("powerchains member category != Array, payload:\n{}", result);
            return fty::unexpected(TRANSLATE_ME("Internal error"));
        }
    } else {
        logError("json deserialize failed (exception reached), payload:\n{}", result);
        return fty::unexpected(TRANSLATE_ME("JSON deserialization failed"));
    }

    if (!si_powerchains) {
        logError("json deserialize failed, payload:\n{}", result);
        return fty::unexpected(TRANSLATE_ME("JSON deserialization failed"));
    }

    result.clear(); // useless, we work now on siResult

    // prepare siResult (asset-id member + powerchains array member)
    cxxtools::SerializationInfo siResult;
    siResult.addMember("asset-id") <<= assetName;
    siResult.addMember("powerchains").setCategory(cxxtools::SerializationInfo::Array);
    cxxtools::SerializationInfo* siResulPowerchains = siResult.findMember("powerchains");
    if (!siResulPowerchains) {
        logError("powerchains member creation failed");
        return fty::unexpected(TRANSLATE_ME("Internal error"));
    }

    // powerchains member is defined (array), loop on si_powerchains items...
    // filter on dst-id == assetName, check src-id & src-socket,
    // keep entries in siResulPowerchains
    for (const auto& xsi : *si_powerchains) {
        const cxxtools::SerializationInfo* si_dstId = xsi.findMember("dst-id");
        if (!si_dstId || si_dstId->isNull()) {
            logError("dst-id member is missing");
            continue;
        }

        std::string dstId;
        si_dstId->getValue(dstId);
        if (dstId != assetName) {
            continue; // filtered
        }

        const cxxtools::SerializationInfo *si_srcId, *si_srcSock;
        si_srcId = xsi.findMember("src-id");
        if (si_srcId == 0 || si_srcId->isNull()) {
            logError("src-id member is missing");
            continue;
        }
        si_srcSock = xsi.findMember("src-socket");
        if (si_srcSock == 0 || si_srcSock->isNull()) {
            logError("src-socket member is missing");
            continue;
        }

        // keep that entry
        siResulPowerchains->addMember("") <<= xsi;
    }

    // dump siResult to result
    if (auto ret = siToString(siResult, beautify)) {
        result = *ret;
    } else {
        logError("serialization to JSON has failed");
        return fty::unexpected(TRANSLATE_ME("JSON serialization failed"));
    }

    logDebug("topology_power_to() success, assetName: {}, result:\n{}", assetName, result);

    return result;
}

//==========================================================================================================================================

fty::Expected<std::string> topologyLocationProcess(
    const std::string& command, const std::string& assetName, const std::string& options, bool beautify)
{
    std::string result;

    std::map<std::string, std::string> param;

    param[command] = assetName;

    // filter/check command, set options
    if (command == "to") {
        if (!options.empty()) {
            logError("unexpected options for command 'to' (options: {})", options);
            return fty::unexpected(TRANSLATE_ME("'option' argument is not allowed with 'to' command"));
        }
    } else if (command == "from") {
        // defaults
        param["recursive"] = "false";
        param["feed_by"]   = "";
        param["filter"]    = "";

        // parse options (ignore unreferenced entry)
        if (!options.empty()) {
            auto si = stringToSi(options);
            if (!si) {
                logError("options malformed, error: {}, options: {}", si.error(), options);
                return fty::unexpected(TRANSLATE_ME("'option' argument describe an invalid JSON payload"));
            }

            if (auto ret = memberValue(*si, "recursive")) {
                param["recursive"] = *ret;
            }
            if (auto ret = memberValue(*si, "feed_by")) {
                param["feed_by"] = *ret;
            }
            if (auto ret = memberValue(*si, "filter")) {
                param["filter"] = *ret;
            }
        }
    } else {
        logError("unexpected '{}' command  (shall be 'to'/'from')", command);
        return fty::unexpected(TRANSLATE_ME("Unexpected '%s' command (shall be 'to'/'from')", command.c_str()));
    }

    // trace param map
    for (const auto& [key, val] : param) {
        log_trace("param['%s'] = '%s'", key.c_str(), val.c_str());
    }

    // request
    int r = topology_location(param, result);
    if (r != 0) {
        logError("topology_location() failed, r: {}, command: {}, assetName: {}", r, command, assetName);
        return fty::unexpected(param["error"]);
    }

    // beautify result, optional
    if (beautify) {
        if (auto res = jsonStringBeautify(result)) {
            result = *res;
        } else {
            logError("beautification failed, error: {}, result: \n{}", res.error(), result);
            return fty::unexpected(TRANSLATE_ME("JSON beautification failed"));
        }
    }

    logDebug("topology_location() success, assetName: {}, result:\n{}", assetName, result);

    return result;
}

//==========================================================================================================================================

fty::Expected<std::string> topologyInputPowerchainProcess(const std::string& assetName, bool beautify)
{
    std::string result;

    std::map<std::string, std::string> param;
    param["id"] = assetName;

    // request
    int r = topology_input_powerchain(param, result);
    if (r != 0) {
        logError("topology_input_powerchain() failed, r: {}, assetName: {}", r, assetName);
        return fty::unexpected(param["error"]);
    }

    // beautify result, optional
    if (beautify) {
        if (auto ret = jsonStringBeautify(result)) {
            result = *ret;
        } else {
            logError("beautification failed, error:{}\n{}", ret.error(), result);
            return fty::unexpected(TRANSLATE_ME("JSON beautification failed"));
        }
    }

    logDebug("topology_input_powerchain() success, assetName: {}, result:\n{}", assetName, result);

    return result;
}

//==========================================================================================================================================
