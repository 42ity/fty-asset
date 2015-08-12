# Based on /etc/dhcp/dhclient-exit-hooks.d/ntp from Debian8 dhclient package
# Included from (patched) /etc/udhcpc/default.script for $BIOS project
# Adapted by Jim Klimov <EvgenyKlimov@eaton.com>, Copyright (C) 2015, Eaton

NTP_CONF=/etc/ntp.conf
NTP_DHCP_CONF=/var/lib/ntp/ntp.conf.dhcp


ntp_server_restart() {
	invoke-rc.d ntp try-restart
}


ntp_server_status() {
	invoke-rc.d ntp status
	# NOTE: successful return means the daemon is running, but
	# it does guarantee we've picked up time from any source
}


ntp_servers_setup_remove() {
	if [ ! -e "$NTP_DHCP_CONF" ]; then
		return
	fi
	rm -f "$NTP_DHCP_CONF"
	ntp_server_restart
}


ntp_servers_setup_add() {
	if [ -e $NTP_DHCP_CONF ] && [ "$new_ntp_servers" = "$old_ntp_servers" ]; then
		echo "Got no changes to apply to DHCP-announced NTP config"
		return
	fi

	if [ -z "$new_ntp_servers" ]; then
		echo "DHCP announced no NTP servers, falling back to static NTP configs..."
		ntp_servers_setup_remove
		return
	fi

	tmp=$(mktemp "$NTP_DHCP_CONF.XXXXXX") || return
	chmod --reference="$NTP_CONF" "$tmp"
	chown --reference="$NTP_CONF" "$tmp"

	echo "DHCP announced NTP servers '$new_ntp_servers' different from old NTP config '$old_ntp_servers', applying..."
	(
	  echo "# This file was copied from $NTP_CONF with the server options changed"
	  echo "# to reflect the information sent by the DHCP server.  Any changes made"
	  echo "# here will be lost at the next DHCP event.  Edit $NTP_CONF instead."
	  echo
	  echo "# NTP server entries received from DHCP server"
	  for server in $new_ntp_servers; do
		echo "server $server iburst"
	  done
	  echo
	  sed -r -e '/^ *(server|peer).*$/d' "$NTP_CONF"
	) >>"$tmp"

	mv "$tmp" "$NTP_DHCP_CONF"

	ntp_server_restart && \
	    echo "NTP service restarted; waiting for it to pick up time (if not failed) so as to sync it onto hardware RTC" && \
	    sleep 60 && ntp_server_status && hwclock -w && echo "Applied current OS clock value to HW clock"
}


ntp_servers_setup() {
	RES=1
	case "$reason" in
		bound|renew|BOUND|RENEW|REBIND|REBOOT)
			ntp_servers_setup_add
			RES=$?
			;;
		deconfig|leasefail|nak|EXPIRE|FAIL|RELEASE|STOP)
			ntp_servers_setup_remove
			RES=$?
			;;
	esac
	echo "Completed $0 for DHCP state $reason, exit code $RES"
	return $RES
}

[ -z "${reason-}" -a -n "$1" ] && reason="$1"
[ -z "${new_ntp_servers-}" ] && new_ntp_servers="" && [ -n "${ntpsrv-}" ] && \
        new_ntp_servers="`for S in $ntpsrv; do echo "$S"; done | sort | uniq`"
if [ -z "${old_ntp_servers-}" ]; then
        old_ntp_servers=""
        if [ -s "$NTP_DHCP_CONF" ]; then
                old_ntp_servers="`cat "$NTP_DHCP_CONF" | egrep '^[ \t]*(server|peer)' | awk '{print $2}' | sort | uniq`"
        else
                old_ntp_servers="`cat "$NTP_CONF" | egrep '^[ \t]*(server|peer)' | awk '{print $2}' | sort | uniq`"
        fi
fi

#set >&2
#set -x

ntp_servers_setup

