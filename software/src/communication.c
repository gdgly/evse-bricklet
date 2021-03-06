/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
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

#include "communication.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/logging/logging.h"

#include "configs/config_evse.h"
#include "configs/config_contactor_check.h"
#include "evse.h"
#include "ads1118.h"
#include "iec61851.h"
#include "led.h"
#include "contactor_check.h"
#include "lock.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_SET_LOW_LEVEL_OUTPUT: return set_low_level_output(message);
		case FID_GET_LOW_LEVEL_STATUS: return get_low_level_status(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}

BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response) {
	response->header.length        = sizeof(GetState_Response);
	response->iec61851_state       = iec61851.state;
	response->led_state            = led.state;
	response->resistance[0]        = ads1118.cp_pe_resistance;
	response->resistance[1]        = ads1118.pp_pe_resistance;
	response->cp_pwm_duty_cycle    = (64000 - ccu4_pwm_get_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER))/64;
	response->contactor_state      = contactor_check.state;
	response->contactor_error      = contactor_check.error;
	response->gpio[0]              = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN) | (XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN) << 1) | (XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN) << 2) | (XMC_GPIO_GetInput(EVSE_RELAY_PIN) << 3);
	response->lock_state           = lock.state;
	response->jumper_configuration = evse.config_jumper_current;
	response->has_lock_switch      = evse.has_lock_switch;
	response->uptime               = system_timer_get_ms();

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_low_level_output(const SetLowLevelOutput *data) {
	return HANDLE_MESSAGE_RESPONSE_EMPTY;

	logd("set_ll pw: %x\n\r", data->password);

	if(data->password != 0x4223B00B) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}
	
	if(evse.low_level_cp_duty_cycle > 1000) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(evse.low_level_motor_duty_cycle > 1000) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	evse.low_level_mode_enabled     = data->low_level_mode_enabled;
	evse.low_level_cp_duty_cycle    = data->cp_duty_cycle;
	evse.low_level_motor_direction  = data->motor_direction;
	evse.low_level_motor_duty_cycle = data->motor_duty_cycle;
	evse.low_level_relay_enabled    = data->relay_enabled;
	evse.low_level_relay_monoflop   = system_timer_get_ms();

	logd("set_ll en: %d, cp\%: %u, motor dir: %d, motor\%: %u, relay: %d, mono: %u\n\r", 
	     evse.low_level_mode_enabled,
	     evse.low_level_cp_duty_cycle,
	     evse.low_level_motor_direction,
	     evse.low_level_motor_duty_cycle,
	     evse.low_level_relay_enabled,
	     evse.low_level_relay_monoflop
	);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_low_level_status(const GetLowLevelStatus *data, GetLowLevelStatus_Response *response) {
	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	response->header.length          = sizeof(GetLowLevelStatus_Response);
	response->low_level_mode_enabled = evse.low_level_mode_enabled;
	response->cp_duty_cycle          = evse.low_level_cp_duty_cycle;
	response->motor_direction        = evse.low_level_motor_direction;
	response->motor_duty_cycle       = evse.low_level_motor_duty_cycle;
	response->relay_enabled          = evse.low_level_relay_enabled;
	response->cp_voltage             = ads1118.cp_voltage;
	response->pp_voltage             = ads1118.pp_voltage;
	response->ac_input[0]            = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC1_PIN) | (XMC_GPIO_GetInput(CONTACTOR_CHECK_AC2_PIN) << 1);
	response->gp_input               = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN);
	response->motor_fault            = XMC_GPIO_GetInput(EVSE_MOTOR_FAULT_PIN);
	response->motor_switch           = XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
