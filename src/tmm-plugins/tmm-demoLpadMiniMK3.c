/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

PLUGIN TEMPLATE
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

// ----------------------------------------------------------------------------
// Launchpad MK3
// ----------------------------------------------------------------------------
#define LP_DEVICE_ID 0x0D
#define LP_SYSEX_HEADER 0xF0,0x00,0x20,0x29,0x02, LP_DEVICE_ID

const uint8_t SX_DEVICE_INQUIRY[] = { 0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7 };
const uint8_t SX_LPMK3_INQUIRY_REPLY_APP[] = { 0xF0, 0x7E, 0x00, 0x06, 0x02, 0x00, 0x20, 0x29, 0x13, 0x01, 0x00, 0x00 } ;
//const uint8_t SX_LPMK3_PGM_MODE = { 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x00, 0x7F, 0xF7} ;
const uint8_t SX_LPMK3_PGM_MODE[]  = { LP_SYSEX_HEADER, 0x0E, 0x01, 0xF7 } ;
const uint8_t SX_LPMK3_DAW_MODE[]  = { LP_SYSEX_HEADER, 0x10, 0x01, 0xF7 } ;
const uint8_t SX_LPMK3_STDL_MODE[] = { LP_SYSEX_HEADER, 0x10, 0x00, 0xF7 } ;
const uint8_t SX_LPMK3_DAW_CLEAR[] = { LP_SYSEX_HEADER, 0x12, 0x01, 0x00, 0X01, 0xF7 } ;

//F0h 00h 20h 29h 02h 0Dh 07h [<loop> <speed> 0x01 R G B [<text>] ] F7h
const uint8_t SX_LPMK3_TEXT_SCROLL[] = { LP_SYSEX_HEADER, 0x07 };

// 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03, 0x03 (Led Index) ( r) (g)  (b)
uint8_t SX_LPMK3_LED_RGB_COLOR[] = { LP_SYSEX_HEADER, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0xF7 };

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
///////////////////////////////////////////////////////////////////////////////
// LaunchPad Mini Mk3 Specifics
///////////////////////////////////////////////////////////////////////////////
// Scroll Text
void ControllerScrollText(const char *message,uint8_t loop, uint8_t speed, uint32_t rgbColor) {
  uint8_t buffer[128];
  uint8_t *pbuff = buffer;

  memcpy(pbuff,SX_LPMK3_TEXT_SCROLL,sizeof(SX_LPMK3_TEXT_SCROLL));
  pbuff += sizeof(SX_LPMK3_TEXT_SCROLL) ;
  *(pbuff ++ ) = loop;  // loop
  *(pbuff ++ ) = speed; // Speed
  *(pbuff ++ ) = 03;    // RGB color

  RGBcolor_t col = { .v = rgbColor};

  *(pbuff ++ ) = col.c.r; // r
  *(pbuff ++ ) = col.c.g; // g
  *(pbuff ++ ) = col.c.b; // b

  strcpy(pbuff, message);
  pbuff += strlen(message);
  *(pbuff ++ ) = 0xF7;

  SeqSendRawMidi(TO_CTRL_EXT,  buffer, pbuff - buffer );

}

// RGB Colors
void ControllerSetPadColorRGB(uint8_t padCt, uint8_t r, uint8_t g, uint8_t b) {

  // 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03, 0x03 (Led Index) ( r) (g)  (b)
  SX_LPMK3_LED_RGB_COLOR[8]  = padCt;
  SX_LPMK3_LED_RGB_COLOR[9]  = r ;
  SX_LPMK3_LED_RGB_COLOR[10] = g ;
  SX_LPMK3_LED_RGB_COLOR[11] = b ;

  SeqSendRawMidi( TO_CTRL_EXT,SX_LPMK3_LED_RGB_COLOR,sizeof(SX_LPMK3_LED_RGB_COLOR));
}

// Mk3 init
int ControllerInitialize() {

  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_STDL_MODE, sizeof(SX_LPMK3_STDL_MODE) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_DAW_CLEAR, sizeof(SX_LPMK3_DAW_CLEAR) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_PGM_MODE, sizeof(SX_LPMK3_PGM_MODE) );

  ControllerScrollText("MPC Midimap",0,21,COLOR_SEA);

  uint8_t midiMsg[3];
  midiMsg[0]=0x92;
  midiMsg[1]=0x63;
  midiMsg[2]=0x2D;

  SeqSendRawMidi(TO_CTRL_EXT,midiMsg,3);

}

// -----------------------------------------------------------------------------
// The MidiMapperStart() function is called before branching to MPC entry point
// -----------------------------------------------------------------------------
void MidiMapperStart() {
  tklog_info("LaunchPad Mini Midimapper plugin by the Kikgen Labs\n");

}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called right after midi initialization
// -----------------------------------------------------------------------------
void MidiMapperSetup() {

    ControllerInitialize();

}
// -----------------------------------------------------------------------------
// sender :
//  MPC_PRIVATE : Event coming from internal MPC port (private or public)
//  CTRL_MPC    : Event coming from the MPC hardware controller
//  MPC_EXTCTRL : Event coming from MPC application port connected to external controller
//                (port appearing as "TKGL MIDI router Mpc Ctrl")
//  CTRL_EXT    : Event coming from external controller hardware
//
// rp contains Alsa sequencer cli and port used in midi connections.
// buffer contains the raw midi message received from "sender"
// size is the number of bytes received from sender
// maxSize is the maxSize allocated for buffer (size of buffer)
//
// A working buffer can defined as e.g. uint8_t buffer_wk[MIDI_DECODER_SIZE]
// SeqSendRawMidi function can be used to send direct midi message to any port connected to the router
// Returning 0 will block any message to be routed to the default destination


bool MidiMapper( uint8_t sender, snd_seq_event_t *ev, uint8_t *buffer, size_t size ) {

      switch (sender) {

        // Event from PUBLIC MPC application rawmidi port
        case FROM_MPC_PUBLIC:
            tklog_info("Midi Event received from MPC PUBLIC\n");

            break;

        // Event from PRIVATE MPC application rawmidi port
        case FROM_MPC_PRIVATE:
            tklog_info("Midi Event received from MPC PRIVATE\n");

            break;

        // Event from MPC hardware internal controller (always private)
        case FROM_CTRL_MPC:
            tklog_info("Midi Event received from CTRL MPC\n");
            dump_event(ev);
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

      }


      return true;
}
