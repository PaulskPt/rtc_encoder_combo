#pragma once
/**
 * Partly Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Paryly Copyright (c) 2020 Pimoroni Ltd.
 * Partly Copyright (c) 2021 Paulus Schulinck @paulsk. 
 * 2021-06-30 10h30 start of combining the rtc and the encoder apps to one rtc_encoder app.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Note @paulsk dd 2021-04-21 at 13h12 PT
 * I continue to build this project from within MS Windows WSL1 / Ubuntu 20.04 because of linker problems I faced when doing the build * from withing VSCode in MS Windows 10 Pro.
 */

#ifndef PSK_MAIN_HPP
#define PSK_MAIN_HPP

#include "../../drivers/rv3028/rv3028.hpp"
#include "../../libraries/pico_explorer/pico_explorer.hpp"
//#include "../../libraries/pico_graphics/pico_graphics.hpp"

#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <string>
#include <vector>
#include <bitset>
#include <cstdint>  // uint8_t etc.
#include <fstream>
#include <iomanip>
#include <iostream>

//#include "../../../../../pico-sdk/src/common/pico_util/include/pico/util/datetime.h"
#include "../../../../../pico-sdk/src/common/pico_base/include/pico/types.h"

using namespace pimoroni;

bool my_debug = false;
bool overall_reset = true;  // use the system/pico reset instead of an rtc clock reset-to-certain-datum

#define yy 0
#define mm 1
#define dd 2
#define wd 3
#define hh 4
#define mi 5
#define ss 6 

const uint8_t USBPWR_PIN = 24;
const uint8_t LED_PIN = 25; // PICO_DEFAULT_LED_PIN;  // Pin 25
std::bitset<4> btns_itm; // x[0-3] is valid
std::bitset<12> setup_stat; // x[0-11] is valid
const std::vector<int> ofs = {20, 60, 110, 180};
const std::vector<int> ofs3 = {5,35,65,95,125,155,185,215, 235};
const std::vector<std::string> b_tns = {"A", "B", "X", "Y"};
const std::vector<std::string> itmlst = {"ss", "mi", "hh", "wd", "dd", "mo", "yy", "rs", "mu", "??"};
const std::vector<std::string> itmlst_h = {"seconds", "minute", "hour", "weekday", "day", "month", "year", "reset", "menu", "help"};
const std::vector<std::string> wdays = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const std::vector<std::string> mnths = {"   ", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const std::vector<std::string> settings =      {" ", "IsLED",    "Is12HR",     "IsALRM",    "IsTIMR",    "bGND",      "fGND",      "ENC",     "EXIT"};
const std::vector<std::string> settings_menu = {" ", "led OnOff", "12/24 hrs", "set Alarm", "set Timer", "b_GND clr", "f_GND clr", "encoder", "menu exit"};
const std::vector<std::string> colours = { "blk", "red", "aub", "ora", "yel", "grn", "blu", "whi"};
const std::vector<std::string> colours_h = { "black", "red", "aubergine", "orange", "yellow", "green", "blue", "white"};

std::vector<int> app_time = {0, 0, 12, 5, 1, 5, 2021};  // {secs, mins, hrs, weekday, date, month, year}
// v_settings cannot be a bitset because bGND and fGND have values that can be > 1
std::vector<int> v_settings = {1, 1, 0, 0, 0, 0, 0, 0};  // IsLED, Is12, IsALRM, IsTIMR, bGND, fGND, IsENC, Exit

// display background colour sets vectors of integer arrays

const std::vector<int> disp_black =  {0,    0,    0};   // black   see: https://rgbcolorcode.com/color/yellow
const std::vector<int> disp_red =    {120,  0,    0};   // aubergine
const std::vector<int> disp_auber =  {120, 40,   60};   // aubergine
const std::vector<int> disp_orange = {255, 128,   0};   // orange
const std::vector<int> disp_yellow = {255, 255,   0};   // yellow
const std::vector<int> disp_green =  {0,   120,   0};   // green
const std::vector<int> disp_blue =   {0,     0, 120};   // blue
const std::vector<int> disp_violet = {170,   0, 255};   // violet
const std::vector<int> disp_white =  {255, 255, 255};   // white

enum menu_order
{
  MENU_MAIN,
  MENU_SETTINGS,
  MENU_BACK
};

enum menu_settings_order
{
  SET_DUMMY,        // 0
  SETT_LED,         // 1
  SETT_12HR,        // 2
  SETT_ALRM,        // 3
  SETT_TIMR,        // 4
  SETT_BGND,        // 5
  SETT_FGND,        // 6
  SETT_ENC,         // 7
  SETT_BACK         // 8
};

enum menu_itm_order
{
  ITM_SECONDS,      // 0
  ITM_MINUTES,      // 1
  ITM_HOURS,        // 2
  ITM_WEEKDAY,      // 3
  ITM_DATE,         // 4
  ITM_MONTH,        // 5
  ITM_YEAR,         // 6
  ITM_RESET,        // 7
  ITM_MENU,         // 8
  ITM_HELP          // 9
};

enum disp_colour_order
{
  DISP_BLACK,       // 0
  DISP_RED,         // 1
  DISP_AUBERGINE,   // 2
  DISP_ORANGE,      // 3
  DISP_YELLOW,      // 4
  DISP_GREEN,       // 5
  DISP_BLUE,        // 6
  DISP_WHITE        // 7
};

enum button_order
{
  BTN_A,   // 0
  BTN_B,   // 1
  BTN_X,   // 2
  BTN_Y    // 3
};

enum z_main_loop_order
{
  DOIT_WEEKDAY,   // 0
  DOIT_MONTH,     // 1
  DOIT_YEAR,      // 2
  DOIT_TIME       // 3
};

enum rgb_colour_order
{
  RED,     // 0
  GREEN,   // 1
  BLUE     // 2
};

typedef struct {
  float r;
  float g;
  float b;
} rgb_t;

enum setup_results_order
{
  SU_RTC_INIT,                 // 0
  SU_RTC_SETUP,                // 1
  SU_REST_FM_INIT_LED,         // 2
  SU_REST_FM_INIT_BF,          // 3
  SU_RTC_UPDATETIME,           // 4
  SU_ENC_SETUP,                // 5
  SU_UPD_APP_TIME,             // 6
  SU_SUCCESSFULL,              // 7
  SU_FAILED,                   // 8
  SU_UNKNOWN,                  // 9
  SU_NOT_FOUND,                // 10
  SU_FOUND                     // 11
};

int menu = MENU_MAIN; // Set to use the main menu
int itm = ITM_HELP;  // Set start value of itm. Was: (int)itmlst.size()-1;   0 = ss, 1 = mi, 2 = hh, 3 = wd, 4 = dd, 5 = mo, 6 = yy, 7 = rs, 8 = ?? (help)
int sett_itm = SETT_LED; // idem for settings

int usr_dat_len_at_backup = 0;  // Holds the length (in bytes) of certain user data being saved to EEPROM (see backp_to_EP() and rest_fm_EP() )
int app_time_IsPM = -1;

char datetime_buf[256];
char *datetime_str = &datetime_buf[0];

int bgnd_colour = DISP_ORANGE; // Global display background colour
int fgnd_colour = DISP_YELLOW;  // Global display foreground (text) colour
int p_x_default = 15;

/*
* Start on Friday 1st of January 2021 00:00:00
* datetime_t is defined in: I:\pico\pico-sdk\src\common\pico_base\include\pico\types.h
* 0 is Sunday, so 5 is Friday
*/

#ifndef TIME_ARRAY_LENGTH
#define TIME_ARRAY_LENGTH 7 // Total number of writable values in device
#endif


// See: https://codereview.stackexchange.com/questions/242052/conversion-into-hexadecimal-using-c , by  by @Malvineous
typedef std::vector<uint8_t> buffer;

// We need to make a custom type so we can control which function the compiler will call.
struct hexbuffer {
    const buffer& innerbuf;
};

// 45 Functions pre-defines:

/*! \brief operator overload (See note at typedef buffer above.
*  \ingroup rtc_encoder_combo
*
* \param vector of char
* \return string
*/
std::ostream& operator << (std::ostream&, const hexbuffer&);

/*! \brief a_itm_handler, handles the clock items
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool a_itm_handler(); 

/*! \brief menu handler function
*  \ingroup rtc_encoder_combo
*
* \param None
* \return int
*/
int a_menu_handler();

/*! \brief backup_to_EP(), backups user data, like foreground/background colours, to User EEPROM memory (00h - 2Ah = 43 bytes
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool backp_to_EP();

/*! \brief btn_ab_itm, directions: previous/next and down/up
*  \ingroup rtc_encoder_combo
*
* \param boolean dr
* \return boolean
*/
bool btn_ab_itm(bool dr);

/*! \brief btn_ab_settings, menu for various settings like built-in LED, 24/12 hour mode, alarm and timer
*  \ingroup rtc_encoder_combo
*
* \param bool  // if True choose itm if false choose sett_itm
* \return boolean
*/
bool btn_ab_settings(bool);

/*! \brief btn_help, shows help info on the lcd display (former part of btn_msgd with ln ==3)
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void btn_help();

/*! \brief btn_ID, gets the Id nr of the button pressed
*  \ingroup rtc_encoder_combo
*
* \param uint8_t, k, the numeric value of the button pressed
* \return string, the letter of the button pressed: A, B, X or Y.
*/
std::string btn_ID(uint8_t);

/*! \brief btn_msg. Prints  a buttons info start msg on REPL
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void btn_msg();

/*! \brief btn_msgd. Prints msg on REP and on display
*  \ingroup rtc_encoder_combo
*
* \param std::string text to be printed on the display
* \param int   row nr, ln, of the display on which the text to be displayed
*
* \return bool
*/
void btn_msgd(std::string, int);

/*! \brief btn_xy_itm, handles the value of the setting count
*  \ingroup rtc_encoder_combo
*
* \param bool dr, the direction of movement: if False: previous, if True: next
* \return None
*/
void btn_xy_itm(bool);

/*! \brief btn_xy_settings, handles the value of the itm count sett_itm
*  \ingroup rtc_encoder_combo
*
* \param bool dr, the direction of movement: if False: previous, if True: next
* \return None
*/
void btn_xy_settings(bool);

/*! \brief ck_btn_press, checks if one of the four buttons has been pressed. If so, returns true. If not returns false
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool ck_btn_press();

/*! \brief ck_YN, presents user with a question and lets user choose by pressing btn Y for 'Yes' or '12' or btn X for 'No' or '24'
*  \ingroup rtc_encoder_combo
*
* \param Txt string, e.g.: "12/24 mode"
* \param btnX string, e.g.: "btn X = 24"
* \param btnY string, e.g.: "btn Y = 12"
* \param None
* \return boolean e.g. true means here : 24 hrs, false 12 hrs
*/
bool ck_YN(std::string, std::string, std::string);

/*! \brief decr_incr_datetime, handles decrease or increase the date-time item selected by the value of itm
*  \ingroup rtc_encoder_combo
*
* \param bool dr (direction: previous/next or down/up)
* \return None
*/
void decr_incr_datetime(bool);

/*! \brief elapsed time
*  \ingroup rtc_encoder_combo
*
* \param None
* \return uint8_t value of the elapsed time in seconds(i.e.: current unix_time - Ut (the initial unix_time))
*/
int elapsedtime();

/*! \brief fail_msgs, functio called by setup() displays failures
*  \ingroup rtc_encoder_combo
*
* \param uint8_t code
* \return None
*/
void fail_msgs(uint8_t);

/*! \brief get_alarm, set/get alarm. ToDo !
*  \ingroup rtc_encoder_combo
*
* \param None
* \return bool
*/
void get_alarm();

/*! \brief get_datetime, updates the rtc's _time array, fills the app_time vector array and prints "datetime saved" to terminal and to the display
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void get_datetime();

/*! \brief hexify, (See note at typedef buffer above.
*  \ingroup rtc_encoder_combo
*
* \param vector of char
* \return string
*/
hexbuffer hexify(const buffer&);

/*! \brief hsv_to_rgb, calculates the rgb value for the display print commands
*  \ingroup rtc_encoder_combo
*
* \param pointer to structure rgb_t
* \return structure rgb_t
*/
rgb_t hsv_to_rgb(rgb_t*);

/*! \brief led_toggle, toggles the built-in led On/Off. It reads and sets the global variable ledIsOn
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void led_toggle();

/*! \brief my_rgb, does certain calculations for randomazing screen color
*  \ingroup rtc_encoder_combo
*
* \param uint8_t rgb_h, an integer value
* \return None
*/
void my_rgb(int);

/*! \brief not_yet, prints a text to the display
*  \ingroup rtc_encoder_combo
*
* \param bool lDsp, to display or not
* \return None
*/
void not_yet(bool);

/*! \brief pass function, a trial to havean equivalent functionality to Python's pass
*  \ingroup rtc_encoder_combo
*
* \param None
* \return bool
*/
bool pass();

/*! \brief pr_in_which_menu, prints to terminal info about in which menu layer the app is
*  \ingroup rtc_encoder_combo
*
* \param None, the direction of movement: if False: previous, if True: next
* \return None
*/
void pr_in_which_menu();

/*! \brief pr_itemsel, prints to REPL and to the display the item selected
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void pr_itmsel();

/*! \brief prep_datetime(), prepares a std::vector with date and time elements to be displayed from withing doit()
*  \ingroup rtc_encoder_combo
*
* \param None
* \return std::vector<string>
*/
std::vector<std::string> prep_datetime();

/*! \brief reset_clk, reset the clock to date '2021-05-01 Sunday (=6) 12:00:00
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool reset_clk();

/*! \brief rst_itm, checks the number of seconds that passed since the last visit to btn_xy_itm() 
*          function (used to control itm. If longer than 30 secs then reset itm to ITM_HELP value
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool reset_itm();

/*! \brief rest_fm_EP(), restores certain data from User EEPROM to this app.
*  \ingroup rtc_encoder_combo
*
* \param None
* \return int. (0 when OK. -1 when retrieve of LED data failed; -2 when retrieve of back-/foreground colour data failed)
*/
int rest_fm_EP();

/*! \brief set_12_24, choose clock 12 hour (with AM/PM) or 24 hour mode
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool set_12_24();

/*! \brief set_alarm, sets an alarm
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool set_alarm();

/*! \brief set_background, sets lcd background or foreground colour, depending on param
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void set_background();

/*! \brief set_BackupSwitchoverMode, set the rtc backup switchover mode on/off
*  \ingroup rtc_encoder_combo
*
* \param int mode 0 = switchover disabled, 1 = direct switching mode, 2 = standby mode, 3 = level switching mode (default)
* \return None
*/
void set_BackupSwitchoverhMode(int);

/*! \brief set_blink, choose if built-in LED has to blink or not
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void set_blink();

/*! \brief set_datetime, updates the rtc's datetime, when the t_chgd flag is True
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool set_datetime();

/*! \brief set_DispColour, clears the display in a colour defined by the value of param disp_colour 
*          if 3rd param is true. Set the background or the foreground colour depending 1st param
*  \ingroup rtc_encoder_combo
*
* \param bool  bf, background (true) or foreground (false)
* \param int Colour
* \param bool cleardisp, clear the lcd if true (used for clearing the lcd to the background color)
* \return None
*/
void set_DispColour(bool, int, bool);

/*! \brief clr_setup_stat(), clears the setup_stat bitset
*  \ingroup rtc_encoder_combo
* \param None
* \return None
*/
void clr_setup_stat();

/*! \brief clr_vsettings(), clears the v_settings vector
*  \ingroup rtc_encoder_combo
* \param None
* \return None
*/
void clr_vsettings();

/*! \brief set_foreground, sets lcd background or foreground colour, depending on param
*  \ingroup rtc_encoder_combo
*
* \param bool
* \return None
*/
void set_foreground();

/*! \brief set_timer, sets a timer
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool set_timer();

/*! \brief setup, does initiation of the rtc among other things
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool setup();

/*! \brief upd_app_time, updates the app_time array with clock data. Return is true
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void upd_app_time();

/*! \brief USBpwr(), returns true if the RPi Pico is powered from USB, false if not
*  \ingroup rtc_encoder_combo
*
* \param None
* \return boolean
*/
bool USBpwr();

/*! \brief w_btn
*  \ingroup rtc_encoder_combo
*
* \param None
* \return uint8_t what button was pressed (0~3)
*/
int w_btn();

/*! \brief z_main_loop, the main function loop of the app
*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void z_main_loop();

/*! \brief _exit(), formal system exit point. Added to prevent a build error: 'undefined reference to _exit when using arm-none-eabi-g++

*  \ingroup rtc_encoder_combo
*
* \param None
* \return None
*/
void _exit(int);

/*! \brief main, the main function of this app
*  \ingroup rtc_encoder_combo
*
* \param None
* \return integer 
*/
int main();

//---------------------- FROM encoder.hpp ---------------------------------------------

void encoder_count_changed();
void encoder_intro(int);
void encoder_disp_a_txt(std::string, int);
void encoder_disp_btn_pr(std::string, bool);
void encoder_ck_btns(bool);
void encoder_clr_btns();
void encoder_disp_cnt();
bool encoder_ck();
void encoder_reset();
bool encoder_doit();
      
#ifndef PSK_ENCODER_HPP
#define PSK_ENCODER_HPP
/*
#include <string>
#include <vector>
#include "../src/main/main.hpp"
*/

#include "../../libraries/breakout_encoder/breakout_encoder.hpp"
/*
#include "../libraries/pico_explorer/pico_explorer.hpp"
//#include "hardware/gpio.h"
//#include "../../../../../pico-sdk/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h"

using namespace pimoroni;
*/
I2C i2c(BOARD::PICO_EXPLORER); // BREAKOUT_GARDEN);
BreakoutEncoder enc(&i2c);

class Cnt_it
{
  private:
    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
    int16_t count;
    int16_t old_count;
    bool stp_cnt = false;
    bool my_debug2 = false;

    const std::vector<int> ofs_enc = {20, 60, 110, 180};

    int p_x_default2 = 15;

  public:
    //--------------------------------------------------
    // Constructors/Destructor
    //--------------------------------------------------
    Cnt_it() {};

  public: 
    //--------------------------------------------------
    // Methods
    //--------------------------------------------------
    /*
    int init();
    void clr_cnts();
    void upd_cnt(int16_t nw_count);
    int16_t have_cnt() const;
    int16_t have_old_cnt() const;
    void st_stp(bool);
    bool is_stp() const;
    */

    int init() 
    {
      return 0;
    };

    void clr_cnts()
    {
      this->count = 0;
      this->old_count = 0;
    }
    void upd_cnt(int16_t nw_count)
    {
      if (nw_count < this->old_count)
        this->count -= (this->old_count - nw_count);
      else
        this->count += (nw_count - this->old_count);

      this->old_count = this->count;
    }

    int16_t have_cnt() const 
    {
      return this->count;
    }

    int16_t have_old_cnt() const 
    {
      return this->old_count;
    }

    void st_stp(bool flag)
    {
      this->stp_cnt = flag;
    }

    bool is_stp() const
    {
      return this->stp_cnt;
    }

  private:
    void upd_cnts();
    
}; // end of class Cnt_it


static const uint8_t STEPS_PER_REV = 24;
int px_dflt = 15;
std::bitset<4> btns_enc; // x[0-3] is valid



const std::vector<int> modTbl
{     // count:
  0, // dummy
  23, //  -1 % 24 = 23.  modTbl.at(1) results in: 23
  22, //  -2 % 24 = 22
  21, //  -3 % 24 = 21
  20, //  -4 % 24 = 20
  19, //  -5 % 24 = 19
  18, //  -6 % 24 = 18
  17, //  -7 % 24 = 17
  16, //  -8 % 24 = 16
  15, //  -9 % 24 = 15
  14, // -10 % 24 = 14
  13, // -11 % 24 = 13
  12, // -12 % 24 = 12
  11, // -13 % 24 = 11
  10, // -14 % 24 = 10
  9, // -15 % 24 =  9
  8, // -16 % 24 =  8
  7, // -17 % 24 =  7
  6, // -18 % 24 =  6 
  5, // -19 % 24 =  5
  4, // -20 % 24 =  4
  3, // -21 % 24 =  3
  2, // -22 % 24 =  2
  1, // -23 % 24 =  1 modTbl.at(23) results in: 1
  0, // -24 % 24 =  0 modTbl.at(24) results in: 0
};

const std::vector<std::vector<int>> clrTbl
{
  // r    g    b
  {255,   0,   0}, // count = 0  red
  {255,  63,   0}, //  1
  {255, 127,   0}, //  2
  {255, 191,   0}, //  3
  {255, 255,   0}, //  4  yellow
  {191, 255,   0}, //  5
  {127, 255,   0}, //  6
  { 63, 253,   0}, //  7
  {  0, 255,   0}, //  8  green
  {  0, 255,  63}, //  9
  {  0, 255, 127}, // 10
  {  0, 255, 191}, // 11
  {  0, 255, 255}, // 12
  {  0, 191, 255}, // 13
  {  0, 127, 255}, // 14
  {  0,  63, 255}, // 15
  {  0,   0, 255}, // 17
  {127,   0, 255}, // 18
  {191,   0, 255}, // 19
  {255,   0, 255}, // 20
  {255,   0, 191}, // 21
  {255,   0, 127}, // 22
  {255,   0,  63}, // 23
};
  

#endif // ifndef PSK_ENCODER_HPP

#endif  // end-of _RTC_PSK_NEW_HPP