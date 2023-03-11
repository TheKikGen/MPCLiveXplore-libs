/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

LAUNCHPAD MINI MK3 FOR IAMFORCE
BASED ON THE DEFAULT USER CONFIGURATION

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

// ----------------------------------------------------------------------------
// Launchpad MK3 module for IamForce
// ----------------------------------------------------------------------------
#define LP_DEVICE_ID 0x0D
#define LP_SYSEX_HEADER 0xF0,0x00,0x20,0x29,0x02, LP_DEVICE_ID

#define CTRL_BT_UP 0x5B
#define CTRL_BT_DOWN 0x5C
#define CTRL_BT_LEFT 0x5D
#define CTRL_BT_RIGHT 0x5E
#define CTRL_BT_SESSION 0x5F
#define CTRL_BT_DRUMS 0x60
#define CTRL_BT_KEYS 0x61
#define CTRL_BT_USER 0x62
#define CTRL_BT_LOGO 0x63
#define CTRL_BT_LAUNCH_1 0x59
#define CTRL_BT_LAUNCH_2 0x4F
#define CTRL_BT_LAUNCH_3 0x45
#define CTRL_BT_LAUNCH_4 0x3B
#define CTRL_BT_LAUNCH_5 0x31
#define CTRL_BT_LAUNCH_6 0x27
#define CTRL_BT_LAUNCH_7 0x1D
#define CTRL_BT_STOP_SM 0x13

#define CTRL_COLOR_BLUE 0x2D
#define CTRL_COLOR_RED  0x05
#define CTRL_COLOR_RED_LT 0x07
#define CTRL_COLOR_GREEN 0x57
#define CTRL_COLOR_ORANGE 0x09


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


///////////////////////////////////////////////////////////////////////////////
// LaunchPad Mini Mk3 Specifics
///////////////////////////////////////////////////////////////////////////////
// Scroll Text
static void ControllerScrollText(const char *message,uint8_t loop, uint8_t speed, uint32_t rgbColor) {
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
///////////////////////////////////////////////////////////////////////////////
// LPMMK3 Set a pad RGB Colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetPadColorRGB(uint8_t padCt, uint8_t r, uint8_t g, uint8_t b) {

  // 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03, 0x03 (Led Index) ( r) (g)  (b)
  SX_LPMK3_LED_RGB_COLOR[8]  = padCt;
  SX_LPMK3_LED_RGB_COLOR[9]  = r ;
  SX_LPMK3_LED_RGB_COLOR[10] = g ;
  SX_LPMK3_LED_RGB_COLOR[11] = b ;

  SeqSendRawMidi( TO_CTRL_EXT,SX_LPMK3_LED_RGB_COLOR,sizeof(SX_LPMK3_LED_RGB_COLOR) );

}


///////////////////////////////////////////////////////////////////////////////
// Show a line from Force pad color cache (source = Force pad#, Dest = ctrl pad #)
///////////////////////////////////////////////////////////////////////////////
static void ControllerDrawPadsLineFromForceCache(uint8_t SrcForce, uint8_t DestCtrl) {

    if ( DestCtrl > 7 ) return;

    uint8_t pf = SrcForce * 8 ;
    uint8_t pl = ( DestCtrl * 10 ) + 11;

    for ( uint8_t i = 0  ; i <  8 ; i++) {
      ControllerSetPadColorRGB(pl, Force_PadColorsCache[pf].c.r, Force_PadColorsCache[pf].c.g,Force_PadColorsCache[pf].c.b);
      pf++; pl++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Refresh the pad surface from Force pad cache
///////////////////////////////////////////////////////////////////////////////
static void ControllerRefreshPadsFromForceCache() {

    for ( int i = 0 ; i< 64 ; i++) {

      // Set pad for external controller eventually
      uint8_t padCt = ( ( 7 - i / 8 ) * 10 + 11 + i % 8 );
      ControllerSetPadColorRGB(padCt, Force_PadColorsCache[i].c.r, Force_PadColorsCache[i].c.g,Force_PadColorsCache[i].c.b);
    }
}


// Mk3 init
static int ControllerInitialize() {

  tklog_info("IamForce : Novation Launchpad Mini MK3 implementtion.\n");
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_STDL_MODE, sizeof(SX_LPMK3_STDL_MODE) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_DAW_CLEAR, sizeof(SX_LPMK3_DAW_CLEAR) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_PGM_MODE, sizeof(SX_LPMK3_PGM_MODE) );

  ControllerScrollText("IamForce",0,21,COLOR_SEA);

  uint8_t midiMsg[3];
  midiMsg[0]=0x92;
  midiMsg[1]=CTRL_BT_UP;
  midiMsg[2]=0x2D;

  SeqSendRawMidi(TO_CTRL_EXT,midiMsg,3);


}

static uint8_t ControllerGetForcePadNote(uint8_t padCt) {
  // Convert pad to Force pad #
  padCt -=  11;
  uint8_t padL  =  7 - padCt  /  10 ;
  uint8_t padC  =  padCt  %  10 ;
  return  padL * 8 + padC + FORCEPADS_NOTE_OFFSET;
}


// Controller Led Mapping
static void ControllerSetMapButtonLed(snd_seq_event_t *ev) {

    int mapVal = -1 ;
    int mapVal2 = -1;


    if ( ev->data.control.param == FORCE_BT_NOTE )       mapVal = CTRL_BT_KEYS;
    if ( ev->data.control.param == FORCE_BT_STEP_SEQ )   mapVal = CTRL_BT_USER;

    else if ( ev->data.control.param == FORCE_BT_LAUNCH )     mapVal = CTRL_BT_SESSION ;
    else if ( ev->data.control.param == FORCE_BT_MASTER )     mapVal = CTRL_BT_SESSION ;


    else if ( ev->data.control.param == FORCE_BT_TAP_TEMPO )  {
          mapVal = CTRL_BT_LOGO;
          mapVal2 = ev->data.control.value == 3 ?  CTRL_COLOR_RED_LT:00 ;
    }
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_1 )   mapVal = CTRL_BT_LAUNCH_1  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_2 )   mapVal = CTRL_BT_LAUNCH_2  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_3 )   mapVal = CTRL_BT_LAUNCH_3  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_4 )   mapVal = CTRL_BT_LAUNCH_4  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_5 )   mapVal = CTRL_BT_LAUNCH_5  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_6 )   mapVal = CTRL_BT_LAUNCH_6  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_7 )   mapVal = CTRL_BT_LAUNCH_7  ;

    else if ( ev->data.control.param == FORCE_BT_RIGHT )  mapVal = CTRL_BT_RIGHT    ;
    else if ( ev->data.control.param == FORCE_BT_LEFT )   mapVal = CTRL_BT_LEFT   ;
    else if ( ev->data.control.param == FORCE_BT_UP )      ;
    else if ( ev->data.control.param == FORCE_BT_DOWN )     mapVal = CTRL_BT_DOWN ;

    else if ( ev->data.control.param == FORCE_BT_MUTE )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_STOP_SM   ;
        CurrentSoloMode = FORCE_SM_MUTE ; // Resynchronize
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_ORANGE ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_SOLO )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_STOP_SM   ;
        CurrentSoloMode = FORCE_SM_SOLO ; // Resynchronize
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_BLUE ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_REC_ARM ) {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_STOP_SM   ;
        CurrentSoloMode = FORCE_SM_REC_ARM ; // Resynchronize
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_RED ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_CLIP_STOP )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_STOP_SM   ;
        CurrentSoloMode = FORCE_SM_CLIP_STOP ; // Resynchronize
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_GREEN ;
      }
    }

    if ( mapVal >=0 ) {
        snd_seq_event_t ev2 = *ev;
        ev2.data.control.param = mapVal;
        if ( mapVal2 >= 0 ) ev2.data.control.value = mapVal2 ;
        else {
          uint8_t colMap[]={0x75,0x01,0X13,0x29,0x57};
          ev2.data.control.value = colMap[ev->data.control.value] ;
        }

        SetMidiEventDestination(&ev2, TO_CTRL_EXT );
        SendMidiEvent(&ev2 );
    }

}

// Refresh Columns pads
static void ControllerRefreshColumnsPads(bool show) {
  if ( show ) {
    // Line 9 = track selection
    // Line 8 = mute modes
    ControllerDrawPadsLineFromForceCache(9,1);
    ControllerDrawPadsLineFromForceCache(8,0);
  }
  else {
    ControllerDrawPadsLineFromForceCache(6,1);
    ControllerDrawPadsLineFromForceCache(7,0);
  }
}

// An event was received from the Launchpad MK3
static bool ControllerEventReceived(snd_seq_event_t *ev) {

  switch (ev->type) {
    case SND_SEQ_EVENT_SYSEX:
      break;

    case SND_SEQ_EVENT_CONTROLLER:
      // Buttons around Launchpad mini pads
      if ( ev->data.control.channel == 0 ) {
        int mapVal = -1;

        if       ( ev->data.control.param == CTRL_BT_UP) { // ^
          // UP holded = shitfmode on the Launchpad
          CtrlShiftMode = ( ev->data.control.value == 0x7F );
          //mapVal = FORCE_BT_SHIFT;

          return false;
        }

        else if  ( ev->data.control.param == CTRL_BT_STOP_SM  ) { // v
          // "Stop Solo Mute" button to manage columns pads

          // Shift is used to launch the 8th line
          if ( CtrlShiftMode )           mapVal = FORCE_BT_LAUNCH_8;
          else {

            ColumnsPadMode = ( ev->data.control.value == 0x7F ) || ColumnsPadModeLocked ;
            if ( ColumnsPadMode ) {
              ControllerRefreshColumnsPads(true);
            }
            else {
              ControllerRefreshColumnsPads(false);
            }
          }
        }

        else if  ( ev->data.control.param == CTRL_BT_DOWN  ) { // v
          // Shift down = UP
          mapVal = CtrlShiftMode ? FORCE_BT_UP : FORCE_BT_DOWN ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LEFT ) { // <

          mapVal = CtrlShiftMode ? FORCE_BT_LEFT : FORCE_BT_COPY  ;
        }
        else if  ( ev->data.control.param == CTRL_BT_RIGHT ) { // >
          mapVal = CtrlShiftMode ? FORCE_BT_RIGHT :FORCE_BT_DELETE   ;
        }
        // Launch buttons
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_1 ) { // >
          mapVal = FORCE_BT_LAUNCH_1 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_2 ) { // >
          mapVal = FORCE_BT_LAUNCH_2 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_3 ) { // >
          mapVal = FORCE_BT_LAUNCH_3 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_4 ) { // >
          if ( ColumnsPadMode  ) {
            // Lock Columns pads mode
            if ( ev->data.control.value !=0 )  ColumnsPadModeLocked = ! ColumnsPadModeLocked;
            return false;
          }
          mapVal = FORCE_BT_LAUNCH_4 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_5 ) { // >
          // Launch 5 generates a FORCE SHIFT FOR columns options setting
          // in columns pads mode
          mapVal = ColumnsPadMode ? FORCE_BT_SHIFT : FORCE_BT_LAUNCH_5 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_6 ) { // >
          // Launch 6 generates a STOP ALL in columns pads mode
          mapVal = ColumnsPadMode ? FORCE_BT_STOP_ALL : FORCE_BT_LAUNCH_6 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_7 ) { // >

          // Laucnh 7 is used to change column pads mode in columns pads mode
          if ( ColumnsPadMode ) {
            if ( ev->data.control.value != 0 ) {
              if ( ++CurrentSoloMode >= FORCE_SM_END ) CurrentSoloMode = 0;
            }
            mapVal = SoloModeButtonMap[CurrentSoloMode];
          }
          else {
            mapVal = FORCE_BT_LAUNCH_7;
          }
        }

        else if  ( ev->data.control.param == CTRL_BT_SESSION ) { // >
          mapVal = CtrlShiftMode ? FORCE_BT_MASTER : FORCE_BT_LAUNCH ;
        }

        else if  ( ev->data.control.param == CTRL_BT_USER ) { // >
          mapVal = FORCE_BT_STEP_SEQ ;
        }

        else if  ( ev->data.control.param == CTRL_BT_KEYS ) { // >
          mapVal = FORCE_BT_NOTE ;
        }

        if ( mapVal >= 0 ) {

            if ( CtrlShiftMode) {


            }

            snd_seq_event_t ev2 = *ev;
            //snd_seq_ev_clear (&ev2);
            // Reroute event to private MPC app port as a MPC button NOTE ON
            ev2.type = SND_SEQ_EVENT_NOTEON;
            ev2.data.note.channel = 0;
            ev2.data.note.note = mapVal;
            ev2.data.note.velocity = ev->data.control.value;
            SetMidiEventDestination(&ev2,TO_MPC_PRIVATE );
            SendMidiEvent(&ev2 );
            return false;

        }

//        dump_event(ev);

      }
      break;

    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
      if ( ev->data.note.channel == 0 ) {
        // Launchpad mini pads
        ev->data.note.note = ControllerGetForcePadNote(ev->data.note.note);
        SetMidiEventDestination(ev,TO_MPC_PRIVATE );
        // If controller column mode, simulate the columns and mutes pads
        if ( ColumnsPadMode) {

            if ( ev->type == SND_SEQ_EVENT_KEYPRESS ) return false;

            uint8_t padFL = (ev->data.note.note - FORCEPADS_NOTE_OFFSET ) / 8 ;
            uint8_t padFC = ( ev->data.note.note - FORCEPADS_NOTE_OFFSET ) % 8 ;

            // Edit current track
            if ( padFL == 0 ) {
              snd_seq_event_t ev2 = *ev;

              ev2.data.note.note = FORCE_BT_EDIT ;
              ev2.data.note.velocity  = ( ev->data.note.velocity > 0 ? 0x7F:00);
              ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC ;
              ev->data.note.velocity = ev2.data.note.velocity;

              if ( ev2.data.note.velocity == 0x7F ) {
                SendMidiEvent(&ev2); // Send Edit Press
                SendMidiEvent(ev); // Send Pad On
                ev->data.note.velocity = 0 ;
                SendMidiEvent(ev); // Send Pad off
              }
              else {
                SendMidiEvent(&ev2); // Send Edit Off
              }
              return false;
            }

            else if ( padFL == 6 ) { // Column pads (on the mini launchpad)
              ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC;
              ev->data.note.velocity == ( ev->data.note.velocity > 0 ? 0x7F:00);

            }
            else if ( padFL == 7 ) { // Mute mode pads (on the mini lLaunchpad)
              ev->data.note.note = FORCE_BT_MUTE_PAD1 + padFC;
              ev->data.note.velocity == ( ev->data.note.velocity > 0 ? 0x7F:00);
            }
        }
        else ev->data.note.channel = 9; // Note Force pad
      }
  }

  return true;
}
