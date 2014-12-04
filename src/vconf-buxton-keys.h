/*
 * libslp-setting
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hakjoo Ko <hakjoo.ko@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VCONF_BUXTON_KEYS_H__
#define __VCONF_BUXTON_KEYS_H__

#include "vconf-internal-keys.h"

/**
 * This file defines keys and values.
 *
 * @file        vconf-keys.h
 * @defgroup    vconf_key Definitions of shared Keys
 * @ingroup     VCONF
 * @author      Hyungdeuk Kim (hd3.kim@samsung.com)
 * @version     0.3
 * @brief       This file has the definitions of shared keys.
 *
 * add keys(key name) and values(enum) here for shared keys....
 *
 */

/*
 * ========================== System Manager Notification
 * ============================= 
 */
/**
 * @defgroup vconf_key_SystemManager System Manager Keys
 * @ingroup vconf_key
 * @addtogroup vconf_key_SystemManager
 * @{
 * @brief Maintainer: giyeol.ok@samsung.com
 */

/**
 * @brief usbhost status
 *
 * 0 : Remove \n
 * 1 : Add \n
 * 2 : Over current \n
 */
#define VCONFKEY_SYSMAN_USB_HOST_STATUS             "memory/sysman/usbhost_status"
enum {
    VCONFKEY_SYSMAN_USB_HOST_DISCONNECTED = 0,
    VCONFKEY_SYSMAN_USB_HOST_CONNECTED,
    VCONFKEY_SYSMAN_USB_HOST_OVERCURRENT
};

/**
 * @brief mmc status
 *
 * 0 : Remove \n
 * 1 : mount \n
 * 2 : insert(not mount) \n
 */
#define VCONFKEY_SYSMAN_MMC_STATUS                  "memory/sysman/mmc"
enum {
    VCONFKEY_SYSMAN_MMC_REMOVED = 0,
    VCONFKEY_SYSMAN_MMC_MOUNTED,
    VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED
};

/**
 * @brief earkey status
 *
 * 0 : not press \n
 * 1 : press \n
 */
#define VCONFKEY_SYSMAN_EARJACKKEY                  "memory/sysman/earjack_key"

/**
 * @brief cradle status
 *
 * 0 : Remove \n
 * 1 : Add \n
 */
#define VCONFKEY_SYSMAN_CRADLE_STATUS               "memory/sysman/cradle_status"

/**
 * @}
 */


/*
 * =============================== Wifi
 * ====================================== 
 */
/**
 * @defgroup vconf_key_Wifi Wifi Keys
 * @ingroup vconf_key
 * @addtogroup vconf_key_Wifi
 * @{
 * @brief Maintainer : dwmax.lee@samsung.com
 */

/**
 * @Wi-Fi Direct state
 *
 * 0: Power off \n
 * 1: Power on \n
 * 2: Discoverable mode \n
 * 3: Connected with peer as GC \n
 * 4: Connected with peer as GO
 */
#define VCONFKEY_WIFI_DIRECT_STATE                  "memory/wifi_direct/state"
enum {
	/** Power off */
    VCONFKEY_WIFI_DIRECT_DEACTIVATED = 0,
	/** Power on */
    VCONFKEY_WIFI_DIRECT_ACTIVATED,
	/** Discoverable mode */
    VCONFKEY_WIFI_DIRECT_DISCOVERING,
	/** Connected with peer as GC */
    VCONFKEY_WIFI_DIRECT_CONNECTED,
	/** Connected with peer as GO */
    VCONFKEY_WIFI_DIRECT_GROUP_OWNER
};

/**
 * @}
 */



/*
 * ================================= BT
 * =====================================
 */
/**
 * @defgroup vconf_key_BT BT Keys
 * @ingroup vconf_key
 * @addtogroup vconf_key_BT
 * @{
 * @brief Maintainer : chanyeol.park@samsung.com
 */

/** \
 * @brief Bluetooth status
 *
 * 0x0000 : Bluetooth OFF \n
 * 0x0001 : Bluetooth ON \n
 * 0x0002 : Discoverable mode \n
 * 0x0004 : In transfering \n
*/
#define VCONFKEY_BT_STATUS                          "db/bluetooth/status"
enum {
	/** Bluetooth OFF */
    VCONFKEY_BT_STATUS_OFF = 0x0000,
	/** Bluetooth ON */
    VCONFKEY_BT_STATUS_ON = 0x0001,
	/** Discoverable mode */
    VCONFKEY_BT_STATUS_BT_VISIBLE = 0x0002,
	/** In transfering */
    VCONFKEY_BT_STATUS_TRANSFER = 0x0004
};

/**
 * @brief Bluetooth Connected status
 *
 * 0x0000 : Not connected \n
 * 0x0001 : Headset connected \n
 * 0x0002 : A2DP headset connected \n
 * 0x0004 : HID connected \n
 * 0x0008 : PAN connected \n
 * 0x0010 : SAP connected \n
 * 0x0020 : PBAP connected \n
*/
#define VCONFKEY_BT_DEVICE                          "memory/bluetooth/device"
enum {
	/** Not connected */
    VCONFKEY_BT_DEVICE_NONE = 0x0000,
	/** Headset connected */
    VCONFKEY_BT_DEVICE_HEADSET_CONNECTED = 0x0001,
	/** A2DP headset connected */
    VCONFKEY_BT_DEVICE_A2DP_HEADSET_CONNECTED = 0x0002,
	/** HID connected */
    VCONFKEY_BT_DEVICE_HID_CONNECTED = 0x0004,
	/** PAN connected */
    VCONFKEY_BT_DEVICE_PAN_CONNECTED = 0x0008,
	/** SAP connected */
    VCONFKEY_BT_DEVICE_SAP_CONNECTED = 0x0010,
	/** PBAP connected */
    VCONFKEY_BT_DEVICE_PBAP_CONNECTED = 0x0020
};


/*
 * Media sound path for BT 
 */
enum {
	/** Media Player Select Speaker */
    VCONFKEY_BT_PLAYER_SELECT_SPEAKER = 0x00,
	/** Media Player Select Bluetooth */
    VCONFKEY_BT_PLAYER_SELECT_BLUETOOTH = 0x01,
	/** BT application Select Speaker */
    VCONFKEY_BT_APP_SELECT_SPEAKER = 0x02,
	/** BT application Select Bluetooth */
    VCONFKEY_BT_APP_SELECT_BLUETOOTH = 0x04
};

/**
 * @}
 */

/*
 * =========================== IDLE lock
 * =======================================
 */
/**
 * @defgroup vconf_key_idleLock idleLock Keys
 * @ingroup vconf_key
 * @addtogroup vconf_key_idleLock
 * @{
 * @brief Maintainer : seungtaek.chung@samsung.com, wonil22.choi@samsung.com hyoyoung.chang@samsung.com angelkim@samsung.com
 */

/**
 * @brief lock screen status
 *
 * VCONFKEY_IDLE_UNLOCK : unlocked state \n
 * VCONFKEY_IDLE_LOCK : locked state \n
 */
#define VCONFKEY_IDLE_LOCK_STATE                    "memory/idle_lock/state"
enum {
	/** unlocked state */
    VCONFKEY_IDLE_UNLOCK = 0x00,
	/** locked state */
    VCONFKEY_IDLE_LOCK
};

/**
 * @brief wallpaper of lock screen
 *
 * Value : Wallpaper file path in the lock screen  \n
 */
#define VCONFKEY_IDLE_LOCK_BGSET                    "db/idle_lock/bgset"

/**
 * @}
 */



/*
 * =========================== pwlock
 * =======================================
 */
/**
 * @defgroup vconf_key_pwlock Lock application for password verification: phone, pin, sum, network, etc.
 * @ingroup vconf_key
 * @addtogroup vconf_key_pwlock
 * @{
 * @brief Maintainer : seungtaek.chung@samsung.com miju52.lee@samsung.com
 *        Used module : pwlock
 *
 */

/**
 * @brief mobex engine status
 *
 * VCONFKEY_PWLOCK_BOOTING_UNLOCK : unlocked state in boointg time \n
 * VCONFKEY_PWLOCK_BOOTING_LOCK : locked state in boointg time \n
 * VCONFKEY_PWLOCK_RUNNING_UNLOCK : unlocked state in running time \n
 * VCONFKEY_PWLOCK_RUNNING_LOCK : locked state in running time \n
 */
#define VCONFKEY_PWLOCK_STATE                       "memory/pwlock/state"
enum {
	/** unlocked state in boointg time */
    VCONFKEY_PWLOCK_BOOTING_UNLOCK = 0x00,
	/** locked state in boointg time */
    VCONFKEY_PWLOCK_BOOTING_LOCK,
	/** unlocked state in running time */
    VCONFKEY_PWLOCK_RUNNING_UNLOCK,
	/** locked state in running time */
    VCONFKEY_PWLOCK_RUNNING_LOCK
};
/**
 * @}
 */



/*
 * =========================== browser
 * =======================================
 */
/**
 * @defgroup vconf_key_browser browser public keys
 * @ingroup vconf_key
 * @addtogroup vconf_key_browser
 * @{
 * @brief Maintainer : sangpyo7.kim@samsung.com ibchang@samsung.com
 *
 */

/**
 * @brief browser user agent string
 *
 * Value : The user agent string currently being used by embeded browser \n
 */
#define VCONFKEY_BROWSER_USER_AGENT		"db/browser/user_agent"

/**
 * @brief browser user agent profile
 *
 * Value : The user agent string profile currently being used by embeded browser for 2G network \n
 */
#define VCONFKEY_BROWSER_USER_AGENT_PROFILE_2G		"db/browser/user_agent_profile_2G"

/**
 * @brief browser user agent profile
 *
 * Value : The user agent string profile currently being used by embeded browser for 3G network \n
 */
#define VCONFKEY_BROWSER_USER_AGENT_PROFILE_3G		"db/browser/user_agent_profile_3G"

/**
 * @brief browser user agent profile
 *
 * Value : The user agent string profile currently being used by embeded browser for 4G network \n
 */
#define VCONFKEY_BROWSER_USER_AGENT_PROFILE_4G		"db/browser/user_agent_profile_4G"

/**
 * @}
 */
#endif				/* __VCONF_BUXTON_KEYS_H__ */
