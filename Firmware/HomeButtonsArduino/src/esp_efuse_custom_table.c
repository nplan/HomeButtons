/*
 * SPDX-FileCopyrightText: 2017-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "esp_efuse.h"
#include <assert.h>
#include "esp_efuse_custom_table.h"

// md5_digest_table 609a3868897231b1efed44e2d3c5e61d
// This file was generated from the file esp_efuse_custom_table.csv. DO NOT CHANGE THIS FILE MANUALLY.
// If you want to change some fields, you need to change esp_efuse_custom_table.csv file
// then run `efuse_common_table` or `efuse_custom_table` command it will generate this file.
// To show efuse_table run the command 'show_efuse_table'.

static const esp_efuse_desc_t USER_DATA_SERIAL_NUMBER[] = {
    {EFUSE_BLK3, 0, 64}, 	 // Serial number - YYWW1234,
};

static const esp_efuse_desc_t USER_DATA_RANDOM_ID[] = {
    {EFUSE_BLK3, 64, 48}, 	 // Random ID - 123XYZ,
};

static const esp_efuse_desc_t USER_DATA_MODEL_ID[] = {
    {EFUSE_BLK3, 112, 16}, 	 // Model ID - AA,
};

static const esp_efuse_desc_t USER_DATA_HW_VERSION[] = {
    {EFUSE_BLK3, 128, 24}, 	 // Hardware version - 1.1,
};





const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_SERIAL_NUMBER[] = {
    &USER_DATA_SERIAL_NUMBER[0],    		// Serial number - YYWW1234
    NULL
};

const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_RANDOM_ID[] = {
    &USER_DATA_RANDOM_ID[0],    		// Random ID - 123XYZ
    NULL
};

const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_MODEL_ID[] = {
    &USER_DATA_MODEL_ID[0],    		// Model ID - AA
    NULL
};

const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_HW_VERSION[] = {
    &USER_DATA_HW_VERSION[0],    		// Hardware version - 1.1
    NULL
};
