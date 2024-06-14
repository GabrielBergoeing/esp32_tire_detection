/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "detection_responder.h"
#include "tensorflow/lite/micro/micro_log.h"

#include "esp_main.h"
#include "driver/gpio.h"
#define LED_PIN GPIO_NUM_4
#if DISPLAY_SUPPORT
#include "image_provider.h"
#include "bsp/esp-bsp.h"

// Camera definition is always initialized to match the trained detection model: 96x96 pix
// That is too small for LCD displays, so we extrapolate the image to 192x192 pix
#define IMG_WD (96 * 2)
#define IMG_HT (96 * 2)

static lv_obj_t *camera_canvas = NULL;
static lv_obj_t *person_indicator = NULL;
static lv_obj_t *label = NULL;

static void create_gui(void)
{
  bsp_display_start();
  bsp_display_backlight_on(); // Set display brightness to 100%
  bsp_display_lock(0);
  camera_canvas = lv_canvas_create(lv_scr_act());
  assert(camera_canvas);
  lv_obj_align(camera_canvas, LV_ALIGN_TOP_MID, 0, 0);

  person_indicator = lv_led_create(lv_scr_act());
  assert(person_indicator);
  lv_obj_align(person_indicator, LV_ALIGN_BOTTOM_MID, -70, 0);
  lv_led_set_color(person_indicator, lv_palette_main(LV_PALETTE_GREEN));

  label = lv_label_create(lv_scr_act());
  assert(label);
  lv_label_set_text_static(label, "Person detected");
  lv_obj_align_to(label, person_indicator, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
  bsp_display_unlock();
}
#endif // DISPLAY_SUPPORT

static void init_gpio(void) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

void RespondToDetection(float flat_tire_score, float full_tire_score, float no_tire_score) {
  int flat_tire_score_int = (flat_tire_score) * 100 + 0.5;
  int full_tire_score_int = (full_tire_score) * 100 + 0.5;
  int no_tire_score_int = (no_tire_score) * 100 + 0.5;

  if (full_tire_score_int >= 60) {
    gpio_set_level(LED_PIN, 1);
  } else {
    gpio_set_level(LED_PIN, 0);
  }

#if DISPLAY_SUPPORT
  if (!camera_canvas) {
    create_gui();
  }

  uint16_t *buf = (uint16_t *) image_provider_get_display_buf();

  bsp_display_lock(0);
  if (full_tire_score_int < 60 && full_tire_score_int < 60 && no_tire_score_int < 60) {
    lv_led_off(class_indicator);
  } else {
    lv_led_on(class_indicator);
  }
  lv_canvas_set_buffer(camera_canvas, buf, IMG_WD, IMG_HT, LV_IMG_CF_TRUE_COLOR);
  bsp_display_unlock();
#endif // DISPLAY_SUPPORT

  MicroPrintf("Flat Tire score:%d%%, Full Tire score %d%%, No Tire score %d%%",
              flat_tire_score_int, full_tire_score_int, no_tire_score_int);
  init_gpio();
}