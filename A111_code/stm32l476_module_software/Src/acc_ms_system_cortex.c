// Copyright (c) Acconeer AB, 2019-2021
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include "acc_integration.h"
#include "acc_ms_system.h"


void acc_ms_system_critical_section_enter(void)
{
	acc_integration_critical_section_enter();
}


void acc_ms_system_critical_section_exit(void)
{
	acc_integration_critical_section_exit();
}
