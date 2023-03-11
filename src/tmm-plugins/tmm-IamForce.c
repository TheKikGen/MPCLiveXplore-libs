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

// MPC Quadran OFFSETS
#define QUADRAN_1 0
#define QUADRAN_2 4
#define QUADRAN_3 32
#define QUADRAN_4 36


// Force columns solo modes
enum ForceSoloModes  {  FORCE_SM_MUTE, FORCE_SM_SOLO, FORCE_SM_REC_ARM, FORCE_SM_CLIP_STOP, FORCE_SM_END };

// Extern globals --------------------------------------------------------------

extern int MPC_Id ;
extern int MPC_Spoofed_Id;
extern const DeviceInfo_t DeviceInfoBloc[];
extern snd_rawmidi_t *raw_outpub;
extern TkRouter_t TkRouter;

// Globals ---------------------------------------------------------------------

// Sysex patterns.
static const uint8_t AkaiSysex[]                  = {0xF0,0x47, 0x7F};
static const uint8_t MPCSysexPadColorFn[]         = {0x65,0x00,0x04};
static const uint8_t IdentityReplySysexHeader[]   = {0xF0,0x7E,0x00,0x06,0x02,0x47};

// To navigate in matrix quadran when MPC spoofing a Force
// 1 quadran is 4 MPC pads
//  Q1 Q2
//  Q3 Q4
static int MPCPadQuadran = QUADRAN_3;

// Force Matrix pads color cache - 10 lines of 8 pads
static RGBcolor_t Force_PadColorsCache[8*10];

// SHIFT Holded mode
// Holding shift will activate the shift mode
static bool ShiftMode=false;

// Shift mode within controller
static bool CtrlShiftMode = false;

// Column pads Mode
static bool ColumnsPadMode = false;
static bool ColumnsPadModeLocked = false;

// Current Solo Mode
static int CurrentSoloMode = FORCE_SM_SOLO;
// Solo mode buttons map : same order as ForceSoloModes enum
static const uint8_t SoloModeButtonMap [] = { FORCE_BT_MUTE,FORCE_BT_SOLO, FORCE_BT_REC_ARM, FORCE_BT_CLIP_STOP };

// shitfmode for knobs (especially for LIve/One having only 4 knobs)
static bool KnobShiftMode = false;
static bool KnobTouch = false;

// Function prototypes ---------------------------------------------------------


static void ForceSetPadColor(const uint8_t MPC_Id, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b );
static void ForceSetPadColorRGB(const uint8_t MPC_Id, const uint8_t padNumber, const uint32_t rgbColorValue) ;

static int MPCGetPadIndex(uint8_t padF);
static uint8_t MPCGetForcePadNote(uint8_t padNote) ;
static void MPCSetMapButtonLed(snd_seq_event_t *ev);
static void MPCSetMapButton(snd_seq_event_t *ev) ;

static bool ControllerEventReceived(snd_seq_event_t *ev);
static int ControllerInitialize();

// Midi controller specific ----------------------------------------------------
// Include here your own controller implementation

#include "Iamforce-LPMK3.h"

// Midi controller specific END ------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Set a Force pad color
///////////////////////////////////////////////////////////////////////////////
// 2 implementations : call with a 32 bits color int value or with r,g,b values
static void ForceSetPadColor(const uint8_t MPC_Id, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b ) {

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

  sysexBuff[p++]  = 0xF7; // End of SYSEX

  // Send the sysex to the MPC controller
  snd_rawmidi_write(raw_outpub,sysexBuff,p);

}

static void ForceSetPadColorRGB(const uint8_t MPC_Id, const uint8_t padNumber, const uint32_t rgbColorValue) {

  // Colors R G B max value is 7f in SYSEX. So the bit 8 is always set to 0.
  RGBcolor_t col = { .v = rgbColorValue};

  uint8_t r = col.c.r ;
  uint8_t g = col.c.g ;
  uint8_t b = col.c.b;

  ForceSetPadColor(MPC_Id, padNumber, r, g , b );

}

///////////////////////////////////////////////////////////////////////////////
// Get MPC pad # from a Force pad index relatively to the current quadran
///////////////////////////////////////////////////////////////////////////////
static int MPCGetPadIndex(uint8_t padF) {

  uint8_t padL = padF / 8 ;
  uint8_t padC = padF % 8 ;
  uint8_t padM = 0x7F ;

  uint8_t PadOffsetL = MPCPadQuadran / 8;
  uint8_t PadOffsetC = MPCPadQuadran % 8;

  // Transpose Force pad to Mpc pad in the 4x4 current quadran
  if ( padL >= PadOffsetL && padL < PadOffsetL + 4 ) {
    if ( padC >= PadOffsetC  && padC < PadOffsetC + 4 ) {
      padM = (  3 - ( padL - PadOffsetL  ) ) * 4 + ( padC - PadOffsetC)  ;
    }
  }

  return padM;
}

///////////////////////////////////////////////////////////////////////////////
// Get Force pad # note from MPC pad note
///////////////////////////////////////////////////////////////////////////////
static uint8_t MPCGetForcePadNote(uint8_t MPCPadNote) {

  // MPC Pads numbering is totally anarchic
  // So it is preferable to use a switch case rather than a mapping table in an array.
  // Forces pads are numbered from top left to bottom right (starting 0)
  // MPC Pad start at bottom left to top right
  // We add the the Quadran offtet to place the 8x8 MPC pads in the Force 8x8 matrix

  int mapVal = -1;

  switch (MPCPadNote) {
    case MPC_PAD_1: mapVal = 24 ; break;
    case MPC_PAD_2: mapVal = 25 ; break;
    case MPC_PAD_3: mapVal = 26 ; break;
    case MPC_PAD_4: mapVal = 27 ; break;
    case MPC_PAD_5: mapVal = 16 ; break;
    case MPC_PAD_6: mapVal = 17 ; break;
    case MPC_PAD_7: mapVal = 18 ; break;
    case MPC_PAD_8: mapVal = 19 ; break;
    case MPC_PAD_9: mapVal = 8  ; break;
    case MPC_PAD_10: mapVal = 9 ; break;
    case MPC_PAD_11: mapVal = 10 ; break;
    case MPC_PAD_12: mapVal = 11 ; break;
    case MPC_PAD_13: mapVal = 0 ;  break;
    case MPC_PAD_14: mapVal = 1 ;  break;
    case MPC_PAD_15: mapVal = 2 ;  break;
    case MPC_PAD_16: mapVal = 3 ;  break;
  }

  if (mapVal >= 0 ) {
    mapVal += FORCEPADS_NOTE_OFFSET + MPCPadQuadran;
  }
  else mapVal = 0x7F; // a default value if unknow pad #

  return mapVal;

}

///////////////////////////////////////////////////////////////////////////////
// Set a mapped MPC button LED from a Force button Led Event
///////////////////////////////////////////////////////////////////////////////
static void MPCSetMapButtonLed(snd_seq_event_t *ev) {

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
  else if ( ev->data.control.param == FORCE_BT_MUTE )       mapVal = MPC_BT_BANK_A;
  else if ( ev->data.control.param == FORCE_BT_ASSIGN_A )   mapVal = MPC_BT_BANK_A;
  else if ( ev->data.control.param == FORCE_BT_ASSIGN_B )   mapVal = MPC_BT_BANK_B;
  else if ( ev->data.control.param == FORCE_BT_SOLO     )   mapVal = MPC_BT_BANK_B;


  if ( mapVal >=0 ) {
    ev->data.control.param = mapVal;

    SendMidiEvent(ev );
  }

}


///////////////////////////////////////////////////////////////////////////////
// Set a mapped MPC button  to a Force button
///////////////////////////////////////////////////////////////////////////////
static void MPCSetMapButton(snd_seq_event_t *ev) {

    int mapVal = -1 ;
    bool shiftReleaseBefore = false;

    if      ( ev->data.note.note == MPC_BT_SHIFT ) {
        ShiftMode = ( ev->data.note.velocity == 0x7F ) ;
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
    else if (  ev->data.note.note == MPC_BT_QLINK_SELECT ) {

      if ( KnobTouch && ev->data.note.velocity == 0x7F ) KnobShiftMode = !KnobShiftMode ;
      tklog_debug("Knob touch = %s  KnobShiftMode %s\n",KnobTouch ? "True":"False",KnobShiftMode ? "True":"False");

      // Set LED
      // snd_seq_event_t ev2;
      // ev2.type = SND_SEQ_EVENT_CONTROLLER;
      // ev2.data.control.channel = 0 ;
      // ev2.data.control.param = MPC_BT_QLINK_SELECT ;
      // ev2.data.control.value = KnobShiftMode ? 1:0;
      // SetMidiEventDestination(&ev2,TO_CTRL_MPC_PRIVATE);
      // SendMidiEvent(&ev2 );

      if ( ! KnobShiftMode ) mapVal = FORCE_BT_KNOBS;

    }
    else if (  ev->data.note.note >= MPC_BT_QLINK1_TOUCH && ev->data.note.note <= MPC_BT_QLINK16_TOUCH ) {
      mapVal = ev->data.note.note - 1 ;
      KnobTouch = ( ev->data.note.velocity == 0x7F ) ;

      tklog_debug("Knob touch = %s \n",KnobTouch ? "True":"False");

      if (  DeviceInfoBloc[MPC_Id].qlinkKnobsCount == 4 ) {
        if ( KnobShiftMode ) mapVal += 4;
      }
      else KnobShiftMode = false;
    }

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
// The MidiMapperSetup() function is called when all ports initialized
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

//       while ( ( buffer[i] == 0xF0 && buffer[i+1] == 0x47 memcmp(&buffer[i],AkaiSysex, sizeof(AkaiSysex) ) == 0  && i < size ) {
        while ( buffer[i] == 0xF0 && buffer[i+1] == 0x47  && i < size ) {

          i += sizeof(AkaiSysex);
          // Change Sysex device id
          buffer[i++] = DeviceInfoBloc[MPC_Id].sysexId;

          // PAD COLOR
          // Sysex : set pad color : FN  F0 47 7F [id] -> 65 00 04 [Pad #] [R] [G] [B] F7
          if ( memcmp(&buffer[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
            i += sizeof(MPCSysexPadColorFn) ;

            uint8_t padF = buffer[i];
            uint8_t padFL = padF / 8;
            uint8_t padFC = padF % 8;

            buffer[i] = MPCGetPadIndex(buffer[i]);
            i++;

            // Update Force pad color cache
            Force_PadColorsCache[padF].c.r = buffer[i++];
            Force_PadColorsCache[padF].c.g = buffer[i++];
            Force_PadColorsCache[padF].c.b = buffer[i++];

            if ( padF < 64 ) {
              uint8_t padCt = ( ( 7 - padF / 8 ) * 10 + 11 + padF % 8 );
              ControllerSetPadColorRGB(padCt, Force_PadColorsCache[padF].c.r, Force_PadColorsCache[padF].c.g,Force_PadColorsCache[padF].c.b);

            }

            //else {
              // Force pads from 64 are status pad (and not notes pads)
              //if ( CtrlShiftMode) ControllerRefreshColumnsPads(true);

              // if ( !ShiftMode ) {
              //   // Get the current track - Line 8
              //   if ( padFL == 8 && Force_PadColorsCache[padF].v == COLOR_RED ) {
              //       uint8_t currentTrack = 1;
              //       currentTrack = padFC + 1 ;
              //       tklog_debug("Current track = %d\n", currentTrack);
              //   }
              //   // Solo & rec modes line
              //   else if (  padFL == 9  ) {
              //     tklog_debug("Solo modes column %d color %0x \n", padFC,Force_PadColorsCache[padF].v);
              //   }
              // }
              // else {
              //
              // }
              //if ( CtrlShiftMode) ControllerRefreshColumnsPads(true);
            //}

          }

          // Reach end of sysex to loop on the next sysex command
          while ( buffer[i++] != 0xF7 ) if ( i>= size ) break;
       }

       if ( ColumnsPadMode) ControllerRefreshColumnsPads(true);

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
             MPCSetMapButtonLed(ev);

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
             if ( ev->data.control.channel == 0  && ev->data.control.param >= 0x10 && ev->data.control.param <= 0x31 ) {

                   if ( KnobShiftMode && DeviceInfoBloc[MPC_Id].qlinkKnobsCount == 4 ) ev->data.control.param +=  4 ;
                   else if ( ev->data.control.param > 0x17 ) return false;
             }

             break;

         case SND_SEQ_EVENT_NOTEON:
         case SND_SEQ_EVENT_NOTEOFF:
         case SND_SEQ_EVENT_KEYPRESS:
             // Buttons on channel 0
             if ( ev->type != SND_SEQ_EVENT_KEYPRESS && ev->data.note.channel == 0 ) {
                MPCSetMapButton(ev) ;
                return false;
             }

             // Mpc Pads on channel 9
             if ( ev->data.note.channel == 9 ) {
               ev->data.note.note = MPCGetForcePadNote(ev->data.note.note);
             }

             break;

       }
       break;

   // Event from MPC application port mapped with external controller

   case FROM_MPC_EXTCTRL:
       tklog_debug("Midi Event received from MPC EXCTRL\n");

       break;

   // Event from external controller HARDWARE

   case FROM_CTRL_EXT:
       tklog_debug("Midi Event received from CTRL EXT\n");
       return ControllerEventReceived(ev);
 }
 return true;

}
