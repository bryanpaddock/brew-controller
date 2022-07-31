#include <Arduino.h>
#include <LCDMenuLib.h>
#include <LiquidCrystal_I2C.h>
#include "main.h"
#include "elements.h"
#include "probes.h"

#define pinAnalogInput 0

// lib config
#define _LCDML_DISP_cfg_button_press_time 150 // button press time in ms
#define _LCDML_DISP_cfg_scrollbar 0           // enable a scrollbar
#define _LCDML_DISP_cfg_cursor 0x7E           // cursor Symbol
#define _LCDML_DISP_cfg_initscreen_time 5000  // time in ms to display the inital screen (unused)

// settings for lcd
#define _LCDML_DISP_cols 16
#define _LCDML_DISP_rows 2

const uint8_t scroll_bar[5][8] = {
    {B10001, B10001, B10001, B10001, B10001, B10001, B10001, B10001}, // scrollbar top
    {B11111, B11111, B10001, B10001, B10001, B10001, B10001, B10001}, // scroll state 1
    {B10001, B10001, B11111, B11111, B10001, B10001, B10001, B10001}, // scroll state 2
    {B10001, B10001, B10001, B10001, B11111, B11111, B10001, B10001}, // scroll state 3
    {B10001, B10001, B10001, B10001, B10001, B10001, B11111, B11111}  // scrollbar bottom
};

byte upArrow[] = {
    B00000,
    B00100,
    B01110,
    B10101,
    B00100,
    B00100,
    B00100,
    B00000};

byte downArrow[] = {
    B00000,
    B00000,
    B00100,
    B00100,
    B10101,
    B01110,
    B00100,
    B00000};

// lcd object
// LiquidCrystal_I2C lcd(0x27);  // Set the LCD I2C address
// LiquidCrystal_I2C lcd(0x27, BACKLIGHT_PIN, POSITIVE);  // Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, _LCDML_DISP_cols, _LCDML_DISP_rows); // Set the LCD I2C address

// *********************************************************************
// LCDML MENU/DISP
// *********************************************************************
// create menu
// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt 4

// LCDML_root        => layer 0
// LCDML_root_X      => layer 1
// LCDML_root_X_X    => layer 2
// LCDML_root_X_X_X  => layer 3
// LCDML_root_... 	 => layer ...

// LCDMenuLib_add(id, group, prev_layer_element, new_element_num, lang_char_array, callback_function)
LCDML_DISP_init(_LCDML_DISP_cnt);
LCDML_DISP_add(0, _LCDML_G1, LCDML_root, 1, "Information", LCDML_FUNC_information);
LCDML_DISP_add(1, _LCDML_G1, LCDML_root, 2, "Settings", LCDML_FUNC_settings);
LCDML_DISP_add(2, _LCDML_G1, LCDML_root, 3, "Start Brew", LCDML_FUNC_brew);
LCDML_DISP_add(3, _LCDML_G1, LCDML_root_3, 1, "Strike", LCDML_FUNC_strike);
LCDML_DISP_add(4, _LCDML_G1, LCDML_root_3, 2, "Boil", LCDML_FUNC_boil);

//   LCDML_DISP_add(4, _LCDML_G1, LCDML_root_3, 2, "Something", LCDML_FUNC);
//   LCDML_DISP_add(5, _LCDML_G1, LCDML_root, 4, "Program", LCDML_FUNC);
//   LCDML_DISP_add(6, _LCDML_G1, LCDML_root_4, 1, "Program 1", LCDML_FUNC);
//   LCDML_DISP_add(7, _LCDML_G1, LCDML_root_4_1, 1, "P1 start", LCDML_FUNC);
//   LCDML_DISP_add(8, _LCDML_G1, LCDML_root_4_1, 2, "Settings", LCDML_FUNC);
//   LCDML_DISP_add(9, _LCDML_G1, LCDML_root_4_1_2, 1, "Warm", LCDML_FUNC);
//   LCDML_DISP_add(10, _LCDML_G1, LCDML_root_4_1_2, 2, "Long", LCDML_FUNC);
//   LCDML_DISP_add(11, _LCDML_G1, LCDML_root_4, 2, "Program 2", LCDML_FUNC_p2);
LCDML_DISP_createMenu(_LCDML_DISP_cnt);

// *********************************************************************
// LCDML BACKEND (core of the menu, do not change here anything yet)
// *********************************************************************
// define backend function
#define _LCDML_BACK_cnt 1 // last backend function id

LCDML_BACK_init(_LCDML_BACK_cnt);
LCDML_BACK_new_timebased_dynamic(0, (20UL), _LCDML_start, LCDML_BACKEND_control);
LCDML_BACK_new_timebased_dynamic(1, (1000UL), _LCDML_stop, LCDML_BACKEND_menu);
LCDML_BACK_create();

// *********************************************************************
// SETUP
// *********************************************************************
void setupMenu()
{
    // LCD Begin
    lcd.init();
    lcd.begin(_LCDML_DISP_cols, _LCDML_DISP_rows);
    lcd.backlight();

    lcd.home(); // go home
    // set special chars for scrollbar
    lcd.createChar(0, (uint8_t *)scroll_bar[0]);
    lcd.createChar(1, (uint8_t *)scroll_bar[1]);
    lcd.createChar(2, (uint8_t *)scroll_bar[2]);
    lcd.createChar(3, (uint8_t *)scroll_bar[3]);
    lcd.createChar(4, (uint8_t *)scroll_bar[4]);

    lcd.createChar(5, upArrow);
    lcd.createChar(6, downArrow);

    lcd.setCursor(0, 0);
    lcd.print(F("booting"));

    // Enable all items with _LCDML_G1
    LCDML_DISP_groupEnable(_LCDML_G1); // enable group 1

    // Enable menu rollover if needed
    LCDML.enRollover();

    // LCDMenu Setup
    LCDML_setup(_LCDML_BACK_cnt);
}

void loopMenu()
{
    LCDML_run(_LCDML_priority);
}

// *********************************************************************
// check some errors - do not change here anything
// *********************************************************************
#if (_LCDML_DISP_rows > _LCDML_DISP_cfg_max_rows)
#error change value of _LCDML_DISP_cfg_max_rows in LCDMenuLib.h
#endif
#if (_LCDML_DISP_cols > _LCDML_DISP_cfg_max_string_length)
#error change value of _LCDML_DISP_cfg_max_string_length in LCDMenuLib.h
#endif

// =====================================================================
//
// CONTROL
//
// =====================================================================
// *********************************************************************
// *********************************************************************
// content:
// (0) Control over serial interface
// (1) Control over one analog input
// (2) Control over 4 - 6 digital input pins (internal pullups enabled)
// (3) Control over encoder (internal pullups enabled)
// (4) Control with Keypad
// (5) Control with an ir remote
// (6) Control with a youstick
// *********************************************************************

#define _LCDML_CONTROL_cfg 1

// therory:
// "#if" is a preprocessor directive and no error, look here:
// (english) https://en.wikipedia.org/wiki/C_preprocessor
// (german)  https://de.wikipedia.org/wiki/C-Pr%C3%A4prozessor

// *********************************************************************
// CONTROL TASK, DO NOT CHANGE
// *********************************************************************
void LCDML_BACK_setup(LCDML_BACKEND_control)
// *********************************************************************
{
    // call setup
    LCDML_CONTROL_setup();
}
// backend loop
boolean LCDML_BACK_loop(LCDML_BACKEND_control)
{
    // call loop
    LCDML_CONTROL_loop();

    // go to next backend function and do not block it
    return true;
}
// backend stop stable
void LCDML_BACK_stable(LCDML_BACKEND_control)
{
}

// *********************************************************************
// *************** (0) CONTROL OVER SERIAL INTERFACE *******************
// *********************************************************************
#if (_LCDML_CONTROL_cfg == 0)
// settings
#define _LCDML_CONTROL_serial_enter 'e'
#define _LCDML_CONTROL_serial_up 'w'
#define _LCDML_CONTROL_serial_down 's'
#define _LCDML_CONTROL_serial_left 'a'
#define _LCDML_CONTROL_serial_right 'd'
#define _LCDML_CONTROL_serial_quit 'q'
// *********************************************************************
// setup
void LCDML_CONTROL_setup()
{
}
// *********************************************************************
// loop
void LCDML_CONTROL_loop()
{
    // check if new serial input is available
    if (Serial.available())
    {
        // read one char from input buffer
        switch (Serial.read())
        {
        case _LCDML_CONTROL_serial_enter:
            LCDML_BUTTON_enter();
            break;
        case _LCDML_CONTROL_serial_up:
            LCDML_BUTTON_up();
            break;
        case _LCDML_CONTROL_serial_down:
            LCDML_BUTTON_down();
            break;
        case _LCDML_CONTROL_serial_left:
            LCDML_BUTTON_left();
            break;
        case _LCDML_CONTROL_serial_right:
            LCDML_BUTTON_right();
            break;
        case _LCDML_CONTROL_serial_quit:
            LCDML_BUTTON_quit();
            break;
        default:
            break;
        }
    }
}
// *********************************************************************
// ******************************* END *********************************
// *********************************************************************

// *********************************************************************
// *************** (1) CONTROL OVER ONE ANALOG PIN *********************
// *********************************************************************
#elif (_LCDML_CONTROL_cfg == 1)
// settings
#define _LCDML_CONTROL_analog_pin pinAnalogInput
// when you did not use a button set the value to zero
#define _LCDML_CONTROL_analog_up_min 500 // Button Up
#define _LCDML_CONTROL_analog_up_max 550
#define _LCDML_CONTROL_analog_down_min 200 // Button Down
#define _LCDML_CONTROL_analog_down_max 250

#define _LCDML_CONTROL_analog_back_min 700 // Button Back
#define _LCDML_CONTROL_analog_back_max 800

#define _LCDML_CONTROL_analog_enter_min 600 // Button Enter
#define _LCDML_CONTROL_analog_enter_max 650

#define _LCDML_CONTROL_analog_left_min 900 // Button Left
#define _LCDML_CONTROL_analog_left_max 950
#define _LCDML_CONTROL_analog_right_min 1000 // Button Right
#define _LCDML_CONTROL_analog_right_max 1024
// *********************************************************************
// setup
void LCDML_CONTROL_setup()
{
}
// *********************************************************************
// loop
void LCDML_CONTROL_loop()
{
    // check debounce timer
    if ((millis() - g_LCDML_DISP_press_time) >= _LCDML_DISP_cfg_button_press_time)
    {
        g_LCDML_DISP_press_time = millis(); // reset debounce timer

        uint16_t value = analogRead(_LCDML_CONTROL_analog_pin); // analogpin for keypad

        if (DEBUG)
        {
            Serial.print(F("analog: "));
            Serial.println(value);
        }

        if (value >= _LCDML_CONTROL_analog_enter_min && value <= _LCDML_CONTROL_analog_enter_max)
        {
            LCDML_BUTTON_enter();
            if (DEBUG)
            {
                Serial.println(F("enter"));
            }
        }
        if (value >= _LCDML_CONTROL_analog_up_min && value <= _LCDML_CONTROL_analog_up_max)
        {
            LCDML_BUTTON_up();
            if (DEBUG)
            {
                Serial.println(F("up"));
            }
        }
        if (value >= _LCDML_CONTROL_analog_down_min && value <= _LCDML_CONTROL_analog_down_max)
        {
            LCDML_BUTTON_down();
            if (DEBUG)
            {
                Serial.println(F("down"));
            }
        }
        if (value >= _LCDML_CONTROL_analog_left_min && value <= _LCDML_CONTROL_analog_left_max)
        {
            LCDML_BUTTON_left();
            if (DEBUG)
            {
                Serial.println(F("left"));
            }
        }
        if (value >= _LCDML_CONTROL_analog_right_min && value <= _LCDML_CONTROL_analog_right_max)
        {
            LCDML_BUTTON_right();
            if (DEBUG)
            {
                Serial.println(F("right"));
            }
        }
        if (value >= _LCDML_CONTROL_analog_back_min && value <= _LCDML_CONTROL_analog_back_max)
        {
            LCDML_BUTTON_quit();
            if (DEBUG)
            {
                Serial.println(F("quit"));
            }
        }
    }
}
// *********************************************************************
// ******************************* END *********************************
// *********************************************************************

#else
#error _LCDML_CONTROL_cfg is not defined or not in range
#endif

// =====================================================================
//
// Output function
//
// =====================================================================

/* ******************************************************************** */
void LCDML_lcd_menu_display()
{
    // check if menu needs an update
    if (LCDML_DISP_update())
    {

        // init vars
        uint8_t n_max = (LCDML.getChilds() >= _LCDML_DISP_rows) ? _LCDML_DISP_rows : (LCDML.getChilds());
        uint8_t scrollbar_min = 0;
        uint8_t scrollbar_max = LCDML.getChilds();
        uint8_t scrollbar_cur_pos = LCDML.getCursorPosAbs();
        uint8_t scroll_pos = ((1. * n_max * _LCDML_DISP_rows) / (scrollbar_max - 1) * scrollbar_cur_pos);

        // update content
        if (LCDML_DISP_update_content())
        {
            // clear menu
            LCDML_lcd_menu_clear();

            // display rows
            for (uint8_t n = 0; n < n_max; n++)
            {
                // set cursor
                lcd.setCursor(1, n);
                // set content
                // with content id you can add special content to your static menu or replace the content
                // the content_id contains the id wich is set on main tab for a menuitem
                switch (LCDML.content_id[n])
                {
                    // case 0:
                    //	lcd.print("special"); // or datetime or other things
                    //	break;

                default: // static content
                    lcd.print(LCDML.content[n]);
                    break;
                }
            }
        }

        // update cursor and scrollbar
        if (LCDML_DISP_update_cursor())
        {

            // display rows
            for (uint8_t n = 0; n < n_max; n++)
            {
                // set cursor
                lcd.setCursor(0, n);

                // set cursor char
                if (n == LCDML.getCursorPos())
                {
                    lcd.write(_LCDML_DISP_cfg_cursor);
                }
                else
                {
                    lcd.write(' ');
                }

                // delete or reset scrollbar
                if (_LCDML_DISP_cfg_scrollbar == 1)
                {
                    if (scrollbar_max > n_max)
                    {
                        lcd.setCursor((_LCDML_DISP_cols - 1), n);
                        lcd.write((uint8_t)0);
                    }
                    else
                    {
                        lcd.setCursor((_LCDML_DISP_cols - 1), n);
                        lcd.print(' ');
                    }
                }
            }

            // display scrollbar
            if (_LCDML_DISP_cfg_scrollbar == 1)
            {
                if (scrollbar_max > n_max)
                {
                    // set scroll position
                    if (scrollbar_cur_pos == scrollbar_min)
                    {
                        // min pos
                        lcd.setCursor((_LCDML_DISP_cols - 1), 0);
                        lcd.write((uint8_t)1);
                    }
                    else if (scrollbar_cur_pos == (scrollbar_max - 1))
                    {
                        // max pos
                        lcd.setCursor((_LCDML_DISP_cols - 1), (n_max - 1));
                        lcd.write((uint8_t)4);
                    }
                    else
                    {
                        // between
                        lcd.setCursor((_LCDML_DISP_cols - 1), scroll_pos / n_max);
                        lcd.write((uint8_t)(scroll_pos % n_max) + 1);
                    }
                }
            }
        }
    }
    // reinit some vars
    LCDML_DISP_update_end();
}

// lcd clear
void LCDML_lcd_menu_clear()
{
    lcd.clear();
    lcd.setCursor(0, 0);
}

// *********************************************************************
// MENU DEFINITIONS
// *********************************************************************

// *********************************************************************
// INFO
// *********************************************************************

void LCDML_DISP_setup(LCDML_FUNC_information)
{
    lcd.setCursor(0, 0);
    lcd.print(F("Brooklyn Brewery"));

    lcd.setCursor(0, 1);
    lcd.print(version);
}

void LCDML_DISP_loop(LCDML_FUNC_information)
{
    if (LCDML_BUTTON_checkAny())
    {
        LCDML_DISP_funcend();
    }
}

void LCDML_DISP_loop_end(LCDML_FUNC_information)
{
}

// *********************************************************************
// SETTINGS
// *********************************************************************

void LCDML_DISP_setup(LCDML_FUNC_settings)
{
    // setup function
    lcd.setCursor(0, 0);
    lcd.print(F("something goes here"));
    lcd.setCursor(0, 1);
    lcd.print(F("Press any key to close"));
}

void LCDML_DISP_loop(LCDML_FUNC_settings)
{
    if (LCDML_BUTTON_checkAny())
    {
        LCDML_DISP_funcend();
    }
}

void LCDML_DISP_loop_end(LCDML_FUNC_settings)
{
}

// *********************************************************************
// BREW
// *********************************************************************

void LCDML_DISP_setup(LCDML_FUNC_brew) {}
void LCDML_DISP_loop(LCDML_FUNC_brew) {}
void LCDML_DISP_loop_end(LCDML_FUNC_brew) {}

// *********************************************************************
// STRIKE
// *********************************************************************

void LCDML_DISP_setup(LCDML_FUNC_strike)
{
    enableStrike();
}

void LCDML_DISP_loop(LCDML_FUNC_strike)
{
    // loop function, can be run in a loop when LCDML_DISP_triggerMenu(xx) is set
    // the quit button works in every DISP function without any checks; it starts the loop_end function

    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(getStrikeTemp()) + (char)223 + "C          ");
    lcd.setCursor(0, 1);
    lcd.print("Set: " + String(getStrikeSetpoint()) + (char)223 + "C");

    lcd.setCursor(15, 0);
    lcd.write(5);

    lcd.setCursor(15, 1);
    lcd.write(6);

    if (LCDML_BUTTON_checkUp())
    {
        setStrikeSetpoint(getStrikeSetpoint() + 1);
        LCDML_BUTTON_resetUp();
    }

    if (LCDML_BUTTON_checkDown())
    {
        setStrikeSetpoint(getStrikeSetpoint() - 1);
        LCDML_BUTTON_resetDown();
    }

    if (LCDML_BUTTON_checkAny())
    {
        LCDML_DISP_funcend();
    }
}

void LCDML_DISP_loop_end(LCDML_FUNC_strike)
{
    disableStrike();
}

// *********************************************************************
// BOIL
// *********************************************************************

void LCDML_DISP_setup(LCDML_FUNC_boil)
{
    enableBoil();
}

void LCDML_DISP_loop(LCDML_FUNC_boil)
{
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(getBoilTemp()) + (char)223 + "C            ");
    lcd.setCursor(0, 1);
    lcd.print("Power: " + String(getBoilDutyCycle()) + " %      ");

    lcd.setCursor(15, 0);
    lcd.write(5);

    lcd.setCursor(15, 1);
    lcd.write(6);

    if (LCDML_BUTTON_checkUp())
    {
        setBoilDutyCycle(getBoilDutyCycle() + 1);
        LCDML_BUTTON_resetUp();
    }

    if (LCDML_BUTTON_checkDown())
    {
        setBoilDutyCycle(getBoilDutyCycle() - 1);
        LCDML_BUTTON_resetDown();
    }

    if (LCDML_BUTTON_checkAny())
    {
        LCDML_DISP_funcend();
    }
}

void LCDML_DISP_loop_end(LCDML_FUNC_boil)
{
    disableBoil();
}
