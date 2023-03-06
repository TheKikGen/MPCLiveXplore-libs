/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

IMAFORCE PLUGIN -- Force Emulation on MPC

-----------------------------------------------------------------------------

  Disclaimer.
  This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/
  or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
  NON COMMERCIAL - PERSONAL USE ONLY : You may not use the material for pure
  commercial closed code solution without the licensor permission.
  You are free to copy and redistribute the material in any medium or format,
  adapt, transform, and build upon the material.
  You must give appropriate credit, a link to the github site
  https://github.com/TheKikGen/USBMidiKliK4x4 , provide a link to the license,
  and indicate if changes were made. You may do so in any reasonable manner,
  but not in any way that suggests the licensor endorses you or your use.
  You may not apply legal terms or technological measures that legally restrict
  others from doing anything the license permits.
  You do not have to comply with the license for elements of the material
  in the public domain or where your use is permitted by an applicable exception
  or limitation.
  No warranties are given. The license may not give you all of the permissions
  necessary for your intended use.  This program is distributed in the hope that
  it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#define _GNU_SOURCE
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>

// midimapper defines -----------------------------------------------------------

#include "tkgl_midimapper.h"

// Log utilities ---------------------------------------------------------------

#include "tkgl_logutil.h"

// -----------------------------------------------------------------------------

// Product code file and diffrent file path to fake
//#define PRODUCT_CODE_PATH "/sys/firmware/devicetree/base/inmusic,product-code"
#define PRODUCT_COMPATIBLE_PATH "/sys/firmware/devicetree/base/compatible"
#define PRODUCT_COMPATIBLE_STR "inmusic,%sinmusic,az01rockchip,rk3288"

// Power supply faking
#define POWER_SUPPLY_ONLINE_PATH "/sys/class/power_supply/az01-ac-power/online"
#define POWER_SUPPLY_VOLTAGE_NOW_PATH "/sys/class/power_supply/az01-ac-power/voltage_now"
#define POWER_SUPPLY_PRESENT_PATH "/sys/class/power_supply/sbs-3-000b/present"
#define POWER_SUPPLY_STATUS_PATH "/sys/class/power_supply/sbs-3-000b/status"
#define POWER_SUPPLY_CAPACITY_PATH "/sys/class/power_supply/sbs-3-000b/capacity"

#define POWER_SUPPLY_ONLINE "1"
#define POWER_SUPPLY_VOLTAGE_NOW "18608000"
#define POWER_SUPPLY_PRESENT "1"
#define POWER_SUPPLY_STATUS "Full"
#define POWER_SUPPLY_CAPACITY "100"


// Colors R G B (nb . max r g b value is 7f. The bit 8 is always set )
#define COLOR_FIRE       0x7F1919
#define COLOR_TANGERINE  0x7F3419
#define COLOR_APRICOT    0x7F5019
#define COLOR_CANARY     0x7F6E19
#define COLOR_LEMON      0x757F19
#define COLOR_CHARTREUSE 0x5A7F19
#define COLOR_NEON       0x3B7F19
#define COLOR_LIME       0x207F19
#define COLOR_CLOVER     0x197F2E
#define COLOR_SEA        0x197F4C
#define COLOR_MINT       0x197F68
#define COLOR_CYAN       0x197C7F
#define COLOR_SKY        0x195D7F
#define COLOR_AZURE      0x19427F
#define COLOR_MIDNIGHT   0x19277F
#define COLOR_INDIGO     0x3A197F
#define COLOR_VIOLET     0x46197F
#define COLOR_GRAPE      0x60197F
#define COLOR_FUSHIA     0x7F197F
#define COLOR_MAGENTA    0x7F1964
#define COLOR_CORAL      0x7F1949

#define COLOR_WHITE      0x7F7F7F
#define COLOR_BLACK      0x000000
#define COLOR_RED        0x7F0000
#define COLOR_BLUE       0x00007F
#define COLOR_GREEN      0x007F00
#define COLOR_YELLOW     0x007F7F
#define COLOR_MAGENTA2   0x7F007F

// Force buttons
#define FORCE_BT_ENCODER 0x6F
#define FORCE_BT_NAVIGATE 0
#define FORCE_BT_KNOBS 1
#define FORCE_BT_MENU 2
#define FORCE_BT_MATRIX 3
#define FORCE_BT_NOTE 4
#define FORCE_BT_MASTER 5
#define FORCE_BT_CLIP 9
#define FORCE_BT_MIXER 11
#define FORCE_BT_LOAD 35
#define FORCE_BT_SAVE 36
#define FORCE_BT_EDIT 37
#define FORCE_BT_DELETE 38
#define FORCE_BT_SHIFT 0x31
#define FORCE_BT_SELECT 52
#define FORCE_BT_TAP_TEMPO 53
#define FORCE_BT_PLUS 54
#define FORCE_BT_MINUS 55
#define FORCE_BT_LAUNCH_1 56
#define FORCE_BT_LAUNCH_2 57
#define FORCE_BT_LAUNCH_3 58
#define FORCE_BT_LAUNCH_4 59
#define FORCE_BT_LAUNCH_5 60
#define FORCE_BT_LAUNCH_6 61
#define FORCE_BT_LAUNCH_7 62
#define FORCE_BT_LAUNCH_8 63
#define FORCE_BT_UNDO 67
#define FORCE_BT_REC 73
#define FORCE_BT_STOP 81
#define FORCE_BT_PLAY 82
#define FORCE_BT_QLINK1_TOUCH 83
#define FORCE_BT_QLINK2_TOUCH 84
#define FORCE_BT_QLINK3_TOUCH 85
#define FORCE_BT_QLINK4_TOUCH 86
#define FORCE_BT_QLINK5_TOUCH 87
#define FORCE_BT_QLINK6_TOUCH 88
#define FORCE_BT_QLINK7_TOUCH 89
#define FORCE_BT_QLINK8_TOUCH 90
#define FORCE_BT_STOP_ALL 95
#define FORCE_BT_UP 112
#define FORCE_BT_DOWN 113
#define FORCE_BT_LEFT 114
#define FORCE_BT_RIGHT 115
#define FORCE_BT_LAUNCH 116
#define FORCE_BT_STEP_SEQ 117
#define FORCE_BT_ARP 118
#define FORCE_BT_COPY 122
#define FORCE_BT_COLUMN_PAD1 96
#define FORCE_BT_COLUMN_PAD2 97
#define FORCE_BT_COLUMN_PAD3 98
#define FORCE_BT_COLUMN_PAD4 99
#define FORCE_BT_COLUMN_PAD5 100
#define FORCE_BT_COLUMN_PAD6 101
#define FORCE_BT_COLUMN_PAD7 102
#define FORCE_BT_COLUMN_PAD8 103
#define FORCE_BT_MUTE_PAD1 41
#define FORCE_BT_MUTE_PAD2 42
#define FORCE_BT_MUTE_PAD3 43
#define FORCE_BT_MUTE_PAD4 44
#define FORCE_BT_MUTE_PAD5 45
#define FORCE_BT_MUTE_PAD6 46
#define FORCE_BT_MUTE_PAD7 47
#define FORCE_BT_MUTE_PAD8 48
#define FORCE_BT_ASSIGN_A 123
#define FORCE_BT_ASSIGN_B 124
#define FORCE_BT_MUTE 91
#define FORCE_BT_SOLO 92
#define FORCE_BT_REC_ARM 93
#define FORCE_BT_CLIP_STOP 94

// MPC Buttons
#define MPC_BT_QLINK_SELECT 0
#define MPC_BT_ENCODER 0x6F
#define MPC_BT_BANK_A 35
#define MPC_BT_BANK_B 36
#define MPC_BT_BANK_C 37
#define MPC_BT_BANK_D 38
#define MPC_BT_FULL_LEVEL 39
#define MPC_BT_16_LEVEL 40
#define MPC_BT_TRACK MUTE 43
#define MPC_BT_NEXT_SEQ 42
#define MPC_BT_PROG_EDIT 02
#define MPC_BT_SAMPLE EDIT 06
#define MPC_BT_PAD_MIXER 115
#define MPC_BT_CHANNEL_MIXER 116
#define MPC_BT_PROJECT 59
#define MPC_BT_PROGRAM 60
#define MPC_BT_PAD_SCENE 61
#define MPC_BT_PAD PARAM 62
#define MPC_BT_SCREEN_CONTROL 63
#define MPC_BT_MENU 123
#define MPC_BT_BROWSE 50
#define MPC_BT_STEP_SEQ 41
#define MPC_BT_SAMPLER 125
#define MPC_BT_XYFX 126
#define MPC_BT_PAD_PERFORM 127
#define MPC_BT_REC_ARM 74
#define MPC_BT_READ_WRITE 75
#define MPC_BT_MUTE 76
#define MPC_BT_SOLO 77
#define MPC_BT_ERASE 9
#define MPC_BT_UNDO 67
#define MPC_BT_COPY 122
#define MPC_BT_F_KEY 78
#define MPC_BT_TAP_TEMPO 53
#define MPC_BT_MAIN 52
#define MPC_BT_KEY0 109
#define MPC_BT_KEY1 100
#define MPC_BT_KEY2 101
#define MPC_BT_KEY3 102
#define MPC_BT_KEY4 103
#define MPC_BT_KEY5 104
#define MPC_BT_KEY6 105
#define MPC_BT_KEY7 106
#define MPC_BT_KEY8 107
#define MPC_BT_KEY9 108
#define MPC_BT_ENTER 6F
#define MPC_BT_PLUS_MINUS 110
#define MPC_BT_MINUS 55
#define MPC_BT_PLUS 54
#define MPC_BT_NOTE_REPEAT 11
#define MPC_BT_SHIFT 49
#define MPC_BT_UP 56
#define MPC_BT_DOWN 57
#define MPC_BT_LEFT 65
#define MPC_BT_RIGHT 66
#define MPC_BT_CENTER 58
#define MPC_BT_LOCATE_BW 68
#define MPC_BT_LOCATE_FW 69
#define MPC_BT_LOCATE 	70
#define MPC_BT_LOCATE_FBW 71
#define MPC_BT_LOCATE_FFW 72
#define MPC_BT_REC 73
#define MPC_BT_STOP 81
#define MPC_BT_OVERDUB 80
#define MPC_BT_PLAY 82
#define MPC_BT_PLAY_START 83

// Mapping tables index offset
#define MPCPADS_TABLE_IDX_OFFSET 0X24
#define FORCEPADS_TABLE_IDX_OFFSET 0X36

// Rbg color struct
typedef union
{
  uint32_t v;
  struct {
    uint8_t b : 8;
    uint8_t g : 8;
    uint8_t r : 8;
    uint8_t : 8;
  } c;
} RGBcolor_t;

// Pads Color cache captured from sysex events
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Force_MPCPadColor_t;


// Extern globals --------------------------------------------------------------

extern int MPC_Id ;
extern int MPC_Spoofed_Id;
extern const DeviceInfo_t DeviceInfoBloc[];
extern snd_rawmidi_t *raw_outpub;
extern TkRouter_t TkRouter;

// Function prototypes ---------------------------------------------------------

static int GetMPCPadIndexFromForcePadIndex(uint8_t padF);
static void SetPadColor(const uint8_t MPC_Id, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b );
static void SetPadColorRGB(const uint8_t MPC_Id, const uint8_t padNumber, const uint32_t rgbColorValue) ;
static uint8_t GetForcePadNoteFromMPCPadNote(uint8_t padNote) ;
static bool ControllerEventReceived(snd_seq_event_t *ev);
static void SetMPCMapButtonLed(snd_seq_event_t *ev);
static void SetMPCMapButton(snd_seq_event_t *ev) ;


// Globals ---------------------------------------------------------------------


// MPC hardware pads : a totally anarchic numbering!
// Force is ordered from 0x36. Top left
enum MPCPads { MPC_PAD1, MPC_PAD2, MPC_PAD3, MPC_PAD4, MPC_PAD5,MPC_PAD6,MPC_PAD7, MPC_PAD8,
              MPC_PAD9, MPC_PAD10, MPC_PAD11, MPC_PAD12, MPC_PAD13, MPC_PAD14, MPC_PAD15, MPC_PAD16 };

// MPC ---------------------     FORCE------------------
// Press = 99 [pad#] [00-7F]     idem
// Release = 89 [pad#] 00        idem
// AFT A9 [pad#] [00-7F]         idem
// MPC PAd # from left bottom    Force pad# from up left to right bottom
// to up right (hexa) =
// 31 37 33 35                   36 37 38 39 3A 3B 3C 3D
// 30 2F 2D 2B                   3E 3F 40 41 42 43 44 45
// 28 26 2E 2C                   ...
// 25 24 2A 52
// wtf !!
//
// (13)31 (14)37 (15)33 (16)35
// (9) 30 (10)2F (11)2D (12)2B
// (5) 28 (6) 26 (7) 2E (8) 2C
// (1) 25 (2) 24 (3) 2A (4) 52

static const uint8_t MPCPadsTable[]
= { MPC_PAD2, MPC_PAD1, MPC_PAD6,  0xff,   MPC_PAD5,    0xff,  MPC_PAD3, MPC_PAD12,
//    0x24,       0x25,     0x26,    (0x27),   0x28,   (0x29),    0x2A,     0x2B,
    MPC_PAD8, MPC_PAD11, MPC_PAD7, MPC_PAD10, MPC_PAD9, MPC_PAD13, 0xff,  MPC_PAD15,
//    0x2C,       0x2D,     0x2E,     0x2F,     0x30,      0x31,   (0x32)   0x33,
  0xff,   MPC_PAD16, 0xff, MPC_PAD14,
// (0x34), 0x35,    (0x36)   0x37,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,
// 0x38 - 0x51
MPC_PAD4 };
// 0x52

static const uint8_t MPCPadsTable2[]
= {
  0x25,0x24,0x2A,0x52,
  0x28,0x26,0x2E,0x2C,
  0x30,0x2F,0x2D,0x2B,
  0x31,0x37,0x33,0x35
};

// Sysex patterns.
static const uint8_t AkaiSysex[]                  = {0xF0,0x47, 0x7F};
static const uint8_t MPCSysexPadColorFn[]                = {0x65,0x00,0x04};
static const uint8_t IdentityReplySysexHeader[]   = {0xF0,0x7E,0x00,0x06,0x02,0x47};

// To navigate in matrix quadran when MPC spoofing a Force
static int MPC_PadOffsetL = 0;
static int MPC_PadOffsetC = 0;

// Force Matrix pads color cache
static Force_MPCPadColor_t Force_PadColorsCache[256];

// SHIFT Holded mode
// Holding shift will activate the shift mode
static bool ShiftMode=false;

// Midi controller specific ----------------------------------------------------

#include "Iamforce-LPMK3.h"

// Midi controller specific END ----------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Set pad colors
///////////////////////////////////////////////////////////////////////////////
// 2 implementations : call with a 32 bits color int value or with r,g,b values
static void SetPadColor(const uint8_t MPC_Id, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b ) {

  uint8_t sysexBuff[128];
  int p = 0;

  // F0 47 7F [3B] 65 00 04 [Pad #] [R] [G] [B] F7

  memcpy(sysexBuff, AkaiSysex,sizeof(AkaiSysex));
  p += sizeof(AkaiSysex) ;

  // Add the current product id
  sysexBuff[p++] = MPC_Id ;

  // Add the pad color fn and pad number and color
  memcpy(&sysexBuff[p], MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn));
  sysexBuff[p++] = padNumber ;

  // #define COLOR_CORAL      00FF0077

  sysexBuff[p++] = r ;
  sysexBuff[p++] = g ;
  sysexBuff[p++] = b ;

  sysexBuff[p++]  = 0xF7;

  // Send the sysex to the MPC controller
  snd_rawmidi_write(raw_outpub,sysexBuff,p);

}

static void SetPadColorRGB(const uint8_t MPC_Id, const uint8_t padNumber, const uint32_t rgbColorValue) {

  // Colors R G B max value is 7f in SYSEX. So the bit 8 is always set to 0.
  RGBcolor_t col = { .v = rgbColorValue};

  uint8_t r = col.c.r ;
  uint8_t g = col.c.g ;
  uint8_t b = col.c.b;

  SetPadColor(MPC_Id, padNumber, r, g , b );

}

///////////////////////////////////////////////////////////////////////////////
// Get MPC pad # from Force pad relatively to the current quadran
///////////////////////////////////////////////////////////////////////////////
static int GetMPCPadIndexFromForcePadIndex(uint8_t padF) {

  uint8_t padL = padF / 8 ;
  uint8_t padC = padF % 8 ;
  uint8_t padM = 0x7F ;

  // Transpose Force pad to Mpc pad in the 4x4 current quadran
  if ( padL >= MPC_PadOffsetL && padL < MPC_PadOffsetL + 4 ) {
    if ( padC >= MPC_PadOffsetC  && padC < MPC_PadOffsetC + 4 ) {
      padM = (  3 - ( padL - MPC_PadOffsetL  ) ) * 4 + ( padC - MPC_PadOffsetC)  ;
    }
  }

  return padM;
}

///////////////////////////////////////////////////////////////////////////////
// Get Force pad # from Force pad note
///////////////////////////////////////////////////////////////////////////////
static uint8_t GetForcePadNoteFromMPCPadNote(uint8_t padNote) {

uint8_t padM = MPCPadsTable[padNote - MPCPADS_TABLE_IDX_OFFSET ];
uint8_t padL = padM / 4  ;
uint8_t padC = padM % 4  ;

// Compute the Force pad id without offset
uint8_t padF = ( 3 - padL + MPC_PadOffsetL ) * 8 + padC + MPC_PadOffsetC ;

return padF + FORCEPADS_TABLE_IDX_OFFSET;

}

///////////////////////////////////////////////////////////////////////////////
// Set a mapped MPC button LED from a Force button Led Event
///////////////////////////////////////////////////////////////////////////////
static void SetMPCMapButtonLed(snd_seq_event_t *ev) {

  int mapVal = -1 ;

  if      ( ev->data.control.param == FORCE_BT_SHIFT )      mapVal = MPC_BT_SHIFT ;
  //else if ( ev->data.control.param == FORCE_BT_ENCODER )    mapVal = MPC_BT_ENCODER ;
  else if ( ev->data.control.param == FORCE_BT_MENU )       mapVal = MPC_BT_MENU ;
  else if ( ev->data.control.param == FORCE_BT_PLAY )       mapVal = MPC_BT_PLAY;
  else if ( ev->data.control.param == FORCE_BT_STOP )       mapVal = MPC_BT_STOP;
  else if ( ev->data.control.param == FORCE_BT_REC )        mapVal = MPC_BT_REC;
  else if ( ev->data.control.param == FORCE_BT_MATRIX )     mapVal = MPC_BT_MAIN;
  else if ( ev->data.control.param == FORCE_BT_ARP )        mapVal = MPC_BT_NOTE_REPEAT;
  else if ( ev->data.control.param == FORCE_BT_COPY )       mapVal = MPC_BT_COPY;
  else if ( ev->data.control.param == FORCE_BT_DELETE )     mapVal = MPC_BT_COPY;
  else if ( ev->data.control.param == FORCE_BT_TAP_TEMPO )  mapVal = MPC_BT_TAP_TEMPO;
  else if ( ev->data.control.param == FORCE_BT_SELECT )     mapVal = MPC_BT_FULL_LEVEL;
  else if ( ev->data.control.param == FORCE_BT_EDIT )       mapVal = MPC_BT_16_LEVEL;
  else if ( ev->data.control.param == FORCE_BT_UNDO )       mapVal = MPC_BT_UNDO;
  else if ( ev->data.control.param == FORCE_BT_STOP_ALL )   mapVal = MPC_BT_STOP;
  else if ( ev->data.control.param == FORCE_BT_NOTE )       mapVal = MPC_BT_PLAY_START;
  else if ( ev->data.control.param == FORCE_BT_LAUNCH )     mapVal = MPC_BT_PLAY_START;
  else if ( ev->data.control.param == FORCE_BT_LOAD )       mapVal = MPC_BT_MENU;
  else if ( ev->data.control.param == FORCE_BT_SAVE )       mapVal = MPC_BT_OVERDUB;
  else if ( ev->data.control.param == FORCE_BT_MIXER )      mapVal = MPC_BT_BANK_D;
  else if ( ev->data.control.param == FORCE_BT_CLIP )       mapVal = MPC_BT_BANK_C;
  else if ( ev->data.control.param == FORCE_BT_PLUS )       mapVal = MPC_BT_PLUS;
  else if ( ev->data.control.param == FORCE_BT_MINUS )      mapVal = MPC_BT_MINUS;
  else if ( ev->data.control.param == FORCE_BT_KNOBS )      mapVal = MPC_BT_QLINK_SELECT;

  if ( mapVal >=0 ) {
    ev->data.control.param = mapVal;

    SendMidiEvent(ev );
  }

}


///////////////////////////////////////////////////////////////////////////////
// Set a mapped MPC button  to a Force button
///////////////////////////////////////////////////////////////////////////////
static void SetMPCMapButton(snd_seq_event_t *ev) {

    int mapVal = -1 ;
    bool shiftReleaseBefore = false;

    if      ( ev->data.note.note == MPC_BT_SHIFT ) {
        ShiftMode = ( ev->data.note.velocity == 0x7F ? true :false) ;
        mapVal = FORCE_BT_SHIFT ;
    }
    else if (  ev->data.note.note == MPC_BT_ENCODER )      mapVal = FORCE_BT_ENCODER ;
    else if (  ev->data.note.note == MPC_BT_PLAY )         mapVal = FORCE_BT_PLAY;
    else if (  ev->data.note.note == MPC_BT_REC )          mapVal = FORCE_BT_REC;
    else if (  ev->data.note.note == MPC_BT_MAIN )         mapVal = FORCE_BT_MATRIX;
    else if (  ev->data.note.note == MPC_BT_NOTE_REPEAT )  mapVal = FORCE_BT_ARP;
    else if (  ev->data.note.note == MPC_BT_TAP_TEMPO )    mapVal = FORCE_BT_TAP_TEMPO;
    else if (  ev->data.note.note == MPC_BT_FULL_LEVEL )   mapVal = FORCE_BT_SELECT;
    else if (  ev->data.note.note == MPC_BT_16_LEVEL )     mapVal = FORCE_BT_EDIT;
    else if (  ev->data.note.note == MPC_BT_UNDO )         mapVal = FORCE_BT_UNDO;
    else if (  ev->data.note.note == MPC_BT_OVERDUB )      mapVal = FORCE_BT_SAVE;
    else if (  ev->data.note.note == MPC_BT_PLUS )         mapVal = FORCE_BT_PLUS;
    else if (  ev->data.note.note == MPC_BT_MINUS )        mapVal = FORCE_BT_MINUS;
    else if (  ev->data.note.note == MPC_BT_QLINK_SELECT ) mapVal = FORCE_BT_KNOBS;


    // Shiftmode that we must intercept
    else if (  ev->data.note.note == MPC_BT_MENU ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_LOAD : FORCE_BT_MENU ) ;
    }
    else if (  ev->data.note.note == MPC_BT_STOP ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_STOP_ALL : FORCE_BT_STOP);
    }
    else if (  ev->data.note.note == MPC_BT_COPY ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_DELETE : FORCE_BT_COPY );
    }
    else if (  ev->data.note.note == MPC_BT_PLAY_START ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_LAUNCH : FORCE_BT_NOTE );
    }
    else if (  ev->data.note.note == MPC_BT_BANK_A ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_ASSIGN_A : FORCE_BT_MUTE );
    }
    else if (  ev->data.note.note == MPC_BT_BANK_B ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_ASSIGN_B : FORCE_BT_SOLO );
    }
    else if (  ev->data.note.note == MPC_BT_BANK_C ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_CLIP     : FORCE_BT_REC_ARM );
    }
    else if (  ev->data.note.note == MPC_BT_BANK_D ) {
      shiftReleaseBefore = ShiftMode;
      mapVal = ( ShiftMode ? FORCE_BT_MIXER    : FORCE_BT_CLIP_STOP );
    }

    //tklog_debug("Button %0x mapped to %0x (Shiftmode = %s)\n",ev->data.note.note,mapVal,ShiftMode ? "SHIFT MODE":"");

    if ( mapVal >=0 )  {

      ev->data.note.note = mapVal;

      // Check if we must send a SHIFT RELEASE event to avoid conflict with Force internal shift status
      if ( shiftReleaseBefore ) {
        snd_seq_event_t ev2 = *ev;
        ev2.type = SND_SEQ_EVENT_NOTEON;
        ev2.data.note.note = FORCE_BT_SHIFT ;
        ev2.data.note.velocity = 0;
        SendMidiEvent(&ev2 );
      }

      SendMidiEvent(ev );

    }

}

// -----------------------------------------------------------------------------
// The MidiMapperStart() function is called before branching to MPC entry point
// -----------------------------------------------------------------------------
void MidiMapperStart() {

  // Set the spoofed product code to catch product code read and
  // power supply status access.
  // MPC_Spoofed_Id = -1 by default (no spoofing)

  MPC_Spoofed_Id = MPC_FORCE ;
}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called when all ports havebeen initialized
// -----------------------------------------------------------------------------
void MidiMapperSetup() {
  tklog_info("IamForce :  Force emulation on MPC devices, by The Kikgen Labs - VBETA-Feb 2023\n");

  if ( strcmp(DeviceInfoBloc[MPC_Id].productCode,DeviceInfoBloc[MPC_FORCE].productCode) == 0  ) {
    tklog_fatal("You can't emulate a Force on a Force !!\n");
    exit(1);
  }

  ControllerInitialize();

}

// -----------------------------------------------------------------------------
// Midi mapper events processing
// -----------------------------------------------------------------------------

bool MidiMapper( uint8_t sender, snd_seq_event_t *ev, uint8_t *buffer, size_t size ) {

    switch (sender) {

   // Event from PUBLIC MPC application rawmidi port
   // RAW MIDI. NO EVENT. USE BUFFER and SIZE (SYSEX)
   case FROM_MPC_PUBLIC:
      {

       int i = 0;
       //tklog_info("Midi RAW Event received from MPC PUBLIC\n");

       // Akai Sysex :  F0 47 7F [id] (...) F7

       while ( memcmp(&buffer[i],AkaiSysex, sizeof(AkaiSysex) ) == 0  && i < size ) {

          i += sizeof(AkaiSysex);
          // Change Sysed device id
          buffer[i++] = DeviceInfoBloc[MPC_Id].sysexId;

          // PAD COLOR
          // Sysex : set pad color : FN  F0 47 7F [id] -> 65 00 04 [Pad #] [R] [G] [B] F7
          if ( memcmp(&buffer[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
            i += sizeof(MPCSysexPadColorFn) ;

            uint8_t padF = buffer[i];
            buffer[i] = GetMPCPadIndexFromForcePadIndex(buffer[i]);
            i++;

            // Update Force pad color cache
            Force_PadColorsCache[padF].r = buffer[i++];
            Force_PadColorsCache[padF].g = buffer[i++];
            Force_PadColorsCache[padF].b = buffer[i++];

            if ( padF < 64 ) {
              uint8_t padCt = ( ( 7 - padF / 8 ) * 10 + 11 + padF % 8 );
              ControllerSetPadColorRGB(padCt, Force_PadColorsCache[padF].r, Force_PadColorsCache[padF].g,Force_PadColorsCache[padF].b);
            }

          }

          // Reach end of sysex to loop on the next sysex command
          while ( buffer[i++] != 0xF7 ) if ( i>= size ) break;
       }

       //if ( i > 0 ) ControllerRefreshPadsFromForceCache();

       break;
     }
   // Event from PRIVATE MPC application rawmidi port
   case FROM_MPC_PRIVATE:
       //tklog_info("Midi Event received from MPC PRIVATE\n");

       switch (ev->type) {
         case SND_SEQ_EVENT_CONTROLLER:
           // Button Led
           if ( ev->data.control.channel == 0 ) {

             // Map with controller leds. Will send a midi msg to the controller
             ControllerSetMapButtonLed(ev);

             // Map with MPC device leds
             SetMPCMapButtonLed(ev);

             return false;

           }
           break;

        }

       break;

   // Event from MPC hardware internal controller
   case FROM_CTRL_MPC:
       tklog_info("Midi Event received from CTRL MPC\n");

       switch (ev->type ) {
         case SND_SEQ_EVENT_SYSEX:
             // Identity request reply
             if ( memcmp(ev->data.ext.ptr,IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader) ) == 0 ) {
               memcpy(ev->data.ext.ptr + sizeof(IdentityReplySysexHeader),DeviceInfoBloc[MPC_FORCE].sysexIdReply, sizeof(DeviceInfoBloc[MPC_FORCE].sysexIdReply) );
             }

             break;

         // KNOBS TURN FROM MPC
         // B0 [10-31] [7F - n] : Qlinks    B0 64 nn : Main encoder
         case SND_SEQ_EVENT_CONTROLLER:
             if ( ev->data.control.channel == 0  && ev->data.control.param >= 0x10 && ev->data.control.param <= 0x32 ) {
                   if ( ShiftMode && DeviceInfoBloc[MPC_Id].qlinkKnobsCount < 16 )
                     ev->data.control.param +=  DeviceInfoBloc[MPC_Id].qlinkKnobsCount;
             }

             break;

         case SND_SEQ_EVENT_NOTEON:
//         case SND_SEQ_EVENT_NOTEOFF:

             // Buttons on channel 0
             if ( ev->data.note.channel == 0 ) {
                SetMPCMapButton(ev) ;
                return false;
             }
             // Mpc Pads on channel 9
             else if ( ev->data.note.channel == 9 ) {
               ev->data.note.note = GetForcePadNoteFromMPCPadNote(ev->data.note.note);
             }
             break;

         case SND_SEQ_EVENT_CHANPRESS:
             // Mpc Pads on channel 9 (Aftertouch)
             if ( ev->data.control.channel == 9 ) {
               ev->data.control.param = GetForcePadNoteFromMPCPadNote(ev->data.control.param);
             }
             break;

       }
       break;

   // Event from MPC application port mapped with external controller
   // (see --tkgl_ctrlname command line option)
   case FROM_MPC_EXTCTRL:
       tklog_info("Midi Event received from MPC EXCTRL\n");

       break;

   // Event from external controller HARDWARE
   // (see --tkgl_ctrlname command line option)
   case FROM_CTRL_EXT:

       tklog_info("Midi Event received from CTRL EXT\n");
       return ControllerEventReceived(ev);

 }
 return true;

}
