/*  =========================================================================
    total_power - Calculation of total power for one asset

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

/*
@header
    total_power - Calculation of total power for one asset
@discuss
@end
*/

#include "total_power.h"
#include <fty/asset/asset-db.h>
#include <fty_common.h>
#include <fty_common_db_connection.h>
#include <fty_log.h>

class ShortAssetInfo
{
public:
    uint32_t    asset_id;
    std::string asset_name;
    uint16_t    subtype_id;

    ShortAssetInfo(uint32_t aasset_id, const std::string& aasset_name, uint16_t asubtype_id)
    {
        asset_id   = aasset_id;
        asset_name = aasset_name;
        subtype_id = asubtype_id;
    };
};

inline bool operator<(const ShortAssetInfo& lhs, const ShortAssetInfo& rhs)
{
    return lhs.asset_id < rhs.asset_id;
}

/**
 * \brief Simple wrapper to make code more readable
 */
static bool isUps(const ShortAssetInfo& device)
{
    return device.subtype_id == persist::asset_subtype::UPS;
}

/**
 * \brief Simple wrapper to make code more readable
 */
static bool isEpdu(const ShortAssetInfo& device)
{
    return device.subtype_id == persist::asset_subtype::EPDU;
}

/**
 *  \brief For the specified asset it derives a set of powered devices
 *
 *  From set of links derives set of elements that at least once were
 *              in a role of a destination device in a link.
 *  Link is: from to
 *
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *  \param[in] element_id - powering device id
 *
 *  \return a set of powered devices. It can be empty.
 */
static std::set<uint32_t> findDests(const std::vector<fty::asset::db::DbAssetLink>& links, uint32_t elementId)
{
    std::set<uint32_t> dests;

    for (const auto& link : links) {
        if (link.srcId == elementId)
            dests.insert(link.destId);
    }
    return dests;
}
/**
 *  \brief Checks if some power device is directly powering devices
 *          in some other racks.
 *
 *  \param[in] device - device to check
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *
 *  \return true or false
 */
static bool is_powering_other_rack(
    const ShortAssetInfo&                           device,
    const std::map<uint32_t, ShortAssetInfo>&       containerDevices,
    const std::vector<fty::asset::db::DbAssetLink>& links)
{
    auto adevice_dests = findDests(links, device.asset_id);
    for (const auto& adevice : adevice_dests) {
        auto it = containerDevices.find(adevice);
        if (it == containerDevices.cend()) {
            // it means, that destination device is out of the container
            return true;
        }
    }
    return false;
}

/**
 *  \brief From old set of border devices generates a new set of border devices
 *
 *  Each device "A" is replaced with the list of devices that were powered by
 *  this device "A".
 *  A border device is a device, that should be taken as a power
 *  source device at first turn.
 *
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *  \param[in][out] border_devices - list of border devices to be updated
 */
static void updateBorderDevices(
    const std::map<uint32_t, ShortAssetInfo>&       containerDevices,
    const std::vector<fty::asset::db::DbAssetLink>& links,
    std::set<ShortAssetInfo>&                       borderDevices)
{
    std::set<ShortAssetInfo> newBorderDevices;
    for (const auto& borderDevice : borderDevices) {
        auto adevice_dests = findDests(links, borderDevice.asset_id);
        for (auto& adevice : adevice_dests) {
            auto it = containerDevices.find(adevice);
            if (it != containerDevices.cend())
                newBorderDevices.insert(it->second);
            else {
                logError("DB can be in inconsistant state or some device has power source in the other container");
                logError("device(as element) {} is not in container", adevice);
                // do nothing in this case
            }
        }
    }
    borderDevices.clear();
    borderDevices.insert(newBorderDevices.begin(), newBorderDevices.end());
}

/**
 *  \brief An implementation of the algorithm.
 *
 *  GOAL: Take the first "smart" devices in every powerchain that are
 *        the closest to "feed".
 *
 *  If the found device is not smart, try to look at upper level. Repeat until
 *  chain ends or until all chains are processed.
 *
 *  Asset container - is an asset for which we want to compute the totl power
 *  Asset container devices - all devices placed in the asset container.
 *
 *  \param[in] devices_in_container - information about all devices in the
 *                          asset container
 *  \param[in] links - information about all links, where at least one end
 *                          belongs to the devices in the container
 *
 *  \return a list of power devices names. It there are no power devices the
 *          list is empty.
 */
static std::vector<std::string> totalPowerV2(
    const std::map<uint32_t, ShortAssetInfo>& containerDevices, const std::vector<fty::asset::db::DbAssetLink>& links)
{

    // the set of all border devices ("starting points")
    std::set<ShortAssetInfo> borderDevices;
    // the set of all destination devices in selected links
    std::set<uint32_t> destDevices;
    //  from (first)   to (second)
    //           +--------------+
    //  B________|______A__C    |
    //           |              |
    //           +--------------+
    //   B is out of the Container
    //   A is in the Container
    //   then A is border device
    for (auto& oneLink : links) {
        logDebug("  cur_link: {}->{}", oneLink.srcId, oneLink.destId);
        auto it = containerDevices.find(oneLink.srcId);
        if (it == containerDevices.end())
        // if in the link first point is out of the Container,
        // the second definitely should be in Container,
        // otherwise it is not a "container"-link
        {
            borderDevices.insert(containerDevices.find(oneLink.destId)->second);
        }
        destDevices.insert(oneLink.destId);
    }
    //  from (first)   to (second)
    //           +-----------+
    //           |A_____C    |
    //           |           |
    //           +-----------+
    //   A is in the Container (from)
    //   C is in the Container (to)
    //   then A is border device
    //
    //   Algorithm: from all devices in the Container we will
    //   select only those that don't have an incoming links
    //   (they are not a destination device for any link)
    for (auto& oneDevice : containerDevices) {
        if (destDevices.find(oneDevice.first) == destDevices.end()) {
            borderDevices.insert(oneDevice.second);
        }
    }

    std::vector<std::string> dvc;
    if (borderDevices.empty()) {
        return dvc;
    }

    // it is not a good idea to delete from collection while iterating it
    std::set<ShortAssetInfo> todelete;
    while (!borderDevices.empty()) {
        for (const auto& border_device : borderDevices) {
            if ((isEpdu(border_device)) ||
                ((isUps(border_device)) && (!is_powering_other_rack(border_device, containerDevices, links)))) {
                dvc.push_back(border_device.asset_name);
                // remove from border
                todelete.insert(border_device);
                continue;
            }
        }
        for (const auto& todel : todelete) {
            borderDevices.erase(todel);
        }
        updateBorderDevices(containerDevices, links, borderDevices);
    }
    return dvc;
}

/**
 *  \brief For the specified asset finds out the devices
 *        that are used for total power computation
 *
 *  \param[in] assetId - id of the asset, for which we need to
 *                       determine devices for total power calculation
 *  \param[out] powerDevices - list of devices for the specified asset
 *                       used for total power computation.
 *                       It's content would be cleared every time
 *                       at the beginning.
 *
 *  \return  0 - in case of success
 *          -1 - in case of internal error
 *          -2 - in case the requested asset was not found
 */
static fty::Expected<void> selectTotalPowerById(uint32_t assetId, std::vector<std::string>& powerDevices)
{
    // select all devices in the container
    std::map<uint32_t, ShortAssetInfo> containerDevices;

    auto func = [&](const fty::db::Row& row) {
        if (row.get<uint16_t>("type_id") != persist::asset_type::DEVICE) {
            return;
        }

        // clang-format off
        uint32_t id = row.get<uint32_t>("asset_id");
        containerDevices.emplace(id, ShortAssetInfo(
            id,
            row.get("name"),
            row.get<uint16_t>("subtype_id")
        ));
        // clang-format on
    };

    if (auto ret = fty::asset::db::selectAssetsByContainer(assetId, func); !ret) {
        logWarn("asset_id='{}': problems appeared in selecting devices", assetId);
        return fty::unexpected(ret.error());
    }

    if (containerDevices.empty()) {
        logDebug("asset_id='{}': has no devices", assetId);
        return fty::unexpected("asset_id='{}': has no devices", assetId);
    }

    auto active = fty::asset::db::selectLinksByContainer(assetId, "active");
    if (!active) {
        logWarn("asset_id='{}': internal problems in links detecting: {}", assetId, active.error());
        return fty::unexpected("asset_id='{}': internal problems in links detecting: {}", assetId, active.error());
    }

    std::vector<fty::asset::db::DbAssetLink> links = *active;

    auto nonActive = fty::asset::db::selectLinksByContainer(assetId, "nonactive");
    if (!nonActive) {
        logWarn("asset_id='{}': internal problems in links detecting: {}", assetId, nonActive.error());
        return fty::unexpected("asset_id='{}': internal problems in links detecting: {}", assetId, nonActive.error());
    }

    links.insert(links.begin(), nonActive->begin(), nonActive->end());

    if (links.empty()) {
        logDebug("asset_id='{}': has no power links", assetId);
        return fty::unexpected("asset_id='{}': has no power links", assetId);
    }

    powerDevices = totalPowerV2(containerDevices, links);
    return {};
}

fty::Expected<std::vector<std::string>> selectDevicesTotalPower(const std::string& assetName, bool test)
{
    std::vector<std::string> powerDevices;
    if (test)
        return powerDevices;

    auto assetId = fty::asset::db::nameToAssetId(assetName);
    if (!assetId) {
        return fty::unexpected(assetId.error());
    }
    if (auto ret = selectTotalPowerById(*assetId, powerDevices); !ret) {
        return fty::unexpected(ret.error());
    }
    return powerDevices;
}
