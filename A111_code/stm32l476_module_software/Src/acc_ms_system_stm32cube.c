// Copyright (c) Acconeer AB, 2019-2021
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "main.h"

#include "acc_hal_integration.h"
#include "acc_integration.h"
#include "acc_ms_system.h"


const acc_hal_t *acc_ms_system_get_hal_implementation(void)
{
	return acc_hal_integration_get_implementation();
}


void acc_ms_system_set_lowest_power_state(uint32_t req_power_state)
{
	acc_integration_set_lowest_power_state(req_power_state);
}


void acc_ms_system_uart_register_read_callback(acc_ms_system_uart_read_func_t callback)
{
	acc_integration_uart_register_read_callback(callback);
}


uint32_t acc_ms_system_get_max_uart_baudrate(void)
{
	return acc_integration_get_max_uart_baudrate();
}


void acc_ms_system_uart_set_baudrate(uint32_t baudrate)
{
	acc_integration_uart_set_baudrate(baudrate);
}


bool acc_ms_system_uart_write_buffer(const void *buffer, size_t buffer_size)
{
	return acc_integration_uart_write_buffer(buffer, buffer_size);
}


int32_t acc_ms_system_uart_get_error_count(void)
{
	return acc_integration_uart_get_error_count();
}


void *acc_ms_system_mem_alloc(size_t size)
{
	return acc_integration_mem_alloc(size);
}


void acc_ms_system_mem_free(void *ptr)
{
	acc_integration_mem_free(ptr);
}


void acc_ms_system_enable_irq(bool enable)
{
	acc_integration_enable_irq(enable);
}


void acc_ms_system_signal_message(void)
{
	acc_integration_signal_message();
}


void acc_ms_system_wait_for_message(uint32_t timeout_ms)
{
	acc_integration_wait_for_message(timeout_ms);
}


uint32_t acc_ms_system_get_time(void)
{
	return acc_integration_get_time();
}


static acc_ms_sensor_interrupt_callback_t isr_callback;


void acc_ms_system_register_sensor_interrupt_callback(acc_ms_sensor_interrupt_callback_t callback)
{
	isr_callback = callback;
}


bool acc_ms_system_is_sensor_interrupt_active(void)
{
	return acc_integration_is_sensor_interrupt_active();
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == A111_SENSOR_INTERRUPT_Pin)
	{
		if (isr_callback != NULL)
		{
			isr_callback();
		}
	}
}
