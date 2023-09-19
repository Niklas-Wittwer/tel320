// Copyright (c) Acconeer AB, 2022
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "acc_detector_presence.h"
#include "acc_hal_definitions.h"
#include "acc_hal_integration.h"
#include "acc_integration.h"
#include "acc_rss.h"
#include "acc_service.h"
#include "acc_version.h"


// Default values for this reference application
// ---------------------------------------------

// Detector configuration settings
#define DEFAULT_SENSOR_ID (1U)
#define RANGE_START_M     (0.12f)
#define RANGE_LENGTH_M    (0.18f)
#define UPDATE_RATE_HZ    (80U)
#define SWEEPS_PER_FRAME  (32U)
#define HWAAS             (60U)
#define PROFILE           (ACC_SERVICE_PROFILE_2)
#define POWER_SAVE_MODE   (ACC_POWER_SAVE_MODE_SLEEP)


// Algorithm constants that could need fine tuning by the user.

// Detection threshold, level at which to trigger a "wave to exit".
#define DETECTION_THRESHOLD (1.4f)

// Cool down threshold, level at which to be able to trigger again.
#define COOL_DOWN_THRESHOLD (1.1f)

// Cool down time, minimal time between triggers in ms.
#define COOL_DOWN_TIME_MS (0)

// Cool down time in ticks, derived from cooldown time and update rate.
#define COOL_DOWN_TIME_TICKS ((COOL_DOWN_TIME_MS * UPDATE_RATE_HZ) / 1000)


/**
 * Configure the detector to the specified configuration
 *
 * @param configuration The detector configuration to configure with application settings
 */
static void configure_detector(acc_detector_presence_configuration_t configuration)
{
	acc_detector_presence_configuration_sensor_set(configuration, DEFAULT_SENSOR_ID);

	acc_detector_presence_configuration_service_profile_set(configuration, PROFILE);
	acc_detector_presence_configuration_start_set(configuration, RANGE_START_M);
	acc_detector_presence_configuration_length_set(configuration, RANGE_LENGTH_M);
	acc_detector_presence_configuration_hw_accelerated_average_samples_set(configuration, HWAAS);
	acc_detector_presence_configuration_sweeps_per_frame_set(configuration, SWEEPS_PER_FRAME);
	acc_detector_presence_configuration_power_save_mode_set(configuration, POWER_SAVE_MODE);
	acc_detector_presence_configuration_update_rate_set(configuration, UPDATE_RATE_HZ);

	acc_detector_presence_configuration_filter_parameters_t filter = acc_detector_presence_configuration_filter_parameters_get(
		configuration);

	filter.intra_frame_weight     = 1.0f;
	filter.intra_frame_time_const = 0.05f;
	filter.output_time_const      = 0.02f;

	acc_detector_presence_configuration_filter_parameters_set(configuration, &filter);
}


int acc_ref_app_wave_to_exit(int argc, char *argv[]);


int acc_ref_app_wave_to_exit(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	printf("Acconeer software version %s\n", acc_version_get());

	const acc_hal_t *hal = acc_hal_integration_get_implementation();

	// Timing
	uint32_t period_length_ms = 1000U / UPDATE_RATE_HZ;

	if (!acc_rss_activate(hal))
	{
		printf("Failed to activate RSS\n");
		return EXIT_FAILURE;
	}

	acc_detector_presence_configuration_t configuration = acc_detector_presence_configuration_create();

	if (configuration == NULL)
	{
		printf("Failed to create detector configuration\n");
		acc_rss_deactivate();
		return EXIT_FAILURE;
	}

	configure_detector(configuration);

	acc_detector_presence_handle_t handle = acc_detector_presence_create(configuration);

	if (handle == NULL)
	{
		printf("Failed to create detector\n");
		acc_detector_presence_configuration_destroy(&configuration);
		acc_detector_presence_destroy(&handle);
		acc_rss_deactivate();
		return EXIT_FAILURE;
	}

	acc_detector_presence_result_t result;

	bool     status       = true;
	bool     cool_trig    = true;
	bool     cool_time    = true;
	uint32_t cool_counter = 0;

	uint32_t last_update_ms = 0;

	status = acc_detector_presence_activate(handle);

	while (status)
	{
		while (last_update_ms != 0 && hal->os.gettime() - last_update_ms < period_length_ms)
		{
			acc_integration_sleep_ms(period_length_ms - (hal->os.gettime() - last_update_ms));
		}

		status         = acc_detector_presence_get_next(handle, &result);
		last_update_ms = hal->os.gettime();

		if (status)
		{
			bool wave_to_exit = false;

			// Trigger "Wave to exit" if presence is detected and cool-down citerias has been met.
			if (result.presence_detected && cool_trig && cool_time)
			{
				wave_to_exit = true;
				cool_trig    = false;
				cool_time    = false;
			}

			// The first cool-down criteria is that the presence score must fall below a threshold
			// before a new "Wave to exit" is triggered
			if (!cool_trig && (result.presence_score < COOL_DOWN_THRESHOLD))
			{
				cool_trig    = true;
				cool_counter = COOL_DOWN_TIME_TICKS;
			}

			// The second cool-down criteria is that a certain time, if set, must elapse before
			// a new "Wave to exit" is triggered
			if (!cool_time && cool_trig)
			{
				if (cool_counter > 0)
				{
					cool_counter -= 1;
				}

				if (cool_counter == 0)
				{
					cool_time = true;
				}
			}

			if (wave_to_exit)
			{
				printf("Wave detected\n");
			}
			else
			{
				printf("No wave detected\n");
			}
		}
	}

	// We will never exit so no need to destroy the configuration or detector

	return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
