// Copyright (c) Acconeer AB, 2019-2023
// All rights reserved

#ifndef ACC_INTEGRATION_H_
#define ACC_INTEGRATION_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


struct acc_integration_thread_handle;

typedef struct acc_integration_thread_handle *acc_integration_thread_handle_t;

struct acc_integration_mutex;

typedef struct acc_integration_mutex *acc_integration_mutex_t;

struct acc_integration_semaphore;

typedef struct acc_integration_semaphore *acc_integration_semaphore_t;

typedef void (*acc_integration_uart_read_func_t)(uint8_t data, uint32_t status);


/**
 * @brief Create thread function
 *
 * @param func Thread func
 * @param param Thread func parameters
 * @param name Name of thread
 *
 * @return A thread handle
 */
acc_integration_thread_handle_t acc_integration_thread_create(void (*func)(void *param), void *param, const char *name);


/**
 * @brief Clean up thread
 *
 * @param handle A thread handle
 */
void acc_integration_thread_cleanup(acc_integration_thread_handle_t handle);


/**
 * @brief Create a mutex
 *
 * @return A mutex
 */
acc_integration_mutex_t acc_integration_mutex_create(void);


/**
 * @brief Destroy a mutex
 *
 * @param mutex A mutex
 */
void acc_integration_mutex_destroy(acc_integration_mutex_t mutex);


/**
 * @brief Lock a mutex
 *
 * @param mutex A mutex
 */
void acc_integration_mutex_lock(acc_integration_mutex_t mutex);


/**
 * @brief Unlock a mutex
 *
 * @param mutex A mutex
 */
void acc_integration_mutex_unlock(acc_integration_mutex_t mutex);


/**
 * @brief Creates a semaphore and returns a pointer to the newly created semaphore
 *
 * @return A pointer to the semaphore on success otherwise NULL
 */
acc_integration_semaphore_t acc_integration_semaphore_create(void);


/**
 * @brief Waits for the semaphore to be available. The task calling this function will be
 * blocked until the semaphore is signaled from another task.
 *
 * @param[in]  sem A pointer to the semaphore to use
 * @param[in]  timeout_ms The amount of time to wait before a timeout occurs
 * @return Returns true on success and false on timeout
 */
bool acc_integration_semaphore_wait(acc_integration_semaphore_t sem, uint16_t timeout_ms);


/**
 * @brief Signal the semaphore.
 *
 * @param[in]  sem A pointer to the semaphore to signal
 */
void acc_integration_semaphore_signal(acc_integration_semaphore_t sem);


/**
 * @brief Deallocates the semaphore
 *
 * @param[in]  sem A pointer to the semaphore to deallocate
 */
void acc_integration_semaphore_destroy(acc_integration_semaphore_t sem);


/**
 * @brief Set the power state the system can go at the lowest
 *
 * @param[in] req_power_state power state
 */
void acc_integration_set_lowest_power_state(uint32_t req_power_state);


/**
 * @brief Used by the client to register a read callback
 *
 * @param callback Function pointer to the callback, NULL to disable
 */
void acc_integration_uart_register_read_callback(acc_integration_uart_read_func_t callback);


/**
 * @brief Get max supported baudrate
 *
 * @return The maximum supported baudrate for this module
 */
uint32_t acc_integration_get_max_uart_baudrate(void);


/**
 * @brief Set baudrate of the UART
 *
 * @param baudrate Baudrate to use
 */
void acc_integration_uart_set_baudrate(uint32_t baudrate);


/**
 * @brief Send array of data on UART
 *
 * @param buffer      The data buffer to be transmitted
 * @param buffer_size The size of the buffer
 *
 * @return True if successful, false otherwise
 */
bool acc_integration_uart_write_buffer(const void *buffer, size_t buffer_size);


/**
 * @brief Get the error count for all UART ports, typically overrun errors when receiving data
 *
 * @return 0 if no errors occurred or -1 if not implemented
 */
int32_t acc_integration_uart_get_error_count(void);


/**
 * @brief Signal that a new message have been posted to any of the queues
 */
void acc_integration_signal_message(void);


/**
 * @brief Wait until a message have been received
 *
 * @param[in] timeout_ms Maximum amount of time to wait in ms
 */
void acc_integration_wait_for_message(uint32_t timeout_ms);


/**
 * @brief Perform initialization related to the operating system used. Called during startup.
 */
void acc_integration_os_init(void);


/**
 * @brief Function to get the current state of the sensor interrupt.
 *
 * @returns true if the sensor interrupt is active
 */
bool acc_integration_is_sensor_interrupt_active(void);


/**
 * @brief Sleep for a specified number of microseconds
 *
 * @param time_usec Time in microseconds to sleep
 */
void acc_integration_sleep_us(uint32_t time_usec);


/**
 * @brief Sleep for a specified number of milliseconds
 *
 * @param time_msec Time in milliseconds to sleep
 */
void acc_integration_sleep_ms(uint32_t time_msec);


/**
 * @brief Allocate dynamic memory
 *
 * @param[in]  size The bytesize of the reuested memory block
 * @return Returns either NULL or a unique pointer to a memory block
 */
void *acc_integration_mem_alloc(size_t size);


/**
 * @brief Allocate dynamic memory
 *
 * Allocate an array of nmemb elements of size bytes each.
 *
 * @param[in]  nmemb The number of elements in the array
 * @param[in]  size The bytesize of the element
 * @return Returns either NULL or a unique pointer to a memory block
 */
void *acc_integration_mem_calloc(size_t nmemb, size_t size);


/**
 * @brief Free dynamic memory
 *
 * @param[in]  ptr A pointer to the memory space to be freed
 */
void acc_integration_mem_free(void *ptr);


/**
 * Enter a critical section
 */
void acc_integration_critical_section_enter(void);


/**
 * Exit a critical section
 */
void acc_integration_critical_section_exit(void);


/**
 * @brief Get current time
 *
 * It is important that this value wraps correctly and uses all bits. I.e. it should count
 * upwards to 2^32 - 1 and then 0 again.
 *
 * @returns Current time as milliseconds
 */
uint32_t acc_integration_get_time(void);


/**
 * @brief Enable and disable IRQ
 *
 * @param enable True for enable and false for disable
 */
void acc_integration_enable_irq(bool enable);


#endif
