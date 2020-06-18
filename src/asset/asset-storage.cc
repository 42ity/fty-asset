/*  =========================================================================
    asset_asset_storage - asset/asset-storage

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
    asset_asset_storage - asset/asset-storage
@discuss
@end
*/

#include "fty_asset_classes.h"

//  Structure of our class

struct _asset_asset_storage_t {
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new asset_asset_storage

asset_asset_storage_t *
asset_asset_storage_new (void)
{
    asset_asset_storage_t *self = (asset_asset_storage_t *) zmalloc (sizeof (asset_asset_storage_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the asset_asset_storage

void
asset_asset_storage_destroy (asset_asset_storage_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        asset_asset_storage_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

