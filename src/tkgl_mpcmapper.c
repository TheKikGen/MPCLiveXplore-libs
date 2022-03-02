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

// Buttons and controls Mapping tables
// SHIFT values have bit 7 set

static int map_ButtonsLeds[MAPPING_TABLE_SIZE];
static int map_ButtonsLeds_Inv[MAPPING_TABLE_SIZE]; // Inverted table

// To navigate in matrix quadran when MPC spoofing a Force
static int MPC_PadOffsetL = 0;
static int MPC_PadOffsetC = 0;

// Force Matric pads color cache
static Force_MPCPadColor_t Force_PadColorsCache[256];

// End user virtual port name
static char *user_virtual_portname = NULL;

// End user virtual port handles
static snd_rawmidi_t *rawvirt_user_in    = NULL ;
static snd_rawmidi_t *rawvirt_user_out   = NULL ;

// MPC Current pad bank.  A-H = 0-7
static int MPC_PadBank = BANK_A ;

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
static typeof(&snd_rawmidi_read) orig_snd_rawmidi_read;
static typeof(&snd_rawmidi_write) orig_snd_rawmidi_write;
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

  tklog_info("%d keys found in section %s . \n",keysCount,btLedMapSectionName);

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

      tklog_info("Item %s%s (%d/0x%02X) mapped to %s%s (%d/0x%02X)\n",srcShift?"(SHIFT) ":"",srcKeyName,srcButtonValue,srcButtonValue,destShift?"(SHIFT) ":"",destKeyName,map_ButtonsLeds[srcButtonValue],map_ButtonsLeds[srcButtonValue]);

    }
    else {
      tklog_error("Configuration file Error : values above 127 / 0x7F found. Check sections [%s] %s, [%s] %s.\n",btLedSrcSectionName,srcKeyName,btLedDestSectionName,destKeyName);
      return;
    }

  } // for
}

///////////////////////////////////////////////////////////////////////////////
// Prepare a fake midi message in the Private midi context
///////////////////////////////////////////////////////////////////////////////
void PrepareFakeMidiMsg(uint8_t buf[]) {
  buf[0]  = 0x00 ;
  buf[1]  = 0x00 ;
  buf[2]  = 0x00 ;
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
	return  -r;
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
		return 0;
	}

  if (snd_seq_subscribe_port(seq, subs) < 0) {
		snd_seq_close(seq);
    tklog_error("Connection of midi port %d:%d to %d:%d failed !\n",src_client,src_port,dest_client,dest_port);
		return 1;
	}

  tklog_info("Connection of midi port %d:%d to %d:%d successfull\n",src_client,src_port,dest_client,dest_port);


	snd_seq_close(seq);
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
	printf("%3d:%-3d ", ev->source.client, ev->source.port);
	switch (ev->type) {
	case SND_SEQ_EVENT_NOTEON:
		printf("Note on                %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_NOTEOFF:
		printf("Note off               %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_KEYPRESS:
		printf("Polyphonic aftertouch  %2d %3d %3d\n",
		       ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
		break;
	case SND_SEQ_EVENT_CONTROLLER:
		printf("Control change         %2d %3d %3d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_PGMCHANGE:
		printf("Program change         %2d %3d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_CHANPRESS:
		printf("Channel aftertouch     %2d %3d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_PITCHBEND:
		printf("Pitch bend             %2d  %6d\n",
		       ev->data.control.channel, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_CONTROL14:
		printf("Control change         %2d %3d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_NONREGPARAM:
		printf("Non-reg. parameter     %2d %5d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_REGPARAM:
		printf("Reg. parameter         %2d %5d %5d\n",
		       ev->data.control.channel, ev->data.control.param, ev->data.control.value);
		break;
	case SND_SEQ_EVENT_SONGPOS:
		printf("Song position pointer     %5d\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_SONGSEL:
		printf("Song select               %3d\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_QFRAME:
		printf("MTC quarter frame         %02xh\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_TIMESIGN:
		// XXX how is this encoded?
		printf("SMF time signature        (%#08x)\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_KEYSIGN:
		// XXX how is this encoded?
		printf("SMF key signature         (%#08x)\n",
		       ev->data.control.value);
		break;
	case SND_SEQ_EVENT_START:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			printf("Queue start               %d\n",
			       ev->data.queue.queue);
		else
			printf("Start\n");
		break;
	case SND_SEQ_EVENT_CONTINUE:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			printf("Queue continue            %d\n",
			       ev->data.queue.queue);
		else
			printf("Continue\n");
		break;
	case SND_SEQ_EVENT_STOP:
		if (ev->source.client == SND_SEQ_CLIENT_SYSTEM &&
		    ev->source.port == SND_SEQ_PORT_SYSTEM_TIMER)
			printf("Queue stop                %d\n",
			       ev->data.queue.queue);
		else
			printf("Stop\n");
		break;
	case SND_SEQ_EVENT_SETPOS_TICK:
		printf("Set tick queue pos.       %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_SETPOS_TIME:
		printf("Set rt queue pos.         %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_TEMPO:
		printf("Set queue tempo           %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_CLOCK:
		printf("Clock\n");
		break;
	case SND_SEQ_EVENT_TICK:
		printf("Tick\n");
		break;
	case SND_SEQ_EVENT_QUEUE_SKEW:
		printf("Queue timer skew          %d\n", ev->data.queue.queue);
		break;
	case SND_SEQ_EVENT_TUNE_REQUEST:
		/* something's fishy here ... */
		printf("Tuna request\n");
		break;
	case SND_SEQ_EVENT_RESET:
		printf("Reset\n");
		break;
	case SND_SEQ_EVENT_SENSING:
		printf("Active Sensing\n");
		break;
	case SND_SEQ_EVENT_CLIENT_START:
		printf("Client start              %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_CLIENT_EXIT:
		printf("Client exit               %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_CLIENT_CHANGE:
		printf("Client changed            %d\n",
		       ev->data.addr.client);
		break;
	case SND_SEQ_EVENT_PORT_START:
		printf("Port start                %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_EXIT:
		printf("Port exit                 %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_CHANGE:
		printf("Port changed              %d:%d\n",
		       ev->data.addr.client, ev->data.addr.port);
		break;
	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		printf("Port subscribed           %d:%d -> %d:%d\n",
		       ev->data.connect.sender.client, ev->data.connect.sender.port,
		       ev->data.connect.dest.client, ev->data.connect.dest.port);
		break;
	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		printf("Port unsubscribed         %d:%d -> %d:%d\n",
		       ev->data.connect.sender.client, ev->data.connect.sender.port,
		       ev->data.connect.dest.client, ev->data.connect.dest.port);
		break;
	case SND_SEQ_EVENT_SYSEX:
		{
			unsigned int i;
			printf("System exclusive         ");
			for (i = 0; i < ev->data.ext.len; ++i)
				printf(" %02X", ((unsigned char*)ev->data.ext.ptr)[i]);
			printf("\n");
		}
		break;
	default:
		printf("Event type %d\n",  ev->type);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Alsa SEQ raw write to a seq port
///////////////////////////////////////////////////////////////////////////////
int SeqSendMidiMsg(snd_seq_t *seqHandle, uint8_t port,  const uint8_t *message, size_t size )
{
  static snd_midi_event_t * midiParser = NULL;

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

  snd_seq_event_t ev;
  snd_seq_ev_clear (&ev);

  snd_midi_event_encode (midiParser, message, size, &ev);
  snd_midi_event_reset_encode (midiParser);

  snd_seq_ev_set_source(&ev, port);

  snd_seq_ev_set_subs (&ev);
  snd_seq_ev_set_direct (&ev);

  snd_seq_event_output (seqHandle, &ev);

  snd_seq_drain_output (seqHandle);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// LaunchPad 3
///////////////////////////////////////////////////////////////////////////////
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

  SeqSendMidiMsg(rp->seq, rp->portCtrl,  buffer, pbuff - buffer );

}

int ControllerInitialize(TkRouter_t *rp) {

  SeqSendMidiMsg(rp->seq, rp->portCtrl,  SX_LPMK3_STDL_MODE, sizeof(SX_LPMK3_STDL_MODE) );
  SeqSendMidiMsg(rp->seq, rp->portCtrl,  SX_LPMK3_DAW_CLEAR, sizeof(SX_LPMK3_DAW_CLEAR) );
  SeqSendMidiMsg(rp->seq, rp->portCtrl,  SX_LPMK3_PGM_MODE, sizeof(SX_LPMK3_PGM_MODE) );

  ControllerScrollText(rp,"The KikGen Labs - - - IamForce",0,21,COLOR_SEA);

  uint8_t midiMsg[3];
  midiMsg[0]=0x92;
  midiMsg[1]=0x63;
  midiMsg[2]=0x2D;

  SeqSendMidiMsg(rp->seq, rp->portCtrl,midiMsg,3);

}

///////////////////////////////////////////////////////////////////////////////
// MIDI EVEN PROCESS AND ROUTE (IN A sub THREAD)
///////////////////////////////////////////////////////////////////////////////
void threadMidiProcessAndRoute(TkRouter_t *rp) {

  snd_seq_event_t *ev;
  uint8_t *buff ;

  do {
      snd_seq_event_input(rp->seq, &ev);
      snd_seq_ev_set_subs(ev);
      snd_seq_ev_set_direct(ev);

      buff = ((uint8_t*)ev->data.ext.ptr) ;
      ssize_t size = ev->data.ext.len;

      // Events from MPC application - Write Private
      if ( ev->source.client == rp->Virt.cliPrivOut ) {
        tklog_info("Event received rawmidi virtual write private ...\n");
        //SeqRawMidiWrite(rp->seq, rp->Mpc.cli, rp->mpcPrivatePort, buff,size );
      }
      else if ( ev->source.client == rp->Virt.cliPubOut ) {
        tklog_info("Event received rawmidi virtual write public ...\n");
        // Route to Mpc hw port public
        snd_seq_ev_set_source(ev, rp->portMpcPub);
        //snd_seq_ev_set_dest(ev, rp->Mpc.cli,rp->Mpc.portPub);
        snd_seq_event_output_direct(rp->seq, ev);
      }
      else if ( ev->source.client == rp->Mpc.cli ) {
        if ( ev->source.port == rp->Mpc.portPriv ) {
          tklog_info("Event received from MPC controller private\n");
          //snd_seq_ev_set_dest(ev, rp->Virt.cliPrivIn,0);
          snd_seq_ev_set_source(ev, rp->portPriv);
          snd_seq_event_output_direct(rp->seq, ev);
        }
      }
      // Events from external controller
      else if ( ev->source.client == rp->Ctrl.cli && ev->source.port == rp->Ctrl.port) {
        tklog_info("Event received from external controller...\n");
        //SeqRawMidiWrite(rp->seq, rp->Mpc.cli, rp->mpcPublicPort, buff,size );
      }

      //dump_event(ev);
      //tklog_info("\n\n");
      snd_seq_free_event(ev);

  } while (snd_seq_event_input_pending(rp->seq, 0) > 0);
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
	orig_snd_rawmidi_read           = dlsym(RTLD_NEXT, "snd_rawmidi_read");
	orig_snd_rawmidi_write          = dlsym(RTLD_NEXT, "snd_rawmidi_write");
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

  // read mapping config file if any
  LoadMappingFromConfFile(configFileName);

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

  // Make connections of our virtuals ports
  // MPC APP <---> VIRTUAL PORTS <---> MPC CONTROLLER PRIVATE & PUBLIC PORTS

	// Private MPC controller port private  out to rawmidi virtual In priv
	// if ( aconnect(	TkRouter.Mpc.cli,TkRouter.Mpc.portPriv , TkRouter.Virt.cliPrivIn, 0 ) < 0 ) {
  //   tklog_fatal("Alsa error while connecting MPC private port to virtual rawmidi private in.\n");
  //   exit(1);
  // }
  //
	// // Virtual rawmidi out priv to Private MPC controller private port
	// if ( aconnect(	TkRouter.Virt.cliPrivOut, 0, TkRouter.Mpc.cli,TkRouter.Mpc.portPriv) < 0 ) {
  //   tklog_fatal("Alsa error while connecting virtual rawmidi private out to MPC Client private port.\n");
  //   exit(1);
  // }
  //
	// // Virtual rawmidi out pub to Public MPC controller
  // // No need to cable the out...
  // if ( aconnect(	TkRouter.Virt.cliPubOut,0, TkRouter.Mpc.cli,TkRouter.Mpc.portPub) < 0 ) {
  //   tklog_fatal("Alsa error while connecting virtual rawmidi public out to MPC Client public port.\n");
  //   exit(1);
  // }

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

  // // Create a user virtual port if asked on the command line
  // if ( user_virtual_portname != NULL) {
  //
  //   sprintf(portname,"[virtual]%s",user_virtual_portname);
  //   if ( snd_rawmidi_open(&rawvirt_user_in, &rawvirt_user_out,  portname, 0 ) < 0 ) {
  //     tklog_fatal("Unable to create virtual user port %s\n",user_virtual_portname);
  //     exit(1);
  //   }
  //   tklog_info("Virtual user port %s succesfully created.\n",user_virtual_portname);
  //   //snd_rawmidi_open(&read_handle, &write_handle, "virtual", 0);
  // }

  fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////
// Clean DUMP of a buffer to screen
////////////////////////////////////////////////////////////////////////////////
static void ShowBufferHexDump(const uint8_t* data, size_t sz, uint8_t nl)
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

////////////////////////////////////////////////////////////////////////////////
// RawMidi dump
////////////////////////////////////////////////////////////////////////////////
static void RawMidiDump(snd_rawmidi_t *rawmidi, char io,char rw,const uint8_t* data, size_t sz) {

  char name[64];
  if ( rawmidi == rawvirt_inpriv ) strcpy(name,"Virt in Private");
  else if ( rawmidi == rawvirt_outpriv ) strcpy(name,"Virt out Private");
  else if ( rawmidi == rawvirt_outpub ) strcpy(name,"Virt out Public");
  else strcpy( name,snd_rawmidi_name(rawmidi) );

  tklog_trace("%s dump snd_rawmidi_%s from controller %s\n",io == 'i'? "Entry":"Post",rw == 'r'? "read":"write",name);
  ShowBufferHexDump(data, sz,16);
  tklog_trace("\n");

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
// Refresh MPC pads colors from Force PAD Colors cache
///////////////////////////////////////////////////////////////////////////////
static void Mpc_ResfreshPadsColorFromForceCache(uint8_t padL, uint8_t padC, uint8_t nbLine) {

  // Write again the color like a Force.
  // The midi modification will be done within the corpse of the hooked fn.
  // Pads from 64 are columns pads

  uint8_t sysexBuff[12] = { 0xF0, 0x47, 0x7F, 0x40, 0x65, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF7};
  //                                                                 [Pad #] [R]   [G]   [B]

  for ( int l = 0 ; l< nbLine ; l++ ) {

    for ( int c = 0 ; c < 4 ; c++ ) {

      int padF = ( l + padL ) * 8 + c + padC;
      sysexBuff[7] = padF;
      sysexBuff[8] = Force_PadColorsCache[padF].r ;
      sysexBuff[9] = Force_PadColorsCache[padF].g;
      sysexBuff[10] = Force_PadColorsCache[padF].b;
      // Send the sysex to the MPC controller
      snd_rawmidi_write(rawvirt_outpriv,sysexBuff,sizeof(sysexBuff));

    }

  }

}

///////////////////////////////////////////////////////////////////////////////
// Show the current MPC quadran within the Force matrix
///////////////////////////////////////////////////////////////////////////////
static void Mpc_ShowForceMatrixQuadran(uint8_t forcePadL, uint8_t forcePadC) {

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

      orig_snd_rawmidi_write(rawvirt_outpriv,sysexBuff,sizeof(sysexBuff));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a pad line on MPC pads from a Force PAD line in the current Colors cache
///////////////////////////////////////////////////////////////////////////////
void Mpc_DrawPadLineFromForceCache(uint8_t forcePadL, uint8_t forcePadC, uint8_t mpcPadL) {

  uint8_t sysexBuff[12] = { 0xF0, 0x47, 0x7F, 0x40, 0x65, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF7};
  //                                                                 [Pad #] [R]   [G]   [B]
  sysexBuff[3] = DeviceInfoBloc[MPC_OriginalId].sysexId;

  uint8_t p = forcePadL*8 + forcePadC ;

  for ( int c = 0 ; c < 4 ; c++ ) {
    sysexBuff[7] = mpcPadL * 4 + c ;
    p = forcePadL*8 + c + forcePadC  ;
    sysexBuff[8]  = Force_PadColorsCache[p].r ;
    sysexBuff[9]  = Force_PadColorsCache[p].g;
    sysexBuff[10] = Force_PadColorsCache[p].b;

    //tklog_debug("[tkgl] MPC Pad Line refresh : %d r g b %02X %02X %02X\n",sysexBuff[7],sysexBuff[8],sysexBuff[9],sysexBuff[10]);

    orig_snd_rawmidi_write(rawvirt_outpriv,sysexBuff,sizeof(sysexBuff));
  }
}

///////////////////////////////////////////////////////////////////////////////
// MIDI READ - APP ON MPC READING AS FORCE
///////////////////////////////////////////////////////////////////////////////
static size_t Mpc_MapReadFromForce(void *midiBuffer, size_t maxSize,size_t size) {

    uint8_t * myBuff = (uint8_t*)midiBuffer;

    size_t i = 0 ;
    while  ( i < size ) {

      // AKAI SYSEX ------------------------------------------------------------
      // IDENTITY REQUEST
      if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader)) == 0 ) {
        // If so, substitue sysex identity request by the faked one
        memcpy(&myBuff[i+sizeof(IdentityReplySysexHeader)],DeviceInfoBloc[MPC_Id].sysexIdReply, sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) );
        i += sizeof(IdentityReplySysexHeader) + sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) ;
        continue;
      }

      // KNOBS TURN (UNMAPPED BECAUSE ARE ALL EQUIVALENT ON ALL DEVICES) ------
      // If it's a shift + knob turn, add an offset
      //  B0 [10-31] [7F - n]
      if (  myBuff[i] == 0xB0 ) {
        if ( ShiftHoldedMode && DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x10 && myBuff[i+1] <= 0x31 )
            myBuff[i+1] +=  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;
        i += 3;
        continue;
      }

      // BUTTONS PRESS / RELEASE------------------------------------------------
      if (  myBuff[i] == 0x90   ) {
        //tklog_debug("Button 0x%02x %s\n",myBuff[i+1], (myBuff[i+2] == 0x7F ? "pressed":"released") );

        // SHIFT pressed/released (nb the SHIFT button can't be mapped)
        // Double click on SHIFT is not managed at all. Avoid it.
        if ( myBuff[i+1] == SHIFT_KEY_VALUE ) {
            ShiftHoldedMode = ( myBuff[i+2] == 0x7F ? true : false );
            // Kill the shift  event because we want to manage this here and not let
            // the MPC app to know that shift is pressed
            //PrepareFakeMidiMsg(&myBuff[i]);
            i +=3 ;
            continue ; // next msg
        }

        //tklog_debug("Shift + key mode is %s \n",ShiftHoldedMode ? "active":"inactive");

        // Exception : Qlink management is hard coded
        // SHIFT "KNOB TOUCH" button :  add the offset when possible
        // MPC : 90 [54-63] 7F      FORCE : 90 [53-5A] 7F  (no "untouch" exists)
        if ( myBuff[i+1] >= 0x54 && myBuff[i+1] <= 0x63 ) {
            myBuff[i+1] -- ; // Map to force Qlink touch

            if (  ShiftHoldedMode && DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16 )
                myBuff[i+1] += DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;

            //tklog_debug("Qlink 0x%02x touch\n",myBuff[i+1] );

            i += 3 ;
            continue ; // next msg
        }

        // Simple key press
        //tklog_debug("Mapping for %d = %d - shift = %d \n", myBuff[i+1], map_ButtonsLeds[ myBuff[i+1] ],map_ButtonsLeds[ myBuff[i+1] +  0x80 ] );

        int mapValue = map_ButtonsLeds[ myBuff[i+1] + ( ShiftHoldedMode ? 0x80 : 0)  ];

        // No mapping. Next msg
        if ( mapValue < 0 ) {
          PrepareFakeMidiMsg(&myBuff[i]);
          i += 3 ;
          //tklog_debug("No mapping found for 0x%02x \n",myBuff[i+1]);

          continue ; // next msg
        }

        //tklog_debug("Mapping found for 0x%02x : 0x%02x \n",myBuff[i+1],mapValue);


        // Manage shift mapping at destination
        // Not a shift mapping.  Disable the current shift mode
        if (  mapValue < 0x80  )  {
          if ( myBuff[i + 2] ==  0x7f ) { // Key press
              if ( ShiftHoldedMode) {
                if ( size > maxSize - 3 ) fprintf(stdout,"Warning : midi buffer overflow when inserting SHIFT key release !!\n");
                memcpy( &myBuff[i + 3 ], &myBuff[i], size - i );
                size +=3;
                myBuff[i + 1] = SHIFT_KEY_VALUE ;   myBuff[i + 2] =  0x00 ; // Button SHIFT release insert
                i += 3 ;
              }
          }
        }
        else { // Shift at destination
          if ( myBuff[i + 2] ==  0x7f ) { // Key press
            if ( ! ShiftHoldedMode) {
              if ( size > maxSize - 3 ) fprintf(stdout,"Warning : midi buffer overflow when inserting SHIFT key press !!\n");
              memcpy( &myBuff[i + 3 ], &myBuff[i], size - i );
              size +=3;
              myBuff[i + 1] = SHIFT_KEY_VALUE ;   myBuff[i + 2] =  0x07 ; // Button SHIFT press insert
              i += 3 ;
            }
          }
          mapValue -= 0x80 ;
        }


        // Key press Post mapping
        // Activate the special column mode when Force spoofed on a MPC
        // Colum mode Button pressed
        switch ( mapValue ) {
          case FORCE_MUTE:
          case FORCE_SOLO:
          case FORCE_REC_ARM:
          case FORCE_CLIP_STOP:
            if ( myBuff[i+2] == 0x7F ) { // Key press
                MPC_ForceColumnMode = mapValue ;
                Mpc_DrawPadLineFromForceCache(8, MPC_PadOffsetC, 3);
                Mpc_ShowForceMatrixQuadran(MPC_PadOffsetL, MPC_PadOffsetC);
            }
            else {
              MPC_ForceColumnMode = -1;
              Mpc_ResfreshPadsColorFromForceCache(MPC_PadOffsetL,MPC_PadOffsetC,4);
            }
            break;
        }
        myBuff[i + 1] = mapValue ;

        i += 3;
        continue; // next msg
      }

      // PADS ------------------------------------------------------------------
      if (  myBuff[i] == 0x99 || myBuff[i] == 0x89 || myBuff[i] == 0xA9 ) {

        // Remap MPC hardware pad
        // Get MPC pad id in the true order
        uint8_t padM = MPCPadsTable[myBuff[i+1] - MPCPADS_TABLE_IDX_OFFSET ];
        uint8_t padL = padM / 4  ;
        uint8_t padC = padM % 4  ;

        // Compute the Force pad id without offset
        uint8_t padF = ( 3 - padL + MPC_PadOffsetL ) * 8 + padC + MPC_PadOffsetC ;

        if (  ShiftHoldedMode || MPC_ForceColumnMode >= 0  ) {
          // Ignore aftertouch in special pad modes
          if ( myBuff[i] == 0xA9 ) {
              PrepareFakeMidiMsg(&myBuff[i]);
              i += 3;
              continue; // next msg
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
              PrepareFakeMidiMsg(&myBuff[i]);
              i += 3;
              continue; // next msg
          }

          // Simulate a button press/release
          // to navigate in the matrix , to start a raw, to manage solo mute
          if ( buttonValue != 0x7F )  {
              //tklog_debug("Matrix shit pad fn = %d \n", buttonValue) ;
              myBuff[i+2] = ( myBuff[i] == 0x99 ? 0x7F:0x00 ) ;
              myBuff[i]   = 0x90; // MPC Button
              myBuff[i+1] = buttonValue;
              i += 3;
              continue; // next msg
          }

          // Column button + pad as quadran
          if ( ( MPC_PadOffsetL != offsetL )  || ( MPC_PadOffsetC != offsetC ) )  {
            MPC_PadOffsetL = offsetL ;
            MPC_PadOffsetC = offsetC ;
            //tklog_debug("Quadran nav = %d \n", buttonValue) ;
            Mpc_ResfreshPadsColorFromForceCache(MPC_PadOffsetL,MPC_PadOffsetC,4);
            Mpc_ShowForceMatrixQuadran(MPC_PadOffsetL, MPC_PadOffsetC);
            PrepareFakeMidiMsg(&myBuff[i]);
            i += 3;
            continue; // next msg
          }

          // Should not be here
          PrepareFakeMidiMsg(&myBuff[i]);

        }

        // Pad as usual
        else  myBuff[i+1] = padF + FORCEPADS_TABLE_IDX_OFFSET;

        i += 3;

      }

      else i++;

    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// MIDI READ - APP ON MPC READING AS MPC
///////////////////////////////////////////////////////////////////////////////
static size_t Mpc_MapReadFromMpc(void *midiBuffer, size_t maxSize,size_t size) {

    uint8_t * myBuff = (uint8_t*)midiBuffer;

    size_t i = 0 ;
    while  ( i < size ) {

      // AKAI SYSEX ------------------------------------------------------------
      // IDENTITY REQUEST
      if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader)) == 0 ) {
        // If so, substitue sysex identity request by the faked one
        memcpy(&myBuff[i+sizeof(IdentityReplySysexHeader)],DeviceInfoBloc[MPC_Id].sysexIdReply, sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) );
        i += sizeof(IdentityReplySysexHeader) + sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) ;
      }

      // KNOBS TURN ------------------------------------------------------------
      else
      if (  myBuff[i] == 0xB0 ) {
        if ( ShiftHoldedMode && DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x10 && myBuff[i+1] <= 0x31 )
            myBuff[i+1] +=  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;
        i += 3;
      }

      // BUTTONS - LEDS --------------------------------------------------------
      else
      if (  myBuff[i] == 0x90   ) { //|| myBuff[i] == 0x80
        // Shift is an exception and  mapping is ignored (the SHIFT button can't be mapped)
        // Double click on SHIFT is not managed at all. Avoid it.
        if ( myBuff[i+1] == SHIFT_KEY_VALUE ) {
            ShiftHoldedMode = ( myBuff[i+2] == 0x7F ? true:false ) ;
        }
        else
        // SHIFT button is current holded
        // SHIFT double click on the MPC side is not taken into account for the moment
        if ( ShiftHoldedMode ) {

          // KNOB TOUCH : If it's a shift + knob "touch", add the offset
          // 90 [54-63] 7F
          if ( DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x54 && myBuff[i+1] <= 0x63 )
                 myBuff[i+1] += DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;

          // Look for shift mapping above 0x7F
          if ( map_ButtonsLeds[ myBuff[i+1] + 0x80  ] >= 0 ) {

              uint8_t mapValue = map_ButtonsLeds[ myBuff[i+1] + 0x80 ];

              // If the SHIFT mapping is also activated at destination, and as the shift key
              // is currently holded, we send only the corresponding button, that will generate the shift + button code,
              // Otherwise, we must release the shift key, by inserting a shift key note off

              // Shift mapped also at destination
              if ( mapValue >= 0x80 ) {
                  myBuff[i+1] = mapValue - 0x80;
              }
              else {
                  // We are holding shift, but the dest key is not a SHIFT mapping
                  // insert SHIFT BUTTON note off in the midi buffer
                  // (we assume brutally we have room; Should check max size)
                  if ( size > maxSize - 3 ) fprintf(stdout,"Warning : midi buffer overflow when inserting SHIFT note off !!\n");
                  memcpy( &myBuff[i + 3 ], &myBuff[i], size - i );
                  size +=3;

                  myBuff[i + 1] = SHIFT_KEY_VALUE ;
                  myBuff[i + 2] = 0x00 ; // Button released
                  i += 3;

                  // Now, map our Key
                  myBuff[ i + 1 ] = mapValue;
              }

          }
          else {
              // If no shift mapping, use the normal mapping
              myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ];
          }
        } // Shiftmode
        else
        if ( map_ButtonsLeds[ myBuff[i+1] ] >= 0 ) {
          //tklog_debug("MAP %d->%d\n",myBuff[i+1],map_ButtonsLeds[ myBuff[i+1] ]);
          myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ];
        }

        i += 3;

      }

      // PADS -----------------------------------------------------------------
      else
      if (  myBuff[i] == 0x99 || myBuff[i] == 0x89 || myBuff[i] == 0xA9 ) {

        i += 3;

      }

      else i++;

    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// MIDI WRITE - APP ON MPC MAPPING TO MPC
///////////////////////////////////////////////////////////////////////////////
static void Mpc_MapAppWriteToMpc(const void *midiBuffer, size_t size) {

  uint8_t * myBuff = (uint8_t*)midiBuffer;

  size_t i = 0 ;
  while  ( i < size ) {

    // AKAI SYSEX
    // If we detect the Akai sysex header, change the harwware id by our true hardware id.
    // Messages are compatibles. Some midi msg are not always interpreted (e.g. Oled)
    if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],AkaiSysex,sizeof(AkaiSysex)) == 0 ) {
        // Update the sysex id in the sysex for our original hardware
        i += sizeof(AkaiSysex) ;
        myBuff[i] = DeviceInfoBloc[MPC_OriginalId].sysexId; // MPC are several...
        i++;

          // SET PAD COLORS SYSEX FN
          // F0 47 7F [3B] -> 65 00 04 [Pad #] [R] [G] [B] F7
        if ( memcmp(&myBuff[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
              i += sizeof(MPCSysexPadColorFn) ;

              uint8_t padF = myBuff[i];
              // Update Force pad color cache
              Force_PadColorsCache[padF].r = myBuff[i + 1 ];
              Force_PadColorsCache[padF].g = myBuff[i + 2 ];
              Force_PadColorsCache[padF].b = myBuff[i + 3 ];

              i += 5 ; // Next msg
        }
    }
    // Buttons-Leds.  In that direction, it's a LED ON / OFF for the button
    // Check if we must remap...
    else
    if (  myBuff[i] == 0xB0  ) {

      if ( map_ButtonsLeds_Inv[ myBuff[i+1] ] >= 0 ) {
        //tklog_debug("MAP INV %d->%d\n",myBuff[i+1],map_ButtonsLeds_Inv[ myBuff[i+1] ]);
        myBuff[i+1] = map_ButtonsLeds_Inv[ myBuff[i+1] ];
      }
      i += 3;
    }
    else i++;
  }
}

///////////////////////////////////////////////////////////////////////////////
// MIDI WRITE - APP ON MPC MAPPING TO FORCE
///////////////////////////////////////////////////////////////////////////////
static void Mpc_MapAppWriteToForce(const void *midiBuffer, size_t size) {

  uint8_t * myBuff = (uint8_t*)midiBuffer;

  bool refreshMutePadLine = false;
  bool refreshOptionPadLines = false;

  size_t i = 0 ;
  while  ( i < size ) {

    // AKAI SYSEX
    // If we detect the Akai sysex header, change the harwware id by our true hardware id.
    // Messages are compatibles. Some midi msg are not always interpreted (e.g. Oled)
    if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],AkaiSysex,sizeof(AkaiSysex)) == 0 ) {
        // Update the sysex id in the sysex for our original hardware
        i += sizeof(AkaiSysex) ;
        myBuff[i] = DeviceInfoBloc[MPC_OriginalId].sysexId;
        i++;

        // SET PAD COLORS SYSEX ------------------------------------------------
        // FN  F0 47 7F [3B] -> 65 00 04 [Pad #] [R] [G] [B] F7
        if ( memcmp(&myBuff[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
            i += sizeof(MPCSysexPadColorFn) ;

            uint8_t padF = myBuff[i];
            uint8_t padL = padF / 8 ;
            uint8_t padC = padF % 8 ;
            uint8_t padM = 0x7F ;

            // Update Force pad color cache
            Force_PadColorsCache[padF].r = myBuff[i + 1 ];
            Force_PadColorsCache[padF].g = myBuff[i + 2 ];
            Force_PadColorsCache[padF].b = myBuff[i + 3 ];

            //tklog_debug("Setcolor for Force pad %d (%d,%d)  %02x%02x%02x\n",padF,padL,padC,myBuff[i + 1 ],myBuff[i + 2 ],myBuff[i + 3 ]);

            // Transpose Force pad to Mpc pad in the 4x4 current quadran
            if ( padL >= MPC_PadOffsetL && padL < MPC_PadOffsetL + 4 ) {
              if ( padC >= MPC_PadOffsetC  && padC < MPC_PadOffsetC + 4 ) {
                padM = (  3 - ( padL - MPC_PadOffsetL  ) ) * 4 + ( padC - MPC_PadOffsetC)  ;
              }
            }

            // Take care of the pad mutes mode line 0 on the MPC, line 8 on Force
            // Shift must not be pressed else it shows the sub option of lines 8/9
            // and not the state of solo/mut/rec arm etc...
            if ( padL == 8 && !ShiftHoldedMode && MPC_ForceColumnMode >= 0   ) refreshMutePadLine = true;
            else if ( ( padL == 8 || padL == 9 ) && ShiftHoldedMode && MPC_ForceColumnMode < 0   ) refreshOptionPadLines = true;

            //tklog_debug("Mpc pad transposed : %d \n",padM);

            // Update the pad# in the midi buffer
            myBuff[i] = padM;

            i += 5 ; // Next msg
        }

    }

    // Buttons-Leds.  In that direction, it's a LED ON / OFF for the button
    // Check if we must remap...

    else
    if (  myBuff[i] == 0xB0  ) {
      if ( map_ButtonsLeds_Inv[ myBuff[i+1] ] >= 0 ) {
        //tklog_debug("MAP INV %d->%d\n",myBuff[i+1],map_ButtonsLeds_Inv[ myBuff[i+1] ]);
        myBuff[i+1] = map_ButtonsLeds_Inv[ myBuff[i+1] ];
      }
      i += 3;
    }

    else i++;
  }

  // Check if mute pad line changed
  if ( refreshMutePadLine ) Mpc_DrawPadLineFromForceCache(8, MPC_PadOffsetC, 3);
  else if ( refreshOptionPadLines ) {

    // Mpc_DrawPadLineFromForceCache(9, 0, 3);
    // Mpc_DrawPadLineFromForceCache(9, 4, 3);
    // Mpc_DrawPadLineFromForceCache(8, 0, 2);
    // Mpc_DrawPadLineFromForceCache(8, 4, 2);

  }

}

///////////////////////////////////////////////////////////////////////////////
// MIDI READ - APP ON FORCE READING AS ITSELF
///////////////////////////////////////////////////////////////////////////////
static size_t Force_MapReadFromForce(void *midiBuffer, size_t maxSize,size_t size) {

    uint8_t * myBuff = (uint8_t*)midiBuffer;

    size_t i = 0 ;
    while  ( i < size ) {

      // AKAI SYSEX ------------------------------------------------------------
      // IDENTITY REQUEST REPLY
      if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader)) == 0 ) {
        // If so, substitue sysex identity request by the faked one
        i += sizeof(IdentityReplySysexHeader) + sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) ;
      }

      // KNOBS TURN ------------------------------------------------------------
      //  B0 [10-31] [7F - n]
      else
      if (  myBuff[i] == 0xB0 ) {

        i += 3;
      }


      // BUTTONS - LEDS --------------------------------------------------------

      else
      if (  myBuff[i] == 0x90   ) { //|| myBuff[i] == 0x80

        // Shift is an exception and  mapping is ignored (the SHIFT button can't be mapped)
        // Double click on SHIFT is not managed at all. Avoid it.
        if ( myBuff[i+1] == SHIFT_KEY_VALUE ) {
            ShiftHoldedMode = ( myBuff[i+2] == 0x7F ? true:false ) ;
        }
        else
        // SHIFT button is current holded
        // SHIFT double click on the MPC side is not taken into account for the moment
        if ( ShiftHoldedMode ) {

          // KNOB TOUCH : If it's a shift + knob "touch", add the offset
          // 90 [54-63] 7F
          if ( DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x54 && myBuff[i+1] <= 0x63 )
                 myBuff[i+1] += DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;

          // Look for shift mapping above 0x7F
          if ( map_ButtonsLeds[ myBuff[i+1] + 0x80  ] >= 0 ) {

              uint8_t mapValue = map_ButtonsLeds[ myBuff[i+1] + 0x80 ];

              // If the SHIFT mapping is also activated at destination, and as the shift key
              // is currently holded, we send only the corresponding button, that will generate the shift + button code,
              // Otherwise, we must release the shift key, by inserting a shift key note off

              // Shift mapped also at destination
              if ( mapValue >= 0x80 ) {
                  myBuff[i+1] = mapValue - 0x80;
              }
              else {
                  // We are holding shift, but the dest key is not a SHIFT mapping
                  // insert SHIFT BUTTON note off in the midi buffer
                  // (we assume brutally we have room; Should check max size)
                  if ( size > maxSize - 3 ) fprintf(stdout,"Warning : midi buffer overflow when inserting SHIFT note off !!\n");
                  memcpy( &myBuff[i + 3 ], &myBuff[i], size - i );
                  size +=3;

                  myBuff[i + 1] = SHIFT_KEY_VALUE ;
                  myBuff[i + 2] = 0x00 ; // Button released
                  i += 3;

                  // Now, map our Key
                  myBuff[ i + 1 ] = mapValue;
              }

          }
          else {
              // If no shift mapping, use the normal mapping
              myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ];
          }
        } // Shiftmode
        else
        if ( map_ButtonsLeds[ myBuff[i+1] ] >= 0 ) {
          //tklog_debug("MAP %d->%d\n",myBuff[i+1],map_ButtonsLeds[ myBuff[i+1] ]);
          myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ];
        }

        i += 3;

      }

      // PADS -----------------------------------------------------------------
      else
      if (  myBuff[i] == 0x99 || myBuff[i] == 0x89 || myBuff[i] == 0xA9 ) {

        i += 3;

      }

      else i++;

    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// MIDI READ - APP ON FORCE READING AS MPC
///////////////////////////////////////////////////////////////////////////////
static size_t Force_MapReadFromMpc(void *midiBuffer, size_t maxSize,size_t size) {

    uint8_t * myBuff = (uint8_t*)midiBuffer;

    size_t i = 0 ;
    while  ( i < size ) {

// AKAI SYSEX ==================================================================

// IDENTITY REQUEST REPLAY -----------------------------------------------------

      if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],IdentityReplySysexHeader,sizeof(IdentityReplySysexHeader)) == 0 ) {
        // If so, substitue sysex identity request by the faked one
        memcpy(&myBuff[i+sizeof(IdentityReplySysexHeader)],DeviceInfoBloc[MPC_Id].sysexIdReply, sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) );
        i += sizeof(IdentityReplySysexHeader) + sizeof(DeviceInfoBloc[MPC_Id].sysexIdReply) ;
      }

// CC KNOBS TURN ---------------------------------------------------------------

      else
      if (  myBuff[i] == 0xB0 ) {
        // If it's a shift + knob turn, add an offset   B0 [10-31] [7F - n]
        if ( ShiftHoldedMode &&  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x10 && myBuff[i+1] <= 0x31 ) {

          myBuff[i+1] +=  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;

        }
       // Todo mapping
        i += 3;
      }

// BUTTONS PRESSED / RELEASED --------------------------------------------------

      else
      if (  myBuff[i] == 0x90   ) { //
        // Shift is an exception and  mapping is ignored (the SHIFT button can't be mapped)
        // Double click on SHIFT is not managed at all. Avoid it.
        if ( myBuff[i+1] == SHIFT_KEY_VALUE ) {
            ShiftHoldedMode = ( myBuff[i+2] == 0x7F ? true:false ) ;
        }
        else
        // SHIFT button is currently holded
        // SHIFT native double click on the MPC side is not taken into account for the moment
        if ( ShiftHoldedMode ) {

          // KNOB TOUCH : If it's a shift + knob "touch", add the offset   90 [54-63] 7F
          // Before the mapping
          if (  DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount < 16
                  && myBuff[i+1] >= 0x54 && myBuff[i+1] <= 0x63 )
          {
            myBuff[i+1] += DeviceInfoBloc[MPC_OriginalId].qlinkKnobsCount;
          }

          // Look for shift mapping above 0x7F
          // If the SHIFT mapping is also activated at destination, and as the shift key
          // is currently holded, we send only the corresponding button, that will generate the shift + button code,
          // Otherwise, we must release the shift key, by inserting a shift key note off
          if ( map_ButtonsLeds[ myBuff[i+1] + 0x80  ] >= 0 ) {

              uint8_t mapValue = map_ButtonsLeds[ myBuff[i+1] + 0x80 ];

              // Shift mapped also at destination
              if ( mapValue >= 0x80 ) {
                  myBuff[i+1] = mapValue - 0x80;
              }
              else {
                  // We are holding shift, but the dest key is not a SHIFT mapping
                  // insert SHIFT BUTTON note off in the midi buffer
                  // (we assume brutally we have room; Should check max size)
                  if ( size > maxSize - 3 ) fprintf(stdout,"Warning : midi buffer overflow when inserting SHIFT note off !!\n");
                  memcpy( &myBuff[i + 3 ], &myBuff[i], size - i );
                  size +=3;

                  myBuff[i + 1] = SHIFT_KEY_VALUE ;
                  myBuff[i + 2] = 0x00 ; // Button released
                  i += 3;

                  // Now, map our Key
                  myBuff[ i + 1 ] = mapValue;
              }

          }
          else {
              // If no shift mapping, use the normal mapping
              myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ] > 0 ? map_ButtonsLeds[ myBuff[i+1] ] : myBuff[i+1]  ;
          }
        } // Shif holded tmode

        // SHift not holded here
        else {

          // Remap the Force Button with the MPC Button
          if ( map_ButtonsLeds[ myBuff[i+1] ] >= 0 ) {

            myBuff[i+1] = map_ButtonsLeds[ myBuff[i+1] ];

          }
          else if ( configFileName != NULL ) {
            // No mapping in the configuration file
            // Erase bank button midi msg - Put a fake midi msg
            PrepareFakeMidiMsg(&myBuff[i]);
          }
        }

        i += 3;
      }

      // PADS -----------------------------------------------------------------
      else
      if (  myBuff[i] == 0x99 || myBuff[i] == 0x89 || myBuff[i] == 0xA9 ) {

          // Remap Force hardware pad
          uint8_t padF = myBuff[i+1] - FORCEPADS_TABLE_IDX_OFFSET ;
          uint8_t padL = padF / 8 ;
          uint8_t padC = padF % 8 ;

          if ( padC >= 2 && padC < 6  && padL > 3  ) {

            // Compute the MPC pad id
            uint8_t p = ( 3 - padL % 4 ) * 4 + (padC -2) % 4;
            myBuff[i+1] = MPCPadsTable2[p];

          } else {
            // Fake event
            PrepareFakeMidiMsg(&myBuff[i]);
          }


        i += 3;

      }

      else i++;

    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////
// MIDI WRITE - APP ON FORCE MAPPING TO MPC
///////////////////////////////////////////////////////////////////////////////
static void Force_MapAppWriteToMpc(const void *midiBuffer, size_t size) {

  uint8_t * myBuff = (uint8_t*)midiBuffer;

  size_t i = 0 ;
  while  ( i < size ) {

    // AKAI SYSEX
    // If we detect the Akai sysex header, change the harwware id by our true hardware id.
    // Messages are compatibles. Some midi msg are not always interpreted (e.g. Oled)
    if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],AkaiSysex,sizeof(AkaiSysex)) == 0 ) {
        // Update the sysex id in the sysex for our original hardware
        i += sizeof(AkaiSysex) ;
        myBuff[i] = DeviceInfoBloc[MPC_OriginalId].sysexId;
        i++;

        // PAD SET COLOR SYSEX - MPC spoofed on a Force
        // F0 47 7F [3B] -> 65 00 04 [Pad #] [R] [G] [B] F7
        if ( memcmp(&myBuff[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
          i += sizeof(MPCSysexPadColorFn) ;

          // Translate pad number
          uint8_t padM = myBuff[i];
          uint8_t padL = padM / 4 ;
          uint8_t padC = padM % 4 ;

          // Place the 4x4 in the 8x8 matrix
          padL += 0 ;
          padC += 2 ;

          myBuff[i] = ( 7 - padL ) * 8 + padC;

          i += 5 ; // Next msg
        }
    }

    // Buttons-Leds.  In that direction, it's a LED ON / OFF for the button
    else
    if (  myBuff[i] == 0xB0  ) {
      if ( map_ButtonsLeds_Inv[ myBuff[i+1] ] >= 0 ) {
        myBuff[i+1] = map_ButtonsLeds_Inv[ myBuff[i+1] ];
      }
      else if ( configFileName != NULL ) {
        // No mapping in the configuration file
        // Erase bank button midi msg - Put a fake midi msg
        PrepareFakeMidiMsg(&myBuff[i]);
      }

      i += 3; // Next midi msg
    }

    else i++;
  }
}

///////////////////////////////////////////////////////////////////////////////
// MIDI WRITE - APP ON FORCE MAPPING TO ITSELF
///////////////////////////////////////////////////////////////////////////////
static void Force_MapAppWriteToForce(const void *midiBuffer, size_t size) {

  uint8_t * myBuff = (uint8_t*)midiBuffer;

  size_t i = 0 ;
  while  ( i < size ) {

    // AKAI SYSEX
    if (  myBuff[i] == 0xF0 &&  memcmp(&myBuff[i],AkaiSysex,sizeof(AkaiSysex)) == 0 ) {
        // Update the sysex id in the sysex for our original hardware
        i += sizeof(AkaiSysex) ;
        //myBuff[i] = DeviceInfoBloc[MPC_OriginalId].sysexId;
        i++;

        // PAD COLOR SYSEX - Store color in the pad color cache
        // SET PAD COLORS SYSEX FN
        // F0 47 7F [3B] -> 65 00 04 [Pad #] [R] [G] [B] F7
        if ( memcmp(&myBuff[i],MPCSysexPadColorFn,sizeof(MPCSysexPadColorFn)) == 0 ) {
              i += sizeof(MPCSysexPadColorFn) ;

              // Update Force pad color cache
              uint8_t padF = myBuff[i];
              Force_PadColorsCache[padF].r = myBuff[i + 1 ];
              Force_PadColorsCache[padF].g = myBuff[i + 2 ];
              Force_PadColorsCache[padF].b = myBuff[i + 3 ];

              i += 5 ; // Next msg
        }

    }

    // Buttons-Leds.  In that direction, it's a LED ON / OFF for the button
    // Check if we must remap...
    else
    if (  myBuff[i] == 0xB0  ) {

      if ( map_ButtonsLeds_Inv[ myBuff[i+1] ] >= 0 ) {
        //tklog_debug("MAP INV %d->%d\n",myBuff[i+1],map_ButtonsLeds_Inv[ myBuff[i+1] ]);
        myBuff[i+1] = map_ButtonsLeds_Inv[ myBuff[i+1] ];
      }
      i += 3;
    }

    else i++;
  }

}

///////////////////////////////////////////////////////////////////////////////
// Alsa Rawmidi read
///////////////////////////////////////////////////////////////////////////////
ssize_t snd_rawmidi_read(snd_rawmidi_t *rawmidi, void *buffer, size_t size) {

  //tklog_debug("snd_rawmidi_read %p : size : %u ", rawmidi, size);

	ssize_t r = orig_snd_rawmidi_read(rawmidi, buffer, size);
  if ( r < 0 ) {
    tklog_error("snd_rawmidi_read error : (%p) size : %u  error %d\n", rawmidi, size,r);
    return r;
  }

  if ( rawMidiDumpFlag  ) RawMidiDump(rawmidi, 'i','r' , buffer, r);


  // Map in all cases if the app writes to the controller
  if ( rawmidi == rawvirt_inpriv  ) {

    // We are running on a Force
    if ( MPC_OriginalId == MPC_FORCE ) {
      // We want to map things on Force it self
      if ( MPC_Id == MPC_FORCE ) {
        r = Force_MapReadFromForce(buffer,size,r);
      }
      // Simulate a MPC on a Force
      else {
        r = Force_MapReadFromMpc(buffer,size,r);
      }
    }
    // We are running on a MPC
    else {
      // We need to remap on a MPC it self
      if ( MPC_Id != MPC_FORCE ) {
        r = Mpc_MapReadFromMpc(buffer,size,r);
      }
      // Simulate a Force on a MPC
      else {
        r = Mpc_MapReadFromForce(buffer,size,r);
      }
    }
  }

  if ( rawMidiDumpPostFlag ) RawMidiDump(rawmidi, 'o','r' , buffer, r);

	return r;
}

///////////////////////////////////////////////////////////////////////////////
// Alsa Rawmidi write
///////////////////////////////////////////////////////////////////////////////
ssize_t snd_rawmidi_write(snd_rawmidi_t * 	rawmidi,const void * 	buffer,size_t 	size) {

  if ( rawMidiDumpFlag ) RawMidiDump(rawmidi, 'i','w' , buffer, size);

  // Map in all cases if the app writes to the controller
  if ( rawmidi == rawvirt_outpriv || rawmidi == rawvirt_outpub  ) {

    // We are running on a Force
    if ( MPC_OriginalId == MPC_FORCE ) {
      // We want to map things on Force it self
      if ( MPC_Id == MPC_FORCE ) {
        Force_MapAppWriteToForce(buffer,size);
      }
      // Simulate a MPC on a Force
      else {
        Force_MapAppWriteToMpc(buffer,size);
      }
    }
    // We are running on a MPC
    else {
      // We need to remap on a MPC it self
      if ( MPC_Id != MPC_FORCE ) {
        Mpc_MapAppWriteToMpc(buffer,size);
      }
      // Simulate a Force on a MPC
      else {
        Mpc_MapAppWriteToForce(buffer,size);
      }
    }
  }


  if ( rawMidiDumpPostFlag ) RawMidiDump(rawmidi, 'o','w' , buffer, size);

	return 	orig_snd_rawmidi_write(rawmidi, buffer, size);

}

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

// ///////////////////////////////////////////////////////////////////////////////
// // Process an input midi seq event
// ///////////////////////////////////////////////////////////////////////////////
// int snd_seq_event_input( snd_seq_t* handle, snd_seq_event_t** ev )
// {
//
//   int r = orig_snd_seq_event_input(handle,ev);
//   // if ((*ev)->type != SND_SEQ_EVENT_CLOCK ) {
//   //      dump_event(*ev);
//   //
//   //
//   //    // tklog_info("[tkgl] Src = %02d:%02d -> Dest = %02d:%02d \n",(*ev)->source.client,(*ev)->source.port,(*ev)->dest.client,(*ev)->dest.port);
//   //    // ShowBufferHexDump(buf, r,16);
//   //    // tklog_info("[tkgl] ----------------------------------\n");
//   //  }
//
//
//   return r;
//
// }

// ///////////////////////////////////////////////////////////////////////////////
// // open
// ///////////////////////////////////////////////////////////////////////////////
// int open(const char *pathname, int flags,...) {
//
// //  printf("(tkgl) Open %s\n",pathname);
//
//    // If O_CREAT is used to create a file, the file access mode must be given.
//    if (flags & O_CREAT) {
//        va_list args;
//        va_start(args, flags);
//        int mode = va_arg(args, int);
//        va_end(args);
//        return orig_open(pathname, flags, mode);
//    } else {
//        return orig_open(pathname, flags);
//    }
// }

//
// ///////////////////////////////////////////////////////////////////////////////
// // read
// ///////////////////////////////////////////////////////////////////////////////
// ssize_t read(int fildes, void *buf, size_t nbyte) {
//
//   return orig_read(fildes,buf,nbyte);
// }
