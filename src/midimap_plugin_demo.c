/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

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

void MidiMapperSetup(TkRouter_t *rp) {
  tklog_info("MidiMapper demo : Setup.\n");

}

ssize_t MidiMapper(uint8_t sender, int evType,TkRouter_t *rp,uint8_t * buffer, ssize_t size, size_t maxSize)
{
    switch (sender) {

      case MPC_PRIVATE:
          //printf("MPC Application midi msg - private/public internal port\n");
          break;

      case CTRL_MPC:


          // This example shows how to link the 1st knob to the 2 knobs on a MPC Live
          tklog_info("MidiMapper demo : knobs 1 linked to kno 2 macro demo.\n");
          if ( buffer[0] == 0xB0 && buffer[1] == 0x10 )  {  // first knob

              uint8_t buffer2[3] = { 0xB0, 0x10, 0x7f}; // Turn event
              uint8_t buffer3[3] = { 0x90, 0x54, 0x7f}; // Simulate touch event
              uint8_t buffer4[3] = { 0x90, 0x54, 0x00}; // Simulate touch event off

              // Send knob  1 value (the knob was already touched)
              buffer2[2] = buffer[2]; // Value
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer2, 3);
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer4, 3); // Untouch 1

              buffer2[1] = 0x11 ; // Knob # 2
              buffer3[1] = 0x55 ;
              buffer4[1] = 0x55 ;

              // It is necessary to "touch" the knob for turn event to be accepted
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer3, 3); // Touch
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer2, 3); // Turn
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer4, 3); // Untouch

              return 0; // Block default route
          }
          break;


      case MPC_EXTCTRL:

          tklog_info("MidiMapper demo : midi event received from MPC Application external controller midi port.\n");
          break;

      case CTRL_EXT:
          tklog_info("MidiMapper demo : midi event received from captured external Controller\n");

          // Capture NOTE ON/OFF
          if ( buffer[0] == 0x90 && buffer[1] == 0x32 )  {

              uint8_t playMsg[3] = { 0x90, 0x52, 0x7f}; // PLAY
              uint8_t stopMsg[3] = { 0x90, 0x51, 0x7f}; // Stop

              static int mpcPlaying = 0;
              if ( buffer[2] != 0 ) { // Note On only

                tklog_info("MidiMapper demo : you pressed the PLAY key\n");

                if ( !mpcPlaying ) {
                  // Send Button PLAY pressed / released
                  SeqSendRawMidi(rp->seq, rp->portPriv,  playMsg, 3);
                  playMsg[2] = 0 ;
                  SeqSendRawMidi(rp->seq, rp->portPriv,  playMsg, 3);
                  mpcPlaying = 1 ;
                  tklog_info("MPC is currently playing\n");

                }
                else {
                  // Send Button STOP pressed / released
                  SeqSendRawMidi(rp->seq, rp->portPriv,  stopMsg, 3);
                  stopMsg[2] = 0 ;
                  SeqSendRawMidi(rp->seq, rp->portPriv,  stopMsg, 3);
                  mpcPlaying = 0 ;
                  tklog_info("MPC was stopped\n");
                }
              }
              return 0; // Do not go to default routing
          }
          break;
    }
    return size;
}
