/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

--------------------------------------------------------------------------
MIDIMAPPER DEMO PLUGIN
----------------------------------------------------------------------------

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
  tklog_info("Welcome to the MPC Midimapper by the Kikgen Labs\n");

}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called when all ports havebeen initialized
// -----------------------------------------------------------------------------
void MidiMapperSetup() {
  tklog_info("Hello world !\n");
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

      // Event from PUBLIC MPC application rawmidi port
      // RAW MIDI. NO EVENT. USE BUFFER and SIZE
      case FROM_MPC_PUBLIC:
          tklog_info("Midi RAW Event received from MPC PUBLIC\n");

          break;

      // Event from PRIVATE MPC application rawmidi port
      case FROM_MPC_PRIVATE:
          tklog_info("Midi Event received from MPC PRIVATE\n");

          break;

      // Event from MPC hardware internal controller
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
