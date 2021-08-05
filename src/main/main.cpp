/**
 * Partly Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Paryly Copyright (c) 2020 Pimoroni Ltd.
 * Partly Copyright (c) 2021 Paulus Schulinck @paulsk. 
 * 2021-06-30 10h30 start of combining the rtc and the encoder apps to one rtc_encoder app.
 * 2021-07-01 at 19h15 PT the app was working.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Note @paulsk dd 2021-04-21 at 13h12 PT
 * I continue to build this project from within MS Windows WSL1 / Ubuntu 20.04 because of 
 * linker problems I faced when doing the build * from withing VSCode in MS Windows 10 Pro.
 */
#ifndef PSK_MAIN
#define PSK_MAIN

#include "main.hpp"
//#include "../../common/pimoroni_common.hpp"
//#include "../../libraries/breakout_encoder/breakout_encoder.hpp"
#include "../../libraries/pico_explorer/pico_explorer.hpp"
#include "../../libraries/pico_graphics/pico_graphics.hpp"
#include "../../libraries/pico_graphics/font8_data.hpp"
//#include "../../drivers/rv3028/rv3028.hpp"

//#include <stdio.h>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <iostream>
#include <bitset>
#include <sstream>
#include <cstdint>  // uint8_t etc.
#include <iomanip>
// Next includes from pimoroni/pico-examples/explorer/demo.cpp
#include <string>
#include <string.h>
#include <math.h>
#include <vector>
#include <array>
#include <algorithm>
#include <float.h>
//#include <cstdlib>

#include "../../../../../pico-sdk/src/common/pico_base/include/pico/types.h" // at the end of this file: typedef struct of datetime_th"
#include "../../../../../pico-sdk/src/common/pico_util/include/pico/util/datetime.h"
#include "../../../../../pico-sdk/src/common/pico_stdlib/include/pico/stdlib.h"

#include "../../../../../pico-sdk/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h"
#include "../../../../../pico-sdk/src/rp2_common/hardware_gpio/include/hardware/gpio.h"
#include "../../../../../pico-sdk/src/rp2_common/hardware_rtc/include/hardware/rtc.h"
#include "../../../../../pico-sdk/src/rp2_common/pico_platform/include/pico/platform.h"  // has things like "count_of"
#include "../../../../../pico-sdk/src/rp2_common/pico_unique_id/include/pico/unique_id.h"

#include "../../../../../micropython/py/vstr.c"

/*
Modifications by @paulsk, starting at 2021-04-05, for use with RPi Pico on a Pimoroni pico explorer base
to make use of the Pimoroni external rv3028 rtc breakout, instead of the Pico's internal rtc).
Note that I added the following functions to rv3028.hpp (main.hpp) and rtc.c (main.cpp):
	bool rtc.get_time(stDateTime *t);
	uint8_t rtc.yearday();
	bool rtc.isleapyear(stDateTime *t);
	uint8_t rtc.days_in_month(int i);
*/

using namespace pimoroni;

uint16_t buf[PicoExplorer::WIDTH * PicoExplorer::HEIGHT];
PicoExplorer pico_explorer(buf); // create an instance of the PicoExplorer object


Cnt_it my_ctr; // instantiate rotary encoder object MOVED TO paulsk_common.hpp

pico_unique_board_id_t p_ID;

bool lStDT = true;
bool t_chgd = false;  // See btn funcs
bool lastHrOfDay = false;
bool ledIsOn = false;
int rgb_h = 0;
bool lUSB;
bool lRTC = false;
bool lENC = false;
int Ut = 0;
int UltItmChgdTime = 0;
RV3028 rtc;  // Create instance of RV3028 object

bool a_itm_handler()
{
  bool dummy;
  uint8_t n = (uint8_t)1000;
  dummy = reset_itm(); // Check if itm value has to be reset to ITM_HELP
  std::string TAG = "a_itm_handler(): button ";
  std::string SFX = " pressed";
  if (pico_explorer.is_pressed(pico_explorer.A))
  {
    std::cout << TAG+"A"+SFX << std::endl;
    btns_itm[BTN_A] = 1;
    if (menu == MENU_MAIN)
      dummy = btn_ab_itm(true);
    else if  (menu == MENU_SETTINGS)
      dummy = btn_ab_settings(true);

    sleep_ms(n); // This delay and its value are important to prevent debounce effect
    return true;
  }
  if (pico_explorer.is_pressed(pico_explorer.B))
  {
    std::cout << TAG+"B"+SFX << std::endl;
    btns_itm[BTN_B] = 1;
    if (menu == MENU_MAIN)
      dummy = btn_ab_itm(false);
    else if (menu == MENU_SETTINGS)
      dummy = btn_ab_settings(false);

    sleep_ms(n);
    return true;
  }
  if (pico_explorer.is_pressed(pico_explorer.X))
  {
    std::cout << TAG+"X"+SFX << std::endl;
   btns_itm[BTN_X] = 1;
    std::cout << "btns: " << std::bitset<4>(btns_itm) << std::endl;
    if (menu == MENU_MAIN)
      btn_xy_itm(true);
    else if (menu == MENU_SETTINGS)
      btn_xy_settings(true);
    sleep_ms(n);
    return true;
  }
  if (pico_explorer.is_pressed(pico_explorer.Y))
  {
    std::cout << TAG+"Y"+SFX << std::endl;
    btns_itm[BTN_Y] = 1;
    std::cout << "btns: " << std::bitset<4>(btns_itm) << std::endl;
    if (menu == MENU_MAIN)
      btn_xy_itm(false);
    else if (menu == MENU_SETTINGS)
      btn_xy_settings(false);
    sleep_ms(n);
    return true;
  }
  else
    return false;
}

int a_menu_handler()
{
  switch(menu)  // global menu defined in main.hpp
  {
    case MENU_MAIN:
      a_itm_handler();
      break;
    case MENU_SETTINGS:
      /*
      if (menu == MENU_MAIN)
        btn_msgd("In main", 0);
      else if (menu == MENU_SETTINGS)
        btn_msgd("In settings", 0);
      */
      a_itm_handler();
      //a_settings_handler();
      break;
    case MENU_BACK:
      btn_msgd("back to Main", 0);
      if (menu - 1 > MENU_MAIN)
        menu--;
      break;
  }
  return menu;
}

/*
  User address space in EEPROM: addresses 00h - 2Ah (= 43 bytes) (see: page 40 of the RV3028-C7 manual)
  and User RAM1 1Fh and User RAM2 20h (see: page 41 of the RV3028-C7 manual)
*/

bool backp_to_EP()
{
  // ToDo: this function has to be checked and tested (2021-05-07).
  /*
    Notes @paulsk:
    see rv3028.cpp: function: rtc.writeMultipleRegisters(uint8_t addr, uint8_t * values, uint8_t len)
  */
  static const std::string TAG = "backp_to_EP(): ";
  static const std::string led_str1 = "Value of the EEPROM USER RAM1 register, LED On/Off bit (bit 0) ";
  static const std::string led_str2 = "changing it is: ";
  static const std::string ram_str1 = "Value of the EEPROM USER RAM2 register, ";
  static const std::string ram_str2 = "ground colour before changing it is: ";
  static const std::string clr_fg = "fore";
  static const std::string clr_bg = "back";
  static const std::string ep_busy = "EEPROM busy. Not able to write data to user EEPROM memory.";

  std::cout << TAG+"entering..." << std::endl;
  usr_dat_len_at_backup = 3; // Save dat_len to a global variable (to keep until the moment of calling rest_fm_EP() )
  bool success = false;
  int loop_cnt = 0;
  uint8_t led = (uint8_t)v_settings.at(SETT_LED); // get the value of the LED On/Off
  uint8_t fg = (uint8_t)v_settings.at(SETT_FGND); // get the value of the foreground
  uint8_t bg = (uint8_t)v_settings.at(SETT_BGND); // get the value of the background
  uint8_t usr_ram1_val = 0;
  uint8_t usr_ram2_val = 0;
  std::cout << "RTC User volatile RAM memory \'USER_RAM1\' usage map:" << std::endl;
  std::cout << "++---+----+----+----++----+----+----+----++" << std::endl;
  std::cout << "| b7 | b6 | b5 | b4 || b3 | b2 | b1 | b0  |" << std::endl;
  std::cout << "++---+----+----+----++----+----+----+----++  RV3028_USER_RAM1" << std::endl;
  std::cout << "|    |    |    |    ||    |    |    | LED |" << std::endl;
  std::cout << "++---+----+----+----++----+----+----+----++\n" << std::endl;

  if (rtc.waitforEEPROM())
  {
    usr_ram1_val = rtc.readRegister(RV3028_USER_RAM1);
    //usr_ram1_val = rtc.readConfigEEPROM_RAMmirror(RV3028_USER_RAM1);
    if (usr_ram1_val == 0xFF)
      std::cout << TAG+"reading from EEPROM USR_RAM1 failed" << std::endl;  
    else
    {
      buffer backp_bfr1 {usr_ram1_val};
      std::cout << TAG+led_str1+"\nbefore "+led_str2 << "\'0x" << hexify(backp_bfr1) << "\'" << std::endl;
      // 1st action: write the LED On/Off data
      usr_ram1_val &= 0xFE;  // Clear b0
      if (led)
        usr_ram1_val |= led;   // Set b0
      else
        usr_ram1_val &= 0;  // Clear b0;

    //Shift values into EEPROM Backup Register
      success = rtc.writeRegister(RV3028_USER_RAM1, usr_ram1_val);
      //success = rtc.writeConfigEEPROM_RAMmirror(RV3028_USER_RAM1, usr_ram1_val);
    }
  }
  if (!success)
  {
    buffer backp_ads1 {RV3028_USER_RAM1};
    std::cout << TAG+ "backup to EEPROM of LED On/Off status failed" << std::endl;
    std::cout << TAG+"backup of LED data to memory register # 0x" << hexify(backp_ads1) << " failed" << std::endl;  
  }
  else
  {
    buffer backp_bfr1a{led};
    std::cout << TAG+led_str1+"\nafter "+led_str2+" 0x" << hexify(backp_bfr1a) << std::endl;
    // Check the value backup'd:
    usr_ram1_val = rtc.readRegister(RV3028_USER_RAM1);
    if (usr_ram1_val == 0xFF)
    {
      std::cout << TAG+"verification (2nd) reading from EEPROM USR_RAM1 failed" << std::endl;
      success = false;
    }
    else
    {
      buffer backp_bfr1a {usr_ram1_val};
      buffer mem_ads {RV3028_USER_RAM1};
      std::cout << TAG+"recheck: value retrieved from backup memory register # 0x" << hexify(mem_ads) << "\nof USR RAM1 is: \'0x" << hexify(backp_bfr1a) << "\'" << std::endl;
    }

    // 2nd action: write the LCD background colour setting to USER_RAM2, lower 4 bits
    std::cout << "RTC User volatile RAM memory \'USER_RAM2\' usage map:" << std::endl;
    std::cout << "++---+----+----+----++----+----+----+----++" << std::endl;
    std::cout << "| b7 | b6 | b5 | b4 || b3 | b2 | b1 | b0  |" << std::endl;
    std::cout << "++---+----+----+----++----+----+----+----++  RV3028_USER_RAM2" << std::endl;
    std::cout << "| fg | fg | fg | fg || bg | bg | bg | bg  |" << std::endl;
    std::cout << "++---+----+----+----++----+----+----+----++\n" << std::endl;
    /*
    *   
    *  There are eight background colours defined for the LCD. The colour that is active, it's disp_colour_order value (0 ~ 7) will be written to the LDB
    *  Only one background colour of the eight colours can be active.
    *  The background colours which are not active, their respective bit will be cleared to '0'
    * 
    *  There are also eight foreground colours defined for the LCD. The colour that is active,it's disp_colour_order value (0 ~ 7) will be written to the LDB
    *  Only one foreground colour of the eight colours can be active.
    *  The foreground colours which are not active, their respective bit will be cleared to '0'
    */

    std::cout << TAG+ram_str1+clr_bg+ram_str2 << std::to_string(bg) << std::endl;
    std::cout << TAG+ram_str1+clr_fg+ram_str2 << std::to_string(fg) << std::endl;
    // Read the EEPROM USER_DATA2 register value
    usr_ram2_val = rtc.readRegister(RV3028_USER_RAM2);
    buffer backp_bfr2 {usr_ram2_val};
    //usr_ram2_val = rtc.readConfigEEPROM_RAMmirror(RV3028_USER_RAM2);
    if (usr_ram2_val == 0xFF)
    {
      std::cout << TAG+"reading from memory register # 0x" << hexify(backp_bfr2) << " failed" << std::endl;  
      success = false;
    }
    else
    {
      buffer mem_ads {RV3028_USER_RAM2};
      std::cout << TAG+"value read from memory register # 0x" << hexify(mem_ads) << " is: \'" << hexify(backp_bfr2) << "\'" << std::endl;
      // 2nd action: write the LCD background colour setting to USER_RAM2, lower 4 bits
      usr_ram2_val &= 0x00;  // clear all the bits 7~0
      usr_ram2_val |= bg;   // logical OR the background colour value into the LSB
      // 3rd action: write the LCD foreground colour setting to USER_RAM2, higher 4 bits
      usr_ram2_val |= (fg << 4); // logical OR the foreground colour value and shift left into the MSB
      success = rtc.writeRegister(RV3028_USER_RAM2, usr_ram2_val);
      //success = rtc.writeConfigEEPROM_RAMmirror(RV3028_USER_RAM2, usr_ram2_val);
      if (success)
      {
        std::cout << TAG+"USER RAM2 value: \'" << hexify(backp_bfr2) << "\'\nsuccessfully written to memory address: 0x" << hexify(mem_ads) << std::endl;
      }
      else
        std::cout << TAG+ "backup to memory register # 0x" << hexify(mem_ads) << "\nof USER_RAM2 (back- and foreground colour values) failed" << std::endl;
    }
  }
  std::cout << TAG+"leaving..." << std::endl;
  return success;
}

// 0 = ss, 1 = mi, 2 = hh, 3 = wd, 4 = dd, 5 = mo, 6 = yy, 7 = rs, 8 = ?? (print help)
bool btn_ab_itm(bool dr)
{
  bool dummy;
  std::cout << "btn_ab_itm(): entered..." << std::endl;
  switch (itm)
  {
    case ITM_SECONDS:
    case ITM_MINUTES:
    case ITM_HOURS:
    case ITM_WEEKDAY:
    case ITM_DATE:
    case ITM_MONTH:
    case ITM_YEAR:
    {
      decr_incr_datetime(dr);  // Set date and/or time items to rtc. This func set the flag t_cghd ! I has to be in front of the set_datetime func!
      upd_app_time(); // read rtc date and time data into app_time vector array
      dummy = set_datetime();  // update the global app_time
      break;
    }
    case ITM_RESET:
    {
      if (overall_reset) // see flag in main.hpp
        encoder_reset();
      else
        dummy = reset_clk();  // reset the clock to a certain date and time
      break;
    }
    case ITM_MENU:
    {
      //a_menu_handler();
      menu = MENU_SETTINGS;
      btn_msgd("menu settings",0);
      std::cout << "btn_ab_itm(): menu value has been changed" << std::endl;
      pr_in_which_menu();
      sleep_ms(1000);
      break;
    }
    case ITM_HELP:
    {
      btn_help();  // display help info
      break;
    }
    default:
    {
      not_yet(true);
      break;
    }
  }
  return true;
}

bool btn_ab_settings(bool disp)
{
  /* This function will handle several settings like:
    - built-in led switch On/Off;
    - choose 12/24 mode clock;
    - set alarm;
    - set timer;
    - rgb encoder breakout.
*/
  bool dummy, success = true;

  switch (sett_itm)
  {
    case SETT_LED:
      set_blink();
      success = backp_to_EP();
      break;
    case SETT_12HR:
      dummy = set_12_24();  // reset the clock to a certain date and time
      success  = backp_to_EP();
      break;
    case SETT_ALRM:
      dummy = set_alarm();  // display help info
      break;
    case SETT_TIMR:
      dummy = set_timer();  // display help info
      break;
    case SETT_BGND:
      set_background();
      success  = backp_to_EP();
      break;
    case SETT_FGND:
      set_foreground();
      success  = backp_to_EP();
      break;  
    case SETT_ENC:
      success = encoder_doit();
      break;   
    case SETT_BACK:
      menu = MENU_MAIN;

      btn_msgd("back to main",0);
      break;
    default:
      not_yet(disp);
      break;
  }
  return success;
}

void btn_help()
{
  std::cout << "btn_help(): entering..." << std::endl;
  bool dummy = false;
  std::string bar = "";
  Point p;
  p.x = 15;
  //pico_explorer.set_font(&font6);
  //my_rgb(rgb_h);
  set_DispColour(true, DISP_AUBERGINE, true);
  sleep_ms(20);
  set_DispColour(false, DISP_YELLOW, false);

  /*
    Note that we're taking the size of vector array ofs3 (used for p.y, the vertical offset) instead of itmlst (or itmlst_h)
    because the latter have too many elements for the display.
    This means that, in this moment, we will not display the last item (?? = help)
  */
  for (int i = TIME_MINUTES; i < ofs3.size(); i++)  
  {
    p.y = ofs3.at(i-1);  // Compensate for starting with TIME_MINUTES (the 2nd item in the vector arrays: itmlst and itmlst_h)
    bar = itmlst.at(i) + " - " + itmlst_h.at(i);
    std::cout << "text for display: " << bar << ", at x:y " << std::to_string(p.x) << ":" << std::to_string(p.y) <<std::endl;
    pico_explorer.text(bar, Point(p.x, p.y), 239, 3);
    sleep_ms(20);
    bar.clear();
  }
  pico_explorer.update();
  sleep_ms(3000);
  std::cout << "btn_help(): leaving..." << std::endl;
  //pico_explorer.set_font(&font8);
}

std::string btn_ID(uint8_t k) 
{
  //std::cout << "btn_ID(): entered..." << std::endl;
  std::string r = " ";
  if(k >= BTN_A && k <= BTN_Y)
    r = b_tns.at(BTN_A);
  std::cout << "btn_ID(): return value: " << r << std::endl;
  return r;
}

void btn_msg()
{
  const auto str1 = "Btns: A: /\\ Incr, B: \\/ Decr, <- X: Next, -> Y: Prev";
  const std::string str2(80,'-');

  std::cout << str2 << std::endl;
  std::cout << str1 << std::endl;
  std::cout << str2 << std::endl;
}

void btn_msgd(std::string txt, int ln)
{
  if (ln == 3)
    return;

  int option = 0; // options to do
  std::string bar, tmp;
  std::vector<std::string> tmpv;
  std::size_t found;
  int le, n = 0;

  if (ln < 0)
    ln = 0;

  //my_rgb(rgb_h);
  set_DispColour(true, bgnd_colour, true);
  Point p;
  p.x = 25;
  set_DispColour(false, fgnd_colour, false);

  le = txt.length();
  found = txt.find("item");
  if (found !=std::string::npos)
  { 
    option = 1;
    if (menu == MENU_MAIN)
      tmpv = {"Item", itmlst_h.at(itm), "selected"};
    else if (menu == MENU_SETTINGS)
      tmpv = {"Item", settings_menu.at(sett_itm), "selected"};
  }
  else
  {
    option = 2;
    if (le > 0)  // We received a text to display
      tmp = txt;
    else 
      tmp.assign("undefined");
  }
  if (option > 0)
  {
    p.y = ofs3.at(0);
    pico_explorer.text("          ", Point(p.x, p.y), 200, 4);  // Clear line 0
    // We have another text to display than the standard date and time info
    if (option == 1)
    {
      for (auto i = 0; i < tmpv.size(); i++)
      {
        p.y = ofs3.at(1+i);
        pico_explorer.text(tmpv.at(i), Point(p.x, p.y), 239, 4);
      }
    }
    else 
    {
      p.y = ofs3.at(1);
      pico_explorer.text(tmp, Point(p.x, p.y), 239, 4);
    }
  }
  else
  {
    // We go to display the standard date and time
    for (auto i = 0; i < 5; i++)
    {
      p.y = ofs3.at(i);
      switch (i)
      {
      case 0:
        n = w_btn(); // check which button is pressed
        if (n >= 0)
        {
          bar.assign("btn 0" + btn_ID(n));
          
          pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
          bar.clear();
        }
        else
        {
          bar.assign("btn ?     ");
          pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
          bar.clear();
        }
        break;
      case 1:
        bar.assign("chg " + itmlst.at(itm));
        pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
        bar.clear();
        break;
      case 2:
        // vector app_time is organized as: {secs, mins, hrs, weekday, date, month, year}
        bar.assign("d ");
        for (int j=TIME_YEAR; j > TIME_WEEKDAY; j--)
        {
          switch(j)
          {
            case TIME_YEAR:
              n = app_time.at(TIME_YEAR);
              tmp.assign(std::to_string(2000+n));
              bar.append(tmp+"-");
              break;
            case TIME_MONTH:
              n = app_time.at(TIME_MONTH);
              if (n < 10)
                tmp.assign("0" + std::to_string(n));
              else
                tmp = std::to_string(n);
              bar.append(tmp+"-");
              break;
            case TIME_DATE:
              n = app_time.at(TIME_DATE);
              if (n < 10)
                tmp.assign("0" + std::to_string(n));
              else
                tmp.assign(std::to_string(n));
              bar.append(tmp);
              break;
          }
        }
        tmp.clear();
        pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
        bar.clear();
        break;
      case 3:
        bar = wdays.at(app_time[TIME_WEEKDAY]);
        pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
        bar.clear();
        break;
      case 4:
        bar.assign("t ");
        n = app_time.at(TIME_HOURS);
        if (n < 10)
          tmp.assign("0" + std::to_string(n));
        else
          tmp = std::to_string(n);
        bar.append(tmp+":");

        n = app_time.at(TIME_MINUTES);
        if (n < 10)
          tmp.assign("0" + std::to_string(n));
        else
          tmp.assign(std::to_string(n));
        bar.append(tmp+":");

        n = app_time.at(TIME_SECONDS);
        if (n < 10)
          tmp.assign("0" + std::to_string(n));
        else
          tmp.assign(std::to_string(n));
        bar.append(tmp);
        tmp.clear();
        pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
        bar.clear();
        break;
      }
    }
  }
  pico_explorer.update();
  sleep_ms(1000);
  set_DispColour(true, bgnd_colour, true); // clear the display into orange colour
}

void btn_xy_itm(bool dr)
{
  std::cout << "btn_xy_itm(): " << std::endl;
  // handle normal item (itm)
  if (dr == false) // Previous
  {
    itm -= 1;
    if (itm < ITM_MINUTES)  // We don't handle 0 (i.e. seconds)
      itm = ITM_HELP; // roll to last menu item
  }
  else if (dr == true)  // Next
  {
    itm += 1;
    if (itm > ITM_HELP)
      itm = ITM_MINUTES; // roll to first menu item
  }
  pr_itmsel();
  UltItmChgdTime = rtc.getUNIX(); // record the time of visiting this function (to be able to reset to ITM_HELP after 30 seconds)
}

void btn_xy_settings(bool dr)
{
  std::cout << "btn_xy_settings(): " << std::endl;
  // Handle settings item (sett_itm)
  if (dr == false) // Previous
  {
    sett_itm -= 1;
    if (sett_itm < SETT_LED)  // We don't handle 0 (i.e. SETT_LED)
      sett_itm = SETT_BACK;   // roll to last menu item
  }
  else if (dr == true)  // Next
  {
    sett_itm += 1;
    if (sett_itm > SETT_BACK)
      sett_itm = SETT_LED; // roll to first menu item
  }
  pr_itmsel();
  UltItmChgdTime = rtc.getUNIX(); // record the time of visiting this function (to be able to reset to ITM_HELP after 30 seconds)
}

bool ck_btn_press()
{
  if (pico_explorer.is_pressed(pico_explorer.A) ||  
    pico_explorer.is_pressed(pico_explorer.B) || 
    pico_explorer.is_pressed(pico_explorer.X) ||  
    pico_explorer.is_pressed(pico_explorer.Y))
  {
    std::cout << "One of the 4 buttons has been pressed" << std::endl;
    return true;
  }
  return false;
}

bool ck_YN(std::string Txt, std::string btnX, std::string btnY)
{
  int my_fgnd_colour = fgnd_colour; // remember the current fgnd colour
  int my_bgnd_colour = bgnd_colour; // idem bgnd colour
  int leTxt = Txt.size();
  int leBtnX = btnX.size();
  int leBtnY = btnY.size();
  uint8_t n = (uint8_t)500;
  bool lRet, dummy = false;
  std::vector<std::string> tmpv;
  static const std::string TAG = "ck_YN(): button ";
  static const std::string SFX = " pressed";
  static const std::string tY = "btn Y = Yes"; // or "btn Y = 24"
  static const std::string tN = "btn X = No";  // or "btn X = 12"
  set_DispColour(true, DISP_RED, true);
  Point p;
  p.x = 15;
  set_DispColour(false, DISP_YELLOW, false);

  btns_itm[BTN_X] = 0;  // Clear the X itm btn flag
  btns_itm[BTN_X] = 0;  // idem Y btn

  if (leTxt > 0)
    tmpv = { Txt, (leBtnX > 0 ? btnX: tN), (leBtnY > 0 ? btnY : tY)};  // Set to an alternativ text and alternative btnX and btnY texts
  else
    tmpv = {"Reset Clock?", tN, tY}; // Default txt
  
  std::vector <int> k = {1, 2, 3};
  for (int i = 0; i < tmpv.size(); i++)
  {
    p.y = ofs3.at(i+k.at(i));
    pico_explorer.text(tmpv.at(i), Point(p.x, p.y), 239, 4);
  }
  pico_explorer.update();
  // Try to clear the button interrupt by calling is_pressed()
  lRet = pico_explorer.is_pressed(pico_explorer.A); // Just another button for dummy op
  while (true)
  {
    if (pico_explorer.is_pressed(pico_explorer.X))
    {
      std::cout << TAG+"X"+SFX << std::endl;
      btns_itm[BTN_X] = 1;
      sleep_ms(500);  // This delay and its value are important to prevent debounce effect
      lRet = false; // btn X press indicates "No"
      break;
    }
    if (pico_explorer.is_pressed(pico_explorer.Y))
    {
      std::cout << TAG+"Y"+SFX << std::endl;
      btns_itm[BTN_Y] = 1;
      sleep_ms(500);  // This delay and its value are important to prevent debounce effect
      lRet = true; // btn X press indicates "Yes"
      break;
    }
  }
  sleep_ms(1000); // don't let this screen go away too rapidly
  // reset the screen colours to the colours at entry of this function
  set_DispColour(true, my_bgnd_colour, true);
  sleep_ms(20);
  set_DispColour(false, my_fgnd_colour, false);
  return lRet;
}

void clr_itm_btns()
{
  for (auto i = 0; i < btns_itm.size(); i++)
    btns_itm[i] = 0;
}

void decr_incr_datetime(bool dr)
{
  if (itm == TIME_SECONDS)
    return;

  static const std::string TAG = "decr_incr_datetime(): ";
  static const std::string wg = "we are going to ";
  static const std::string rg = "the result of get";
  static const std::string vo = "the value of: ";
  static const std::string nv = "() set to new value: ";
  static const std::string ck = "check: the result of rtc.getTime() is: ";
  int days, n, idx = 0;
  std::cout << TAG+"entered... The value of itm is: " << std::to_string(itm) << " = " <<  itmlst_h.at(itm) << std::endl;

  switch(itm)
  {
    case TIME_MINUTES:
      n = (int)rtc.getMinutes();
      break;
    case TIME_HOURS:
      n = (int)rtc.getHours();
      break;
    case TIME_WEEKDAY:
      n = (int)rtc.getWeekday();
      break;
    case TIME_DATE:
      n = (int)rtc.getDate();
      break;
    case TIME_MONTH:
      n = (int)rtc.getMonth();
      break;
    case TIME_YEAR:
      n = (int)rtc.getYear();
      break;
    default:
      not_yet(true);  // we don't handle seconds and we don't handle (here) 'help'
      return;
  }
  std::cout << TAG+wg << (dr ? "increase" : "decrease") << vo << itmlst_h.at(itm) << std::endl;
  std::cout << TAG+rg << itmlst_h.at(itm) << "() is " << std::to_string(n) << std::endl;

  if (dr == false)
    n--;
  else
    n++;

  switch(itm)
  {
    case TIME_MINUTES:
      if (!dr && n < 0)
        n = 59;
      else if (dr && n > 59)
        n = 0;
      rtc.setMinutes((uint8_t)n);
      break;
    case TIME_HOURS:
      //Build_Hour is 0-23, convert to 1-12 if needed (see rtc.setToCompilerTime() )
      if (!dr && n < 0)
          n = 23;
      else if (dr && n > 23)
        n = 0;
      rtc.setHours((uint8_t)n);
      if (v_settings.at(SETT_12HR) == 1)
        app_time_IsPM = rtc.is12Hour() ? (rtc.isPM() ? 1 : 0) : -1;
      break;
    case TIME_WEEKDAY:
      if (!dr && n < 0)
        n = 6;
      else if (dr && n > 6)
        n = 0;
      rtc.setWeekday((uint8_t)n);
      break;
    case TIME_DATE:
      days = (int)rtc.daysInMonth((uint8_t)app_time[TIME_YEAR], (uint8_t)app_time[TIME_MONTH]);  // get the days in the given month (leapyear respected)
      if (!dr && n < 0)
        n = days;
      else if (dr && n > days)
        n = 0;
      rtc.setDate((uint8_t)n);
      break;
    case TIME_MONTH:
      if (!dr && n < 1)
        n = 12;
      else if (dr && n > 12)
        n = 1;
      rtc.setMonth((uint8_t)n);
      break;
    case TIME_YEAR:
      if (!dr && n < 0)
        n = 99;
      else if (dr && n > 99)
        n = 0;
      rtc.setYear((uint8_t)2000+n);
      break;
  }
  std::cout << TAG << itmlst_h.at(itm) << nv << std::to_string(n) << std::endl;
  std::cout << TAG+ck << rtc.stringTimeStamp() << std::endl;
  t_chgd = true;  // Set the global flag to indicate that a date and/or time item has been changed
}

int elapsedtime() 
{
  return (rtc.getUNIX() - Ut);
}

void get_alarm()
{
  //The below variables control what the date will be set to
  /*
  int sec = 0;
  int minute = 59;
  int hour = 18;
  int day = 5;
  int date = 8;
  int month = 5;
  int year = 2021;
  */
  //The below variables control what the alarm will be set to
  int alm_minute = 0;
  int alm_hour = 20;
  int alm_date_or_weekday = 2;
  bool alm_isweekday = false;
  uint8_t alm_mode = 0;
  /*********************************
    Set the alarm mode in the following way:
    0: When minutes, hours and weekday/date match (once per weekday/date)
    1: When hours and weekday/date match (once per weekday/date)
    2: When minutes and weekday/date match (once per hour per weekday/date)
    3: When weekday/date match (once per weekday/date)
    4: When hours and minutes match (once per day)
    5: When hours match (once per day)
    6: When minutes match (once per hour)
    7: All disabled – Default value
    If you want to set a weekday alarm (alm_isweekday = true), set 'alm_date_or_weekday' from 0 (Sunday) to 6 (Saturday)
  ********************************/
  //Enable alarm interrupt
  //rtc.enableAlarmInterrupt(alm_minute, alm_hour, alm_date_or_weekday, alm_isweekday, alm_mode);
  //rtc.disableAlarmInterrupt();  //Only disables the interrupt (not the alarm flag)

  //PRINT TIME
  if (rtc.updateTime() == false) //Updates the time variables from RTC
  {
    std::cout << "RTC failed to update" << std::endl;
  } else {
    std::string currentTime = rtc.stringTimeStamp();
    std::cout << currentTime << "     \'s\' = set time" << std::endl;
  }
  
  //Read Alarm Flag
  if (rtc.readAlarmInterruptFlag()) {
    std::cout << "ALARM!!!!" << std::endl;
    rtc.clearAlarmInterruptFlag();
    sleep_ms(3000);
  }
}

void get_datetime()
{
  bool lPrintIt, dummy = false;
  uint8_t n;
  char c[11];
  char c2[5];
  rtc.updateTime();
  sleep_ms(20);
  upd_app_time();  // update the get datetime values from rtc and put them in app_time vector array
  std::cout << "get_datetime(): new time copied to app_time. \n";

  if (lPrintIt)
  {
    for (auto i = 0; i < TIME_ARRAY_LENGTH; i++)
    {
      n = app_time.at(i);
      if (i == TIME_YEAR)
        sprintf(c2,"20%d",n);
      else
        sprintf(c2,"%d",n);
      switch (i)
      {
        case TIME_SECONDS:
          strcpy(c,"Seconds:  ");
          break;
        case TIME_MINUTES:
          strcpy(c,"Minutes:  ");
          break;
        case TIME_HOURS:
          strcpy(c,"Hours:    ");
          break;
        case TIME_WEEKDAY:
          strcpy(c,"Weekday:  ");
          break;
        case TIME_DATE:
          strcpy(c,"Day:      ");
          break;
        case TIME_MONTH:
          strcpy(c,"Month:    ");
          break;
        case TIME_YEAR:
          strcpy(c,"Year:     ");
          break;
        default:
          strcpy(c,"Unknown   ");
          break;
      }   // end switch case...
      std::cout << c << c2 << std::endl;
    }  // end for ...
  }  // end if ...
  btn_msgd("datetime saved", 0);
}

void fail_msgs(uint8_t code)
{
  std::cout << "fail_msgs(): value rcvd param code is: " << std::to_string(code) << std::endl;
  static const std::string TAG = "Error occurred in setup(): ";
  static const std::vector<std::string> s = { 
    "rtc.init()",
    "rtc.reset()",
    "LED data ",
    "Back-/Foreground data ",
    "rtc.updateTime()",
    "encoder ",
    "upd_app_time()",
    " successfull.",
    " failed.",
    " not defined.",
    " not found",
    " found"
  };
  int n = 0;
  std::string s2, s3 = "";
  static const std::string s4 = "restore from memory register ";
  // called from setup()
 
  if (code > 0)
  {
    for (auto i=0; i<setup_stat.size(); i++)
    {
      n = setup_stat[i];
      switch (i)
      {
        case SU_RTC_INIT:
          s2 = s.at(SU_RTC_INIT)+s.at((n ==1) ? SU_SUCCESSFULL : SU_FAILED);
          break;
        case SU_RTC_SETUP:
          s2 = s.at(SU_RTC_SETUP)+s.at((n ==1) ? SU_SUCCESSFULL : SU_FAILED);
          break;
        case SU_REST_FM_INIT_LED:
          s2 = s4+s.at(SU_REST_FM_INIT_LED)+s.at((n ==1) ? SU_SUCCESSFULL : SU_FAILED);
          break;
        case SU_REST_FM_INIT_BF:
          s2 = s4+s.at(SU_REST_FM_INIT_LED)+" and " +s.at(SU_REST_FM_INIT_BF)+s.at((n ==1) ? SU_SUCCESSFULL : SU_FAILED);
          break;
        case SU_RTC_UPDATETIME:
          s2 = s.at(SU_RTC_UPDATETIME)+s.at((n ==1) ? SU_SUCCESSFULL : SU_FAILED);
          break;
        case SU_ENC_SETUP:
          s2 = s.at(SU_ENC_SETUP)+s.at((n == 1) ? SU_FOUND : SU_NOT_FOUND);
          break;
        case SU_UPD_APP_TIME:
          s2 = s.at(SU_UPD_APP_TIME)+s.at(SU_FAILED);
          break;
        case SU_FAILED:
          s2 = s4+s.at(SU_FAILED);
        default:
          s2 = s.at(SU_UNKNOWN);
      }
      std::cout << TAG+s2 << std::endl;
    }
  }
}

/***
*    buffer example{'5', '1', '5', '7', '9'};
*
*    // We can use our function to write to the console.
*    std::cout << hexify(example) << std::endl;
*/
// This is our iostream function.
hexbuffer hexify(const buffer& b)
{
    return { b };
}

// This operator overload is what does all the work. 
std::ostream& operator << (std::ostream& s, const hexbuffer& h)
{
    s << std::setw(2) << std::setfill('0') << std::hex;

    // Write the hex data.
    for (auto c : h.innerbuf) {
        s << (int)c << ' ';
    }
    return s;
}

rgb_t hsv_to_rgb(rgb_t *s_rgb)
{
  /*
    // From: I:\pico\pimoroni-pico\examples\pico_unicorn\demo.cpp
    for(uint8_t y = 0; y < 7; y++) {
      for(uint8_t x = 0; x < 16; x++) {
        uint8_t r, g, b;
        float h = float(x) / 63.0f + float(i) / 500.0f;
        h = h - float(int(h));
        float s = 1.0f;//(sin(float(i) / 200.0f) * 0.5f) + 0.5f;
        float v = (float(y) / 8.0f) + 0.05f;
        from_hsv(h, s, v, r, g, b);

        pico_unicorn.set_pixel(x, y, r, g, b);
        j = j + 1;
      }
    }
    */
  float rr;
  float rg;
  float rb;

  if (s_rgb->g == 0.0)
  {
    rr = s_rgb->b;
    rg = s_rgb->b;
    rb = s_rgb->b;
  }
  else
  {
    rr = s_rgb->r;
    rg = s_rgb->g;
    rb = s_rgb->b;
  }
  float v = s_rgb->b;
  int i = int(rr * 6.0);
  float f = (rr * 6.0) - i;
  float p = rb * (1.0 - rg);
  float q = rb * (1.0 - rg * (f));
  float t = rb * (1.0 - rg * (1.0 - f));
  i = i % 6;
  switch (i)
  {
    case 0:
    {
      rr = v;
      rg = t;
      rb = p;
      break;
    }
    case 1:
    {
      rr = q;
      rg = v;
      rb = p;
      break;
    }
    case 2:
    {
      rr = p;
      rg = v;
      rb = t;
      break;
    }
    case 3:
    {
      rr = p;
      rg = q;
      rb = v;
      break;
    }
    case 4:
    {
      rr = t;
      rg = p;
      rb = v;
      break;
    }
    case 5:
    {
      rr = v;
      rg = p;
      rb = q;
      break;
    }
  }
  rgb_t *rgbn;
  rgbn->r = rr;
  rgbn->g = rg;
  rgbn->b = rb;

  return *rgbn;
}

void led_toggle() 
{
  if (v_settings.at(SETT_LED)) // Blink the built-in LED is flag is true
  {
    if (ledIsOn)
    {
      gpio_put(LED_PIN, 0);
      ledIsOn = false;
    }
    else
    {
      gpio_put(LED_PIN, 1);
      ledIsOn = true;
    }
  }
  else
  { // We will not blink no more, but check if the LED was still on. If so, switch it off
    if (ledIsOn)
    {
      gpio_put(LED_PIN, 0);
      ledIsOn = false;
    }
  }
}

void my_rgb(int h)
{
  int rn, gn, bn = 0;
  rgb_t *s_rgb, *t;
  s_rgb->r = h/360.0;
  s_rgb->g = 0.00;

  *t = hsv_to_rgb(s_rgb);
  for (auto i = 1; i < 4; i++)
  {
    switch (i)
    {
      case 1:
        rn = int(255 * t->r);
        break;
      case 2:
        gn = int(255 * t->g);
        break;
      case 3:
        bn = int(255 * t->b);
        break;
    }
  }
  pico_explorer.set_pen(rn, gn, bn);
}

void not_yet(bool lDsp)
{
  bool dummy = false;
  std::string bar = "";
  std::vector<std::string> dt = {"Not", "implemented", "yet!"};
  if (lDsp)
  {
    const std::vector<uint8_t> ofs2 = {20, 60, 100, 140, 180};
    Point p;
    p.x = 5;
    //my_rgb(rgb_h);
    set_DispColour(true, DISP_RED, true);
    sleep_ms(20);
    set_DispColour(false, DISP_YELLOW, false);
    int le = dt.size();
    // fill-in date && time
    std::cout << "not_yet():";

    for (auto i = 0; i < le; i++)
    {
      p.y = ofs2.at(i+1);
      std::cout << dt.at(i) << (i < 2 ? " " : "");
      pico_explorer.text(dt.at(i), Point(p.x, p.y), 239, 4);
    }
    std::cout << "" << std::endl;
      // dont't display the first character (a space)
    bar.clear();
    pico_explorer.update();
    sleep_ms(1000);
  }
}

// see: https://stackoverflow.com/questions/20382278/is-there-an-equivalent-of-pythons-pass-in-c-std11
bool pass()  // my tial for an equivalent to Python's pass
{
  return true;
}

void pr_in_which_menu()
{
  std::cout << "We are in menu: ";
  if (menu == MENU_MAIN)
    std::cout << "main";
  else if (menu == MENU_SETTINGS)
    std::cout << "settings";
  std::cout << "" << std::endl;
}

void pr_itmsel()
{
  bool dummy;
  if (lUSB)
    std::cout << "Item: ";
    if (menu == MENU_MAIN)
      std::cout << std::to_string(itm) << " ("<< itmlst.at(itm);
    else if (menu == MENU_SETTINGS)
      std::cout << std::to_string(sett_itm) << " ("<< settings_menu.at(sett_itm);
    std::cout << ") selected" << std::endl;
  btn_msgd("item", 0);
}

std::vector<std::string> prep_datetime()
{
  std::vector <std::string> dt(4);
  std::string dd_p, hh_p, mi_p, ss_p, am_p;

  if (app_time.at(TIME_DATE) < 10)
    dd_p = " 0" + std::to_string(app_time.at(TIME_DATE));
  else
    dd_p = ' ' +std::to_string(app_time.at(TIME_DATE));

  if (app_time.at(TIME_HOURS) < 10)
  {
    if (v_settings.at(SETT_12HR) == 1)
    {
      hh_p = std::to_string(app_time.at(TIME_HOURS));
    }
    else
    {
      hh_p = '0' + std::to_string(app_time.at(TIME_HOURS));
    }
  }
  else
    hh_p = std::to_string(app_time.at(TIME_HOURS));

  if (app_time.at(TIME_MINUTES) < 10)
    mi_p = '0' + std::to_string(app_time.at(TIME_MINUTES));
  else
    mi_p = std::to_string(app_time.at(TIME_MINUTES));

  if (v_settings.at(SETT_12HR) == 0)  // Clock is in 24 hour mode
    if (app_time.at(TIME_SECONDS) < 10)
      ss_p = '0' + std::to_string(app_time.at(TIME_SECONDS));
    else
      ss_p = std::to_string(app_time.at(TIME_SECONDS));

    am_p = "A";
    if (v_settings.at(SETT_12HR) == 1 && (app_time_IsPM))
      am_p = "P";

  dt = 
  {
    wdays.at(app_time.at(TIME_WEEKDAY)),    // = DOIT_WEEKDAY
    mnths.at(app_time.at(TIME_MONTH)) + dd_p, // = DOIT_MONTH
    "20" + std::to_string(app_time.at(TIME_YEAR)),  // = DOIT_YEAR
    hh_p + ":" + mi_p + ((v_settings.at(SETT_12HR) == 1) ? " "+am_p+"M" : ":"+ss_p)  // = DOIT_TIME
  };  

  return dt;
}

bool reset_clk()
{
  bool lRet;
  lRet = ck_YN("Reset Clock?", "btn X = No", "btn Y = Yes");
  if (lRet) // Prevent inintended reset by asking the user if he really wants to reset the clock
  {
    uint8_t time[TIME_ARRAY_LENGTH];
    uint8_t *p;
    time[TIME_SECONDS] = rtc.DECtoBCD((uint8_t)0);
    time[TIME_MINUTES] = rtc.DECtoBCD((uint8_t)0);
    time[TIME_HOURS] = rtc.DECtoBCD((uint8_t)12);
    time[TIME_WEEKDAY] = rtc.DECtoBCD((uint8_t)6);
    time[TIME_DATE] = rtc.DECtoBCD((uint8_t)1);
    time[TIME_MONTH] = rtc.DECtoBCD((uint8_t)5);
    time[TIME_YEAR] = rtc.DECtoBCD((uint8_t)21);
    p = &time[0];
    lRet = rtc.setTime(p,TIME_ARRAY_LENGTH);
    sleep_ms(100);
    rtc.updateTime();
    sleep_ms(100);
    std::cout << "reset_clk(): clock reset to \"2021-05-01 Sun(=6) 12:00:00\". \n    Result: " <<
      (lRet ? "Successful" : "Failed") << ".\n    Check (rtc.stringDT()): " <<
      rtc.stringDT() << std::endl;
  }
  return lRet;
}

bool reset_itm()
{
  bool itm_is_reset = false;
  int curUt = rtc.getUNIX(); // record the time of visiting this function (to be able to reset to ITM_HELP after 30 seconds)
  if (curUt - UltItmChgdTime > 30)
  {
    itm_is_reset = true;
    itm = ITM_HELP;  // Reset itm to Help.
    menu = MENU_MAIN; // Reset menu to MENU_MAIN
    UltItmChgdTime = rtc.getUNIX(); // update curUt
  }
  return itm_is_reset;
}

/*
  User address space in EEPROM: addresses 00h - 2Ah (= 43 bytes) (see: page 40 of the RV3028-C7 manual)
  and User RAM1 1Fh and User RAM2 20h (see: page 41 of the RV3028-C7 manual)
*/
int rest_fm_EP()
{
  short int nRetval = 0;
  int usr_dat_len_at_restore = 0;
  static const std::string TAG = "rest_fm_EP(): ";
  static const std::string clr_str0 = "the LCD ";
  static const std::string clr_str1 = "ground colour value retrieved from EEPROM USR_RAM2 is: ";
  static const std::string ep_busy = "EEPROM busy. Not able to read data from user EEPROM memory.";
  uint8_t usr_ram1_val, usr_ram2_val = 0;
  uint8_t led, bg, bf, fg = 0;
  int loop_cnt = 0;

  std::cout << TAG+"entering..." << std:: endl;

  //led = rtc.readConfigEEPROM_RAMmirror(RV3028_USER_RAM1);
  led = rtc.readRegister(RV3028_USER_RAM1);

  buffer mem_ads{RV3028_USER_RAM1};
  if (led == 0xFF)
  {
    nRetval = -1;
    std::cout << TAG+" unable to read LED On/Off value from register # 0x" << hexify(mem_ads) << std::endl;
  }
  else
  {
    led &= 0x01;    //Clear all bits except the LED Bit
    buffer rest_bfr1{led};
    v_settings.at(SETT_LED) = (int)led;
    std::cout << TAG+"the LED On/Off value retrieved from register # 0x" << hexify(mem_ads) << "is: \'0x" <<  hexify(rest_bfr1) << "\'" << std::endl;
    std::cout << TAG+"v_settings.at(SETT_LED) has been set to: " << std::to_string((int)led) << std::endl;
  }
  bf = rtc.readRegister(RV3028_USER_RAM2);
  //bf = rtc.readConfigEEPROM_RAMmirror(RV3028_USER_RAM2);
  buffer rest_bfr2a{bf};
  buffer mem_ads2{RV3028_USER_RAM2};
  std::cout << TAG+"usr_ram2_val (LCD back- and foreground colour data) retrieved from register # 0x" << hexify(mem_ads2) << " = \'0x" << hexify(rest_bfr2a) << "\'" << std::endl;
  if (bf == 0xFF)
  {
    nRetval = -2;
    std::cout << TAG+" unable to read LCD back- and foreground colour value from memory register # 0x" << hexify(mem_ads2) << std::endl;
  }
  else
  {
    fg = bf & 0x0f;
    bg = (bf & 0xf0) >> 4;
    std::cout << TAG+"usr_ram2_val & 0x0f = \'" << std::to_string(bf) << "\' & \'" << std::to_string(0x0f) << "\' = " << std::to_string(bg) << "= " << colours_h.at(bg) << std::endl;
    std::cout << TAG+"usr_ram2_val & 0xf0 = \'" << std::to_string(bf) << "\' & \'" << std::to_string(0xf0) << "\' = " << std::to_string(fg) << "= " << colours_h.at(fg) << std::endl;
    bg = bf; // Copy retrieved value
    bg &= 0x0f; // clear LSB bits
    v_settings.at(SETT_BGND) = (int)bg;
    bgnd_colour = v_settings.at(SETT_BGND);
    std::cout << TAG+clr_str0+"back"+clr_str1 <<  std::to_string(bg) << std::endl;
    fg = bf;
    fg = ((bf & 0xf0) >> 4);  // logical and with 0xf0 (mask LSB) and then shift MSB bits to LSB
    v_settings.at(SETT_FGND) = (int)fg;
    fgnd_colour = v_settings.at(SETT_FGND);
    std::cout << TAG+clr_str0+"fore"+clr_str1 <<  std::to_string(fg) << std::endl;
  }
  if (led == 0 && bg == 0 && fg == 0)
    nRetval = -3; // When all retrieved data is zero, return with error value -3
  std::cout << TAG+"leaving with return value: " << std::to_string(nRetval)  << std::endl;
  return nRetval;
}

bool set_12_24()
{
  bool cur1224Hr = false; // Assume a 24Hr clock mode
  /*  This function let the user choose 12 or 24 hour clock mode */
  v_settings.at(SETT_12HR) = ck_YN("12/24 Hr ?", "btn X = 24", "btn Y = 12") ? 1 : 0;
  if (v_settings.at(SETT_12HR) == 1)
    rtc.set12Hour();
  else
    rtc.set24Hour();
  
  // re-check:
  cur1224Hr = rtc.is12Hour();
  std::cout << "set_12_24(): the rtc reports it is set for " << (cur1224Hr == 1 ? "12" : "24") << " Hr Mode." << std::endl;
  std::cout << "set_12_24(): v_settomgs-at(SETT_12HR) value is: " << (v_settings.at(SETT_12HR) ? "12" : "24") << " hour mode" << std::endl;
  return cur1224Hr;
}

bool set_alarm()
{
  not_yet(true);
  return true;
}

/*
  Note that the functions set_background() and set_foreground() are almost identical.
  I tried to merge them but then ran in to trouble. That is why I 're-installed' the initial two functions
*/
void set_background()
{
  bool dummy, lExitLoop, lStart = false;
  int CurColour = DISP_BLACK;
  static const std::string TAG = "set_background(): button ";
  static const std::string SFX = " pressed.";
  static const std::string CBC = "Current background colour ";
  static const std::string BFC = "before chang:";
  static const std::string AFC = "after change:";
  static const std::vector<std::string> btn_txts = {"Colour OK?", "btn A = Prev", "btn B = Next", "btn X = No", "btn Y = Yes"};
  int n = 200;

  /* 
    Wait until or btn A or btn B has been pressed. This is done to prevent that a 
    previous main menu keypress causes the loop below to jump from the 
    start black colour to the next colour
  */
  do
  {  
    if (pico_explorer.is_pressed(pico_explorer.A) ||  pico_explorer.is_pressed(pico_explorer.B))
      lExitLoop = true;
  } while (!lExitLoop);
  sleep_ms(100);  // wait a bit to get the keypress been served

  lStart = true;
  lExitLoop = false;
  do  // loop
  {
    if (lStart)
    {
      // set the background or the foreground colour to value of CurColour

      // We're going to handle the background colour
      set_DispColour(true, CurColour, true);  // set the background to value of nCurColour
      sleep_ms(20);
      set_DispColour(false, fgnd_colour, false);

      Point p;
      p.x = 15;

      for (int i = 1; i < 6; i++)
      {
        p.y = ofs3.at(i);
        pico_explorer.text(btn_txts[i-1], Point(p.x, p.y), 239, 3);
      }
      p.y = ofs3.at(6);
      pico_explorer.text("clr: " + colours_h.at(CurColour), Point(p.x, p.y), 239, 3);
      pico_explorer.update();
      lStart = false;
    }
    if (pico_explorer.is_pressed(pico_explorer.A))
    {
      std::cout << TAG+"A"+SFX << std::endl;
      std::cout << CBC+BFC << colours_h.at(CurColour) << std::endl;
      CurColour += 1;
      //if (colours.at(CurColour) == "whi")
      if (CurColour == DISP_WHITE)
        CurColour = DISP_BLACK; // 'Wrap around' forward to start color
      std::cout << CBC+AFC << colours_h.at(CurColour) << std::endl;
      sleep_ms(n);
      lStart = true;
    }
    if (pico_explorer.is_pressed(pico_explorer.B))
    {
      std::cout << TAG+"B"+SFX << std::endl;
      std::cout << CBC+BFC << colours_h.at(CurColour) << std::endl;
      CurColour -= 1;
      //if (colours.at(CurColour) == "blk")
      if (CurColour == DISP_BLACK)
        CurColour = DISP_WHITE; // 'Wrap arounc' backward to the ultimate color
      std::cout << CBC+AFC << colours_h.at(CurColour) << std::endl;
      sleep_ms(n);
      lStart = true;
    }
    if (pico_explorer.is_pressed(pico_explorer.X))
    {
      std::cout << TAG+"X"+SFX << std::endl;
      sleep_ms(n);
      lExitLoop = false;
    }
    if (pico_explorer.is_pressed(pico_explorer.Y))
    {
      std::cout << TAG+"Y"+SFX << std::endl;
      sleep_ms(n);
      lExitLoop = true;
    }
  } while (!lExitLoop); // end-of INNER loop

  v_settings.at(SETT_BGND) = CurColour;
  bgnd_colour = CurColour; // Set the global background colour to the chosen colour: CurColour
  std::cout << TAG+"leaving..." << std::endl;
}

/*
  setBackupSwitchoverMode by Constantin Koch, student
  See: https://github.com/constiko/RV-3028_C7-Arduino_Library/blob/master/examples/Example5-BackupSwitchoverMode/Example5-BackupSwitchoverMode.ino 
  See also the RV-3028-C7_App-Manual.pdf #page=45

  See also:
  //rtc.enableTrickleCharge(TCR_3K);   //series resistor 3kOhm
  //rtc.enableTrickleCharge(TCR_5K); //series resistor 5kOhm
  //rtc.enableTrickleCharge(TCR_9K); //series resistor 9kOhm
  //rtc.enableTrickleCharge(TCR_15K); //series resistor 15kOhm
  //rtc.disableTrickleCharge(); //Trickle Charger disabled

*/
void set_BackupSwitchoverhMode(int mode)
{
  bool Retval = true;

  if (mode >= 0 && mode <= 3)
  {
    //Backup Switchover Mode
    std::cout << "Config EEPROM 0x37 before: " << std::endl;
    std::cout << rtc.readConfigEEPROM_RAMmirror(0x37) << std::endl;

    switch(mode)
    {
      case 0:  // Switchover disabled
        rtc.setBackupSwitchoverMode(mode);
        break;
      case 1:  // Direct Switching mode
        rtc.setBackupSwitchoverMode(mode);
        break;
      case 2:  // Standby Mode
        rtc.setBackupSwitchoverMode(mode);
        break;
      case 3:  // Default mode
        rtc.setBackupSwitchoverMode(mode);
        break;
      default:
        std::cout << "setBackupSwitchoverMode(): only modes 0, 1, 2 or 3 possible" << std::endl;
        Retval = false;
        break;
    }
    std::cout << "Config EEPROM 0x37 after: " << std::endl;
    std::cout << rtc.readConfigEEPROM_RAMmirror(0x37) << std::endl;
  }
}

void set_blink()
{
  bool lBlinkState = v_settings.at(SETT_LED);  // copy the status of the global BlinkBILed flag

  v_settings.at(SETT_LED) = (ck_YN("Blink LED ?", "", "") ?  1 : 0);

  std::cout << "set_blink(): blink the built-in LED is set from: " << (lBlinkState ? "Yes" : "No") << " to: " << (v_settings.at(SETT_LED) ? "Yes" : "No") << std::endl;
}

bool set_datetime()
{
  bool lRet = false;
  uint8_t time[TIME_ARRAY_LENGTH];
  uint8_t hour;
  uint8_t *p;  // pointer to time array
  if (t_chgd)
  {
    for (int i=TIME_SECONDS; i< TIME_ARRAY_LENGTH; i++)
    {
      if (i == TIME_SECONDS)
        time[i] = rtc.DECtoBCD((uint8_t)0);  // Make seconds zero
      else if (i == TIME_HOURS)
      {
        hour = app_time.at(i);
        if (rtc.is12Hour() && rtc.isPM())
        {
          hour |= (1 << HOURS_AM_PM);  // Set the PM bit
          app_time_IsPM = 1;
        }
        else
        {
          hour &= ~(1 << HOURS_AM_PM);  // Clear the PM bit
          app_time_IsPM = 0;
        }
        time[i] = rtc.DECtoBCD((uint8_t)hour);
      }
      else
      {
        std::cout << "set_datetime(): going to set " << itmlst_h.at(i) << " to: " << std::to_string(app_time.at(i)) << std::endl;
        time[i] = rtc.DECtoBCD((uint8_t)app_time.at(i));
      }
    }
    p = &time[0];
    lRet = rtc.setTime(p, TIME_ARRAY_LENGTH);
    sleep_ms(20); // was: 100
    std::cout << "set_datetime(): the result of rtc.setTime(*time, len) is: " << (lRet ? "Successful" : "Failed") << std::endl;
    if (!lRet)
    {
      return lRet;
    }
    else
    {
      rtc.updateTime();
      sleep_ms(20);
      upd_app_time();  // copy the rtc clock data into app_time array
    }
  }
  return lRet;
}

void set_DispColour(bool bf, int disp_colour, bool cleardisp)
{
  static const std::string TAG = "Set_DispColour(): ";
  static const std::string T2 = "setting the ";
  static const std::string SFX = "ground colour";
  static const std::vector<std::string> bfs = {"back", "fore"};
  static const std::string CTBST = "colour to be set to: ";
  static const std::string DSP = "display ";

  if (disp_colour < DISP_BLACK || disp_colour > DISP_WHITE)
  {
    if (bf)  // Are we doing background or foreground colour ?
    {
      std::cout << TAG+T2+"back"+SFX << std::endl;
      disp_colour = bgnd_colour;  // Set the background colour to the value of the global variable bgnd_colour
      v_settings.at(SETT_BGND) = bgnd_colour;
    }
    else
    {
      std::cout << TAG+T2+"fore"+SFX << std::endl;
      disp_colour = fgnd_colour; // Set the foreground colour to the value of the global variable fgnd_colour
    }
  }
  /*
  if (bf)
    std::cout << TAG+DSP+bfs.at(0)+CTBST << std::to_string(disp_colour) << " = " << colours_h.at(disp_colour) << std::endl;
  else
    std::cout << TAG+DSP+bfs.at(1)+CTBST << std::to_string(disp_colour) << " = " << colours_h.at(disp_colour) << std::endl;
  */
  switch (disp_colour)  
  {
    case DISP_BLACK:
      pico_explorer.set_pen(disp_black[RED], disp_black[GREEN], disp_black[BLUE]);
      break;
    case DISP_RED:
      pico_explorer.set_pen(disp_red[RED], disp_red[GREEN], disp_red[BLUE]);
      break;
    case DISP_ORANGE:
      pico_explorer.set_pen(disp_orange[RED], disp_orange[GREEN], disp_orange[BLUE]);
      break;
    case DISP_YELLOW:
      pico_explorer.set_pen(disp_yellow[RED], disp_yellow[GREEN], disp_yellow[BLUE]);
      break;
    case DISP_GREEN:
      pico_explorer.set_pen(disp_green[RED], disp_green[GREEN], disp_green[BLUE]);
      break;
    case DISP_BLUE:
      pico_explorer.set_pen(disp_blue[RED], disp_blue[GREEN], disp_blue[BLUE]);
      break;
    case DISP_AUBERGINE:
      pico_explorer.set_pen(disp_auber[RED], disp_auber[GREEN], disp_auber[BLUE]);
      break;
    case DISP_WHITE:
      pico_explorer.set_pen(disp_white[RED], disp_white[GREEN], disp_white[BLUE]);
      break;
    default:
      pico_explorer.set_pen(disp_green[RED], disp_green[GREEN], disp_green[BLUE]);
      break;
  }
  if (cleardisp)
    pico_explorer.clear();
}

void clr_setup_stat()
{
  for (auto i=0; i < setup_stat.size(); i++)
    setup_stat[i] = 0;
}

void clr_vsettings()
{
  for (auto i=0; i < v_settings.size(); i++)
    if (i < SETT_12HR)
      v_settings.at(i) = 1;
    else
      v_settings.at(i) = 0;
}

/*
  Note that the functions set_background() and set_foreground() are almost identical.
  I tried to merge them but then ran in to trouble. That is why I 're-installed' the initial two functions
*/
void set_foreground()
{
  bool dummy, lExitLoop, lStart = false;
  int CurColour = DISP_BLACK;
  static const std::string TAG = "set_foreground(): button ";
  static const std::string SFX = " pressed.";
  static const std::string CFC = "Current foreground colour ";
  static const std::string BFC = "before chang:";
  static const std::string AFC = "after change:";
  static const std::vector<std::string> btn_txts = {"Colour OK?", "btn A = Prev", "btn B = Next", "btn X = No", "btn Y = Yes"};
  int n = 200;
  /* 
    Wait until or brn A or btn B has been pressed. This is done to prevent that a 
    previous main menu keypress causes the loop below to jump from the 
    start black colour to the next colour
  */
  do
  {  
    if (pico_explorer.is_pressed(pico_explorer.A) ||  pico_explorer.is_pressed(pico_explorer.B))
      lExitLoop = true;
  } while (!lExitLoop);
  sleep_ms(100);  // wait a bit to get the keypress been served

  lStart = true;
  lExitLoop = false;
  do  // loop
  {
    if (lStart)
    {
      // We're going to handle the foreground colour
      set_DispColour(true, bgnd_colour, true);  // the background colour will be fixed. We're going to choose the foreground colour
      sleep_ms(20);
      set_DispColour(true, CurColour, false);
      Point p;
      p.x = 15;

      for (int i = 1; i < 6; i++)
      {
        p.y = ofs3.at(i);
        pico_explorer.text(btn_txts[i-1], Point(p.x, p.y), 239, 3);
      }
      p.y = ofs3.at(6);
      pico_explorer.text("clr: " + colours_h.at(CurColour), Point(p.x, p.y), 239, 3);
      pico_explorer.update();
      lStart = false;
    }
    if (pico_explorer.is_pressed(pico_explorer.A))
    {
      std::cout << TAG+"A"+SFX << std::endl;
      std::cout << CFC+BFC << colours_h.at(CurColour) << std::endl;
      CurColour += 1;
      if (colours.at(CurColour) == "whi")
        CurColour = DISP_BLACK; // 'Wrap around' forward to start color
      std::cout << CFC+AFC << colours_h.at(CurColour) << std::endl;
      sleep_ms(n);
      lStart = true;
    }
    if (pico_explorer.is_pressed(pico_explorer.B))
    {
      std::cout << TAG+"B"+SFX << std::endl;
      std::cout << CFC+BFC << colours_h.at(CurColour) << std::endl;
      CurColour -= 1;
      if (colours.at(CurColour) < "blk")
        CurColour = DISP_WHITE; // 'Wrap arounc' backward to the ultimate color
      std::cout << CFC+AFC << colours_h.at(CurColour) << std::endl;
      sleep_ms(n);
      lStart = true;
    }
    if (pico_explorer.is_pressed(pico_explorer.X))
    {
      std::cout << TAG+"X"+SFX << std::endl;
      sleep_ms(n);
      lExitLoop = false;
    }
    if (pico_explorer.is_pressed(pico_explorer.Y))
    {
      std::cout << TAG+"Y"+SFX << std::endl;
      sleep_ms(n);
      lExitLoop = true;
    }
  } while (!lExitLoop); // end-of INNER loop

  v_settings.at(SETT_FGND) = CurColour;
  fgnd_colour = CurColour; 
}

bool set_timer()
{
  bool dummy, lstop, isblk, t_chgd, l_1stloop, l_2ndloop, alrm_loop = false;
  bool lstart = true;
  int t_hh, t_mi, t_ss = 0;
  uint8_t max_loop = 0;
  int btn = 0;
  std::string bar, tmp;
  std::size_t found;
  int le, n = 0;
  bool alrm, mode_HM;
  static const uint8_t dly = (uint8_t)1000;
  static const std::string TAG = "set_timer(): button ";
  static const std::string SFX = " pressed";

  //my_rgb(rgb_h);
  set_DispColour(true, bgnd_colour, true);
  Point p;
  p.x = p_x_default;  // 15
  set_DispColour(false, fgnd_colour, false);

  t_hh = 0;
  t_mi = 2;
  //t_hh = app_time.at(TIME_HOURS);
  //t_mi = app_time.at(TIME_MINUTES);

  static const std::vector <std::string> tmpv = { 
    "SET - TIMER",
    "HH : MM : SS",
    "00 : 00 : 00",
    "A  B  X   Y",
    "U  D H/M OK",
    "btn Y  Exit"};
  /* 
    Wait until or btn A or btn B has been pressed. This is done to prevent that a 
    previous main menu keypress causes the loop below to jump from the 
    start black colour to the next colour
  */
  do
  { ; } while (!ck_btn_press()); // Wait for a button press to proceed
  sleep_ms(100);  //

  for (auto i = 0; i < tmpv.size()-1; i++)  // don't print the last (empty) item
    {
      p.y = ofs3.at(i);
      pico_explorer.text(tmpv.at(i), Point(p.x, p.y), 239, 4);
    }
  pico_explorer.update();

  alrm = false;
  mode_HM = true; // Set to Hours
  lstop = false;
  l_1stloop = true;
  btn = 0;

  do  // 1st loop: timer setup
  {
    /* code */
    if (pico_explorer.is_pressed(pico_explorer.A))
    {
      btn = 1;
      std::cout << TAG+"A"+SFX << std::endl;
      if (mode_HM)
      {
        t_hh +=1;
        if (t_hh > 99)
          t_hh = 99;
      }
      else 
      {
        t_mi +=1;
        if (t_mi >59)
          t_mi = 59; 
      }
      sleep_ms(dly);
    }
    if (pico_explorer.is_pressed(pico_explorer.B))
    {
      btn= 2;
      std::cout << TAG+"B"+SFX << std::endl;
      if (mode_HM)
      {
        t_hh -=1;
        if (t_hh < 0)
          t_hh = 0;
      }
      else
      {
        t_mi -=1;
        if (t_mi < 0)
          t_mi = 0;
      }
      sleep_ms(dly);
    }
    if (pico_explorer.is_pressed(pico_explorer.X))
    {
      btn = 3;
      std::cout << TAG+"X"+SFX << std::endl;
      mode_HM = !mode_HM; // toggle mode HM
      sleep_ms(dly);
    }
    if (pico_explorer.is_pressed(pico_explorer.Y))
    {
      btn = 4;
      std::cout << TAG+"Y"+SFX << std::endl;
      l_2ndloop = true;
      l_1stloop = false;  // exit the 1st loop
      sleep_ms(dly);
    }

    if (btn == 1 or btn == 2)
      t_chgd = true;  // global variable
    else
      t_chgd = false;
    // Update HH:MM:SS
    btn = 0;

    if (t_chgd)
    {
      bar = std::to_string(t_hh)+" : "+std::to_string(t_mi)+": 00";
      set_DispColour(true, bgnd_colour, true);
      set_DispColour(false, fgnd_colour, false);
      for (auto i = 0; i < tmpv.size()-1; i++)  // don't print the last (empty) item
      {
        p.y = ofs3.at(i);
        if (i == 2)
          pico_explorer.text(bar, Point(p.x, p.y), 239, 4);
        else
          pico_explorer.text(tmpv.at(i), Point(p.x, p.y), 239, 4);
      }
      pico_explorer.update();
      t_chgd = false;
      bar.clear();
    }
    if (lstop)
      break;
  } while (l_1stloop);

  lstop = false;
  // 2nd loop: display the down-counting timer
  do
  {
    if (!alrm)
    {
      if (ck_btn_press()) // exit this function when a keypress interrupts the timer countdown
      {
        menu = MENU_MAIN; // Force to go back to the main menu
        btn_msgd("menu main",0);
        return true;  // exit this function here 
      }
      led_toggle(); // blink LED
      sleep_ms(1000); // 1 second
      t_ss -= 1;
      if (t_ss < 0)
      {
        t_ss = 59;
        t_mi -=1;
        if (t_mi < 0)
        {
          t_mi = 59;
          t_hh -= 1;
          if (t_hh < 0)
            t_hh = 0;
        }
      }
      if (t_hh == 0 && t_mi == 0 && t_ss == 0)
        alrm = true;
    }
    set_DispColour(true, bgnd_colour, true);
    set_DispColour(false, fgnd_colour, false);
    p.y = ofs3.at(2);
    pico_explorer.text("Countdown", Point(p.x, p.y), 239, 4);
    bar = std::to_string(t_hh)+" : "+std::to_string(t_mi)+" : "+std::to_string(t_ss);
    p.y = ofs3.at(4);
    pico_explorer.text(bar, Point(p.x, p.y), 239, 5);
    pico_explorer.update();
    bar.clear();
    lstart = false;
    //sleep_ms(20);  // loop delay

    if (alrm)
    {
      alrm_loop = true;
      max_loop = 15;
      isblk = true;
      bar = "A L A R M";
      set_DispColour(true, DISP_BLACK, true);
      do
      {
        set_DispColour(false, DISP_YELLOW, false);
        p.y = ofs3.at(2);
        pico_explorer.text(bar, Point(p.x, p.y), 239, 5);
        p.y = ofs3.at(4);
        pico_explorer.text(tmpv.at(5), Point(p.x, p.y), 239, 5);
        pico_explorer.update();
        for (auto i=0; i < max_loop; i++)
        {
          set_DispColour(true, (isblk ? DISP_BLACK : DISP_RED), true);
          isblk = !isblk; // toggle

          if (ck_btn_press())  // Check for button press
          {
            alrm_loop = false; 
            l_2ndloop = false;  // force exit 2nd loop
            break; 
          }
          sleep_ms(100);
        } // end-of for... loop
        alrm = false;
      } while (alrm_loop);
      bar.clear();
      set_DispColour(true, bgnd_colour, true);
      sleep_ms(1000);
    }  // end-if (alrm)
  } while (l_2ndloop);
  menu = MENU_MAIN; // Force to go back to the main menu
  btn_msgd("menu main",0);
  return true;
}

bool setup()
{
  bool dummy, lSetUsrDefault = true;
  bool m24Hr, noTrickleChg, lvlSwMode = true;
  short int rest_result = -3;  // provoke defaults for LED, back- and foreground colours if rest_fm_EP() not called.
  uint8_t nRes = 0;
  uint8_t setup_result = 0x0; // each function call we increase with a factor 2
  static const std::string TAG = "setup(): ";
  static const std::string str2(80,'-');
  static const std::string errstr = "Something went wrong, check wiring";
  static const std::string bfstr = "LCD back-/foreground colours defaulted to: ";
  static const std::string ledstr = "LED On/Off defaulted to: ";
  static const std::string rwt = "Read/Write Time - RTC Example";
  clr_setup_stat(); // Clear the setup_stat bitset
  clr_vsettings(); // Clear the v_settings vector
  gpio_init(USBPWR_PIN);
  gpio_set_dir(USBPWR_PIN, GPIO_IN);
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  lUSB = USBpwr();

  pico_explorer.init();  // does not return a result (void)   THIS FUNCTION CALL HAS TO BE HERE...AT THE TOP, BEFORE ALL OTHER CALLS (TO WHATEVER, e.g. rtc.init() etcetera)
  sleep_ms(1000);

  nRes = rtc.init();
  sleep_ms(500);

  lRTC = rtc.setup(m24Hr, noTrickleChg, lvlSwMode);  //true, true, true);
  sleep_ms(500);

  std::cout << "\n"+str2 << std::endl;
  if (nRes != 51)  // result of rtc.init()  51 = 0x33 if this RV3028 breakout module is connected. 
  {                // If the module is not connected then the value will be differen, oftern having the value F in the MSB.
    setup_result |= 0x1;     
    fail_msgs(setup_result);
    return false;
  }
  else
  {
    uint8_t pid;
    uint8_t vid;
    pid = ((nRes & 0xf0) >> 4);
    vid = (nRes & 0xf);
    buffer rtc_IDv1 {nRes};
    buffer rtc_IDv2 {pid, vid};
    std::cout << TAG+"RTC online! hardware ID: \'" << std::to_string(nRes) << "\', in hex: \'0x" << hexify(rtc_IDv1) << 
      "\'. Part. ID: \'" << std::to_string(pid) << "\', Version: \'" << std::to_string(vid) << "\'" << std::endl;
  }
  
  setup_stat[SU_RTC_INIT] = lRTC ? 0 : 1;
  sleep_ms(500); 

  rest_result = rest_fm_EP(); // read user data (SETT_LED, SETT_12HR, SETT_BGND and SETT_FGND) from rtc's user EEPROM memory to v_settings vector array elements
  std::cout << TAG+"result received from call to rest_fm_EP() is: " << std::to_string(rest_result) << std::endl;
  if (rest_result == -1)
  {  // Retrieval LED data failed
    v_settings.at(SETT_LED) = 1;  // Blink LED
    setup_stat[SU_REST_FM_INIT_LED] = 1;
  }
  else if (rest_result <= -2)  // Retrieval Back/Foreground data failed
  {
    setup_stat[SU_REST_FM_INIT_BF] = 1;
    bgnd_colour = DISP_BLACK;  // Default the global variable for background colour of LCD
    v_settings.at(SETT_BGND) = DISP_BLACK;
    fgnd_colour = DISP_YELLOW; // Default the foreground colour (text font colour)
    v_settings.at(SETT_FGND) = DISP_YELLOW;
    std::cout << TAG+bfstr << colours_h.at(bgnd_colour) << "/" << colours_h.at(fgnd_colour) << std::endl;

    if (rest_result == -3)
    {
      setup_stat[SU_RTC_UPDATETIME] = 1;
      v_settings.at(SETT_LED) = 1;  // Also default LED
      std::cout << TAG+ledstr<< v_settings.at(SETT_LED) << std::endl;
    }
  }
  if (enc.init()) 
  {
    lENC = true;
    enc.set_led(0,0,0); // Switch off the rgb encoder led
  }
  else
  {
    lENC = false;
    setup_stat[SU_ENC_SETUP] = 1;
  }

// HERE WAS: pico_explorer.init(); << MOVED TO THE TOP OF setup() ---- THIS WAS THE SOLUTION FOR NOT WORKING FUNCTION CALLS BELOW !!!!!

  set_DispColour(true, DISP_BLACK, true);  // Set display background colour to ... and fill the display with this colour

  pico_explorer.set_audio_pin(pico_explorer.GP0);  // does not return a result (void)
  sleep_ms(1000);

  pico_explorer.set_font(&font6);

  set_DispColour(false, DISP_YELLOW, false);  // Set display foreground colour 

  dummy = rtc.updateTime(); // Update the datetime registers
  sleep_ms(500);
  setup_result |= dummy ? 0x0 : 0x20;
  setup_stat[SU_RTC_UPDATETIME] = (dummy == false) ? 0 : 1;

  upd_app_time();  // fill the app_time array with the rtc clock data
  setup_stat[SU_UPD_APP_TIME] = 1;

  setup_result = 0;
  for (auto i = 0; i < setup_stat.size(); i++)
  {
    if (setup_stat[i] == 1)
      setup_result += 1;
  }

  std::cout << TAG+rwt << std::endl;
  if (setup_result > 0)
    fail_msgs(setup_result);

  std::cout << TAG+"leaving..." << std::endl;
  return true;
}

void upd_app_time()
{
  static const std::string TAG = "upd_app_time(): ";
  //std::cout << "upd_app_time(): entered... Calling rtc.updateTime()" << std::endl;
  rtc.updateTime();  // load latest time from rtc registers
  sleep_ms(20);
  for (int i = TIME_SECONDS; i < TIME_ARRAY_LENGTH; i++)
  {
    switch(i)
    {
      case TIME_SECONDS:
        app_time.at(TIME_SECONDS) = (int)rtc.getSeconds();
        break;
      case TIME_MINUTES:
        app_time.at(TIME_MINUTES) = (int)rtc.getMinutes();
        break;
      case TIME_HOURS:
        app_time.at(TIME_HOURS) = (int)rtc.getHours();

        //Build_Hour is 0-23, convert to 1-12 if needed (see rtc.setToCompilerTime() )
        if (rtc.is12Hour())
        {
          int hour = app_time.at(TIME_HOURS);

          bool pm = false;

          if(hour == 0)
            hour += 12;
          else if(hour == 12)
            pm = true;
          else if(hour > 12)
          {
            hour -= 12;
            pm = true;
          }

          app_time.at(TIME_HOURS) = hour; //Load the modified hours
        
          if(pm == true) app_time.at(TIME_HOURS) |= (1<<HOURS_AM_PM); //Set AM/PM bit if needed
        }

        break;
      case TIME_WEEKDAY:
        app_time.at(TIME_WEEKDAY) = (int)rtc.getWeekday();
        break;
      case TIME_DATE:
        app_time.at(TIME_DATE) = (int)rtc.getDate();
        break;
      case TIME_MONTH:
        app_time.at(TIME_MONTH) = (int)rtc.getMonth();
        break;
      case TIME_YEAR:
        app_time.at(TIME_YEAR) = (int)rtc.getYear();
        break;
    }
  }

  std::cout << TAG+" the hours value of the rtc   is: " << std::to_string(rtc.getHours()) << std::endl;
  std::cout << TAG+" the result of rtc.is12hour() is: " << std::to_string(rtc.is12Hour()) << std::endl;
  if (rtc.is12Hour())
  { 
    v_settings.at(SETT_12HR) = 1;
    std::cout << TAG+" the result of rtc.isPM() is: " << std::to_string(rtc.isPM()) << std::endl;
    if ( rtc.isPM())
      app_time_IsPM = 1;
    else
      app_time_IsPM = 0;
  }
  else
    v_settings.at(SETT_12HR) = 0;
    app_time_IsPM = -1;  // Indicate we're not using AM/PM in this moment

  std::string s = TAG+"app_time vector array is set to: " +
    std::to_string(app_time.at(TIME_YEAR)) + "-" +
    std::to_string(app_time.at(TIME_MONTH)) + "-" +
    std::to_string(app_time.at(TIME_DATE)) + 
    ", weekday: " + 
    std::to_string(app_time.at(TIME_WEEKDAY)) +
    ", t: " +
    std::to_string(app_time.at(TIME_HOURS)) + ":" +
    std::to_string(app_time.at(TIME_MINUTES));

  if (v_settings.at(SETT_12HR) == 1 )
    if (app_time_IsPM)  // Clock is in 12HR mode
      s += " PM";
    else 
      s += " AM";
  else
    s += ":"+std::to_string(app_time.at(TIME_SECONDS));  // Clock is in 24HR mode

  std::cout << s << std::endl;
}

bool USBpwr() 
{
  return gpio_get(USBPWR_PIN);
}

int w_btn()
{
  //std::cout << "w_btn(): entered..." << std::endl;
  int n = -1;
  int idx = 0;
  for (auto i = 0; i < btns_itm.size(); i++)
  {
      if (btns_itm[i] == 1)
      {
        std::cout << "button " << b_tns.at(i) << " pressed." << std::endl;
        btns_itm[i] = 0;  // clear the button
      }
  }
  return n;
}

/*
*
* HERE START THE ENCODER FUNCTIONS
*
*/
void encoder_count_changed(bool lStrt) 
{
  int16_t count = my_ctr.have_cnt(); // get the current count value
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  int8_t idx = 0;
  std::vector<int> rgb = {0,0,0};
  Point p;
  p.x = 10;
  //int pe = 220;
  //int fnt_corps = 6;

  if (my_debug)
    printf("\nCount: %d", count);

  if (lStrt)
  {
    r = disp_white.at(RED);
    g = disp_white.at(GREEN);
    b = disp_white.at(BLUE);
  }

  if (count < 0)
    idx = -count;
  else
    idx = count;

  idx = idx % STEPS_PER_REV;  // range 0-23 only. Needed as index to clrTbl
  if (my_debug)
  {
    printf("\nclrTbl.size(): %d", clrTbl.size());
    printf("\nidx: %d\n", idx);
  }
  //sleep_ms(1000);

  if (count < 0)
  {  //          correct for situation that -1 % 24 gives 23 and -23 % 24 gives 1
    idx = modTbl.at(idx);  // correct step 2)  if idx is 23 then make it 1 ... if idx is 1 then make it 23
    if (my_debug)
      printf("corrected idx: %d\n", idx);
  }

  if (idx >= clrTbl.size())
    idx = clrTbl.size()-1; // set idx to last vector element
  else if (idx < 0)
    idx = 0;
  rgb = clrTbl.at(idx);
  r = rgb.at(0);
  g = rgb.at(1);
  b = rgb.at(2);
  if (my_debug) 
  {
    printf("rgb = %d,%d,%d\n", r, g, b);
    bgnd_colour =  DISP_AUBERGINE;
    fgnd_colour =  DISP_YELLOW;
    set_DispColour(true, bgnd_colour, true);  // clear screen in colour
    sleep_ms(20);
    set_DispColour(true, fgnd_colour, false);
    encoder_disp_a_txt(std::to_string(r),0);
    encoder_disp_a_txt(std::to_string(g),1);
    encoder_disp_a_txt(std::to_string(b),2);
    sleep_ms(3000);
  }
  enc.set_led(r, g, b);
}

void encoder_intro(int choice) 
{
  static const std::string s1 = "Encoder ";
  static const std::string s1a = "found ";
  static const std::string s2 = "at address: ";
  std::string s3 = "";
  static const std::string s4 = s1 + "not " +s1a;

  int pe = 239;
  int fnt_corps = 4;
  Point p;
  p.x = px_dflt;
  set_DispColour(true, bgnd_colour, true);  // clear the screen
  sleep_ms(20);
  set_DispColour(true, fgnd_colour, false);
  if (choice == 1) 
  {
    p.y = ofs.at(0);
    s3.append(std::to_string(enc.get_address()));
    //enc.set_direction(BreakoutEncoder::DIRECTION_CCW);    // Uncomment this to flip the direction
    if (my_debug)
      std::cout << s1 << s1a << " " << s2 << std::hex << "0x" << enc.get_address() << std::endl;

    for (int i=1; i <4; i++) {
      p.y = ofs.at(i);
      if (i == 1)
        pico_explorer.text(s1+s1a, Point(p.x, p.y), pe, fnt_corps);
      else if (i == 2)
        pico_explorer.text(s2, Point(p.x, p.y), pe, fnt_corps);
      else if (i == 3)
        pico_explorer.text(s3, Point(p.x, p.y), pe, fnt_corps);
    }
  }
  else if (choice == 2) 
  {
    if (my_debug)
      std::cout << s4 << std::endl;
    p.y = ofs.at(3);
    for (int i=1; i <4; i++) {
      p.y = ofs.at(i);
      if (i == 1)
        pico_explorer.text(s1, Point(p.x, p.y), pe, fnt_corps);
      else if (i == 2)
        pico_explorer.text("not", Point(p.x, p.y), pe, fnt_corps);
      else if (i == 3)
        pico_explorer.text(s1a, Point(p.x, p.y), pe, fnt_corps);
    }
  }
  pico_explorer.update();
  sleep_ms(2000);
}

void encoder_disp_a_txt(std::string s, int row) 
{
  int pe = 239;
  int fnt_corps = 4;
  int n = ofs.size();
  int n2;
  Point p;
  p.x = px_dflt;
  if (row >= 0 && row < n) 
  {
    p.y = ofs.at(row);
    if (s.size() > 0) {
      n2 = s.size() < 11 ? s.size() : 10;
      pico_explorer.text(s.substr(0,n2), Point(p.x, p.y), pe, fnt_corps);
      pico_explorer.update();
    }
  }
} 

void encoder_disp_btn_pr(std::string s, bool nop)
{
  if (nop)
  {
    encoder_disp_a_txt("btn not",2);
    encoder_disp_a_txt("in use",3);
  }
  else
  {
    if (s.size()>0)
    {
      encoder_disp_a_txt("button "+s,2); // display text on 3rd ofs item p.y
      encoder_disp_a_txt("pressed",3);
    }
  }
}
    
bool encoder_IsBtnPressed()
{
  if (btns_enc[BTN_A]==1 || btns_enc[BTN_B]==1 || btns_enc[BTN_X]==1 || btns_enc[BTN_Y]==1)
    return true;
  return false;
}

void encoder_ck_btns(bool show_txt)
{
  bool nop = true;
  bool lRet = false; // used for ck_YN() double-check for evt. reset
  encoder_clr_btns(); // clear the enc btn flags

  if (my_debug && show_txt)
  {
    std::cout << "To proceed: press A,B,X or Y button" << std::endl;
    encoder_disp_a_txt("Push btn:", 1);
  }

  if (pico_explorer.is_pressed(pico_explorer.A))
  {
    btns_enc[BTN_A] = 1;
    encoder_disp_btn_pr(b_tns.at(BTN_A), nop);
    sleep_ms(500);  // Prevent debounce effect 
    return;
  }
  if (pico_explorer.is_pressed(pico_explorer.B))
  {
    btns_enc[BTN_B] = 1;
    encoder_disp_btn_pr(b_tns.at(BTN_B), nop);
    sleep_ms(500);
    return;
  }
  if (pico_explorer.is_pressed(pico_explorer.X))
  {
    my_ctr.st_stp(true); // indicate to stop
    btns_enc[BTN_X] = 1;
    sleep_ms(500);
    return;  
  }
  if (pico_explorer.is_pressed(pico_explorer.Y))
  {
    btns_enc[BTN_Y] = 1;
    encoder_disp_btn_pr(b_tns.at(BTN_Y), false);
    sleep_ms(500);
    return;
  }
  return; 
}

void encoder_disp_cnt()
{
  static const std::string cnt = "count ";
  set_DispColour(true, bgnd_colour, true);  // clear screen in colour
  sleep_ms(20);
  set_DispColour(true, fgnd_colour, false);
  encoder_disp_a_txt(cnt + " " + std::to_string(my_ctr.have_cnt()), 1);
}

bool encoder_ck()
{
  if (enc.init()) 
  {
    lENC = true;
    set_DispColour(true, bgnd_colour, true);  // clear screen in colour
    sleep_ms(20);
    set_DispColour(true, fgnd_colour, false);
    encoder_disp_a_txt("enc found", 1);
    encoder_intro(1);  // Display text about encoder found
    sleep_ms(1000);
    set_DispColour(true, bgnd_colour, true);  // clear screen in colour
    sleep_ms(20);
    set_DispColour(true, fgnd_colour, false);
    enc.clear_interrupt_flag();
  }
  else
  {
    encoder_intro(2);  // Display text about encoder not found
    return false;
  }
  return true;
}

bool encoder_doit() 
{
  int16_t nw_count = 0;
  std::string btn = "0";
  std::string s = "";
  bool lenc_Start = true;
  bool toggle = false;
  bool lRes = true;
  bool lYN = false; // used for ck_YN() before calling reset
  set_DispColour(true, bgnd_colour, true);  
  sleep_ms(20);
  set_DispColour(true, fgnd_colour, false);
  encoder_disp_a_txt("encoder", 0);
  encoder_disp_a_txt("test", 1);
  sleep_ms(2000);
  my_ctr.clr_cnts(); // zero all private count values

  lRes = encoder_ck();
  if (lRes)
  {
    enc.clear_interrupt_flag();  // added to free evt. interrupt locks
    while(true) 
    {
      gpio_put(PICO_DEFAULT_LED_PIN, toggle);
      toggle = !toggle;
      if (enc.get_interrupt_flag()) 
      {
        nw_count = enc.read();  // the read() calls the ioe.clear_interrupt_flag()
        if (lenc_Start || (nw_count != my_ctr.have_old_cnt())) {  // show count also at startup
          my_ctr.upd_cnt(nw_count);
          encoder_count_changed(lenc_Start);
          encoder_disp_cnt();
          if (lenc_Start)
            lenc_Start = false;
        }
        //enc.clear_interrupt_flag();
      }
      encoder_ck_btns(false); // Check if a btn has been pressed. If so, handle it
      if (my_ctr.is_stp())
      {
        bool lYN = ck_YN("Reset Pico?", "btn X = No", "btn Y = Yes"); // Prevent inintended reset by asking the user if he really wants to reset the clock
        if (btns_itm[BTN_Y] == 1)
        {
          /** 
          *  https://www.raspberrypi.org/forums/viewtopic.php?p=1870355, post by cleverca on 2021-05-27.
          *  \param pc If Zero, a standard boot will be performed, if non-zero this is the program counter to jump to on reset.
          *  \param sp If \p pc is non-zero, this will be the stack pointer used.
          *  \param delay_ms Initial load value. Maximum value 0x7fffff, approximately 8.3s.
          *
          * *            (pc, sp, delay_ms) */
          encoder_reset();
        }
        encoder_clr_btns();
        break;
      }
      else if (btns_enc[BTN_Y] == 1) // Return to the main menu
      {
        encoder_clr_btns();
        //sett_itm = SETT_BACK;
        my_ctr.clr_cnts(); // set the count and old_count to 0
        if (v_settings.at(SETT_LED) == 0 && ledIsOn)
          led_toggle();
        break;
      }
      else if (encoder_IsBtnPressed())
      {
        sleep_ms(1000); // let the 'btn not in use' msg stay for a moment
        encoder_disp_cnt();  // method to wipe away btn msg
      }
      sleep_ms(100); // loop delay
    }
  }
  enc.set_led(0,0,0); // Switch off the rgb encoder led
  return lRes;
}

void encoder_clr_btns()
{
  for (auto i = 0; i < btns_enc.size(); i++)
    btns_enc[i] = 0;
  my_ctr.st_stp(false); // reset stop flag
}

void encoder_reset()
{
  set_DispColour(true,  bgnd_colour, true);
  set_DispColour(false,  fgnd_colour, false);
  encoder_disp_a_txt("reset !", 3);
  sleep_ms(2000);
  /*            (pc, sp, delay_ms) */
  watchdog_reboot(0, 0, 0x7fffff); // varying delay_ms between 0x1fffff and 0x7fffff does not make much difference!
}

void z_main_loop() // I give this function this name, to have it alfabetically at the end, before main(), which is at the real end (as usual)
{
  int i = 0;
  bool lStart = true;
  bool lUpd = true;
  bool dummy = false;
  long loopnr = 0;
  std::vector<std::string> dt(4);
  int le, n = 0;
  long et = 0;
  Point p;

  while(true) 
  {
    clr_itm_btns();
    dummy = a_menu_handler();  // chk for menu state, next handle btn press
    if (t_chgd)
      std::cout << "z_main_loop(): the value of t_chgd is: " << (t_chgd ? "True" : "False") << std::endl;
      i = 0;
    if (lStDT) {
      get_datetime(); // update global app_time
    }
    lStDT = false;
    if (i % 10 == 0)
    {
      lUpd = rtc.updateTime();
      sleep_ms(20);
      upd_app_time(); // update app_time vector array
      //rgb_h += 1;
      //my_rgb(rgb_h);
      p.x = p_x_default;
      if (lStart || t_chgd)
      {
        if (lStart)
          std::cout << "(re)start date && time: " << rtc.stringDT() << std::endl;
        set_DispColour(true, bgnd_colour, true);
        sleep_ms(10);
        set_DispColour(false, fgnd_colour, false);
        dt = prep_datetime(); // get the texts to be displayed in a vector array

        for (auto j = 0; j < dt.size(); j++)
        {
          p.y = ofs.at(j);
          pico_explorer.text(dt.at(j), Point(p.x, p.y), 239, (j == 3) ? 6 : 5 );
        }
        pico_explorer.update();
        dt.clear();
        lStart = false;
        t_chgd = false; 
      }
      else 
      {
        dt = prep_datetime(); // get the texts to be displayed in a vector array
        et = elapsedtime();

        std::cout << "Loop nr: " << std::to_string(loopnr) << ". Elapsed time: " << std::to_string(et) << " seconds. In minutes: " <<  std::to_string(et/60) << std::endl;
        pr_in_which_menu();
        std::cout << "t: " <<  dt.at(DOIT_TIME)  << std::endl;
        set_DispColour(true, bgnd_colour, true);
        sleep_ms(10);
        set_DispColour(false, fgnd_colour, false);
        for (auto j = 0; j < dt.size(); j++)
        {
          p.y = ofs.at(j);
          pico_explorer.text(dt.at(j), Point(p.x, p.y), 239, (j == 3) ? 6 : 5 );
        }
        pico_explorer.update();
        sleep_ms(10);
        dt.clear();
      }
      if (rtc.getHours() == 0)
      {
        if (lastHrOfDay)  // the current hour is 0. We are at a new day if flag lastHrOfDay is true
        {
          i = 9;  // force i % 10 == 0 in next loop
          lStart = true;
          lastHrOfDay = false;
        }
      }
      led_toggle();
      sleep_ms(20); 
    }  // end-of if ...
    i += 1;
    if (i == 255)
      i = 0;
    loopnr += 1;
    if (loopnr == 65535)
      loopnr = 0;
  }  // end of while ...
}

void _exit(int status) {
  printf("_exit(): status %d", status); // This function added to prevent a compiler error 'unidentified reference
  //                                       to _exit
};

/// \tag::hello_rtc_main[]
int main()
{
  bool lRet = true;
  stdio_init_all();
  sleep_ms(500);  // Delay for 10 seconds to let the desktop PC serial program 'catch' the RPi Pico's output
  std::cout << "Hello RTC!\n" << std::endl;

  lRet = setup();
  if (lRet == true)
  {
    Ut = rtc.getUNIX();  // Set Unixtime for first time
    //rtc.set_datetime(&t);
    bool res = false;
    std::cout << "Real-time clock test." << std::endl;
    std::cout << "Current datetime stamp is: " << rtc.stringDate() << "." << std::endl;
    btn_msg();
    z_main_loop();
    std::cout << "main(): entering perpetual loop...\nIf using Thonny IDE: click on: \"Stop/Restart backend\" or press: \"<Ctrl>+C\"\n$$$_" << std::endl;
  }
  int cnt = 0;
  btn_help(); // was:  btn_msgd(" ", 3);
  while(true)
  {
    led_toggle();
    sleep_ms(500);
    cnt += 1;
    if (cnt  > 0 && cnt % 50 == 0) {
      std::cout << "In perpetual loop of main(). RTC is: " << ((lRTC == true) ? "Online" : "Offline") << std::endl;
      btn_help(); // was:  btn_msgd(" ", 3);
    }
    if (cnt > 1000)
      cnt = 0;
  }
  return 0;  // in C++ main() always has to return an int
/// \end::hello_rtc_main[]
}

#endif // PSK_MAIN_HH