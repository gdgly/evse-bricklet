/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iec61851.c: Implementation of IEC 61851 EVSE state machine
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

#include "iec61851.h"

#include <stdint.h>
#include <string.h>

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "configs/config_evse.h"
#include "ads1118.h"
#include "lock.h"
#include "evse.h"
#include "contactor_check.h"
#include "led.h"
#include "button.h"

// Resistance between CP/PE
// inf  Ohm -> no car present
// 2700 Ohm -> car present
//  880 Ohm -> car charging
//  240 Ohm -> car charging with ventilation
// ==>
// > 10000 -> State A
// >  1790 -> State B
// >  560 -> State C
// >  150 -> State D
// <  150 -> State E/F
#define IEC61851_CP_RESISTANCE_STATE_A 10000
#define IEC61851_CP_RESISTANCE_STATE_B  1790
#define IEC61851_CP_RESISTANCE_STATE_C   560
#define IEC61851_CP_RESISTANCE_STATE_D   150


// Resistance between PP/PE
// 1000..2200 Ohm => 13A
// 330..1000 Ohm  => 20A
// 150..330 Ohm   => 32A
// 75..150 Ohm    => 63A
#define IEC61851_PP_RESISTANCE_13A 1000
#define IEC61851_PP_RESISTANCE_20A  330
#define IEC61851_PP_RESISTANCE_32A  150


IEC61851 iec61851;

uint32_t iec61851_get_ma_from_pp_resistance(void) {
	if(ads1118.pp_pe_resistance >= 1000) {
		return 13000; // 13A
	} else if(ads1118.pp_pe_resistance >= 330) {
		return 20000; // 20A
	} else if(ads1118.pp_pe_resistance >= 150) {
		return 32000; // 32A
	} else {
		return 64000; // 64A
	}
}

uint32_t iec61851_get_ma_from_jumper(void) {
	switch(evse.config_jumper_current) {
		case EVSE_CONFIG_JUMPER_CURRENT_6A:  return 6000;
		case EVSE_CONFIG_JUMPER_CURRENT_10A: return 10000;
		case EVSE_CONFIG_JUMPER_CURRENT_13A: return 13000;
		case EVSE_CONFIG_JUMPER_CURRENT_16A: return 16000;
		case EVSE_CONFIG_JUMPER_CURRENT_20A: return 20000;
		case EVSE_CONFIG_JUMPER_CURRENT_25A: return 25000;
		case EVSE_CONFIG_JUMPER_CURRENT_32A: return 32000;
		case EVSE_CONFIG_JUMPER_SOFTWARE: return evse.config_jumper_current_software;
		default: return 6000;
	}
}

uint32_t iec61851_get_max_ma(void) {
	return MIN(iec61851_get_ma_from_pp_resistance(), iec61851_get_ma_from_jumper());
}

// Duty cycle in pro mille (1/10 %)
uint16_t iec61851_get_duty_cycle_for_ma(uint32_t ma) {
	uint32_t duty_cycle;
	if(ma <= 51000) {
		duty_cycle = ma/60; // For 6A-51A: xA = %duty*0.6
	} else {
		duty_cycle = ma/250 + 640; // For 51A-80A: xA= (%duty - 64)*2.5
	}

	// The standard defines 8% as minimum and 100% as maximum
	return BETWEEN(80, duty_cycle, 1000); 
}

void iec61851_state_a(void) {
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
	led.state = LED_STATE_OFF;

	if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A) {
		button_reset();
	}
}

void iec61851_state_b(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);
	led.state = LED_STATE_ON;
}

void iec61851_state_c(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
	led.state = LED_STATE_BREATHING;
}

void iec61851_state_d(void) {
	// State D is not supported
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
	led.state = LED_STATE_BLINKING;
}

void iec61851_state_ef(void) {
	// In case of error apply +12V to CP, disable contactor
	evse_set_output(1000, false);
	led.state = LED_STATE_BLINKING;
	// TODO: Add different blinking states for different errors
}

void iec61851_tick(void) {
	if(contactor_check.error != 0) {
		iec61851.state = IEC61851_STATE_EF;
	} else if(evse.config_jumper_current == EVSE_CONFIG_JUMPER_UNCONFIGURED) {
		// We don't allow the jumper to be unconfigured
		iec61851.state = IEC61851_STATE_EF;
	} else if(button.was_pressed) {
		iec61851.state = IEC61851_STATE_A;
	} else {
		// Wait for ADC measurements to be valid
		if(ads1118.cp_invalid_counter > 0) {
			return;
		}

		if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A) {
			iec61851.state = IEC61851_STATE_A;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_B) {
			iec61851.state = IEC61851_STATE_B;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_C) {
			iec61851.state = IEC61851_STATE_C;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_D) {
			iec61851.state = IEC61851_STATE_D;
		} else {
			iec61851.state = IEC61851_STATE_EF;
		}
	}

	switch(iec61851.state) {
		case IEC61851_STATE_A:  iec61851_state_a();  break;
		case IEC61851_STATE_B:  iec61851_state_b();  break;
		case IEC61851_STATE_C:  iec61851_state_c();  break;
		case IEC61851_STATE_D:  iec61851_state_d();  break;
		case IEC61851_STATE_EF: iec61851_state_ef(); break;
	}
}

void iec61851_init(void) {
	memset(&iec61851, 0, sizeof(IEC61851));
}

