// Copyright (c) Acconeer AB, 2019-2023
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

#include "acc_integration.h"


#define STM32_MAX_BAUDRATE 1000000

#define UART_RX_MAX_PACKET_SIZE 10


static void acc_integration_start_uart_rx_dma(void);


static void USART_Handle_RTO(void);


extern UART_HandleTypeDef MS_UART_HANDLE;

/**
 * @brief Interrupt handlers
 */
typedef struct
{
	acc_integration_uart_read_func_t uart_callback;
	uint8_t                          rx_buffer[UART_RX_MAX_PACKET_SIZE];
	int                              error_count;
	UART_HandleTypeDef               *inst;
} uart_handle_t;

static uart_handle_t uart_handle =
{
	.uart_callback = NULL,
	.error_count   = 0,
	.inst          = &MS_UART_HANDLE,
};

static volatile bool uart_tx_complete;
static volatile bool signal_active;


static inline void disable_interrupts(void)
{
	__disable_irq();
	__DSB();
	__ISB();
}


static inline void enable_interrupts(void)
{
	__enable_irq();
	__DSB();
	__ISB();
}


void acc_integration_sleep_ms(uint32_t time_msec)
{
	HAL_Delay(time_msec);
}


void acc_integration_sleep_us(uint32_t time_usec)
{
	uint32_t time_msec = (time_usec / 1000) + 1;

	HAL_Delay(time_msec);
}


void acc_integration_set_lowest_power_state(uint32_t req_power_state)
{
	(void)req_power_state;
}


static void acc_integration_start_uart_rx_dma(void)
{
	/* Abort old Receive_DMA */
	HAL_UART_AbortReceive(uart_handle.inst);

	/* Enable Receiver Timeout to be able to receive packets shorter than a full DMA packet */
	if (HAL_UART_EnableReceiverTimeout(uart_handle.inst) != HAL_OK)
	{
		Error_Handler();
	}

	/* Setup Receiver Timeout length */
	HAL_UART_ReceiverTimeout_Config(uart_handle.inst, 100);

	/* Start new DMA receive */
	HAL_UART_Receive_DMA(uart_handle.inst, uart_handle.rx_buffer, sizeof(uart_handle.rx_buffer));
}


static void USART_Handle_RTO(void)
{
	if (uart_handle.uart_callback == NULL)
	{
		return;
	}

	/* Do we have pending RX bytes? */
	size_t packet_length = sizeof(uart_handle.rx_buffer) - __HAL_DMA_GET_COUNTER(uart_handle.inst->hdmarx);

	if (packet_length > 0)
	{
		/* Process the pending bytes of data */
		for (size_t idx = 0; idx < packet_length; idx++)
		{
			uart_handle.uart_callback(uart_handle.rx_buffer[idx], 0);
		}
	}
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *h_uart)
{
	if (h_uart->ErrorCode == HAL_UART_ERROR_RTO)
	{
		USART_Handle_RTO();
	}
	else
	{
		uart_handle.error_count++;

		/* Error occured, abort and prepare for another packet */
		HAL_UART_AbortReceive(uart_handle.inst);
		HAL_UART_AbortTransmit(uart_handle.inst);
	}

	/* Prepare for another packet, no need to setup receive timeout again */
	HAL_UART_Receive_DMA(uart_handle.inst, uart_handle.rx_buffer, sizeof(uart_handle.rx_buffer));
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *h_uart)
{
	(void)h_uart;

	uart_tx_complete = true;
}


void acc_integration_uart_register_read_callback(acc_integration_uart_read_func_t callback)
{
	if (uart_handle.uart_callback == NULL)
	{
		uart_handle.uart_callback = callback;
		acc_integration_start_uart_rx_dma();
	}
	else
	{
		uart_handle.uart_callback = callback;
	}
}


uint32_t acc_integration_get_max_uart_baudrate(void)
{
	return STM32_MAX_BAUDRATE;
}


void acc_integration_uart_set_baudrate(uint32_t baudrate)
{
	if (baudrate <= STM32_MAX_BAUDRATE)
	{
		HAL_UART_AbortReceive(uart_handle.inst);
		HAL_UART_DeInit(uart_handle.inst);
		uart_handle.inst->Init.BaudRate = baudrate;
		if (HAL_UART_Init(uart_handle.inst) != HAL_OK)
		{
			Error_Handler();
		}

		acc_integration_start_uart_rx_dma();
	}
}


bool acc_integration_uart_write_buffer(const void *buffer, size_t buffer_size)
{
	HAL_StatusTypeDef hal_status = HAL_BUSY;

	uart_tx_complete = false;

	/**
	 * Disable interrupts while accessing the UART through the HAL.
	 * This is needed because we access and restart the UART
	 * within interrupt context:
	 * The HAL_UART_Transmit_DMA will trigger a lock function in the HAL.
	 * If an interrupt occur before we exit this function we can end up with
	 * a HAL_BUSY return from the function HAL_UART_Receive_DMA.
	 */
	acc_integration_critical_section_enter();
	hal_status = HAL_UART_Transmit_DMA(uart_handle.inst, (uint8_t *)buffer, buffer_size);
	acc_integration_critical_section_exit();

	if (hal_status != HAL_OK)
	{
		return false;
	}

	while (!uart_tx_complete)
	{
		// Turn off interrupts
		disable_interrupts();
		// Check once more so that the interrupt have not occurred
		if (!uart_tx_complete)
		{
			__WFI();
		}

		// Enable interrupt again, the ISR will execute directly after this
		enable_interrupts();
	}

	return true;
}


int32_t acc_integration_uart_get_error_count(void)
{
	return uart_handle.error_count;
}


void acc_integration_enable_irq(bool enable)
{
	(void)enable;
}


bool acc_integration_is_sensor_interrupt_active(void)
{
	return HAL_GPIO_ReadPin(A111_SENSOR_INTERRUPT_GPIO_Port, A111_SENSOR_INTERRUPT_Pin) == GPIO_PIN_SET;
}


void acc_integration_signal_message(void)
{
	signal_active = true;
}


void acc_integration_wait_for_message(uint32_t timeout_ms)
{
	uint32_t start = HAL_GetTick();

	while (!signal_active && (HAL_GetTick() - start) < timeout_ms)
	{
		// Turn off interrupts
		disable_interrupts();
		// Check once more so that the interrupt have not occurred
		if (!signal_active)
		{
			__WFI();
		}

		// Enable interrupt again, the ISR will execute directly after this
		enable_interrupts();
	}

	// Reset for next call
	signal_active = false;
}


uint32_t acc_integration_get_time(void)
{
	return HAL_GetTick();
}


void *acc_integration_mem_alloc(size_t size)
{
	return malloc(size);
}


void *acc_integration_mem_calloc(size_t nmemb, size_t size)
{
	return calloc(nmemb, size);
}


void acc_integration_mem_free(void *ptr)
{
	free(ptr);
}
