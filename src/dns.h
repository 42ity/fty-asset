/*  =========================================================================
    dns - DNS and networking helper

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

#ifndef DNS_H_INCLUDED
#define DNS_H_INCLUDED

//  @interface

FTY_ASSET_EXPORT std::set<std::string>
    name_to_ip4 (const char *name);

FTY_ASSET_EXPORT std::set<std::string>
    name_to_ip6 (const char *name);

FTY_ASSET_EXPORT std::set<std::string>
    name_to_ip (const char *name);

FTY_ASSET_EXPORT std::string
    ip_to_name (const char *ip);

FTY_ASSET_EXPORT std::map<std::string,std::set<std::string>>
    local_addresses();

FTY_ASSET_EXPORT void
    dns_test (bool verbose);

//  @end

#endif
