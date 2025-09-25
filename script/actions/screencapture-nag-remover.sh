#!/bin/bash
# From https://github.com/luckman212/screencapture-nag-remover

SELF='screencapture-nag-remover'
FQPN=$(realpath "$0")
PLIST="$HOME/Library/Group Containers/group.com.apple.replayd/ScreenCaptureApprovals.plist"
AGENT_PLIST="$HOME/Library/LaunchAgents/$SELF.plist"
MDM_PROFILE="$HOME/Downloads/macOS_15.1_DisableScreenCaptureAlerts.mobileconfig"
TCC_DB='/Library/Application Support/com.apple.TCC/TCC.db'
FUTURE=$(/bin/date -j -v+100y +"%Y-%m-%d %H:%M:%S +0000")
INTERVAL=86400 #run every 24h

IFS='.' read -r MAJ MIN _ < <(/usr/bin/sw_vers --productVersion)
if (( MAJ < 15 )); then
	echo >&2 "this tool requires macOS 15 (Sequoia)"
	exit
fi

_os_is_151_or_higher() {
	(( MAJ >= 15 )) && (( MIN > 0 ))
}

_fda_settings() {
	/usr/bin/open 'x-apple.systempreferences:com.apple.preference.security?Privacy_AllFiles'
}

_open_device_management() {
	/usr/bin/open 'x-apple.systempreferences:com.apple.preferences.configurationprofiles'
}

_bundleid_to_name() {
	local APP_NAME
	APP_NAME=$(/usr/bin/mdfind kMDItemCFBundleIdentifier == "$1" 2>/dev/null)
	echo "${APP_NAME##*/}"
}

_create_plist() {
	cat <<-EOF 2>/dev/null >"$PLIST"
	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
	<plist version="1.0">
	<dict>
	</dict>
	</plist>
	EOF
}

_bounce_daemons() {
	/usr/bin/killall -HUP replayd
	/usr/bin/killall -u "$USER" cfprefsd
}

_nagblock() {
	local APP_NAME
	if _os_is_151_or_higher; then
		if [[ -z $1 ]]; then
			echo >&2 "supply the bundle ID of the app"
			return 1
		fi
		APP_NAME=$(_bundleid_to_name "$1")
		echo "disabling nag for $1${APP_NAME:+ ($APP_NAME)}"
		/usr/bin/defaults write "$PLIST" "$1" -dict \
			kScreenCaptureApprovalLastAlerted -date "$FUTURE" \
			kScreenCaptureApprovalLastUsed -date "$FUTURE"
		(( c++ ))
	else
		if [[ -z $1 ]]; then
			echo >&2 "supply complete pathname to the binary inside the app bundle"
			return 1
		fi
		[[ -e $1 ]] || { echo >&2 "$1 does not exist"; return 1; }
		IFS='/' read -ra PARTS <<< "$1"
		for p in "${PARTS[@]}"; do
			if [[ $p == *.app ]]; then
				APP_NAME=$p
				break
			fi
		done
		echo "disabling nag for ${APP_NAME:-$1}"
		/usr/bin/defaults write "$PLIST" "$1" -date "$FUTURE"
		(( c++ ))
		return 0
	fi
}

_enum_apps() {
	[[ -e $PLIST ]] || return 1
	if _os_is_151_or_higher; then
		/usr/bin/plutil -convert raw -o - -- "$PLIST"
	else
		/usr/bin/plutil -convert xml1 -o - -- "$PLIST" |
		/usr/bin/sed -n "s/.*<key>\(.*\)<\/key>.*/\1/p"
	fi
}

_generate_mdm_profile() {
UUID1=$(/usr/bin/uuidgen)
UUID2=$(/usr/bin/uuidgen)
/bin/cat <<EOF >"$MDM_PROFILE"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>PayloadContent</key>
	<array>
		<dict>
			<key>PayloadDisplayName</key>
			<string>Restrictions</string>
			<key>PayloadIdentifier</key>
			<string>com.apple.applicationaccess.${UUID2}</string>
			<key>PayloadType</key>
			<string>com.apple.applicationaccess</string>
			<key>PayloadUUID</key>
			<string>${UUID2}</string>
			<key>PayloadVersion</key>
			<integer>1</integer>
			<key>forceBypassScreenCaptureAlert</key>
			<true/>
		</dict>
	</array>
	<key>PayloadDescription</key>
	<string>Disables additional screen capture alerts on macOS 15.1 or higher</string>
	<key>PayloadDisplayName</key>
	<string>DisableScreenCaptureAlert</string>
	<key>PayloadIdentifier</key>
	<string>com.apple.applicationaccess.forceBypassScreenCaptureAlert</string>
	<key>PayloadScope</key>
	<string>System</string>
	<key>PayloadType</key>
	<string>Configuration</string>
	<key>PayloadUUID</key>
	<string>${UUID1}</string>
	<key>PayloadVersion</key>
	<integer>1</integer>
	<key>TargetDeviceType</key>
	<integer>5</integer>
</dict>
</plist>
EOF
#Apple prohibits self-installing TCC profiles, they can only be pushed via MDM
#/usr/bin/open "$MDM_PROFILE"
#_open_device_management
echo "import ${MDM_PROFILE##*/} into your MDM to provision it"
/usr/bin/open -R "$MDM_PROFILE"
}

_uninstall_launchagent() {
	/bin/launchctl bootout gui/$UID "$AGENT_PLIST" 2>/dev/null
	/bin/rm 2>/dev/null "$AGENT_PLIST"
	echo "uninstalled $SELF LaunchAgent"
}

_install_launchagent() {
	_uninstall_launchagent &>/dev/null
	read -r FDA_TEST < <(/usr/bin/sqlite3 "$TCC_DB" <<-EOS
	SELECT COUNT(client)
	FROM access
	WHERE
		client = '/bin/bash' AND
		service = 'kTCCServiceSystemPolicyAllFiles' AND
		auth_value = 2
	EOS
	)
	if (( FDA_TEST == 0 )); then
		/bin/cat <<-EOF >&2
		┌──────────────────────────────────────────────────────────────────────────────────────┐
		│  For the LaunchAgent to work properly, you must grant Full Disk Access to /bin/bash  │
		│                                                                                      │
		│  The Full Disk Access settings panel will now be opened. Press the (+) button near   │
		│  the bottom of the window, then press [⌘cmd + ⇧shift + g] and type '/bin/bash' and   │
		│  click Open to get it to appear in the app list.                                     │
		│                                                                                      │
		│  Once that's all done, run the --install command again.                              │
		└──────────────────────────────────────────────────────────────────────────────────────┘
		EOF
		sleep 3
		_fda_settings
		return 1
	fi
	/bin/cat >"$AGENT_PLIST" <<-EOF
	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
	<plist version="1.0">
	<dict>
		<key>Label</key>
		<string>$SELF.agent</string>
		<key>ProgramArguments</key>
		<array>
			<string>/bin/bash</string>
			<string>--norc</string>
			<string>--noprofile</string>
			<string>$FQPN</string>
		</array>
		<key>StandardErrorPath</key>
		<string>/private/tmp/$SELF.stderr</string>
		<key>StandardOutPath</key>
		<string>/private/tmp/$SELF.stdout</string>
		<key>StartInterval</key>
		<integer>$INTERVAL</integer>
		<key>WorkingDirectory</key>
		<string>/private/tmp</string>
	</dict>
	</plist>
	EOF
	/bin/chmod 644 "$PLIST"
	if /bin/launchctl bootstrap gui/$UID "$AGENT_PLIST"; then
		echo "installed $SELF LaunchAgent"
	fi
}

_manual_add_desc() {
	if _os_is_151_or_higher ; then
		echo "-a,--add <bundle_id>   manually create an entry"
	else
		echo "-a,--add <path>        manually create an entry (supply full path to binary)"
	fi
}

case $1 in
	-h|--help)
		/bin/cat <<-EOF

		a tool to help suppress macOS Sequoia's persistent ScreenCapture alerts
		usage: ${0##*/} [args]
		    -r,--reveal            show ${PLIST##*/} in Finder
		    -p,--print             print current values
		    $(_manual_add_desc)
		    --reset                initialize empty ${PLIST##*/}
		    --generate_profile     generate configuration profile for use with your MDM server
		    --profiles             opens Device Management in System Settings
		    --install              install LaunchAgent to ensure alerts continue to be silenced
		    --uninstall            remove LaunchAgent
		EOF
		if _os_is_151_or_higher; then /bin/cat <<-EOF

		    ┌────────────────────────────────────────────────────────────────────────────────────┐
		    │  macOS 15.1 introduced an official method for suppressing ScreenCapture alerts     │
		    │  for ALL apps on Macs enrolled in an MDM server (Jamf, Addigy, Mosyle etc).        │
		    │                                                                                    │
		    │  A configuration profile to enable this can be generated using --generate_profile  │
		    └────────────────────────────────────────────────────────────────────────────────────┘

		EOF
		fi
		exit
		;;
	-r|--reveal)
		if [[ -e $PLIST ]]; then
			/usr/bin/open -R "$PLIST"
		else
			/usr/bin/open "$(/usr/bin/dirname "$PLIST")"
		fi
		exit
		;;
	-p|--print)
		if [[ -e $PLIST ]]; then
			/usr/bin/plutil -p "$PLIST"
		else
			echo >&2 "${PLIST##*/} does not exist"
		fi
		exit
		;;
	--reset) _create_plist || echo >&2 "error, could not create ${PLIST##*/}"; exit;;
	--generate_profile) _generate_mdm_profile; exit;;
	--profiles) _open_device_management; exit;;
	--install) _install_launchagent; exit;;
	--uninstall) _uninstall_launchagent; exit;;
esac

[[ -e $PLIST ]] || _create_plist
if ! /usr/bin/touch "$PLIST" 2>/dev/null; then
	if [[ -n $__CFBundleIdentifier ]]; then
		TERMINAL_NAME=$(_bundleid_to_name "$__CFBundleIdentifier")
	fi
	echo >&2 "Full Disk Access permissions are missing${TERMINAL_NAME:+ for $TERMINAL_NAME}"
	exit 1
fi

case $1 in
	-a|--add)
		_nagblock "$2"
		_bounce_daemons
		exit
		;;
	-*) echo >&2 "invalid arg: $1"; exit 1;;
esac

c=0
while read -r APP_PATH ; do
	[[ -n $APP_PATH ]] || continue
	_nagblock "$APP_PATH"
done < <(_enum_apps)

#bounce daemons if any changes were made so the new settings take effect
(( c > 0 )) && _bounce_daemons

exit 0
