/*  =========================================================================
    topology_shared_location_helpers - class description

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
    topology_shared_location_helpers -
@discuss
@end
*/

#include "location_helpers.h"

/*!
 * \file location_helpers.cc
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Jim Klimov <EvgenyKlimov@Eaton.com>
 * \brief Not yet documented file
 */

#include <list>
#include <fty_common_db_asset.h>
#include <fty_common.h>
#include <fty_common_utf8.h>

#include "utilspp.h"
#include <cleanup.h>

int asset_location_r(asset_msg_t** asset_msg, std::string& json)
{
    uint32_t element_id = asset_msg_element_id(*asset_msg);
    int type_id = asset_msg_type(*asset_msg);
    std::string name = asset_msg_name(*asset_msg);
    std::string type_name = asset_msg_type_name(*asset_msg);

    std::pair<std::string,std::string> elem_names = DBAssets::id_to_name_ext_name (element_id);
    if (element_id != 0 && elem_names.first.empty () && elem_names.second.empty ())
        return -1; //HTTP_INTERNAL_SERVER_ERROR;

    json += "{";
    json += "\"name\" : \"" + UTF8::escape (elem_names.second) + "\", ";
    json += "\"id\" : \"" + UTF8::escape (name) + "\",";
    json += "\"type\" : \"" + persist::typeid_to_type(static_cast<uint16_t>(type_id)) + "\",";
    if ( (type_id == persist::asset_type::DEVICE ) ||
         (type_id == persist::asset_type::GROUP) ) {
        json += "\"sub_type\" : \"" + utils::strip (type_name) + "\"";
    }
    else {
        json += "\"sub_type\" : \"N_A\"";
    }

    std::list<zframe_t *> frames;
    std::list<std::string> names;

    frames.push_back(asset_msg_get_dcs(*asset_msg));
    names.push_back("datacenters");
    frames.push_back(asset_msg_get_rooms(*asset_msg));
    names.push_back("rooms");
    frames.push_back(asset_msg_get_rows(*asset_msg));
    names.push_back("rows");
    frames.push_back(asset_msg_get_racks(*asset_msg));
    names.push_back("racks");
    frames.push_back(asset_msg_get_devices(*asset_msg));
    names.push_back("devices");
    frames.push_back(asset_msg_get_grps(*asset_msg));
    names.push_back("groups");

    asset_msg_destroy(asset_msg);

    bool first_contains = true;
    _scoped_zframe_t *it_f = NULL;
    _scoped_zmsg_t *zmsg = NULL;
    _scoped_asset_msg_t *item = NULL;

    while (!(frames.empty() || names.empty()))
    {
        it_f = frames.front();
        frames.pop_front();
        std::string name_it = names.front();
        names.pop_front();
        if (it_f == NULL)
            continue;
#if CZMQ_VERSION_MAJOR == 3
        byte *buffer = zframe_data(it_f);
        if (buffer == NULL)
            goto err_cleanup;
        zmsg = zmsg_decode(buffer, zframe_size(it_f));
#else
        zmsg = zmsg_decode (it_f);
#endif
        if (zmsg == NULL || !zmsg_is (zmsg))
            goto err_cleanup;
        zframe_destroy(&it_f);

        _scoped_zmsg_t *pop = NULL;
        bool first = true;
        while((pop = zmsg_popmsg(zmsg)) != NULL) { // caller owns zmgs_t
            if (!is_asset_msg (pop))
                goto err_cleanup;
            item = asset_msg_decode(&pop); // zmsg_t is freed
            if (item == NULL)
                goto err_cleanup;
            if (first == false) {
                json += ", ";
            }
            else {
                if (first_contains == false) {
                    json += ", ";
                } else {
                    first_contains = false;
                    json += ", \"contains\" : { ";
                }
                json += "\"" + UTF8::escape (name_it)+ "\" : [";
                first = false;
            }
            if (asset_location_r(&item, json) != 0) //HTTP_OK)
                goto err_cleanup;
            asset_msg_destroy(&item);
        }
        zmsg_destroy(&zmsg);

        if (first == false)
            json += "]";
    }

    if (!first_contains) {
        json += "}"; // level-1 "contains"
    }
    else if (type_id != persist::asset_type::DEVICE ) {
        json += ", \"contains\":[]";
    }

    json += "}"; // json closing curly bracket

    return 0; //HTTP_OK;

err_cleanup:
    zmsg_destroy (&zmsg);
    asset_msg_destroy (&item);
    zframe_destroy(&it_f);
    while (!(frames.empty())) {
        it_f = frames.front();
        frames.pop_front();
        zframe_destroy(&it_f);
    }
    json = "";
    return -2; //HTTP_INTERNAL_SERVER_ERROR;
}
