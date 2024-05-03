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

#include <catch2/catch.hpp>
#include "../src/topology/msg/asset_msg.h"
#include <iostream>

// TEST_CASE disabled due to assertion on socket type support.
// bios code, deprecated, not used and untested.

TEST_CASE("asset_msg test", "[.disabled]")
{
    printf (" * asset_msg: \n");

    // TODO: properly fix this in template zproto : zproto_codec_c_v1.gsl
    // so as to not lose the fix upon regeneration of code

    //  @selftest
    //  Simple create/destroy test
    asset_msg_t *self = asset_msg_new (0);
    assert (self);
    asset_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-asset_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-asset_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    asset_msg_t *copy;
    self = asset_msg_new (ASSET_MSG_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_name (self, "Life is short but Now lasts for ever");
    asset_msg_set_location (self, 123);
    asset_msg_set_location_type (self, 123);
    asset_msg_set_type (self, 123);
    asset_msg_ext_insert (self, "Name", "Brutus");
    asset_msg_ext_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (streq (asset_msg_name (self), "Life is short but Now lasts for ever"));
        assert (asset_msg_location (self) == 123);
        assert (asset_msg_location_type (self) == 123);
        assert (asset_msg_type (self) == 123);
        assert (asset_msg_ext_size (self) == 2);
        assert (streq (asset_msg_ext_string (self, "Name", "?"), "Brutus"));
        assert (asset_msg_ext_number (self, "Age", 0) == 43);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_DEVICE);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_device_type (self, "Life is short but Now lasts for ever");
    asset_msg_groups_append (self, "Name: %s", "Brutus");
    asset_msg_groups_append (self, "Age: %d", 43);
    asset_msg_powers_append (self, "Name: %s", "Brutus");
    asset_msg_powers_append (self, "Age: %d", 43);
    asset_msg_set_ip (self, "Life is short but Now lasts for ever");
    asset_msg_set_hostname (self, "Life is short but Now lasts for ever");
    asset_msg_set_fqdn (self, "Life is short but Now lasts for ever");
    asset_msg_set_mac (self, "Life is short but Now lasts for ever");
    zmsg_t *device_msg = zmsg_new ();
    asset_msg_set_msg (self, &device_msg);
    zmsg_addstr (asset_msg_msg (self), "Hello, World");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (streq (asset_msg_device_type (self), "Life is short but Now lasts for ever"));
        assert (asset_msg_groups_size (self) == 2);
        assert (streq (asset_msg_groups_first (self), "Name: Brutus"));
        assert (streq (asset_msg_groups_next (self), "Age: 43"));
        assert (asset_msg_powers_size (self) == 2);
        assert (streq (asset_msg_powers_first (self), "Name: Brutus"));
        assert (streq (asset_msg_powers_next (self), "Age: 43"));
        assert (streq (asset_msg_ip (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_hostname (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_fqdn (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_mac (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (asset_msg_msg (self)) == 1);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_type (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (asset_msg_type (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_RETURN_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    zmsg_t *return_element_msg = zmsg_new ();
    asset_msg_set_msg (self, &return_element_msg);
    zmsg_addstr (asset_msg_msg (self), "Hello, World");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (zmsg_size (asset_msg_msg (self)) == 1);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_UPDATE_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    zmsg_t *update_element_msg = zmsg_new ();
    asset_msg_set_msg (self, &update_element_msg);
    zmsg_addstr (asset_msg_msg (self), "Hello, World");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (zmsg_size (asset_msg_msg (self)) == 1);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_INSERT_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    zmsg_t *insert_element_msg = zmsg_new ();
    asset_msg_set_msg (self, &insert_element_msg);
    zmsg_addstr (asset_msg_msg (self), "Hello, World");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (zmsg_size (asset_msg_msg (self)) == 1);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_DELETE_ELEMENT);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_type (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (asset_msg_type (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_OK);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_FAIL);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_error_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_error_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_ELEMENTS);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_type (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_type (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_RETURN_ELEMENTS);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_element_ids_insert (self, "Name", "Brutus");
    asset_msg_element_ids_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_ids_size (self) == 2);
        assert (streq (asset_msg_element_ids_string (self, "Name", "?"), "Brutus"));
        assert (asset_msg_element_ids_number (self, "Age", 0) == 43);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_LOCATION_FROM);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_recursive (self, 123);
    asset_msg_set_filter_type (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (asset_msg_recursive (self) == 123);
        assert (asset_msg_filter_type (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_LOCATION_TO);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_RETURN_LOCATION_TO);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_type (self, 123);
    asset_msg_set_name (self, "Life is short but Now lasts for ever");
    asset_msg_set_type_name (self, "Life is short but Now lasts for ever");
    zmsg_t *return_location_to_msg = zmsg_new ();
    asset_msg_set_msg (self, &return_location_to_msg);
    zmsg_addstr (asset_msg_msg (self), "Hello, World");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (asset_msg_type (self) == 123);
        assert (streq (asset_msg_name (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_type_name (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (asset_msg_msg (self)) == 1);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_RETURN_LOCATION_FROM);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_type (self, 123);
    asset_msg_set_name (self, "Life is short but Now lasts for ever");
    asset_msg_set_type_name (self, "Life is short but Now lasts for ever");
    zframe_t *return_location_from_dcs = zframe_new ("Captcha Diem", 12);
    asset_msg_set_dcs (self, &return_location_from_dcs);
    zframe_t *return_location_from_rooms = zframe_new ("Captcha Diem", 12);
    asset_msg_set_rooms (self, &return_location_from_rooms);
    zframe_t *return_location_from_rows = zframe_new ("Captcha Diem", 12);
    asset_msg_set_rows (self, &return_location_from_rows);
    zframe_t *return_location_from_racks = zframe_new ("Captcha Diem", 12);
    asset_msg_set_racks (self, &return_location_from_racks);
    zframe_t *return_location_from_devices = zframe_new ("Captcha Diem", 12);
    asset_msg_set_devices (self, &return_location_from_devices);
    zframe_t *return_location_from_grps = zframe_new ("Captcha Diem", 12);
    asset_msg_set_grps (self, &return_location_from_grps);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (asset_msg_type (self) == 123);
        assert (streq (asset_msg_name (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_type_name (self), "Life is short but Now lasts for ever"));
        assert (zframe_streq (asset_msg_dcs (self), "Captcha Diem"));
        assert (zframe_streq (asset_msg_rooms (self), "Captcha Diem"));
        assert (zframe_streq (asset_msg_rows (self), "Captcha Diem"));
        assert (zframe_streq (asset_msg_racks (self), "Captcha Diem"));
        assert (zframe_streq (asset_msg_devices (self), "Captcha Diem"));
        assert (zframe_streq (asset_msg_grps (self), "Captcha Diem"));
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_POWER_FROM);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_POWERCHAIN_DEVICE);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    asset_msg_set_type_name (self, "Life is short but Now lasts for ever");
    asset_msg_set_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        assert (streq (asset_msg_type_name (self), "Life is short but Now lasts for ever"));
        assert (streq (asset_msg_name (self), "Life is short but Now lasts for ever"));
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_RETURN_POWER);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    zframe_t *return_power_devices = zframe_new ("Captcha Diem", 12);
    asset_msg_set_devices (self, &return_power_devices);
    asset_msg_powers_append (self, "Name: %s", "Brutus");
    asset_msg_powers_append (self, "Age: %d", 43);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (zframe_streq (asset_msg_devices (self), "Captcha Diem"));
        assert (asset_msg_powers_size (self) == 2);
        assert (streq (asset_msg_powers_first (self), "Name: Brutus"));
        assert (streq (asset_msg_powers_next (self), "Age: 43"));
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_POWER_TO);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_POWER_GROUP);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }
    self = asset_msg_new (ASSET_MSG_GET_POWER_DATACENTER);

    //  Check that _dup works on empty message
    copy = asset_msg_dup (self);
    assert (copy);
    asset_msg_destroy (&copy);

    asset_msg_set_element_id (self, 123);
    //  Send twice from same object
    asset_msg_send_again (self, output);
    asset_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = asset_msg_recv (input);
        assert (self);
        assert (asset_msg_routing_id (self));

        assert (asset_msg_element_id (self) == 123);
        asset_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}
