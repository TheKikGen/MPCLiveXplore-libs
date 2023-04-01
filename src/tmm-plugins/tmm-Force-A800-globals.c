/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

ROLAND A800 MIDI KEYBOARD GLOBAL MODE FOR FORCE
USE DEFAULT A800 FACTORY SETTINGS.

----------------------------------------------------------------------------
ROLAND A800 global for Force
----------------------------------------------------------------------------
A800 has :

- 8 buttons (CC on channel 0)
- 9 knobs (CC on channel 0), absolute values
- 9 faders (CC on channel 0)
- 8 pads (Notes/AFT on channel 9)

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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <alsa/asoundlib.h>

// midimapper defines -----------------------------------------------------------

#include "tkgl_midimapper.h"

// Log utilities ---------------------------------------------------------------

#include "tkgl_logutil.h"



// A800 controls ----------------------------------------------------------------

#define CTRL_BT_FIRST 0x15
#define CTRL_BT_FAST_REVERSE 0X16
#define CTRL_BT_FAST_FORWARD 0X17
#define CTRL_BT_LAST 0X18
#define CTRL_BT_STOP 0X19
#define CTRL_BT_PLAY 0X1A
#define CTRL_BT_PAUSE 0X1B
#define CTRL_BT_REC 0X1C
#define CTRL_BT_B1 0X50
#define CTRL_BT_B2 0X51
#define CTRL_BT_B3 0X52
#define CTRL_BT_B4 0X53
#define CTRL_BT_HOLD 0X40


// Extern globals --------------------------------------------------------------

extern int MPC_Id ;
extern const DeviceInfo_t DeviceInfoBloc[];


// Globals ---------------------------------------------------------------------

// USed by next seq macro
static int CurrentSequence = -1;
static int CurrentMatrixVOffset=0;


// Functions prototypes --------------------------------------------------------

static void ForceMacro_NextSeq(int step);

// -----------------------------------------------------------------------------
// The MidiMapperStart() function is called before branching to MPC entry point
// -----------------------------------------------------------------------------
void MidiMapperStart() {

}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called when all ports havebeen initialized
// -----------------------------------------------------------------------------
void MidiMapperSetup() {
  tklog_info("Roland A800 global mapping for Akai Force, by The Kikgen Labs - V1.1-0223\n");

  if ( DeviceInfoBloc[MPC_Id].productCode != DeviceInfoBloc[MPC_FORCE].productCode  ) {
    tklog_info("This plugin is not compatible with your %s device\n",DeviceInfoBloc[MPC_Id].productString);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Midi mapper events processing
///////////////////////////////////////////////////////////////////////////////
bool MidiMapper( uint8_t sender, snd_seq_event_t *ev, uint8_t *buffer, size_t size ) {

    static int currentSequence = 0;
    static int currentMatrixVOffset=0;
    static int currentTrack = 0 ;
    static uint8_t arpMode = 0;

    switch (sender) {

      // Event from PRIVATE MPC application port
      case FROM_MPC_PRIVATE:

          if (ev->type == SND_SEQ_EVENT_CONTROLLER && ev->data.control.channel == 0 ) {
            // LED to Button
            if       ( ev->data.control.param == 0x76) { // ARP LED
                arpMode =  ev->data.control.value != 0 ? 1 : 0;
            }



          }



          break;

      case FROM_CTRL_EXT:

          switch (ev->type) {

      	   case SND_SEQ_EVENT_NOTEON:
           case SND_SEQ_EVENT_NOTEOFF:
           case SND_SEQ_EVENT_KEYPRESS:
              //
              // if ( ev->data.note.channel == 9 ) {
              //
              //   // Capture A800 pads and remap them to FORCE PADS
              //   // A800 pads  : 99/89 [24-31] [00-7F]
              //   // Force pads : 99/89 [36-75] [00-7F]
              //   //
              //   // The 2 last lines of Force pads are :
              //   //        66 67 68 69   (6A 6B 6C 6D)
              //   //        6E 6F 70 71   (72 73 74 75)
              //   //
              //   int mapVal = -1;
              //   if       ( ev->data.note.note == 0x24 ) mapVal = 0x6E ; // Pad 1
              //   else if  ( ev->data.note.note == 0x26 ) mapVal = 0x6F ;
              //   else if  ( ev->data.note.note == 0x2A ) mapVal = 0x70 ;
              //   else if  ( ev->data.note.note == 0x2E ) mapVal = 0x71 ;
              //   else if  ( ev->data.note.note == 0x2B ) mapVal = 0x66 ; // Pad 4
              //   else if  ( ev->data.note.note == 0x2D ) mapVal = 0x67 ;
              //   else if  ( ev->data.note.note == 0x33 ) mapVal = 0x68 ;
              //   else if  ( ev->data.note.note == 0x31 ) mapVal = 0x69 ;
              //
              //   if ( mapVal > 0) {
              //       ev->data.note.note = mapVal;
              //       // Reroute event to private MPC app port
              //       SetMidiEventDestination(ev,TO_MPC_PRIVATE );
              //   }
              // }
              break;

           case SND_SEQ_EVENT_CONTROLLER:
              if ( ev->data.control.channel == 0 ) {

                 // Map A800 buttons to force buttons
                 // CC 0x15 to 0x1C on midi channel 0 - Value : 0x7F : press, 0 : release.

                int mapVal = -1;


                switch (ev->data.control.param)
                {
                case CTRL_BT_FIRST:
                  if ( ev->data.control.value == 0x7F ) ForceMacro_NextSeq(0);
                  break;

                case CTRL_BT_LAST:
                  if ( ev->data.control.value == 0x7F ) ForceMacro_NextSeq(100);
                  break;

                case CTRL_BT_FAST_REVERSE:
                  if ( ev->data.control.value == 0x7F ) ForceMacro_NextSeq(1);
                  break;

                case CTRL_BT_FAST_FORWARD:
                  if ( ev->data.control.value == 0x7F ) ForceMacro_NextSeq(0);
                  break;

                case CTRL_BT_STOP:
                  mapVal =  FORCE_BT_STOP;
                  break;

                case CTRL_BT_PLAY:
                  mapVal =  FORCE_BT_PLAY;
                  break;

                case CTRL_BT_PAUSE:
                  mapVal =  FORCE_BT_STOP_ALL;
                  break;

                case CTRL_BT_REC:
                  mapVal =  FORCE_BT_REC;
                  break;

                case CTRL_BT_B1:
                  if ( arpMode == ( ev->data.control.param == 0xFF ? 1 : 0 ) ) return false;
                  mapVal = FORCE_BT_ARP ;
                  break;


                default:
                  break;
                }

                if ( mapVal >= 0 ) SendDeviceKeyEvent(mapVal,ev->data.control.value);

                return false;
              }
          } // // End switch ev type

    } // End Switch sender

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// FORCE macros
///////////////////////////////////////////////////////////////////////////////

static void ForceMacro_NextSeq(int step) {

  if ( step == 100 ) CurrentSequence = 7;
  else if (step == 0) CurrentSequence = 0;
  else CurrentSequence += step;

  if ( CurrentSequence > 7  ) {
        CurrentSequence = 7;
        CurrentMatrixVOffset++;
        // Send a Matrix down button
        SendDeviceKeyPress(FORCE_BT_DOWN);
  }
  else if ( CurrentSequence < 0 ) {
    CurrentSequence = 0;
    if ( CurrentMatrixVOffset ) {
        CurrentMatrixVOffset--;
        SendDeviceKeyPress(FORCE_BT_UP); 
    }
  }
//tklog_debug("Current suequence = %d\n",CurrentSequence);
  SendDeviceKeyPress(FORCE_BT_LAUNCH_1 + CurrentSequence); 
}
