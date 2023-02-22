/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MPCMAPPER  LD_PRELOAD library.
This "low-level" library allows you to hijack the MPC/Force application to add
your own midi mapping to input and output midi messages.

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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <libgen.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

// mpcmapper defines -----------------------------------------------------------

#include "tkgl_mpcmapper.h"

// Log utilities ---------------------------------------------------------------

#include "tkgl_logutil.h"

// Function prototypes ---------------------------------------------------------

static void ShowBufferHexDump(const uint8_t* data, ssize_t sz, uint8_t nl);
static ssize_t AllMpcProcessRead( uint8_t *buffer, size_t maxSize, ssize_t size, bool isSysex );

int AllMpc_MapPadFromDevice(TkRouter_t *rp,uint8_t * buffer, ssize_t size, size_t maxSize) ;
int AllMpc_MapButtonFromDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size , size_t maxSize);
int AllMpc_MapEncoderFromDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize );
int AllMpc_MapLedToDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize );
int AllMpc_MapSxPadColorToDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize );
void Mpc_DrawPadLineFromForceCache(TkRouter_t *rp, uint8_t forcePadL, uint8_t forcePadC, uint8_t mpcPadL) ;
static void Mpc_ShowForceMatrixQuadran(TkRouter_t *rp, uint8_t forcePadL, uint8_t forcePadC);

// Globals ---------------------------------------------------------------------

// Ports and clients used by mpcmapper and router thread
TkRouter_t TkRouter = {

    // Mpc surface controller
    .Mpc.card = -1, .Mpc.cli = -1,  .Mpc.portPriv =-1, .Mpc.portPub =-1,

    // Tkgl virtual ports exposing MPC rawmidi app ports
    .Virt.cliPrivOut =-1, .Virt.cliPrivIn = -1, .Virt.cliPubOut = -1,

    // External midi controller
    .Ctrl.card =-1, .Ctrl.cli = -1 , .Ctrl.port = -1,

    // Router ports
    .cli = -1,
    .portPriv = -1, .portCtrl = -1, .portPub = -1,
    .portMpcPriv = -1, .portMpcPub = -1,

    // Seq
    .seq = NULL,
};

// Midi router Thread blocker
pthread_t    MidiRouterThread;

// Raw midi dump flag (for debugging purpose)
static uint8_t rawMidiDumpFlag = 0 ;     // Before transformation
static uint8_t rawMidiDumpPostFlag = 0 ; // After our tranformation


// Config file name
static char *configFileName = NULL;
static uint8_t doMappingFlag = 0 ; // 1 if we have a valid mapping


// Buttons and controls Mapping tables
// SHIFT values have bit 7 set

static int map_ButtonsLeds[MAPPING_TABLE_SIZE];
static int map_ButtonsLeds_Inv[MAPPING_TABLE_SIZE]; // Inverted table

// Custom controller
static int mapCtrl_ButtonsLeds[MAPPING_TABLE_SIZE];
static int mapCtrl_ButtonsLeds_Inv[MAPPING_TABLE_SIZE]; // Inverted table

// To navigate in matrix quadran when MPC spoofing a Force
static int MPC_PadOffsetL = 0;
static int MPC_PadOffsetC = 0;

// Force Matric pads color cache
static Force_MPCPadColor_t Force_PadColorsCache[256];

// // End user virtual port name
// static char *user_virtual_portname = NULL;
//
// // End user virtual port handles
// static snd_rawmidi_t *rawvirt_user_in    = NULL ;
// static snd_rawmidi_t *rawvirt_user_out   = NULL ;

// SHIFT Holded mode
// Holding shift will activate the shift mode
static bool ShiftHoldedMode=false;

// Columns modes in Force simulated on a MPC
static int MPC_ForceColumnMode = -1 ;

// Our MPC product id (index in the table)
static int MPC_OriginalId = -1;
// The one used
static int MPC_Id = -1;
// and the spoofed one,
static int MPC_SpoofedId = -1;

// Internal product code file handler to change on the fly when the file will be opened
// That avoids all binding stuff in shell
static int product_code_file_handler = -1 ;
static int product_compatible_file_handler = -1 ;
// Power supply file handlers
static int pws_online_file_handler = -1 ;
static int pws_voltage_file_handler = -1 ;
static FILE *fpws_voltage_file_handler;

static int pws_present_file_handler = -1 ;
static int pws_status_file_handler = -1 ;
static int pws_capacity_file_handler = -1 ;

// MPC alsa names
static char mpc_midi_private_alsa_name[20];
static char mpc_midi_public_alsa_name[20];

// Our midi controller port name
static char anyctrl_name[128] ;

// Virtual rawmidi pointers to fake the MPC app
static snd_rawmidi_t *rawvirt_inpriv  = NULL;
static snd_rawmidi_t *rawvirt_outpriv = NULL ;
static snd_rawmidi_t *rawvirt_outpub  = NULL ;

// Alsa API hooks declaration
static typeof(&snd_rawmidi_open) orig_snd_rawmidi_open;
static typeof(&snd_rawmidi_close) orig_snd_rawmidi_close;
//static typeof(&snd_rawmidi_read) orig_snd_rawmidi_read;
//static typeof(&snd_rawmidi_write) orig_snd_rawmidi_write;
static typeof(&snd_seq_create_simple_port) orig_snd_seq_create_simple_port;
static typeof(&snd_midi_event_decode) orig_snd_midi_event_decode;
static typeof(&snd_seq_open) orig_snd_seq_open;
static typeof(&snd_seq_close) orig_snd_seq_close;
static typeof(&snd_seq_port_info_set_name) orig_snd_seq_port_info_set_name;
//static typeof(&snd_seq_event_input) orig_snd_seq_event_input;

// Globals used to rename a virtual port and get the client id.  No other way...
static int  snd_seq_virtual_port_rename_flag  = 0;
static char snd_seq_virtual_port_newname [30];
static int  snd_seq_virtual_port_clientid=-1;

// Other more generic APIs
static typeof(&open64) orig_open64;
//static typeof(&read) orig_read;
//static typeof(&open) orig_open;
static typeof(&close) orig_close;

///////////////////////////////////////////////////////////////////////////////
// Match string against a regular expression
///////////////////////////////////////////////////////////////////////////////
int match(const char *string, const char *pattern)
{
    int    status;
    regex_t    re;

    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
        return(0);      /* Report error. */
    }
    status = regexec(&re, string, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
        return(0);      /* Report error. */
    }
    return(1);
}

///////////////////////////////////////////////////////////////////////////////
// Read a key,value pair from our config file (INI file format)
///////////////////////////////////////////////////////////////////////////////
// Can return the full key list in the sectionKeys
// If sectionKeys is NULL, will return the value of the key if matching

static int GetKeyValueFromConfFile(const char * confFileName, const char *sectionName,const char* key, char value[], char sectionKeys[][INI_FILE_KEY_MAX_LEN], size_t keyMaxCount ) {

  FILE *fp = fopen(confFileName, "r");
  if ( fp == NULL) return -1;

  char line [INI_FILE_LINE_MAX_LEN];

  bool   withinMySection = false;
  bool   parseFullSection = (keyMaxCount > 0 );

  int keyIndex = 0;

  char *p = NULL;
  char *v = NULL ;
  int i = 0;


  while (fgets(line, sizeof(line), fp)) {

    // Remove spaces before
    p = line;
    while (isspace (*p)) p++;

    // Empty line or comments
    if ( p[0] == '\0' || p[0] == '#' || p[0] == ';' ) continue;

    // Remove spaces after
    for ( i = strlen (p) - 1; (isspace (p[i]));  i--) ;
    p[i + 1] = '\0';

    // A section ?
    if ( p[0] == '[' )  {

      if ( p[strlen(p) - 1 ] == ']' ) {
        withinMySection = ( strncmp(p + 1,sectionName,strlen(p)-2 ) == 0 ) ;
        //tklog_debug("Section detected.  %s \n",withinMySection? sectionName:"ignored");
        continue;
      }
      else {
        tklog_error("Error in Configuration file : ']' missing : %s. \n",line);
        return -1; // Syntax error

      }

    }


    // Section was already found : read the value of the key
    if ( withinMySection ) {
          //tklog_debug("Configuration file read line in searched section  : %s\n",line);
      // Next section  ? stop
      if ( p[0] == '[' ) break;

      // Search "=" sign
      char * v = strstr(p,"=");
      if ( v == NULL ) {
          tklog_error("Error in Configuration file : '=' missing : %s. \n",line);
          return -1; // Syntax error
      }
      *v = '\0';
      v++;  // V now point on the value

      // Remove spaces after the key name (before =)
      for ( i = strlen (p) - 1; (isspace (p[i]));  i--) ;
      p[i + 1] = '\0';

      // Remove spaces before the value (after =)
      while (isspace (*v)) v++;

      // We need the full section in a char * array
      if ( parseFullSection ) {

        if ( keyIndex < keyMaxCount ) {
          strncpy(&sectionKeys[keyIndex][0],p,INI_FILE_KEY_MAX_LEN - 1);
          keyIndex++;
        }
        else {
          tklog_warn("Maximum of %d keys by section reached for key %s. The key is ignored. \n",keyMaxCount,p);
        }
      }
      else {
        // Check the key value

        if ( strcmp( p,key ) == 0 ) {
          strcpy(value,v);
          keyIndex = 1;
          //fprintf(stdout,"Loaded : %s.\n",line);

          break;
        }
      }
    }
  }
  fclose(fp);
  //fprintf(stdout,"config file closed.\n");


  return keyIndex ;
}

///////////////////////////////////////////////////////////////////////////////
// Load mapping tables from config file
///////////////////////////////////////////////////////////////////////////////
static void LoadMappingFromConfFile(const char * confFileName) {

  // shortcuts
  char *ProductStrShort     = DeviceInfoBloc[MPC_OriginalId].productStringShort;
  char *currProductStrShort = DeviceInfoBloc[MPC_Id].productStringShort;

  // Section name strings
  char btLedMapSectionName[64];
  char btLedSrcSectionName[64];
  char btLedDestSectionName[64];

  // Keys and values
  char srcKeyName[64];
  char destKeyName[64];

  char myKey[64];
  char myValue[64];

  // Array of keys in the section
  char keyNames [MAPPING_TABLE_SIZE][INI_FILE_KEY_MAX_LEN] ;

  int  keysCount = 0;

  // Initialize global mapping tables
  for ( int i = 0 ; i < MAPPING_TABLE_SIZE ; i++ ) {

    map_ButtonsLeds[i] = -1;
    map_ButtonsLeds_Inv[i] = -1;

  }

  if ( confFileName == NULL ) return ;  // No config file ...return

  // Make section names
  sprintf(btLedMapSectionName,"Map_%s_%s", ProductStrShort,currProductStrShort);
  sprintf(btLedSrcSectionName,"%s_Controller",ProductStrShort);
  sprintf(btLedDestSectionName,"%s_Controller",currProductStrShort);


  // Get keys of the mapping section. You need to pass the key max len string corresponding
  // to the size of the string within the array
  keysCount = GetKeyValueFromConfFile(confFileName, btLedMapSectionName,NULL,NULL,keyNames,MAPPING_TABLE_SIZE) ;

  if (keysCount < 0) {
    tklog_error("Configuration file %s read error.\n", confFileName);
    return ;
  }

  doMappingFlag = 1;

  tklog_info("  %d keys found in section %s . \n",keysCount,btLedMapSectionName);

  if ( keysCount <= 0 ) {
    tklog_error("Missing section %s in configuration file %s or syntax error. No mapping set. \n",btLedMapSectionName,confFileName);
    return;
  }

  // Read the Buttons & Leds mapping section entries
  for ( int i = 0 ; i < keysCount ; i++ ) {

    // Buttons & Leds mapping

    // Ignore parameters
    if ( strncmp(keyNames[i],"_p_",3) == 0 ) continue;

    strcpy(srcKeyName, keyNames[i] );

    if (  GetKeyValueFromConfFile(confFileName, btLedMapSectionName,srcKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Value not found for %s key in section[%s].\n",srcKeyName,btLedMapSectionName);
      continue;
    }
    // Mapped button name
    // Check if the reserved keyword "(SHIFT)_" is present
    // Shift mode of a src button
    bool srcShift = false;
    if ( strncmp(srcKeyName,"(SHIFT)_",8) == 0 )  {
        srcShift = true;
        strcpy(srcKeyName, &srcKeyName[8] );
    }

    strcpy(destKeyName,myValue);
    bool destShift = false;
    if ( strncmp(destKeyName,"(SHIFT)_",8) == 0 )  {
        destShift = true;
        strcpy(destKeyName, &destKeyName[8] );
    }

    // Read value in original device section
    if (  GetKeyValueFromConfFile(confFileName, btLedSrcSectionName,srcKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Value not found for %s key in section[%s].\n",srcKeyName,btLedSrcSectionName);
      continue;
    }

    // Save the button value
    int srcButtonValue =  strtol(myValue, NULL, 0);

    // Read value in target device section
    if (  GetKeyValueFromConfFile(confFileName, btLedDestSectionName,destKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Error *** Value not found for %s key in section[%s].\n",destKeyName,btLedDestSectionName);
      continue;
    }

    int destButtonValue = strtol(myValue, NULL, 0);

    if ( srcButtonValue <=127 && destButtonValue <=127 ) {

      // If shift mapping, set the bit 7
      srcButtonValue   = ( srcShift  ? srcButtonValue  + 0x80 : srcButtonValue );
      destButtonValue  = ( destShift ? destButtonValue + 0x80 : destButtonValue );

      map_ButtonsLeds[srcButtonValue]      = destButtonValue;
      map_ButtonsLeds_Inv[destButtonValue] = srcButtonValue;

      tklog_info("  Item %s%s (%d/0x%02X) mapped to %s%s (%d/0x%02X)\n",srcShift?"(SHIFT) ":"",srcKeyName,srcButtonValue,srcButtonValue,destShift?"(SHIFT) ":"",destKeyName,map_ButtonsLeds[srcButtonValue],map_ButtonsLeds[srcButtonValue]);

    }
    else {
      tklog_error("Configuration file Error : values above 127 / 0x7F found. Check sections [%s] %s, [%s] %s.\n",btLedSrcSectionName,srcKeyName,btLedDestSectionName,destKeyName);
      return;
    }

  } // for
}

///////////////////////////////////////////////////////////////////////////////
// Load Custome controller mapping tables from config file
///////////////////////////////////////////////////////////////////////////////
static void LoadCtrlMappingFromConfFile(const char * confFileName) {

  // shortcuts
  char *currProductStrShort = DeviceInfoBloc[MPC_Id].productStringShort;

  // Section name strings
  char ctrlMapSectionName[64];
  char ctrlSrcSectionName[64];
  char ctrlDestSectionName[64];

  // Keys and values
  char srcKeyName[64];
  char destKeyName[64];

  char myKey[64];
  char myValue[64];

  // Array of keys in the section
  char keyNames [MAPPING_TABLE_SIZE][INI_FILE_KEY_MAX_LEN] ;

  int  keysCount = 0;

  // Initialize global mapping tables
  for ( int i = 0 ; i < MAPPING_TABLE_SIZE ; i++ ) {

    mapCtrl_ButtonsLeds[i] = -1;
    mapCtrl_ButtonsLeds_Inv[i] = -1;

  }

  if ( confFileName == NULL ) return ;  // No config file ...return

  // Make section names
  sprintf(ctrlMapSectionName,"Map_%s_%s", CTRL_CUSTOM,currProductStrShort);
  sprintf(ctrlSrcSectionName,"%s_Controller",CTRL_CUSTOM);
  sprintf(ctrlDestSectionName,"%s_Controller",currProductStrShort);


  // Get keys of the mapping section. You need to pass the key max len string corresponding
  // to the size of the string within the array
  keysCount = GetKeyValueFromConfFile(confFileName, ctrlMapSectionName,NULL,NULL,keyNames,MAPPING_TABLE_SIZE) ;

  if (keysCount < 0) {
    tklog_error("Configuration file %s read error.\n", confFileName);
    return ;
  }

  tklog_info("  %d keys found in section %s . \n",keysCount,ctrlMapSectionName);

  if ( keysCount <= 0 ) {
    tklog_error("Missing section %s in configuration file %s or syntax error. No mapping set. \n",ctrlMapSectionName,confFileName);
    return;
  }

  // Read the Buttons & Leds mapping section entries
  for ( int i = 0 ; i < keysCount ; i++ ) {

    // Buttons & Leds mapping

    // Ignore parameters
    if ( strncmp(keyNames[i],"_p_",3) == 0 ) continue;

    strcpy(srcKeyName, keyNames[i] );

    if (  GetKeyValueFromConfFile(confFileName, ctrlMapSectionName,srcKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Value not found for %s key in section[%s].\n",srcKeyName,ctrlMapSectionName);
      continue;
    }
    // Mapped button name
    // Check if the reserved keyword "(SHIFT)_" is present
    // Shift mode of a src button
    bool srcShift = false;
    if ( strncmp(srcKeyName,"(SHIFT)_",8) == 0 )  {
        srcShift = true;
        strcpy(srcKeyName, &srcKeyName[8] );
    }

    strcpy(destKeyName,myValue);
    bool destShift = false;
    if ( strncmp(destKeyName,"(SHIFT)_",8) == 0 )  {
        destShift = true;
        strcpy(destKeyName, &destKeyName[8] );
    }

    // Read value in original device section
    if (  GetKeyValueFromConfFile(confFileName, ctrlSrcSectionName,srcKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Value not found for %s key in section[%s].\n",srcKeyName,ctrlSrcSectionName);
      continue;
    }

    // Save the button value
    int srcButtonValue =  strtol(myValue, NULL, 0);

    // Read value in target device section
    if (  GetKeyValueFromConfFile(confFileName, ctrlDestSectionName,destKeyName,myValue,NULL,0) != 1 ) {
      tklog_error("Error *** Value not found for %s key in section[%s].\n",destKeyName,ctrlDestSectionName);
      continue;
    }

    int destButtonValue = strtol(myValue, NULL, 0);

    if ( srcButtonValue <=127 && destButtonValue <=127 ) {

      // If shift mapping, set the bit 7
      srcButtonValue   = ( srcShift  ? srcButtonValue  + 0x80 : srcButtonValue );
      destButtonValue  = ( destShift ? destButtonValue + 0x80 : destButtonValue );

      mapCtrl_ButtonsLeds[srcButtonValue]      = destButtonValue;
      mapCtrl_ButtonsLeds_Inv[destButtonValue] = srcButtonValue;

      tklog_info("  Item %s%s (%d/0x%02X) mapped to %s%s (%d/0x%02X)\n",srcShift?"(SHIFT) ":"",srcKeyName,srcButtonValue,srcButtonValue,destShift?"(SHIFT) ":"",destKeyName,mapCtrl_ButtonsLeds[srcButtonValue],mapCtrl_ButtonsLeds[srcButtonValue]);

    }
    else {
      tklog_error("Configuration file Error : values above 127 / 0x7F found. Check sections [%s] %s, [%s] %s.\n",ctrlSrcSectionName,srcKeyName,ctrlDestSectionName,destKeyName);
      return;
    }

  } // for
}

///////////////////////////////////////////////////////////////////////////////
// Set pad colors
///////////////////////////////////////////////////////////////////////////////
// 2 implementations : call with a 32 bits color int value or with r,g,b values
void SetPadColor(const uint8_t MPC_Id, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b ) {

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
  snd_rawmidi_write(rawvirt_outpriv,sysexBuff,p);

}

void SetPadColorRGB(const uint8_t MPC_Id, const uint8_t padNumber, const uint32_t rgbColorValue) {

  // Colors R G B max value is 7f in SYSEX. So the bit 8 is always set to 0.
  RGBcolor_t col = { .v = rgbColorValue};

  uint8_t r = col.c.r ;
  uint8_t g = col.c.g ;
  uint8_t b = col.c.b;

  SetPadColor(MPC_Id, padNumber, r, g , b );

}

///////////////////////////////////////////////////////////////////////////////
// Get an ALSA card from a matching regular expression pattern
///////////////////////////////////////////////////////////////////////////////
int GetCardFromShortName(const char *pattern) {
   int card = -1;
   char* shortname = NULL;

   if ( snd_card_next(&card) < 0) return -1;
   while (card >= 0) {
   	  if ( snd_card_get_name(card, &shortname) == 0  ) {
       if (match(shortname,pattern) ) return card;
     }
   	 if ( snd_card_next(&card) < 0) break;
   }

   return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Get an ALSA sequencer client , port and alsa card  from a regexp pattern
///////////////////////////////////////////////////////////////////////////////
// Will return 0 if found,.  if trueName or card are NULL, they are ignored.
int GetSeqClientFromPortName(const char * pattern, char trueName[], int *card, int *clientId, int *portId) {

	if ( pattern == NULL) return -1;
	char port_name[128];
  int c= -1;
  int r= -1;

	snd_seq_t *seq;
	if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		tklog_error("Impossible to open default seq (GetSeqClientFromPortName).\n");
		return -1;
	}

	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(seq, cinfo) >= 0) {
		/* reset query info */
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);
    while (snd_seq_query_next_port(seq, pinfo) >= 0) {
			sprintf(port_name,"%s %s",snd_seq_client_info_get_name(cinfo),snd_seq_port_info_get_name(pinfo));
      if (match(port_name,pattern) ) {
        if ( card != NULL ) {
          c = GetCardFromShortName(snd_seq_client_info_get_name(cinfo));
          if ( c < 0 ) break;
          *card = c;
        }
        *clientId = snd_seq_port_info_get_client(pinfo);
        *portId = snd_seq_port_info_get_port(pinfo);
        if ( trueName != NULL ) strcpy(trueName,snd_seq_port_info_get_name(pinfo));
        //tklog_debug("(hw:%d) Client %d:%d - %s%s\n",c, snd_seq_port_info_get_client(pinfo),snd_seq_port_info_get_port(pinfo),port_name );
        r = 0;
        break;
			}
		}
	}

	snd_seq_close(seq);
	return  r;
}

///////////////////////////////////////////////////////////////////////////////
// Get the last port ALSA sequencer client
///////////////////////////////////////////////////////////////////////////////
int GetSeqClientLast() {

	int r = -1;

	snd_seq_t *seq;
	if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		fprintf(stderr,"*** Error : impossible to open default seq\n");
		return -1;
	}

	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(seq, cinfo) >= 0) {
		/* reset query info */
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(seq, pinfo) >= 0) {
				r = snd_seq_client_info_get_client(cinfo) ;
        //fprintf(stdout,"client %s -- %d port %s %d\n",snd_seq_client_info_get_name(cinfo) ,r, snd_seq_port_info_get_name(pinfo), snd_seq_port_info_get_port(pinfo) );
		}
	}

	snd_seq_close(seq);
	return  r;
}

///////////////////////////////////////////////////////////////////////////////
// ALSA aconnect utility API equivalent
///////////////////////////////////////////////////////////////////////////////
int aconnect(int src_client, int src_port, int dest_client, int dest_port) {
	int queue = 0, convert_time = 0, convert_real = 0, exclusive = 0;
	snd_seq_port_subscribe_t *subs;
	snd_seq_addr_t sender, dest;
	int client;
	char addr[10];

	snd_seq_t *seq;
	if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		tklog_error("Impossible to open default seq\n");
		return -1;
	}

	if ((client = snd_seq_client_id(seq)) < 0) {
		tklog_error("Impossible to get seq client id\n");
		snd_seq_close(seq);
		return - 1;
	}

	/* set client info */
	if (snd_seq_set_client_name(seq, "ALSA Connector") < 0) {
		tklog_error("Set client name failed\n");
		snd_seq_close(seq);
		return -1;
	}

	/* set subscription */
	sprintf(addr,"%d:%d",src_client,src_port);
	if (snd_seq_parse_address(seq, &sender, addr) < 0) {
		snd_seq_close(seq);
		tklog_error("Invalid source address %s\n", addr);
		return -1;
	}

	sprintf(addr,"%d:%d",dest_client,dest_port);
	if (snd_seq_parse_address(seq, &dest, addr) < 0) {
		snd_seq_close(seq);
		tklog_error("Invalid destination address %s\n", addr);
		return -1;
	}

	snd_seq_port_subscribe_alloca(&subs);
	snd_seq_port_subscribe_set_sender(subs, &sender);
	snd_seq_port_subscribe_set_dest(subs, &dest);
	snd_seq_port_subscribe_set_queue(subs, queue);
	snd_seq_port_subscribe_set_exclusive(subs, exclusive);
	snd_seq_port_subscribe_set_time_update(subs, convert_time);
	snd_seq_port_subscribe_set_time_real(subs, convert_real);

	if (snd_seq_get_port_subscription(seq, subs) == 0) {
		snd_seq_close(seq);
    tklog_info("Connection of midi port %d:%d to %d:%d already subscribed\n",src_client,src_port,dest_client,dest_port);
		return -1;
	}

  if (snd_seq_subscribe_port(seq, subs) < 0) {
		snd_seq_close(seq);
    tklog_error("Connection of midi port %d:%d to %d:%d failed !\n",src_client,src_port,dest_client,dest_port);
		return -1;
	}

  tklog_info("Connection of midi port %d:%d to %d:%d successfull\n",src_client,src_port,dest_client,dest_port);


	snd_seq_close(seq);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Get MPC hardware name from sysex id
///////////////////////////////////////////////////////////////////////////////
static int GetIndexOfMPC_Id(uint8_t id){
	for (int i = 0 ; i < _END_MPCID ; i++ )
		if ( DeviceInfoBloc[i].sysexId == id ) return i;
	return -1;
}

const char * GetHwNameFromMPC_Id(uint8_t id){
	int i = GetIndexOfMPC_Id(id);
	if ( i >= 0) return DeviceInfoBloc[i].productString ;
	else return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Clean dump of a midi seq event
///////////////////////////////////////////////////////////////////////////////
static void dump_event(const snd_seq_event_t *ev)
{
	tklog_trace("Source port : %3d:%-3d\n", ev->source.client, ev->source.port);
	switch (ev->type) {
	case SND_SEQ_EVENT_NOTEON:
		tklog_trace("Note on                %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_NOTEOFF:
		tklog_trace("Note off               %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_KEYPRESS:
		tklog_trace("Polyphonic aftertouch  %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_CONTROLLER:
		tklog_trace("Control change         %2d %3d %3d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_PGMCHANGE:
		tklog_trace("Program change         %2d %3d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_CHANPRESS:
		tklog_trace("Channel aftertouch     %2d %3d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_PITCHBEND:
		tklog_trace("Pitch bend             %2d  %6d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_CONTROL14:
		tklog_trace("Control change         %2d %3d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_NONREGPARAM:
		tklog_trace("Non-reg. parameter     %2d %5d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_REGPARAM:
		tklog_trace("Reg. parameter         %2d %5d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_SONGPOS:
		tklog_trace("Song position pointer     %5d\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_SONGSEL:
		tklog_trace("Song select               %3d\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_QFRAME:
		tklog_trace("MTC quarter frame         %02xh\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_TIMESIGN:
		// XXX how is this encoded?
		tklog_trace("SMF time signature        (%#08x)\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_KEYSIGN:
		// XXX how is this encoded?
		tklog_trace("SMF key signature         (%#08x)\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_START:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			tklog_trace("Queue start               %d\n",
			       ev->data.queue.queue);
		else
			tklog_trace("Start\n");
		break;
	case SND_SEQ_EVENT_CONTINUE:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			tklog_trace("Queue continue            %d\n",
			       ev->data.queue.queue);
		else
			tklog_trace("Continue\n");
		break;
	case SND_SEQ_EVENT_STOP:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			tklog_trace("Queue stop                %d\n",
			       ev->data.queue.queue);
		else
			tklog_trace("Stop\n");
		break;
	case SND_SEQ_EVENT_SETPOS_TICK:
		tklog_trace("Set tick queue pos.       %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_SETPOS_TIME:
		tklog_trace("Set rt queue pos.         %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_TEMPO:
		tklog_trace("Set queue tempo           %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_CLOCK:
		tklog_trace("Clock\n");
		break;
	case SND_SEQ_EVENT_TICK:
		tklog_trace("Tick\n");
		break;
	case SND_SEQ_EVENT_QUEUE_SKEW:
		tklog_trace("Queue timer skew          %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_TUNE_REQUEST:
		/* something's fishy here ... */
		tklog_trace("Tuna request\n");
		break;
	case SND_SEQ_EVENT_RESET:
		tklog_trace("Reset\n");
		break;
	case SND_SEQ_EVENT_SENSING:
		tklog_trace("Active Sensing\n");
		break;
	case SND_SEQ_EVENT_CLIENT_START:
		tklog_trace("Client start              %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_CLIENT_EXIT:
		tklog_trace("Client exit               %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_CLIENT_CHANGE:
		tklog_trace("Client changed            %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_PORT_START:
		tklog_trace("Port start                %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_EXIT:
		tklog_trace("Port exit                 %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_CHANGE:
		tklog_trace("Port changed              %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		tklog_trace("Port subscribed           %d:%d -> %d:%d\n",
		       ev->data.connect.sender.client, ev->data.connect.sender.port,
		       ev->data.connect.dest.client, ev->data.connect.dest.port);
		break;
	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		tklog_trace("Port unsubscribed         %d:%d -> %d:%d\n",
		       ev->data.connect.sender.client, ev->data.connect.sender.port,
		       ev->data.connect.dest.client, ev->data.connect.dest.port);
		break;
	case SND_SEQ_EVENT_SYSEX:
		{
			unsigned int i;
			tklog_trace("System exclusive         \n");
      ShowBufferHexDump((uint8_t*)ev->data.ext.ptr, ev->data.ext.len,16 );
			tklog_trace("\n");
		}
		break;
	default:
		tklog_trace("Event type %d\n",  ev->type);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Alsa SEQ raw write to a seq port
///////////////////////////////////////////////////////////////////////////////
// (rawmidi_virt.c snd_rawmidi_virtual_write inspiration !)
int SeqSendRawMidi(snd_seq_t *seqHandle, uint8_t port,  const uint8_t *buffer, size_t size ) {

  static snd_midi_event_t * midiParser = NULL;
  snd_seq_event_t ev;
	ssize_t result = 0;
	ssize_t size1;
	int err;

  // Start the MIDI parser
  if (midiParser == NULL ) {
   if (snd_midi_event_new(MIDI_DECODER_SIZE, &midiParser ) < 0) return -1 ;
   snd_midi_event_init(midiParser);
   snd_midi_event_no_status(midiParser,1); // No running status
  }

  if ( size > MIDI_DECODER_SIZE ) {
    snd_midi_event_free (midiParser);
    snd_midi_event_new (size, &midiParser);
  }

  snd_seq_ev_clear (&ev);

	while (size > 0) {
		size1 = snd_midi_event_encode (midiParser, buffer, size, &ev);
    snd_midi_event_reset_encode (midiParser);

		if (size1 <= 0) break;
		size -= size1;
		result += size1;
		buffer += size1;
		if (ev.type == SND_SEQ_EVENT_NONE) continue;

    snd_seq_ev_set_subs (&ev);
    snd_seq_ev_set_source(&ev, port);
		snd_seq_ev_set_direct (&ev);
		err = snd_seq_event_output (seqHandle, &ev);

		if (err < 0) {
			return result > 0 ? result : err;
		}
	}

	if (result > 0)
  snd_seq_drain_output (seqHandle);

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// Alsa SEQ : get an event as raw midi data
///////////////////////////////////////////////////////////////////////////////
ssize_t SeqReadEventRawMidi(snd_seq_event_t *ev,void *buffer, size_t size) {

    static snd_midi_event_t * midiParser = NULL;
    int r = -1;

    // Start the MIDI parser
    if (midiParser == NULL ) {
     if (snd_midi_event_new(MIDI_DECODER_SIZE, &midiParser ) < 0) return -1;
     snd_midi_event_init(midiParser);
     snd_midi_event_no_status(midiParser,1); // No running status
    }

    if ( size > MIDI_DECODER_SIZE ) {
      snd_midi_event_free (midiParser);
      snd_midi_event_new (size, &midiParser);
    }

    if (ev->type == SND_SEQ_EVENT_SYSEX) {
        // With sysex, we need to forward in the event queue to get all the data
        r = ev->data.ext.len;
        if ( r > size ) {
          tklog_error("Sysex buffer overflow in SeqReadEventRawMidi : %d / %d.\n",r,size);
          r = -1;
        } else memcpy(buffer,ev->data.ext.ptr, r );
    }
    else {
        r = snd_midi_event_decode (midiParser, buffer, size, ev);
        snd_midi_event_reset_encode (midiParser);
    }

    return r;
}

///////////////////////////////////////////////////////////////////////////////
// LaunchPad Mini Mk3 Specifics
///////////////////////////////////////////////////////////////////////////////
// Scroll Text
void ControllerScrollText(TkRouter_t *rp,const char *message,uint8_t loop, uint8_t speed, uint32_t rgbColor) {
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

  SeqSendRawMidi(rp->seq, rp->portCtrl,  buffer, pbuff - buffer );

}

// RGB Colors
void ControllerSetPadColorRGB(TkRouter_t *rp,uint8_t padCt, uint8_t r, uint8_t g, uint8_t b) {

  // 0xF0, 0x00, 0x20, 0x29, 0x02, 0x0D, 0x03, 0x03 (Led Index) ( r) (g)  (b)
  SX_LPMK3_LED_RGB_COLOR[8]  = padCt;
  SX_LPMK3_LED_RGB_COLOR[9]  = r ;
  SX_LPMK3_LED_RGB_COLOR[10] = g ;
  SX_LPMK3_LED_RGB_COLOR[11] = b ;

  SeqSendRawMidi(rp->seq, rp->portCtrl,SX_LPMK3_LED_RGB_COLOR,sizeof(SX_LPMK3_LED_RGB_COLOR));
}

// Mk3 init
int ControllerInitialize(TkRouter_t *rp) {

  SeqSendRawMidi(rp->seq, rp->portCtrl,  SX_LPMK3_STDL_MODE, sizeof(SX_LPMK3_STDL_MODE) );
  SeqSendRawMidi(rp->seq, rp->portCtrl,  SX_LPMK3_DAW_CLEAR, sizeof(SX_LPMK3_DAW_CLEAR) );
  SeqSendRawMidi(rp->seq, rp->portCtrl,  SX_LPMK3_PGM_MODE, sizeof(SX_LPMK3_PGM_MODE) );

  ControllerScrollText(rp,"***IamForce",0,21,COLOR_SEA);

  uint8_t midiMsg[3];
  midiMsg[0]=0x92;
  midiMsg[1]=0x63;
  midiMsg[2]=0x2D;

  SeqSendRawMidi(rp->seq, rp->portCtrl,midiMsg,3);

}

///////////////////////////////////////////////////////////////////////////////
// MIDI EVEN PROCESS AND ROUTE (IN A sub THREAD)
///////////////////////////////////////////////////////////////////////////////
void threadMidiProcessAndRoute(TkRouter_t *rp) {

  snd_seq_event_t *ev;
  uint8_t buffer[MIDI_DECODER_SIZE] ;
  ssize_t size = 0;

  bool padsColorUpdated = false;

  do {
      int r = snd_seq_event_input(rp->seq, &ev) ;
      if (  r  <= 0 )  continue;

      // Get a raw bytes buffer from event (easier to manage)
      size =  SeqReadEventRawMidi(ev,buffer, sizeof(buffer)) ;
      if ( size <= 0 ) continue;

      r = size ;
      if ( rawMidiDumpFlag && r > 0 ) {
        tklog_trace("TKGL_Router dump entry (%d bytes):\n",r);
        ShowBufferHexDump(buffer,r,16);
        tklog_trace("\n");
        //dump_event(ev);
      }


      snd_seq_ev_set_subs(ev);
      snd_seq_ev_set_direct(ev);


      //dump_event(ev);

      // -----------------------------------------------------------------------
      // Events from MPC application - Write Private / Write Public
      // -----------------------------------------------------------------------
      if ( ev->source.client == rp->Virt.cliPrivOut || ev->source.client == rp->Virt.cliPubOut ) {
        switch (ev->type) {
          case SND_SEQ_EVENT_CONTROLLER:
            // Button Led
            if ( ev->data.control.channel == 0 ) {

              if ( doMappingFlag ) {
                // Check if we have a mapping match for external controller (Launchpad)
                if ( rp->Ctrl.cli >= 0 ) {
                  int mapValue = mapCtrl_ButtonsLeds_Inv[buffer[1]];
                  if ( mapValue >= 0 ) {
                    // Replace the value
                    //tklog_debug("Map value found %d \n",mapValue);
                    uint8_t buff2[3]; // Do not modify the original buffer
                    buff2[0] = buffer[0] ;
                    buff2[1] = mapValue;
                    buff2[2] = buffer[2];
                    SeqSendRawMidi(rp->seq, rp->portCtrl,  buff2, sizeof(buff2) );
                  }
                }

                // MPC device
                r = AllMpc_MapLedToDevice( rp, buffer,size,sizeof(buffer) );
              }
            }
            break;
          case SND_SEQ_EVENT_SYSEX:

            // Akai hardware sysex (only the first event)
            if (  memcmp(buffer ,AkaiSysex,sizeof(AkaiSysex)) == 0 ) {

              // Mpc device
              uint8_t *buff2 = buffer + sizeof(AkaiSysex);
              buff2[0] = DeviceInfoBloc[MPC_OriginalId].sysexId;
              buff2++;
              // Sysex : set pad color : FN  F0 47 7F [3B] -> 65 00 04 [Pad #] [R] [G] [B] F7
              if ( memcmp(buff2,MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
                  buff2 += sizeof(MPCSysexPadColorFn) ;


                  AllMpc_MapSxPadColorToDevice(rp, buff2,size, sizeof(buffer) - (buff2 - buffer) );
                  padsColorUpdated = true;
              }
            }
            break;
        }
        // Route to the right hw write port
        if ( r > 0 ) {
          int destPort = -1;
          if ( ev->source.client == rp->Virt.cliPrivOut  ) destPort = rp->portMpcPriv ;
          else if  ( ev->source.client == rp->Virt.cliPubOut ) destPort = rp->portMpcPub ;
          if ( destPort >= 0 ) SeqSendRawMidi(rp->seq, destPort,  buffer, r );
        }
      }

      // -----------------------------------------------------------------------
      // Event from MPC hw device
      // -----------------------------------------------------------------------

      else
      if ( ev->source.client == rp->Mpc.cli ) {
        if ( ev->source.port == rp->Mpc.portPriv ) {
          switch (ev->type) {
            case SND_SEQ_EVENT_SYSEX:
              // substitue sysex identity request by the faked one only on the first packet
              if (  buffer[0] == 0xF0 &&  memcmp(buffer,IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader)) == 0 ) {
                memcpy(buffer + sizeof(IdentityReplySysexHeader),DeviceInfoBloc[MPC_Id].sysexIdReply, sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) );
              }
              break;

            case SND_SEQ_EVENT_CONTROLLER:
              // KNOBS TURN & Encoder (UNMAPPED BECAUSE ARE ALL EQUIVALENT ON ALL DEVICES)
              // B0 [10-31] [7F - n] : Qlinks    B0 64 nn : Main encoder
              if ( ev->data.control.channel == 0 ) {
                if (  buffer[1] == 0x64 ) {
                  ;// Nothing now
                }
                else if ( ( buffer[1] >= 0x10 && buffer[1] <= 0x31 ) ) { //
                  r = AllMpc_MapEncoderFromDevice( rp, buffer,size,sizeof(buffer) );
                }
              }
              break;
            case SND_SEQ_EVENT_NOTEON:
            case SND_SEQ_EVENT_NOTEOFF:
            case SND_SEQ_EVENT_CHANPRESS:
              // Buttons on channel 0
              if ( ev->data.control.channel == 0 && ev->type != SND_SEQ_EVENT_CHANPRESS) {
                r = AllMpc_MapButtonFromDevice( rp, buffer,size,sizeof(buffer) );
              }
              else
              // Mpc Pads on channel 9
              if ( ev->data.control.channel == 9 ) {
                r = AllMpc_MapPadFromDevice( rp, buffer, size, sizeof(buffer) );
              }
              break;
          }
          if ( r > 0 ) SeqSendRawMidi(rp->seq, rp->portPriv,  buffer, r );
        }
      }

      // -----------------------------------------------------------------------
      // Events from external controller
      // -----------------------------------------------------------------------
      else
      if ( ev->source.client == rp->Ctrl.cli && ev->source.port == rp->Ctrl.port) {
        //dump_event(ev);
        switch (ev->type) {
          case SND_SEQ_EVENT_SYSEX:
            break;


          case SND_SEQ_EVENT_CONTROLLER:
            // Buttons around pads
            if ( ev->data.control.channel == 0 ) {

              // Check an eventual mapping
              int mapValue = mapCtrl_ButtonsLeds[buffer[1]];
              if ( mapValue >= 0 ) {
                // Replace the value
                //tklog_debug("Map value found %d \n",mapValue);
                buffer[0] = 0x90 ; // Note On = button
                buffer[1] = mapValue;
                SeqSendRawMidi(rp->seq, rp->portPriv,  buffer, size );
              }
            }
            break;
          case SND_SEQ_EVENT_NOTEON:
          case SND_SEQ_EVENT_NOTEOFF:
          case SND_SEQ_EVENT_CHANPRESS:
            if ( ev->data.control.channel == 0 ) {
              // Convert pad #
              uint8_t padCt = buffer[1] - 11;
              uint8_t padL  =  7 - padCt  /  10 ;
              uint8_t padC  =  padCt  %  10 ;
              uint8_t padF  = padL * 8 + padC + FORCEPADS_TABLE_IDX_OFFSET;

              buffer[0] = ( buffer[0] & 0xF0 ) + 9; // Change midi channel to 9
              buffer[1] = padF;

              // Send to Mpc App private in
              SeqSendRawMidi(rp->seq, rp->portPriv,  buffer, size );
            }
            break;
        }
      }

    if ( rawMidiDumpPostFlag && r > 0 ) {
      tklog_trace("TKGL_Router dump post (%d bytes):\n",r);
      ShowBufferHexDump(buffer,r,16);
      tklog_trace("\n");
      //dump_event(ev);
    }

  } while (snd_seq_event_input_pending(rp->seq, 0) > 0);

  // Take care of our eventual special options pad lines
  if (   padsColorUpdated  && !ShiftHoldedMode && MPC_ForceColumnMode >= 0   ) {
      AllMpc_MapSxPadColorToDevice( rp ,NULL,0,0) ; // Refresh options command
      Mpc_ShowForceMatrixQuadran(rp, MPC_PadOffsetL, MPC_PadOffsetC);
  }

}

///////////////////////////////////////////////////////////////////////////////
// MIDI ROUTER MAIN THREAD
///////////////////////////////////////////////////////////////////////////////
void* threadMidiRouter(void *pdata) {

  int npfd;
  struct pollfd *pfd;

  TkRouter_t * tkr = (TkRouter_t *) pdata;

  ControllerInitialize(tkr);

  // Polling
  npfd = snd_seq_poll_descriptors_count(tkr->seq, POLLIN);
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(tkr->seq, pfd, npfd, POLLIN);

  tklog_info("Midi router thread started. Waiting for events...\n");

  while (1) {
    if (poll(pfd, npfd, 100000) > 0) {
      threadMidiProcessAndRoute(tkr);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Create SEQ router simple port
///////////////////////////////////////////////////////////////////////////////
int  CreateSimpleRouterPort(snd_seq_t *seq, const char* name ) {
  char portname[64];
  int p = -1;

  // Create router ports
  sprintf(portname, "%s %s Private", ROUTER_SEQ_NAME,name);
  if ( (  p = snd_seq_create_simple_port(seq, portname,
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE |
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ |
        SND_SEQ_PORT_CAP_DUPLEX,
        SND_SEQ_PORT_TYPE_HARDWARE | SND_SEQ_PORT_TYPE_MIDI_GENERIC
      ) )  < 0) {
    tklog_fatal("Error creating sequencer port %s.\n",portname);
    exit(1);
  }
  return p;
}

///////////////////////////////////////////////////////////////////////////////
// Show MPCMAPPER HELP
///////////////////////////////////////////////////////////////////////////////
void ShowHelp(void) {

  tklog_info("\n") ;
  tklog_info("--tgkl_help               : Show this help\n") ;
  tklog_info("--tkgl_ctrlname=<name>    : Use external controller containing <name>\n") ;
  tklog_info("--tkgl_iamX               : Emulate MPC X\n") ;
  tklog_info("--tkgl_iamLive            : Emulate MPC Live\n") ;
  tklog_info("--tkgl_iamForce           : Emulate Force\n") ;
  tklog_info("--tkgl_iamOne             : Emulate MPC One\n") ;
  tklog_info("--tkgl_iamLive2           : Emulate MPC Live Mk II\n") ;
  //tklog_info("--tkgl_virtualport=<name> : Create end user virtual port <name>\n") ;
  tklog_info("--tkgl_mididump           : Dump original raw midi flow\n") ;
  tklog_info("--tkgl_mididumpPost       : Dump raw midi flow after transformation\n") ;
  tklog_info("--tkgl_configfile=<name>  : Use configuration file <name>\n") ;
  tklog_info("\n") ;
  exit(0);
}

///////////////////////////////////////////////////////////////////////////////
// Make ld_preload hooks
///////////////////////////////////////////////////////////////////////////////
static void makeLdHooks() {
  // System call hooks
  orig_open64 = dlsym(RTLD_NEXT, "open64");
  //orig_open   = dlsym(RTLD_NEXT, "open");
  orig_close  = dlsym(RTLD_NEXT, "close");
  //orig_read   = dlsym(RTLD_NEXT, "read");

	// Alsa hooks
	orig_snd_rawmidi_open           = dlsym(RTLD_NEXT, "snd_rawmidi_open");
  orig_snd_rawmidi_close          = dlsym(RTLD_NEXT, "snd_rawmidi_close");
	//orig_snd_rawmidi_read           = dlsym(RTLD_NEXT, "snd_rawmidi_read");
	//orig_snd_rawmidi_write          = dlsym(RTLD_NEXT, "snd_rawmidi_write");
	orig_snd_seq_create_simple_port = dlsym(RTLD_NEXT, "snd_seq_create_simple_port");
	orig_snd_midi_event_decode      = dlsym(RTLD_NEXT, "snd_midi_event_decode");
  orig_snd_seq_open               = dlsym(RTLD_NEXT, "snd_seq_open");
  orig_snd_seq_close              = dlsym(RTLD_NEXT, "snd_seq_close");
  orig_snd_seq_port_info_set_name = dlsym(RTLD_NEXT, "snd_seq_port_info_set_name");
  //orig_snd_seq_event_input        = dlsym(RTLD_NEXT, "snd_seq_event_input");

}

///////////////////////////////////////////////////////////////////////////////
// Setup tkgl anyctrl
///////////////////////////////////////////////////////////////////////////////
static void tkgl_init()
{

  makeLdHooks();

  // Read product code
  char product_code[4];
  int fd = open(PRODUCT_CODE_PATH,O_RDONLY);
  read(fd,&product_code,4);

  // Find the id in the product code table
  for (int i = 0 ; i < _END_MPCID; i++) {
    if ( strncmp(DeviceInfoBloc[i].productCode,product_code,4) == 0 ) {
      MPC_OriginalId = i;
      break;
    }
  }
  if ( MPC_OriginalId < 0) {
    tklog_fatal("Error while reading the product-code file\n");
    exit(1);
  }
  tklog_info("Original Product code : %s (%s)\n",DeviceInfoBloc[MPC_OriginalId].productCode,DeviceInfoBloc[MPC_OriginalId].productString);

  if ( MPC_SpoofedId >= 0 ) {
    tklog_info("Product code spoofed to %s (%s)\n",DeviceInfoBloc[MPC_SpoofedId].productCode,DeviceInfoBloc[MPC_SpoofedId].productString);
    MPC_Id = MPC_SpoofedId ;
  } else MPC_Id = MPC_OriginalId ;

  // Fake the power supply ?
  if ( DeviceInfoBloc[MPC_OriginalId].fakePowerSupply ) tklog_info("The power supply will be faked to allow battery mode.\n");

  // read mpc device mapping config file if any
  tklog_info("MPCs controller mapping :\n");
  LoadMappingFromConfFile(configFileName);

  tklog_info("Custom controller mapping :\n");
  LoadCtrlMappingFromConfFile(configFileName);

  // Retrieve MPC midi card info
  if ( GetSeqClientFromPortName(CTRL_MPC_ALL_PRIVATE,NULL,&TkRouter.Mpc.card,&TkRouter.Mpc.cli,&TkRouter.Mpc.portPriv) < 0 ) {
    tklog_fatal("Error : MPC controller card/seq client not found (regex pattern is '%s')\n",CTRL_MPC_ALL_PRIVATE);
    exit(1);
  }
  TkRouter.Mpc.portPub = 0 ; // Public port is 0
	sprintf(mpc_midi_private_alsa_name,"hw:%d,0,%d",TkRouter.Mpc.card,TkRouter.Mpc.portPriv);
	sprintf(mpc_midi_public_alsa_name,"hw:%d,0,%d",TkRouter.Mpc.card,TkRouter.Mpc.portPub);
	tklog_info("MPC controller Private hardware port Alsa name is %s. Sequencer id is %d:%d.\n",mpc_midi_private_alsa_name,TkRouter.Mpc.cli,TkRouter.Mpc.portPriv);
	tklog_info("MPC controller Public  hardware port Alsa name is %s. Sequencer id is %d:%d.\n",mpc_midi_public_alsa_name,TkRouter.Mpc.cli,TkRouter.Mpc.portPub);

	// Get our controller seq  port client
	if ( anyctrl_name  != NULL) {

		// Initialize card id for public and private
    if ( GetSeqClientFromPortName(anyctrl_name,anyctrl_name,&TkRouter.Ctrl.card,&TkRouter.Ctrl.cli,&TkRouter.Ctrl.port) < 0 ) {
      tklog_error("Error : %s controller card/seq client not found. Parameter ignored.\n",anyctrl_name);
      anyctrl_name[0] = 0;
    }
    else {
       tklog_info("%s controller hardware port Alsa name is hw:%d,0,%d. Sequencer id is %d:%d.\n",anyctrl_name,TkRouter.Ctrl.card,TkRouter.Ctrl.port,TkRouter.Ctrl.cli,TkRouter.Ctrl.port);
    }

	}

	// Create 3 virtuals rawmidi ports : Private I/O, Public O that will expose rawmidi internal ports
  // The port is always 0. This is the standard behaviour of Alsa virtual rawmidi.
  TkRouter.Virt.cliPrivIn  = snd_rawmidi_open(&rawvirt_inpriv, NULL,  "[virtual]Virt MPC Rawmidi In Private", 2);
  TkRouter.Virt.cliPrivOut = snd_rawmidi_open(NULL, &rawvirt_outpriv, "[virtual]Virt MPC Rawmidi Out Private", 3);
  TkRouter.Virt.cliPubOut  = snd_rawmidi_open(NULL, &rawvirt_outpub,  "[virtual]Virt MPC Rawmidi Out Public", 3);

	if ( TkRouter.Virt.cliPrivIn < 0 || TkRouter.Virt.cliPrivOut < 0 || TkRouter.Virt.cliPubOut < 0 ) {
		tklog_fatal("Impossible to create one or many virtual rawmidi ports\n");
		exit(1);
	}

  tklog_info("Virtual Mpc rawmidi private input port %d:0  created.\n",TkRouter.Virt.cliPrivIn);
	tklog_info("Virtual Mpc rawmidi private output port %d:0 created.\n",TkRouter.Virt.cliPrivOut);
	tklog_info("Virtual Mpc rawmidi public output port %d:0 created.\n",TkRouter.Virt.cliPubOut);

  // Create Router Private and public port for external access with alsa sequencer
  // This will allow to process all midi flow with Alsa sequencer within the Router thread
  // and avoid dirty patching within snd_rawmidi_read/write
  if (snd_seq_open(&TkRouter.seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    tklog_fatal("Error opening router ALSA sequencer (tkgl_init).\n");
    exit(1);
  }

  // Change the name of the seq with our for a better view in aconnect...
  snd_seq_set_client_name(TkRouter.seq, ROUTER_SEQ_NAME);
  TkRouter.cli = snd_seq_client_id(TkRouter.seq) ;

  // Create router ports
  TkRouter.portPriv    = CreateSimpleRouterPort(TkRouter.seq, "Priv" );
  TkRouter.portCtrl    = CreateSimpleRouterPort(TkRouter.seq, "Ctrl" );
  TkRouter.portPub     = CreateSimpleRouterPort(TkRouter.seq, "Pub" );
  TkRouter.portMpcPriv = CreateSimpleRouterPort(TkRouter.seq, "Mpc Priv" );
  TkRouter.portMpcPub  = CreateSimpleRouterPort(TkRouter.seq, "Mpc Pub" );
  tklog_info("Midi Router created as client %d.\n",TkRouter.cli);

  // Router ports
  if ( aconnect(	TkRouter.Virt.cliPrivOut,0 , TkRouter.cli,TkRouter.portPriv) < 0 ) {
   tklog_fatal("Alsa error while connecting private virtual rawmidi to router.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.cli,TkRouter.portPriv, TkRouter.Virt.cliPrivIn,0  ) < 0 ) {
   tklog_fatal("Alsa error while connecting router to private virtual rawmidi.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.Virt.cliPubOut,0 , TkRouter.cli,TkRouter.portPub) < 0 ) {
   tklog_fatal("Alsa error while connecting public virtual rawmidi to router.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.Mpc.cli,TkRouter.Mpc.portPriv,TkRouter.cli,TkRouter.portMpcPriv ) < 0 ) {
   tklog_fatal("Alsa error while connecting MPC private port to router.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.cli,TkRouter.portMpcPriv, TkRouter.Mpc.cli,TkRouter.Mpc.portPriv ) < 0 ) {
   tklog_fatal("Alsa error while connecting MPC private port to router.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.cli,TkRouter.portMpcPub, TkRouter.Mpc.cli,TkRouter.Mpc.portPub ) < 0 ) {
   tklog_fatal("Alsa error while connecting MPC public port to router.\n");
   exit(1);
  }

  if ( aconnect(	TkRouter.cli,TkRouter.portPub, TkRouter.Mpc.cli,TkRouter.Mpc.portPub ) < 0 ) {
   tklog_fatal("Alsa error while connecting MPC public port to router.\n");
   exit(1);
  }

	// Connect our controller if used
	if (TkRouter.Ctrl.cli >= 0) {

    if ( aconnect(	TkRouter.Ctrl.cli, TkRouter.Ctrl.port, TkRouter.cli,TkRouter.portCtrl) < 0 ) {
     tklog_fatal("Alsa error while connecting external controller to router.\n");
     exit(1);
    }

    if ( aconnect( TkRouter.cli,TkRouter.portCtrl,	TkRouter.Ctrl.cli, TkRouter.Ctrl.port) < 0 ) {
     tklog_fatal("Alsa error while connecting router to external controller.\n");
     exit(1);
    }
	}

  //   // Start Router thread
  if ( pthread_create(&MidiRouterThread, NULL, threadMidiRouter, (void*)&TkRouter) < 0) {
    tklog_fatal("Unable to create midi router thread\n");
    exit(1);
  }

  fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////
// Clean DUMP of a buffer to screen
////////////////////////////////////////////////////////////////////////////////
static void ShowBufferHexDump(const uint8_t* data, ssize_t sz, uint8_t nl)
{
    uint8_t b;
    char asciiBuff[33];
    uint8_t c=0;

    for (uint16_t idx = 0 ; idx < sz; idx++) {
		  	if ( c == 0 && idx >= 0) tklog_trace("");
        b = (*data++);
  		  fprintf(stdout,"%02X ",b);
        asciiBuff[c++] = ( b >= 0x20 && b< 127? b : '.' ) ;
        if ( c == nl || idx == sz -1 ) {
          asciiBuff[c] = 0;
		      for (  ; c < nl; c++  ) fprintf(stdout,"   ");
          c = 0;
          fprintf(stdout," | %s\n", asciiBuff);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// MPC Main hook
///////////////////////////////////////////////////////////////////////////////
int __libc_start_main(
    int (*main)(int, char **, char **),
    int argc,
    char **argv,
    int (*init)(int, char **, char **),
    void (*fini)(void),
    void (*rtld_fini)(void),
    void *stack_end)
{

    // Find the real __libc_start_main()...
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    // Banner
    fprintf(stdout,"\n%s",TKGL_LOGO);
    tklog_info("---------------------------------------------------------\n");
  	tklog_info("TKGL_MPCMAPPER Version : %s\n",VERSION);
    tklog_info("(c) The KikGen Labs.\n");
    tklog_info("https://github.com/TheKikGen/MPC-LiveXplore\n");
  	tklog_info("---------------------------------------------------------\n");

    // Show the command line
    tklog_info("MPC args : ") ;

    for ( int i = 1 ; i < argc ; i++ ) {
      fprintf(stdout, "%s ", argv[i] ) ;
    }
    fprintf(stdout, "\n") ;
    tklog_info("\n") ;

    // Scan command line
    char * tkgl_SpoofArg = NULL;

    for ( int i = 1 ; i < argc ; i++ ) {

      // help
      if ( ( strcmp("--tkgl_help",argv[i]) == 0 ) ) {
         ShowHelp();
      }
      else
      if ( ( strncmp("--tkgl_ctrlname=",argv[i],16) == 0 ) && ( strlen(argv[i]) >16 ) ) {
         strcpy(anyctrl_name,argv[i] + 16);
         tklog_info("--tgkl_ctrlname specified for %s midi controller\n",anyctrl_name) ;
      }
      else
      // Spoofed product id
      if ( ( strcmp("--tkgl_iamX",argv[i]) == 0 ) ) {
        MPC_SpoofedId = MPC_X;
        tkgl_SpoofArg = argv[i];
      }
      else
      if ( ( strcmp("--tkgl_iamLive",argv[i]) == 0 ) ) {
        MPC_SpoofedId = MPC_LIVE;
        tkgl_SpoofArg = argv[i];
      }
      else
      if ( ( strcmp("--tkgl_iamForce",argv[i]) == 0 ) ) {
        MPC_SpoofedId = MPC_FORCE;
        tkgl_SpoofArg = argv[i];
      }
      else
      if ( ( strcmp("--tkgl_iamOne",argv[i]) == 0 ) ) {
        MPC_SpoofedId = MPC_ONE;
        tkgl_SpoofArg = argv[i];
      }
      else
      if ( ( strcmp("--tkgl_iamLive2",argv[i]) == 0 ) ) {
        MPC_SpoofedId = MPC_LIVE_MK2;
        tkgl_SpoofArg = argv[i];
      }
      else
      // // End user virtual port visible from the MPC app
      // if ( ( strncmp("--tkgl_virtualport=",argv[i],19) == 0 ) && ( strlen(argv[i]) >19 ) ) {
      //    user_virtual_portname = argv[i] + 19;
      //    tklog_info("--tkgl_virtualport specified as %s port name\n",user_virtual_portname) ;
      // }
      // else
      // Dump rawmidi
      if ( ( strcmp("--tkgl_mididump",argv[i]) == 0 ) ) {
        rawMidiDumpFlag = 1 ;
        tklog_info("--tkgl_mididump specified : dump original raw midi message (ENTRY)\n") ;
      }
      else
      if ( ( strcmp("--tkgl_mididumpPost",argv[i]) == 0 ) ) {
        rawMidiDumpPostFlag = 1 ;
        tklog_info("--tkgl_mididumpPost specified : dump raw midi message after transformation (POST)\n") ;
      }
      else
      // Config file name
      if ( ( strncmp("--tkgl_configfile=",argv[i],18) == 0 ) && ( strlen(argv[i]) >18 )  ) {
        configFileName = argv[i] + 18 ;
        tklog_info("--tkgl_configfile specified. File %s will be used for mapping\n",configFileName) ;
      }


    }

    if ( MPC_SpoofedId >= 0 ) {
      tklog_info("%s specified. %s spoofing.\n",tkgl_SpoofArg,DeviceInfoBloc[MPC_SpoofedId].productString ) ;
    }

    // Initialize everything
    tkgl_init();

    int r =  orig(main, argc, argv, init, fini, rtld_fini, stack_end);

    tklog_info("End of mcpmapper.\n" ) ;

    // ... and call main again
    return r ;
}

///////////////////////////////////////////////////////////////////////////////
// ALSA snd_rawmidi_open hooked
///////////////////////////////////////////////////////////////////////////////
// This function allows changing the name of a virtual port, by using the
// naming convention "[virtual]port name"
// and if creation succesfull, will return the client number. Port is always 0 if virtual.

int snd_rawmidi_open(snd_rawmidi_t **inputp, snd_rawmidi_t **outputp, const char *name, int mode)
{
	//tklog_debug("snd_rawmidi_open name %s mode %d\n",name,mode);

  // Rename the virtual port as we need
  // Port Name must not be emtpy - 30 chars max
  if ( strncmp(name,"[virtual]",9) == 0 ) {
    int l = strlen(name);
    if ( l <= 9 || l > 30 + 9 ) return -1;

    // Prepare the renaming of the virtual port
    strcpy(snd_seq_virtual_port_newname,name + 9);
    snd_seq_virtual_port_rename_flag = 1;

    // Create the virtual port via the fake Alsa rawmidi virtual open
    int r = orig_snd_rawmidi_open(inputp, outputp, "virtual", mode);
    if ( r < 0 ) return r;

    // Get the port id that was populated in the port creation sub function
    // and reset it
    r = snd_seq_virtual_port_clientid;
    snd_seq_virtual_port_clientid=-1;

    //tklog_debug("PORT ID IS %d\n",r);

    return r;

  }

	// Substitute the hardware private input port by our input virtual ports

	else if ( strcmp(mpc_midi_private_alsa_name,name) == 0   ) {

		// Private In
		if (inputp) *inputp = rawvirt_inpriv;
		else if (outputp) *outputp = rawvirt_outpriv ;
		else return -1;
		tklog_info("%s substitution by virtual rawmidi successfull\n",name);

		return 0;
	}

	else if ( strcmp(mpc_midi_public_alsa_name,name) == 0   ) {

		if (outputp) *outputp = rawvirt_outpub;
		else return -1;
		tklog_info("%s substitution by virtual rawmidi successfull\n",name);

		return 0;
	}

	return orig_snd_rawmidi_open(inputp, outputp, name, mode);
}

///////////////////////////////////////////////////////////////////////////////
// ALSA snd_rawmidi_close hooked
///////////////////////////////////////////////////////////////////////////////
int snd_rawmidi_close	(	snd_rawmidi_t * 	rawmidi	)
{

	//tklog_debug("snd_rawmidi_close handle :%p\n",rawmidi);

	return orig_snd_rawmidi_close(rawmidi);
}

///////////////////////////////////////////////////////////////////////////////
// Get MPC pad # from Force pad relatively to the current quadran
///////////////////////////////////////////////////////////////////////////////
static int Mpc_GetPadIndexFromForcePadIndex(uint8_t padF) {

  uint8_t padL = padF / 8 ;
  uint8_t padC = padF % 8 ;
  uint8_t padM = 0x7F ;

  if ( MPC_OriginalId == MPC_FORCE ) return padM; // Only for Mpcs

  // Transpose Force pad to Mpc pad in the 4x4 current quadran
  if ( padL >= MPC_PadOffsetL && padL < MPC_PadOffsetL + 4 ) {
    if ( padC >= MPC_PadOffsetC  && padC < MPC_PadOffsetC + 4 ) {
      padM = (  3 - ( padL - MPC_PadOffsetL  ) ) * 4 + ( padC - MPC_PadOffsetC)  ;
    }
  }

  return padM;
}

///////////////////////////////////////////////////////////////////////////////
// Refresh MPC pads colors from Force PAD Colors cache
///////////////////////////////////////////////////////////////////////////////
static void Mpc_ResfreshPadsColorFromForceCache(TkRouter_t *rp, uint8_t padL, uint8_t padC, uint8_t nbLine) {

  if ( MPC_OriginalId == MPC_FORCE ) return; // Only for Mpcs

  uint8_t sysexBuff[12] = { 0xF0, 0x47, 0x7F, 0x3B, 0x65, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF7};
  //                                                                 [Pad #] [R]   [G]   [B]

  sysexBuff[3] = DeviceInfoBloc[MPC_OriginalId].sysexId;

  for ( int l = 0 ; l< nbLine ; l++ ) {

    for ( int c = 0 ; c < 4 ; c++ ) {

      int padF = ( l + padL ) * 8 + c + padC;
      sysexBuff[7] = Mpc_GetPadIndexFromForcePadIndex(padF) ;
      sysexBuff[8] = Force_PadColorsCache[padF].r ;
      sysexBuff[9] = Force_PadColorsCache[padF].g;
      sysexBuff[10] = Force_PadColorsCache[padF].b;
      SeqSendRawMidi(rp->seq, rp->portMpcPriv,  sysexBuff, sizeof(sysexBuff) );
      tklog_debug("Mpc_ResfreshPadsColorFromForceCache...\n");

    }

  }

}

///////////////////////////////////////////////////////////////////////////////
// Show the current MPC quadran within the Force matrix
///////////////////////////////////////////////////////////////////////////////
static void Mpc_ShowForceMatrixQuadran(TkRouter_t *rp, uint8_t forcePadL, uint8_t forcePadC) {

  uint8_t sysexBuff[12] = { 0xF0, 0x47, 0x7F, 0x40, 0x65, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF7};
  //                                                                 [Pad #] [R]   [G]   [B]
  sysexBuff[3] = DeviceInfoBloc[MPC_OriginalId].sysexId;

  uint8_t q = ( forcePadL == 4 ? 0 : 1 ) * 4 + ( forcePadC == 4 ? 3 : 2 ) ;

  for ( int l = 0 ; l < 2 ; l++ ) {
    for ( int c = 0 ; c < 2 ; c++ ) {
      sysexBuff[7] = l * 4 + c + 2 ;
      if ( sysexBuff[7] == q ) {
        sysexBuff[8]  =  0x7F ;
        sysexBuff[9]  =  0x7F ;
        sysexBuff[10] =  0x7F ;
      }
      else {
        sysexBuff[8]  =  0x7F ;
        sysexBuff[9]  =  0x00 ;
        sysexBuff[10] =  0x00 ;
      }
      //tklog_debug("[tkgl] MPC Pad quadran : l,c %d,%d Pad %d r g b %02X %02X %02X\n",forcePadL,forcePadC,sysexBuff[7],sysexBuff[8],sysexBuff[9],sysexBuff[10]);

      //orig_snd_rawmidi_write(rawvirt_outpriv,sysexBuff,sizeof(sysexBuff));
      // Direct send to hw private port
      SeqSendRawMidi(rp->seq, rp->portMpcPriv,  sysexBuff, sizeof(sysexBuff) );
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a pad line on MPC pads from a Force PAD line in the current Colors cache
///////////////////////////////////////////////////////////////////////////////
void Mpc_DrawPadLineFromForceCache(TkRouter_t *rp, uint8_t forcePadL, uint8_t forcePadC, uint8_t mpcPadL) {

  uint8_t const sysexBuff[12] = { 0xF0, 0x47, 0x7F, 0x40, 0x65, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF7};
  //                                                                       [Pad #] [R]   [G]   [B]
  // A line of 4 pads
  uint8_t buffer[4 * sizeof(sysexBuff)];
  uint8_t * buff2 = buffer;

  for ( int c = 0 ; c < 4 ; c++ ) {
    memcpy(buff2, sysexBuff, sizeof(sysexBuff) );
    buff2[3] = DeviceInfoBloc[MPC_OriginalId].sysexId;
    buff2[7] = mpcPadL * 4 + c ;
    uint8_t p = forcePadL*8 + c + forcePadC  ;
    buff2[8]  = Force_PadColorsCache[p].r ;
    buff2[9]  = Force_PadColorsCache[p].g;
    buff2[10] = Force_PadColorsCache[p].b;
    buff2 += sizeof(sysexBuff);
    //tklog_debug("[tkgl] MPC Pad Line refresh : %d r g b %02X %02X %02X\n",sysexBuff[7],sysexBuff[8],sysexBuff[9],sysexBuff[10]);

  }

  //orig_snd_rawmidi_write(rawvirt_outpriv,sysexBuff,sizeof(sysexBuff));
  SeqSendRawMidi(rp->seq, rp->portMpcPriv,  buffer, sizeof(buffer) );

}

///////////////////////////////////////////////////////////////////////////////
// PROCESS SYSEX SET PAD COLOR SENT TO MPC/FORCE  DEVICE
///////////////////////////////////////////////////////////////////////////////
// If all parameters are null, this will make a refresh options
int AllMpc_MapSxPadColorToDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize ) {

  int r = size ;

  if ( ( MPC_OriginalId != MPC_FORCE && MPC_Id != MPC_FORCE )
       || (  MPC_OriginalId == MPC_Id ) )
  {
    // Any MPC to any MPC or Force to Force : no spoofing at all
    // Nothing to do
   ;
  }
  else {
    // Remap Force hardware pad to MPC Pads
    if ( MPC_OriginalId == MPC_FORCE ) {

      // Translate pad number

      uint8_t padM = buffer[0];
      uint8_t padL = padM / 4 ;
      uint8_t padC = padM % 4 ;

      // Place the 4x4 in the 8x8 matrix
      padL += 0 ;
      padC += 2 ;

      buffer[0] = ( 7 - padL ) * 8 + padC;

    }
    else {

      // Refresh command
      if ( buffer == NULL && size == 0 && maxSize == 0 ) {
        Mpc_DrawPadLineFromForceCache(rp, 8, MPC_PadOffsetC, 3);

        return 0;
      }

      // Remap Mpc hardware pad to Force Pads
      uint8_t padF = buffer[0];

      // Update Force pad color cache
      Force_PadColorsCache[padF].r = buffer[1];
      Force_PadColorsCache[padF].g = buffer[2];
      Force_PadColorsCache[padF].b = buffer[3];

      buffer[0] = Mpc_GetPadIndexFromForcePadIndex(padF) ;

      // Set pad for external controller eventually
      if ( rp->Ctrl.cli >= 0 ) {
          uint8_t padCt = ( ( 7 - padF / 8 ) * 10 + 11 + padF % 8 );
          ControllerSetPadColorRGB(rp,padCt, Force_PadColorsCache[padF].r, Force_PadColorsCache[padF].g,Force_PadColorsCache[padF].b);
      }
    }
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS Leds MIDI EVENT SENT TO MPC/FORCE  DEVICE
///////////////////////////////////////////////////////////////////////////////
int AllMpc_MapLedToDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize ) {

  // SHift is always accepted as unmappable
  if ( buffer[1] == SHIFT_KEY_VALUE ) return size;

  // Check if we must remap...

  if ( map_ButtonsLeds_Inv[ buffer[1] ] >= 0 ) {
      buffer[1] = map_ButtonsLeds_Inv[ buffer[1] ];
  } else {
    return 0; // No mapping found. Block event
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS Encoders Qlink MIDI EVENT RECEIVED FROM MPC/FORCE  DEVICE
///////////////////////////////////////////////////////////////////////////////
int AllMpc_MapEncoderFromDevice( TkRouter_t *rp, uint8_t * buffer, ssize_t size, size_t maxSize ) {

  if ( ShiftHoldedMode && DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16 ) {
    buffer[1] +=  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;
    }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS PADS MIDI EVENT RECEIVED FROM MPC/FORCE  DEVICE
///////////////////////////////////////////////////////////////////////////////
int AllMpc_MapPadFromDevice(TkRouter_t *rp,uint8_t * buffer, ssize_t size, size_t maxSize ) {

  int r = size ;

  if ( ( MPC_OriginalId != MPC_FORCE && MPC_Id != MPC_FORCE )
       || (  MPC_OriginalId == MPC_Id ) )
  {
    // Any MPC to any MPC or Force to Force : no spoofing at all
    // Nothing to do
   ;
  }
  else {
    // Remap Force hardware pad to MPC Pads
    if ( MPC_OriginalId == MPC_FORCE ) {

      // Remap Force hardware pad
      uint8_t padF = buffer[1] - FORCEPADS_TABLE_IDX_OFFSET ;
      uint8_t padL = padF / 8 ;
      uint8_t padC = padF % 8 ;

      if ( padC >= 2 && padC < 6  && padL > 3  ) {

        // Compute the MPC pad id
        uint8_t p = ( 3 - padL % 4 ) * 4 + (padC -2) % 4;
        buffer[1] = MPCPadsTable2[p];

      }
      else {
        //  block event
        return 0;
      }

    }
    // Remap MPC hardware pad to Force pad + special function
    else {
      uint8_t padM = MPCPadsTable[buffer[1] - MPCPADS_TABLE_IDX_OFFSET ];
      uint8_t padL = padM / 4  ;
      uint8_t padC = padM % 4  ;

      // Compute the Force pad id without offset
      uint8_t padF = ( 3 - padL + MPC_PadOffsetL ) * 8 + padC + MPC_PadOffsetC ;
      if (  ShiftHoldedMode || MPC_ForceColumnMode >= 0  ) {
        // Ignore aftertouch in special pad modes
        if ( buffer[0] == 0xA9 ) {
            return 0 ; // block this one
        }

        uint8_t buttonValue = 0x7F;
        uint8_t offsetC = MPC_PadOffsetC;
        uint8_t offsetL = MPC_PadOffsetL;

        // Columns solo mute mode on first pad line
        if ( MPC_ForceColumnMode >= 0 &&  padL == 3  ) padM = 0x7F ; // Simply to pass in the switch case
        // LAUNCH ROW SHIFT + PAD  in column 0 = Launch the corresponding line
        else if ( ShiftHoldedMode && padC == 0 ) padM = 0x7E ; // Simply to pass in the switch case

        switch (padM) {
          // COlumn pad mute,solo   Pads botom line  90 29-30 00/7f
          case 0x7F:
            buttonValue = 0x29 + padC + MPC_PadOffsetC;
            break;
          // Launch row.
          case 0x7E:
            buttonValue = padF / 8 + 56; // Launch row
            break;
          // Matrix Navigation left Right  Up Down need shift
          case 9:
            if ( ShiftHoldedMode) buttonValue = FORCE_LEFT;
            break;
          case 11:
            if ( ShiftHoldedMode) buttonValue = FORCE_RIGHT;
            break;
          case 14:
            if ( ShiftHoldedMode) buttonValue = FORCE_UP;
            break;
          case 10:
            if ( ShiftHoldedMode) buttonValue = FORCE_DOWN;
            break;
          // PAd as quadran
          case 6:
            if ( MPC_ForceColumnMode >= 0 ) { offsetL = offsetC = 0; }
            break;
          case 7:
            if ( MPC_ForceColumnMode >= 0 ) { offsetL = 0; offsetC = 4; }
            break;
          case 2:
            if ( MPC_ForceColumnMode >= 0 ) { offsetL = 4; offsetC = 0; }
            break;
          case 3:
            if ( MPC_ForceColumnMode >= 0 ) { offsetL = 4; offsetC = 4; }
            break;
          default:
            return 0;
        }

        // Simulate a button press/release
        // to navigate in the matrix , to start a raw, to manage solo mute
        if ( buttonValue != 0x7F )  {
            //tklog_debug("Matrix shit pad fn = %d \n", buttonValue) ;
            buffer[2] = ( buffer[0] == 0x99 ? 0x7F:0x00 ) ;
            buffer[0]   = 0x90; // MPC Button
            buffer[1] = buttonValue;
            return r ; // next msg
        }

        // Column button + pad as quadran
        if ( ( MPC_PadOffsetL != offsetL )  || ( MPC_PadOffsetC != offsetC ) )  {
          MPC_PadOffsetL = offsetL ;
          MPC_PadOffsetC = offsetC ;
          //tklog_debug("Quadran nav = %d \n", buttonValue) ;
          Mpc_ResfreshPadsColorFromForceCache(rp, MPC_PadOffsetL,MPC_PadOffsetC,4);
          Mpc_ShowForceMatrixQuadran(rp, MPC_PadOffsetL, MPC_PadOffsetC);
          // Block event
          return 0;
        }

        // Should not be here
        return 0;
      }

      // Pad as usual
      else  buffer[1] = padF + FORCEPADS_TABLE_IDX_OFFSET;

      return r ;
    }
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS BUTTONS MIDI EVENT RECEIVED FROM MPC/FORCE  DEVICE
///////////////////////////////////////////////////////////////////////////////
int AllMpc_MapButtonFromDevice( TkRouter_t * rp, uint8_t * buffer, ssize_t size, size_t maxSize ) {
  int r = size ;

  uint8_t buttonId = buffer[1];
  bool buttonPressed = ( buffer[0] == 0x90 && buffer[2] == 0x7F ? true:false ) ;
  bool qLink = false;

  // Not mappable keys ---------------------------------------------------------

  // Shift is universal
  if (  buffer[0] == 0x90 && buttonId == SHIFT_KEY_VALUE   ) {
    ShiftHoldedMode = buttonPressed ;
    return r;
  }

  // Qlink touch/untouch : MPC : 90/80 [54-63] 7F   Force : 90/80 53-5a 7f
  if (   ( MPC_OriginalId == MPC_FORCE && buttonId >= 0x53 && buttonId <= 0x5A )
      || ( MPC_OriginalId != MPC_FORCE && buttonId >= 0x54 && buttonId <= 0x63 )
    )
  {

    //tklog_debug("Touch / untouch knob %02x \n", buffer[1]);

    // If it's a shift + knob "touch", add the offset
    if (  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16 && ShiftHoldedMode ) {
        buffer[1] += DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;
    }

    if ( MPC_OriginalId != MPC_Id && ( MPC_OriginalId == MPC_FORCE || MPC_Id == MPC_FORCE ) ) {
      buffer[1] += ( MPC_OriginalId == MPC_FORCE ? 1 : -1 ) ;
    }
    return r;
  }

  // Return if noteoff . Only Qlink send that (untouch)
  if (  buffer[0] == 0x80 ) return size;

  // Mappable keys -------------------------------------------------------------

  // SHIFT button is currently holded
  // Look for shift mapping above 0x7F
  // If the SHIFT mapping is also activated at destination, and as the shift key
  // is currently holded, we send only the corresponding button, that will
  // generate the shift + button code, Otherwise, we must release the shift key,
  //  by inserting a shift key note off
  int mapValue;

  if ( ShiftHoldedMode ) {
      if ( (mapValue = map_ButtonsLeds[ buttonId + 0x80  ] ) >= 0 ) {
        //tklog_debug("Mapvalue %02x shift mode mapping found .\n",mapValue);

        if ( mapValue >= 0x80 ) {
          buffer[1] = mapValue - 0x80;   // Shift mapped also at destination
        }
        else {
          // We are holding shift, but the dest key is not a SHIFT mapping
          // insert SHIFT BUTTON release in the midi buffer
          // (we assume brutally we have room; Should check max size)
          if ( size > maxSize - 3 ) {
            return -1;
          }
          // make room for the shift key in the buffer
          memcpy( buffer + 3, buffer, 3 );
          r += 3;
          buffer[1] = SHIFT_KEY_VALUE ;
          buffer[2] = 0x00 ; // Button released
          // Now, forward in the buffer, and map our Key
          buffer +=3;
          buffer[1] = mapValue;
        }
      }
      else if ( ( mapValue = map_ButtonsLeds[ buttonId ] ) >= 0 ) {
        // If no shift mapping, use the normal mapping
        buffer[1] = mapValue  ;
      }
      else { // No Mapping. Block event
        return 0;
      }
  }
  // Not in  Shift mode
  else if ( ( mapValue = map_ButtonsLeds[ buttonId ] ) >= 0  ) {
    buffer[1] = mapValue  ;
  }
  else  {  // block event if No mapping
    return 0;
  }

  // Specific Hardware Mapping -------------------------------------------------
  if ( ( MPC_OriginalId != MPC_FORCE && MPC_Id != MPC_FORCE )
       || (  MPC_OriginalId == MPC_Id ) )
  {
    // Any MPC to any MPC or Force to Force : no spoofing at all
    // Nothing to do
    return r ;
  }
  else {
    // Remap Force hardware button to MPC button
    if ( MPC_OriginalId == MPC_FORCE ) {

    }
    // Remap MPC Hardware button to MPC button
    else {
      // Key press Post mapping
      // Activate the special column mode when Force spoofed on a MPC
      // Colum mode Button pressed
      switch ( mapValue ) {
        case FORCE_MUTE:
        case FORCE_SOLO:
        case FORCE_REC_ARM:
        case FORCE_CLIP_STOP:
          if ( buffer[2] == 0x7F ) { // Key press
              MPC_ForceColumnMode = mapValue ;
              Mpc_DrawPadLineFromForceCache(rp, 8, MPC_PadOffsetC, 3);
              Mpc_ShowForceMatrixQuadran(rp, MPC_PadOffsetL, MPC_PadOffsetC);
          }
          else {
            MPC_ForceColumnMode = -1;
            Mpc_ResfreshPadsColorFromForceCache(rp,MPC_PadOffsetL,MPC_PadOffsetC,4);
            tklog_debug("Force column mode off.\n",mapValue);
          }
          break;
      }
      return r;
    }
  }
  return r;
}

// ///////////////////////////////////////////////////////////////////////////////
// // Alsa Rawmidi read
// ///////////////////////////////////////////////////////////////////////////////
// ssize_t snd_rawmidi_read(snd_rawmidi_t *rawmidi, void *buffer, size_t size) {
//
//   //tklog_debug("snd_rawmidi_read %p : size : %u ", rawmidi, size);
// 	ssize_t r = orig_snd_rawmidi_read(rawmidi, buffer, size);
//   // if ( rawMidiDumpFlag  ) RawMidiDump(rawmidi, 'i','r' , buffer, r);
// 	return r;
// }
//
// ///////////////////////////////////////////////////////////////////////////////
// // Alsa Rawmidi write
// ///////////////////////////////////////////////////////////////////////////////
// ssize_t snd_rawmidi_write(snd_rawmidi_t *rawmidi,const void *buffer,size_t size) {
//
//   // if ( rawMidiDumpFlag ) RawMidiDump(rawmidi, 'i','w' , buffer, size);
// 	return 	orig_snd_rawmidi_write(rawmidi, buffer, size);
// }

///////////////////////////////////////////////////////////////////////////////
// Alsa open sequencer
///////////////////////////////////////////////////////////////////////////////
int snd_seq_open (snd_seq_t **handle, const char *name, int streams, int mode) {

  //tklog_debug("snd_seq_open %s (%p) \n",name,handle);

  return orig_snd_seq_open(handle, name, streams, mode);

}

///////////////////////////////////////////////////////////////////////////////
// Alsa close sequencer
///////////////////////////////////////////////////////////////////////////////
int snd_seq_close (snd_seq_t *handle) {

  //tklog_debug("snd_seq_close. Hanlde %p \n",handle);

  return orig_snd_seq_close(handle);

}

///////////////////////////////////////////////////////////////////////////////
// Alsa set a seq port name
///////////////////////////////////////////////////////////////////////////////
void snd_seq_port_info_set_name	(	snd_seq_port_info_t * 	info, const char * 	name )
{
  //tklog_debug("snd_seq_port_info_set_name %s (%p) \n",name);


  return snd_seq_port_info_set_name	(	info, name );
}

///////////////////////////////////////////////////////////////////////////////
// Alsa create a simple seq port
///////////////////////////////////////////////////////////////////////////////
int snd_seq_create_simple_port	(	snd_seq_t * 	seq, const char * 	name, unsigned int 	caps, unsigned int 	type )
{
  tklog_info("Port creation : %s\n",name);

  snd_seq_client_info_t *cinfo;
  snd_seq_client_info_alloca(&cinfo);
  snd_seq_get_client_info (seq, cinfo);


  // Rename virtual port correctly. Impossible with the native Alsa...
  if ( strncmp (" Virtual RawMIDI",name,16) && snd_seq_virtual_port_rename_flag  )
  {
    //tklog_info("Virtual port renamed to %s \n",snd_seq_virtual_port_newname);
    snd_seq_virtual_port_rename_flag = 0;
    int r = orig_snd_seq_create_simple_port(seq,snd_seq_virtual_port_newname,caps,type);
    if ( r < 0 ) return r;

    // Get port information
    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_get_port_info(seq, 0, pinfo);
    snd_seq_virtual_port_clientid =  snd_seq_port_info_get_client(pinfo);

    return r;
  }

	// We do not allow MPC ports creation by MPC app for our device or our virtuals ports
  // Because this could lead to infinite midi loop in the MPC midi end user settings
	if (  strncmp("MPC",snd_seq_client_info_get_name(cinfo),3) == 0 ) {

      if ( ( TkRouter.Ctrl.cli >= 0 && strstr(name, anyctrl_name) )

     // In some specific cases, public and private ports could appear in the APP when spoofing,
     // because port names haven't the same prefixes (eg. Force vs MPC). The consequence is
     // that the MPC App receives midi message of buttons and encoders in midi tracks.
     // So we mask here Private and Public ports eventually requested by MPC App, which
     // should be only internal rawmidi ports.

     // This match will also catch our TKGL virtual ports having Private or Public suffix.
     ||  ( ( match(name,".* Public$|.* Private$" )) )

     )
     {
        tklog_info("MPC app Port %s creation canceled.\n",name);
        return -1;
     }
 }
 return orig_snd_seq_create_simple_port(seq,name,caps,type);
}

///////////////////////////////////////////////////////////////////////////////
// Decode a midi seq event
///////////////////////////////////////////////////////////////////////////////
long snd_midi_event_decode	(	snd_midi_event_t * 	dev,unsigned char * 	buf,long 	count, const snd_seq_event_t * 	ev )
{

	// Disable running status to be a true "raw" midi. Side effect : disabled for all ports...
	snd_midi_event_no_status(dev,1);
  long r = orig_snd_midi_event_decode(dev,buf,count,ev);

	return r ;

}

///////////////////////////////////////////////////////////////////////////////
// close
///////////////////////////////////////////////////////////////////////////////
int close(int fd) {

  if ( fd == product_code_file_handler )            product_code_file_handler = -1;
  else if ( fd == product_compatible_file_handler ) product_compatible_file_handler = -1;
  else if ( fd == pws_online_file_handler   )       pws_online_file_handler   = -1 ;
  else if ( fd == pws_voltage_file_handler  )       pws_voltage_file_handler  = -1 ;
  else if ( fd == pws_present_file_handler  )       pws_present_file_handler  = -1 ;
  else if ( fd == pws_status_file_handler   )       pws_status_file_handler   = -1 ;
  else if ( fd == pws_capacity_file_handler )       pws_capacity_file_handler = -1 ;

  return orig_close(fd);
}

///////////////////////////////////////////////////////////////////////////////
// fake_open : use memfd to create a fake file in memory
///////////////////////////////////////////////////////////////////////////////
int fake_open(const char * name, char *content, size_t contentSize) {
  int fd = memfd_create(name, MFD_ALLOW_SEALING);
  write(fd,content, contentSize);
  lseek(fd, 0, SEEK_SET);
  return fd  ;
}

///////////////////////////////////////////////////////////////////////////////
// open64
///////////////////////////////////////////////////////////////////////////////
// Intercept all file opening to fake those we want to.

int open64(const char *pathname, int flags,...) {
   // If O_CREAT is used to create a file, the file access mode must be given.
   // We do not fake the create function at all.
   if (flags & O_CREAT) {
       va_list args;
       va_start(args, flags);
       int mode = va_arg(args, int);
       va_end(args);
      return  orig_open64(pathname, flags, mode);
   }

   // Existing file
   // Fake files sections

   // product code
   if ( product_code_file_handler < 0 && strcmp(pathname,PRODUCT_CODE_PATH) == 0 ) {
     // Create a fake file in memory
     product_code_file_handler = fake_open(pathname,DeviceInfoBloc[MPC_Id].productCode, strlen(DeviceInfoBloc[MPC_Id].productCode) ) ;
     return product_code_file_handler ;
   }

   // product compatible
   if ( product_compatible_file_handler < 0 && strcmp(pathname,PRODUCT_COMPATIBLE_PATH) == 0 ) {
     char buf[64];
     sprintf(buf,PRODUCT_COMPATIBLE_STR,DeviceInfoBloc[MPC_Id].productCompatible);
     product_compatible_file_handler = fake_open(pathname,buf, strlen(buf) ) ;
     return product_compatible_file_handler ;
   }

   // Fake power supply files if necessary only (this allows battery mode)
   else
   if ( DeviceInfoBloc[MPC_OriginalId].fakePowerSupply ) {

     if ( pws_voltage_file_handler   < 0 && strcmp(pathname,POWER_SUPPLY_VOLTAGE_NOW_PATH) == 0 ) {
        pws_voltage_file_handler = fake_open(pathname,POWER_SUPPLY_VOLTAGE_NOW,strlen(POWER_SUPPLY_VOLTAGE_NOW) ) ;
        return pws_voltage_file_handler ;
     }

     if ( pws_online_file_handler   < 0 && strcmp(pathname,POWER_SUPPLY_ONLINE_PATH) == 0 ) {
        pws_online_file_handler = fake_open(pathname,POWER_SUPPLY_ONLINE,strlen(POWER_SUPPLY_ONLINE) ) ;
        return pws_online_file_handler ;
     }

     if ( pws_present_file_handler  < 0 && strcmp(pathname,POWER_SUPPLY_PRESENT_PATH) == 0 ) {
       pws_present_file_handler = fake_open(pathname,POWER_SUPPLY_PRESENT,strlen(POWER_SUPPLY_PRESENT)  ) ;
       return pws_present_file_handler ;
     }

     if ( pws_status_file_handler   < 0 && strcmp(pathname,POWER_SUPPLY_STATUS_PATH) == 0 ) {
       pws_status_file_handler = fake_open(pathname,POWER_SUPPLY_STATUS,strlen(POWER_SUPPLY_STATUS) ) ;
       return pws_status_file_handler ;
     }

     if ( pws_capacity_file_handler < 0 && strcmp(pathname,POWER_SUPPLY_CAPACITY_PATH) == 0 ) {
       pws_capacity_file_handler = fake_open(pathname,POWER_SUPPLY_CAPACITY,strlen(POWER_SUPPLY_CAPACITY) ) ;
       return pws_capacity_file_handler ;
     }
   }

   return orig_open64(pathname, flags) ;
}
