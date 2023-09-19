// Copyright (c) Acconeer AB, 2019-2022
// All rights reserved

#ifndef ACC_MODULE_SERVER_H_
#define ACC_MODULE_SERVER_H_

#include <stdint.h>
/*
 * Main entry function for the module server.
 *
 * @param[in] product_id The product ID of this product. This value can be read from the product id register.
 */
int ms_main(uint32_t product_id);


/*
 * Below functions should be called prior to calling ms_main() to enable
 * the modes that is desired. Modes that are not enabled will not be supported
 * and the linker can discard the code which reduces the binary size.
 */
void acc_ms_enable_distance(void);


void acc_ms_enable_envelope(void);


void acc_ms_enable_iq(void);


void acc_ms_enable_obstacle(void);


void acc_ms_enable_power_bin(void);


void acc_ms_enable_presence(void);


void acc_ms_enable_sparse(void);


void acc_ms_enable_i2c(void);


#endif
