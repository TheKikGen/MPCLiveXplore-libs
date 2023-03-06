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


static bool CtrlShiftMode = false;

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

  //tklog_debug("Controller Set PAD RGB %d => %d %d %d \n", padCt,r,g,b);

  SeqSendRawMidi( TO_CTRL_EXT,SX_LPMK3_LED_RGB_COLOR,sizeof(SX_LPMK3_LED_RGB_COLOR) );

}

// Refresh the pad surface from Force pad cache
void ControllerRefreshPadsFromForceCache() {

    uint8_t msg[3] = {0x90, 0x00, 0x00};

    for ( int i = 0 ; i< 64 ; i++) {

      // Set pad for external controller eventually
      uint8_t padCt = ( ( 7 - i / 8 ) * 10 + 11 + i % 8 );

      msg[1] = padCt ;

      //6 bits xxrgbRGB (EGA)
      // xx xx xx
      // xx xx 11

      // 128 / 4 values
      // R Value 00 to 11
      uint8_t R = Force_PadColorsCache[i].r  ;
      uint8_t G = Force_PadColorsCache[i].g  ;
      uint8_t B = Force_PadColorsCache[i].b  ;

      uint8_t c =   R  << 4 +  G << 2  + B ;

      //tklog_debug("r %d  g  %d  b  %d / C =  %d \n", R,G,B,c);

      msg[2] =  c;
      SeqSendRawMidi( TO_CTRL_EXT,msg,3 );

      //ControllerSetPadColorRGB(padCt, Force_PadColorsCache[i].r, Force_PadColorsCache[i].g,Force_PadColorsCache[i].b);
    }
}


// Mk3 init
int ControllerInitialize() {

  tklog_info("IamForce : Novation Launchpad Mini MK3 implementtion.\n");
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_STDL_MODE, sizeof(SX_LPMK3_STDL_MODE) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_DAW_CLEAR, sizeof(SX_LPMK3_DAW_CLEAR) );
  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK3_PGM_MODE, sizeof(SX_LPMK3_PGM_MODE) );

  ControllerScrollText("IamForce",0,21,COLOR_SEA);

  uint8_t midiMsg[3];
  midiMsg[0]=0x92;
  midiMsg[1]=0x63;
  midiMsg[2]=0x2D;

  SeqSendRawMidi(TO_CTRL_EXT,midiMsg,3);

}

uint8_t GetForcePAdFromControllerPad(uint8_t padCt) {
  // Convert pad to Force pad #
  padCt -=  11;
  uint8_t padL  =  7 - padCt  /  10 ;
  uint8_t padC  =  padCt  %  10 ;
  return  padL * 8 + padC + FORCEPADS_TABLE_IDX_OFFSET;
}


// Controller Led Mapping
int ControllerSetMapButtonLed(snd_seq_event_t *ev) {


    int mapVal = -1 ;

    if      ( ev->data.control.param == FORCE_BT_SHIFT )      ;
    else if ( ev->data.control.param == FORCE_BT_ENCODER )    ;
    else if ( ev->data.control.param == FORCE_BT_MENU )       ;
    else if ( ev->data.control.param == FORCE_BT_PLAY )       ;
    else if ( ev->data.control.param == FORCE_BT_STOP )       ;
    else if ( ev->data.control.param == FORCE_BT_REC )        ;
    else if ( ev->data.control.param == FORCE_BT_MATRIX )     ;
    else if ( ev->data.control.param == FORCE_BT_ARP )        ;
    else if ( ev->data.control.param == FORCE_BT_COPY )       ;
    else if ( ev->data.control.param == FORCE_BT_DELETE )     ;
    else if ( ev->data.control.param == FORCE_BT_TAP_TEMPO )  ;
    else if ( ev->data.control.param == FORCE_BT_SELECT )     ;
    else if ( ev->data.control.param == FORCE_BT_EDIT )       ;
    else if ( ev->data.control.param == FORCE_BT_UNDO )       ;
    else if ( ev->data.control.param == FORCE_BT_STOP_ALL )   ;
    else if ( ev->data.control.param == FORCE_BT_NOTE )       mapVal = CTRL_BT_KEYS;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_1 )   mapVal = CTRL_BT_LAUNCH_1  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_2 )   mapVal = CTRL_BT_LAUNCH_2  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_3 )   mapVal = CTRL_BT_LAUNCH_3  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_4 )   mapVal = CTRL_BT_LAUNCH_4  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_5 )   mapVal = CTRL_BT_LAUNCH_5  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_6 )   mapVal = CTRL_BT_LAUNCH_6  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_7 )   mapVal = CTRL_BT_LAUNCH_7  ;

    else if ( ev->data.control.param == FORCE_BT_LAUNCH )     mapVal = CTRL_BT_SESSION ;
    else if ( ev->data.control.param == FORCE_BT_LOAD )       ;
    else if ( ev->data.control.param == FORCE_BT_SAVE )       ;
    else if ( ev->data.control.param == FORCE_BT_MIXER )      ;
    else if ( ev->data.control.param == FORCE_BT_CLIP )       ;
    else if ( ev->data.control.param == FORCE_BT_PLUS )       ;
    else if ( ev->data.control.param == FORCE_BT_MINUS )      ;
    else if ( ev->data.control.param == FORCE_BT_KNOBS )      ;


    if ( mapVal >=0 ) {
        snd_seq_event_t ev2 = *ev;
        ev2.data.control.param = mapVal;
        SetMidiEventDestination(&ev2, TO_CTRL_EXT );
        SendMidiEvent(&ev2 );
    }

    return mapVal;

}


// An event was received from the Launchpad MK3
bool ControllerEventReceived(snd_seq_event_t *ev) {

  switch (ev->type) {
    case SND_SEQ_EVENT_SYSEX:
      break;

    case SND_SEQ_EVENT_CONTROLLER:
      // Buttons around pads
      if ( ev->data.control.channel == 0 ) {
        int mapVal = -1;

        if       ( ev->data.control.param == CTRL_BT_UP) { // ^
          // UP holded = shitfmode on the Launchpad
          CtrlShiftMode = ev->data.control.value == 0x7F ? true:false;
          //mapVal = FORCE_BT_SHIFT;
          return false;
        }
        else if  ( ev->data.control.param == CTRL_BT_DOWN  ) { // v
          // Shift down = UP
          mapVal = CtrlShiftMode ? FORCE_BT_UP : FORCE_BT_DOWN ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LEFT ) { // <
          mapVal = FORCE_BT_LEFT ;
        }
        else if  ( ev->data.control.param == CTRL_BT_RIGHT ) { // >
          mapVal = FORCE_BT_RIGHT ;
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
          mapVal = FORCE_BT_LAUNCH_4 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_5 ) { // >
          mapVal = FORCE_BT_LAUNCH_5 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_6 ) { // >
          mapVal = FORCE_BT_LAUNCH_6 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_7 ) { // >
          mapVal = FORCE_BT_LAUNCH_7 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_SESSION ) { // >
          mapVal = FORCE_BT_LAUNCH ;
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
      if ( ev->data.note.channel == 0 ) {
        ev->data.note.channel = 9;
        ev->data.note.note = GetForcePAdFromControllerPad(ev->data.note.note);
        SetMidiEventDestination(ev,TO_MPC_PRIVATE );
      }

    case SND_SEQ_EVENT_CHANPRESS:
      if ( ev->data.control.channel == 0 ) {
        ev->data.control.channel = 9;
        ev->data.control.param = GetForcePAdFromControllerPad(ev->data.control.param);
        SetMidiEventDestination(ev,TO_MPC_PRIVATE );
      }
      break;
  }

  return true;
}
