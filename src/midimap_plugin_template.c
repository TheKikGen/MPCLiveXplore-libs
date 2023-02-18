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

// -----------------------------------------------------------------------------
// The MidiMapperStart() function is called before braching to MPC entry point
// -----------------------------------------------------------------------------
void MidiMapperStart() {


}

// -----------------------------------------------------------------------------
// The MidiMapperSetup() function is called right after midi initialization
// -----------------------------------------------------------------------------
void MidiMapperSetup(TkRouter_t *rp) {


}
// -----------------------------------------------------------------------------
// MidiMapper main function
//
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

ssize_t MidiMapper(uint8_t sender, int evType,TkRouter_t *rp,uint8_t * buffer, ssize_t size, size_t maxSize)
{
    switch (sender) {

      case MPC_PRIVATE:
          // Put code here to capture midi flow from internal midi
          break;

      case CTRL_MPC:
          // Put code here to capture midi flow from MPC controller
          break;

      case MPC_EXTCTRL:
          // Put code here to capture midi flow from MPC App TKGL port
          break;

      case CTRL_EXT:
          // Put code here to capture midi flow from external controller
          break;
    }
    return size;
}
