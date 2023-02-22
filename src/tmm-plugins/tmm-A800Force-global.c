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

A800 has :

- 8 buttons (CC on channel 0)
- 9 knobs (CC on channel 0)
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

// -----------------------------------------------------------------------------
// The MidiMapperStart() function is called before branching to MPC entry point
// -----------------------------------------------------------------------------
void MidiMapperStart() {
  tklog_info("Roland A800 global mapping for Akai Force, by The Kikgen Labs - V1-0223\n");

}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called when all ports havebeen initialized
// -----------------------------------------------------------------------------
void MidiMapperSetup() {

}

// -----------------------------------------------------------------------------
// MidiMapper is the main plugin function
// -----------------------------------------------------------------------------
// sender indicates the origin of the midi event :
//
//  FROM_MPC_PUBLIC  :  raw midi flow received from the MPC application
//                      You need to use the buffer and size paramters here.

//  FROM_MPC_PRIVATE :  Alsa SEQ event coming from internal MPC application private port
//
//  FROM_CTRL_MPC    :  Alsa SEQ event coming from MPC hardware surface controller
//
//  FROM_MPC_EXTCTRL :  Alsa SEQ event coming from MPC application port connected
//                      to our external controller if any (port appearing as "TKGL MIDI _(port name)")
//
//  FROM_CTRL_EXT    :  Alsa SEQ event coming from external controller hardware
//
// ev : Alsa SEQ encoded event.
//
// buffer contains the raw midi message received from "sender" (only from public)
// size is the number of raw midi bytes received from sender within buffer
//
// SeqSendRawMidi function can be used to send direct midi message to any port (but public) connected to the router
// orig_snd_rawmidi_write can be used to write ram midi to public port.
//
// Returning false will block any message to be routed to the default destination
// so you can route yourself here, avoiding original message to be sent.
//

bool MidiMapper( uint8_t sender, snd_seq_event_t *ev, uint8_t *buffer, size_t size ) {

    switch (sender) {

      case FROM_CTRL_EXT:

          switch (ev->type) {

      	   case SND_SEQ_EVENT_NOTEON:
           case SND_SEQ_EVENT_KEYPRESS:
            {
              if ( ev->data.note.channel == 9 ) {

                // Capture A800 pads and remap them to LAUNCH 1-8 Force buttons
                int mapVal = -1;
                if       ( ev->data.note.note == 0x24 ) mapVal = 0x38 ; // PAD 1 to LAUNCH1
                else if  ( ev->data.note.note == 0x26 ) mapVal = 0x39 ;
                else if  ( ev->data.note.note == 0x2A ) mapVal = 0x3A ;
                else if  ( ev->data.note.note == 0x2E ) mapVal = 0x3B ;
                else if  ( ev->data.note.note == 0x2B ) mapVal = 0x3C ;
                else if  ( ev->data.note.note == 0x2D ) mapVal = 0x3D ;
                else if  ( ev->data.note.note == 0x33 ) mapVal = 0x3E ;
                else if  ( ev->data.note.note == 0x31 ) mapVal = 0x3F ;

                if ( mapVal > 0) {
                    if ( ev->type == SND_SEQ_EVENT_KEYPRESS ) return false; // remove aftertouch

                    ev->data.note.channel = 0;
                    ev->data.note.note = mapVal;
                    // If velocity is > 0 from A800, we must generate a button on msg
                    if ( ev->data.note.velocity > 0 ) ev->data.note.velocity = 0x7F;

                    // Reroute event to private MPC app port
                    SetMidiEventDestination(ev,TO_MPC_PRIVATE );
                }
                return true; // True = send event
              }
              break;
            }
           case SND_SEQ_EVENT_CONTROLLER:
            {
              if ( ev->data.control.channel == 0 ) {

                  // Map A800 buttons to force buttons
                  int mapVal = -1;
                  // A800 buttons mapped to Force buttons
                  if       ( ev->data.control.param == 0x15 ) mapVal = 0x03 ; // <| to Matrix
                  else if  ( ev->data.control.param == 0x16 ) mapVal = 0x0B ; // << to Mixer
                  else if  ( ev->data.control.param == 0x17 ) mapVal = 0x09 ; // >> to Clip
                  else if  ( ev->data.control.param == 0x18 ) mapVal = 0x01 ; // >| to Knob
                  else if  ( ev->data.control.param == 0x19 ) mapVal = 0x51 ; // Stop to stop
                  else if  ( ev->data.control.param == 0x1A ) mapVal = 0x52 ; // Play to Play
                  else if  ( ev->data.control.param == 0x1B ) mapVal = 0x5F ; // Pause to stop ALL
                  else if  ( ev->data.control.param == 0x1C ) mapVal = 0x49 ; // Rec to Rec

                  if ( mapVal >= 0 ) {
                      // Reroute event to private MPC app port as a MPC button NOTE ON
                      ev->type = SND_SEQ_EVENT_NOTEON;
                      ev->data.note.channel = 0;
                      ev->data.note.note = mapVal;
                      ev->data.note.velocity = ev->data.control.value;
                      SetMidiEventDestination(ev,TO_MPC_PRIVATE );
                  }
                  return true; // True = send event
              }
              break;
            }

          } // end switch ev type

          break;
    } // End Switch sender

    return true;
}
