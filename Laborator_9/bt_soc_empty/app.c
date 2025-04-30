/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "gatt_db.h"
#include "em_timer.h"
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Flags
volatile bool button_pressed = false;
bool button_notify_enabled = false;


void GPIO_ODD_IRQHandler(void)
{
  // Stergere flag intrerupere
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);
  button_pressed = true;
}

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  // Activare ramura clock periferic GPIO
  CMU_ClockEnable(cmuClock_GPIO, true);

  // LED: PA4 as push-pull output
  GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 0);

  // Button: PC7 as input with pull filter
  GPIO_PinModeSet(gpioPortC, 7, gpioModeInputPullFilter, 1);
  GPIO_ExtIntConfig(gpioPortC, 7, 7, true, true, true);

  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);

}

/**************************************************************************//**
 * Application Process Action
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  if (button_pressed) {
    button_pressed = false;
    uint8_t btn_val = GPIO_PinInGet(gpioPortC, 7) ? 1 : 0;

    sl_bt_gatt_server_write_attribute_value(gattdb_BUTTON_IO, 0, sizeof(btn_val), &btn_val);

    if (button_notify_enabled) {
      sl_bt_gatt_server_notify_all(gattdb_BUTTON_IO, sizeof(btn_val), &btn_val);
    }
  }
}

/**************************************************************************//**
 * Bluetooth event handler
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t recv_val;
  size_t recv_len;

  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      //display only
      sc = sl_bt_sm_configure(0, sl_bt_sm_io_capability_displayonly);
      app_assert_status(sc);

      //bondable_mode
      sc = sl_bt_sm_set_bondable_mode(1);
      app_assert_status(sc);

       //gen passkey
      sc = sl_bt_sm_set_passkey(123456);
      app_assert_status(sc);

      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      sc = sl_bt_advertiser_set_timing(advertising_set_handle, 160, 160, 0, 0);
      app_assert_status(sc);

      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      //increase security
      sc = sl_bt_sm_increase_security(evt->data.evt_connection_opened.connection);
      app_assert_status(sc);

      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_LED_IO) {
        sl_bt_gatt_server_read_attribute_value(gattdb_LED_IO, 0, sizeof(recv_val), &recv_len, &recv_val);
        if (recv_val) {
          GPIO_PinOutSet(gpioPortA, 4);
        }

        else {
          GPIO_PinOutClear(gpioPortA, 4);
        }
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_BUTTON_IO) {
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_notification) {
          button_notify_enabled = true;
        } else {
          button_notify_enabled = false;
        }
      }
      break;

   // case sl_bt_evt_connection_parameters_id:
   //   if(evt->evt_gatt_server_characteristic_status.sl_bt_evt_connection_parameters)

    //  break;

    //print passkey
    case sl_bt_evt_sm_passkey_display_id:
      printf("Enter this passkey on your phone: %06lu\n", evt->data.evt_sm_passkey_display.passkey);
      break;

//print bonded
    case sl_bt_evt_sm_bonded_id:
      printf("Bonding successful.\n");
      break;

//print failed
    case sl_bt_evt_sm_bonding_failed_id:
      printf("Bonding failed. Reason: 0x%02x\n", evt->data.evt_sm_bonding_failed.reason);
      break;

    default:
      break;
  }
}
