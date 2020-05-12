/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.h: TFP protocol message handling
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"

// Default functions
BootloaderHandleMessageResponse handle_message(const void *data, void *response);
void communication_tick(void);
void communication_init(void);

// Constants

#define EVSE_BOOTLOADER_MODE_BOOTLOADER 0
#define EVSE_BOOTLOADER_MODE_FIRMWARE 1
#define EVSE_BOOTLOADER_MODE_BOOTLOADER_WAIT_FOR_REBOOT 2
#define EVSE_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_REBOOT 3
#define EVSE_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT 4

#define EVSE_BOOTLOADER_STATUS_OK 0
#define EVSE_BOOTLOADER_STATUS_INVALID_MODE 1
#define EVSE_BOOTLOADER_STATUS_NO_CHANGE 2
#define EVSE_BOOTLOADER_STATUS_ENTRY_FUNCTION_NOT_PRESENT 3
#define EVSE_BOOTLOADER_STATUS_DEVICE_IDENTIFIER_INCORRECT 4
#define EVSE_BOOTLOADER_STATUS_CRC_MISMATCH 5

#define EVSE_STATUS_LED_CONFIG_OFF 0
#define EVSE_STATUS_LED_CONFIG_ON 1
#define EVSE_STATUS_LED_CONFIG_SHOW_HEARTBEAT 2
#define EVSE_STATUS_LED_CONFIG_SHOW_STATUS 3

// Function and callback IDs and structs
#define FID_SET_LOW_LEVEL_OUTPUT 1
#define FID_GET_LOW_LEVEL_STATUS 2


typedef struct {
	TFPMessageHeader header;
	bool low_level_mode_enabled;
	uint16_t cp_duty_cycle;
	bool motor_direction;
	uint16_t motor_duty_cycle;
	uint16_t relay_enabled;
	uint32_t password;
} __attribute__((__packed__)) SetLowLevelOutput;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetLowLevelStatus;

typedef struct {
	TFPMessageHeader header;
	bool low_level_mode_enabled;
	uint16_t cp_duty_cycle;
	uint16_t motor_direction;
	uint16_t motor_duty_cycle;
	uint16_t relay_enabled;
	int16_t cp_voltage;
	int16_t pp_voltage;
	uint8_t ac_input[1];
	bool gp_input;
	bool motor_fault;
	bool motor_switch;
} __attribute__((__packed__)) GetLowLevelStatus_Response;


// Function prototypes
BootloaderHandleMessageResponse set_low_level_output(const SetLowLevelOutput *data);
BootloaderHandleMessageResponse get_low_level_status(const GetLowLevelStatus *data, GetLowLevelStatus_Response *response);

// Callbacks


#define COMMUNICATION_CALLBACK_TICK_WAIT_MS 1
#define COMMUNICATION_CALLBACK_HANDLER_NUM 0
#define COMMUNICATION_CALLBACK_LIST_INIT \


#endif
