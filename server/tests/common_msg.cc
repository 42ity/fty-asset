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
#include "../src/topology/msg/common_msg.h"
#include <iostream>

// TEST_CASE disabled due to assertion on socket type support.
// bios code, deprecated, not used and untested.

TEST_CASE("common_msg test", "[.disabled]")
{
    printf (" * common_msg test: \n");

    //  @selftest
    //  Simple create/destroy test
    common_msg_t *self = common_msg_new (0);
    assert (self);
    common_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-common_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-common_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    common_msg_t *copy;
    self = common_msg_new (COMMON_MSG_GET_MEASURE_TYPE_I);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mt_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_mt_id (self) == 123);
        common_msg_destroy (&self);
    }

    self = common_msg_new (COMMON_MSG_GET_MEASURE_TYPE_S);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mt_name (self, "Life is short but Now lasts for ever");
    common_msg_set_mt_unit (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (streq (common_msg_mt_name (self), "Life is short but Now lasts for ever"));
        assert (streq (common_msg_mt_unit (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }

    self = common_msg_new (COMMON_MSG_GET_MEASURE_SUBTYPE_I);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mt_id (self, 123);
    common_msg_set_mts_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_mt_id (self) == 123);
        assert (common_msg_mts_id (self) == 123);
        common_msg_destroy (&self);
    }

    self = common_msg_new (COMMON_MSG_GET_MEASURE_SUBTYPE_S);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mt_id (self, 123);
    common_msg_set_mts_name (self, "Life is short but Now lasts for ever");
    common_msg_set_mts_scale (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_mt_id (self) == 123);
        assert (streq (common_msg_mts_name (self), "Life is short but Now lasts for ever"));
        assert (common_msg_mts_scale (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_MEASURE_TYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mt_id (self, 123);
    common_msg_set_mt_name (self, "Life is short but Now lasts for ever");
    common_msg_set_mt_unit (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_mt_id (self) == 123);
        assert (streq (common_msg_mt_name (self), "Life is short but Now lasts for ever"));
        assert (streq (common_msg_mt_unit (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_MEASURE_SUBTYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_mts_id (self, 123);
    common_msg_set_mt_id (self, 123);
    common_msg_set_mts_scale (self, 123);
    common_msg_set_mts_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_mts_id (self) == 123);
        assert (common_msg_mt_id (self) == 123);
        assert (common_msg_mts_scale (self) == 123);
        assert (streq (common_msg_mts_name (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_FAIL);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_errtype (self, 123);
    common_msg_set_errorno (self, 123);
    common_msg_set_errmsg (self, "Life is short but Now lasts for ever");
    common_msg_aux_insert (self, "Name", "Brutus");
    common_msg_aux_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_errtype (self) == 123);
        assert (common_msg_errorno (self) == 123);
        assert (streq (common_msg_errmsg (self), "Life is short but Now lasts for ever"));
        assert (common_msg_aux_size (self) == 2);
        assert (streq (common_msg_aux_string (self, "Name", "?"), "Brutus"));
        assert (common_msg_aux_number (self, "Age", 0) == 43);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DB_OK);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    common_msg_aux_insert (self, "Name", "Brutus");
    common_msg_aux_insert (self, "Age", "%d", 43);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_rowid (self) == 123);
        assert (common_msg_aux_size (self) == 2);
        assert (streq (common_msg_aux_string (self, "Name", "?"), "Brutus"));
        assert (common_msg_aux_number (self, "Age", 0) == 43);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (streq (common_msg_name (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_client_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_UPDATE_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    zmsg_t *update_client_msg = zmsg_new ();
    common_msg_set_msg (self, &update_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_client_id (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_client_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    zmsg_t *return_client_msg = zmsg_new ();
    common_msg_set_msg (self, &return_client_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_rowid (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_NEW_MEASUREMENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_name (self, "Life is short but Now lasts for ever");
    common_msg_set_device_name (self, "Life is short but Now lasts for ever");
    common_msg_set_device_type (self, "Life is short but Now lasts for ever");
    common_msg_set_mt_id (self, 123);
    common_msg_set_mts_id (self, 123);
    common_msg_set_value (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (streq (common_msg_client_name (self), "Life is short but Now lasts for ever"));
        assert (streq (common_msg_device_name (self), "Life is short but Now lasts for ever"));
        assert (streq (common_msg_device_type (self), "Life is short but Now lasts for ever"));
        assert (common_msg_mt_id (self) == 123);
        assert (common_msg_mts_id (self) == 123);
        assert (common_msg_value (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_CLIENT_INFO);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    common_msg_set_device_id (self, 123);
    zchunk_t *client_info_info = zchunk_new ("Captcha Diem", 12);
    common_msg_set_info (self, &client_info_info);
    common_msg_set_date (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_client_id (self) == 123);
        assert (common_msg_device_id (self) == 123);
        assert (memcmp (zchunk_data (common_msg_info (self)), "Captcha Diem", 12) == 0);
        assert (common_msg_date (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_CINFO);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_cinfo_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_cinfo_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_CINFO);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_cinfo_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_cinfo_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_CINFO);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    zmsg_t *return_cinfo_msg = zmsg_new ();
    common_msg_set_msg (self, &return_cinfo_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_rowid (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DEVICE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_devicetype_id (self, 123);
    common_msg_set_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_devicetype_id (self) == 123);
        assert (streq (common_msg_name (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_DEVICE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_device_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_device_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_DEVICE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_device_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_device_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_DEVICE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    zmsg_t *return_device_msg = zmsg_new ();
    common_msg_set_msg (self, &return_device_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_rowid (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DEVICE_TYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_name (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (streq (common_msg_name (self), "Life is short but Now lasts for ever"));
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_INSERT_DEVTYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    zmsg_t *insert_devtype_msg = zmsg_new ();
    common_msg_set_msg (self, &insert_devtype_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_DELETE_DEVTYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_devicetype_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_devicetype_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_DEVTYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_rowid (self, 123);
    zmsg_t *return_devtype_msg = zmsg_new ();
    common_msg_set_msg (self, &return_devtype_msg);
    zmsg_addstr (common_msg_msg (self), "Hello, World");
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_rowid (self) == 123);
        assert (zmsg_size (common_msg_msg (self)) == 1);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_GET_CLIENT);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_client_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_client_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_GET_CINFO);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_cinfo_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_cinfo_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_GET_DEVICE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_device_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_device_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_GET_DEVTYPE);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_devicetype_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_devicetype_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_GET_LAST_MEASUREMENTS);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_device_id (self, 123);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_device_id (self) == 123);
        common_msg_destroy (&self);
    }
    self = common_msg_new (COMMON_MSG_RETURN_LAST_MEASUREMENTS);

    //  Check that _dup works on empty message
    copy = common_msg_dup (self);
    assert (copy);
    common_msg_destroy (&copy);

    common_msg_set_device_id (self, 123);
    common_msg_set_device_name (self, "Life is short but Now lasts for ever");
    common_msg_measurements_append (self, "Name: %s", "Brutus");
    common_msg_measurements_append (self, "Age: %d", 43);
    //  Send twice from same object
    common_msg_send_again (self, output);
    common_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = common_msg_recv (input);
        assert (self);
        assert (common_msg_routing_id (self));

        assert (common_msg_device_id (self) == 123);
        assert (streq (common_msg_device_name (self), "Life is short but Now lasts for ever"));
        assert (common_msg_measurements_size (self) == 2);
        assert (streq (common_msg_measurements_first (self), "Name: Brutus"));
        assert (streq (common_msg_measurements_next (self), "Age: 43"));
        common_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}
