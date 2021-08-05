/**
 * Partly Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Partly Copyright (c) 2020 Pimoroni Ltd.
 * Partly Copyright (c) 2021 Paulus Schulinck @paulsk. 
 * 2021-06-30 Starting to combine rtc and rgc encoder apps to one app
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSK_ENCODER_HH
#define PSK_ENCODER_HH

//#include "../../main/main.hpp"
#include "encoder.hpp"
//#include "../../common/pimoroni_common.hpp"

//#include "../../libraries/pico_explorer/pico_explorer2.hpp"

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "pico/stdlib.h"
#include "../../../../../pico-sdk/src/rp2_common/hardware_gpio/include/hardware/gpio.h"
#include "../../../../../pico-sdk/src/rp2_common/hardware_i2c/include/hardware/i2c.h"
#include "../../../../../pico-sdk/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h"

//using namespace pimoroni;


int encoder_doit(int16_t *ptr) 
{

  int16_t * my_exp = ptr;

  bool lStart = true;
  int16_t nw_count = 0;
  std::string btn = "0";
  std::string s = "";
  bool lStart = true;
  bool toggle = false;

  Cnt_it my_ctr; // instantiate object

  /* the background colour will be fixed.  We're going to choose the foreground colour */
  set_DispColour(true, bgnd_colour2, true);  
  sleep_ms(20);
  set_DispColour(true, fgnd_colour2, false);
  disp_a_txt("encoder", 0);
  disp_a_txt("test", 1);
  sleep_ms(2000);
  my_ctr.clr_cnts(); // zero all private count values

  if (enc.init()) 
  {
    enc.clear_interrupt_flag();  // added to free evt. interrupt locks
    bgnd_colour2 =  DISP_AUBERGINE2;
    fgnd_colour2 =  DISP_YELLOW2;
    set_DispColour(true, bgnd_colour2, true);  // clear screen in colour
    sleep_ms(20);
    set_DispColour(true, fgnd_colour2, false);
    disp_a_txt("enc found", 1);
    intro(1);  // Display text about encoder found
    sleep_ms(1000);
    set_DispColour(true, bgnd_colour2, true);  // clear screen in colour
    sleep_ms(20);
    set_DispColour(true, fgnd_colour2, false);
    enc.clear_interrupt_flag();
    
    while(true) 
    {
      gpio_put(PICO_DEFAULT_LED_PIN, toggle);
      toggle = !toggle;
      if (enc.get_interrupt_flag()) 
      {
        nw_count = enc.read();  // the read() calls the ioe.clear_interrupt_flag()
        if (lStart || (nw_count != my_ctr.have_old_cnt())) {  // show count also at startup
          my_ctr.upd_cnt(nw_count);
          count_changed();
          disp_cnt();
          if (lStart)
            lStart = false;
        }
        //enc.clear_interrupt_flag();
      }
      ck_btns(false); // Check if a btn has been pressed. If so, handle it
      if (my_ctr.is_stp())
      {
        /** 
        *  https://www.raspberrypi.org/forums/viewtopic.php?p=1870355, post by cleverca on 2021-05-27.
        *  \param pc If Zero, a standard boot will be performed, if non-zero this is the program counter to jump to on reset.
        *  \param sp If \p pc is non-zero, this will be the stack pointer used.
        *  \param delay_ms Initial load value. Maximum value 0x7fffff, approximately 8.3s.
        *
        * *            (pc, sp, delay_ms) */
        clr_btns();
        disp_a_txt("reset !", 3);
        //sleep_ms(1000);
        /*            (pc, sp, delay_ms) */
        watchdog_reboot(0, 0, 0x7fffff); // varying delay_ms between 0x1fffff and 0x7fffff does not make much difference!
      }
      else if (my_ctr.IsBtnPressed())
      {
        sleep_ms(1000); // let the 'btn not in use' msg stay for a moment
        disp_cnt();  // method to wipe away btn msg
      }
      sleep_ms(100); // loop delay
    }
    sleep_ms(100); // loop delay
  }
  return 0;
}

#endif // PSK_MAIN_HH