#include "ops-linux.h"
#include <string.h>

#define N_KEYS 0
#define SATURNKEYSFILENAME "/.saturnkeys"

char key_filename[100];

int n_keys = N_KEYS;

static char keydesc[255][128] = {};
static unsigned char key[255][2][128] = {};

static void LoadKeys() {
	FILE *in;
	unsigned int sline, cline, rline, val, t;
	unsigned char tmp_challenge_key[128], tmp_response_key[128], tmp_keydesc[128];
	char line[130];
	char sval[3];

	sprintf(key_filename, "%s%s", getenv("HOME"), SATURNKEYSFILENAME);
	in = fopen(key_filename,"r");
	if (in == 0) 
		return;

	// reset n_keys to default
	n_keys=N_KEYS;
  
	Log ("INFO: Loading .saturnkeys from home directory...");

	sline=0; cline=0; rline=0;

	while (fgets(line, 127, in) !=0 ) {
		if (line[strlen(line)-1] == 0x0a)
			line[strlen(line)-1] = 0; // get rid of newline
    
		// if we haven't seen an 'S' yet...(camera_Write(cmd_get_challenge,31,SHORT_TIMEOUT)
		if (sline == 0) {
			if (line[0] != 'S')
				continue;
			strcpy((char*)tmp_keydesc, (char*)line+2);
			++sline;
			continue;
		}

		// if we have seen an 'S' and are collecting 'C's
		if ((sline == 1) && (cline < 4)) {
			if (line[0] != 'C') {
				Log ("ERROR: Improperly formatted .saturnkeys (check C lines)");
				return;
			}
			for (t = 0; t < 32; ++t) {
				sval[0] = line[2+(t*2)];
				sval[1] = line[3+(t*2)];
				sval[2] = 0;
				val = (unsigned char)strtoul(sval, 0, 16);
				tmp_challenge_key[cline*32+t] = val;
			}
			++cline;
			continue;
		}

		if ((cline == 4) && (rline < 4)) {
			if (line[0] != 'R') {
				Log ("ERROR: Improperly formatted .saturnkeys (check R lines)");
				return;
			}
			for (t = 0; t < 32; ++t) {
				sval[0] = line[2+(t*2)];
				sval[1] = line[3+(t*2)];
				sval[2] = 0;
				val = (unsigned char)strtoul(sval, 0, 16);
				tmp_response_key[rline*32+t] = val;
			}
			++rline;
		}

		if ((sline == 1) && (cline == 4) && (rline == 4)) {
			sline = 0; cline = 0; rline = 0;
			strcpy(keydesc[n_keys], (char*)tmp_keydesc);
			memcpy(key[n_keys][0], tmp_challenge_key, 128);
			memcpy(key[n_keys][1], tmp_response_key, 128);
			n_keys++;
		}
	}  
	fclose(in);

	return;
}

void LogKeyDump(unsigned char *buffer, char *linedescription) {
	int s, t;
	char line[66];
	char linedescstring[2];
	char tmp[4];

	line[0] = '\0';

	linedescstring[0] = *linedescription;
	linedescstring[1] = '\0';

	for (t = 0; t < 4; t++) {
		strcat(line, linedescription);
		strcat(line, " ");
		for (s = 0; s < 32; s++) {
			sprintf(tmp, "%02x", buffer[(t*32)+s]);
			strcat(line, tmp);
		}
		Log( "%s", line );
		line[0] = '\0';
	}
}


static int GetKey(unsigned char *buffer, int offset) {
	int t;
	struct parameters {
		u32 index;
		u32 keydata;
	} data;
  
	data.keydata = cpu_to_le32(0);
  
	for (t = 0; t < 0x80; t += 4) {
		data.index = cpu_to_le32(offset+t); data.keydata = cpu_to_le32(0);
		if (ControlMessageWrite(0xfe00,(int*)&data.index,4,TIMEOUT)==0) {
			Log("ERROR: Failed at 0xfe write (set response read index)");
			return FALSE;
		}

		if (ControlMessageRead(0xff00,(int*)&data.keydata,4,TIMEOUT)==0) {
			Log("ERROR: Failed at 0xff read (read 4 bytes of response)");
			return FALSE;
		}
		*(int*)&buffer[t] = data.keydata;
	}

	return TRUE;
}


static int PutKey(unsigned char *buffer, int offset) {
	int t;
	struct parameters {
		u32 index;
		u32 keydata;
	} data;

	for (t = 0; t < 0x80; t += 4) {
		data.keydata = *(int*)&buffer[t];
		data.index = cpu_to_le32(t + offset);

		if (ControlMessageWrite(0xfa01,(int*)&data.index,8,TIMEOUT)==0) {
			Log( "ERROR: Failed at 0xfa write (return 4 bytes of response)" );
			return FALSE;
		}
	}
  
	data.index = cpu_to_le32(0x1a0);
	if (ControlMessageWrite(0xfe01,(int*)&data.index,4,TIMEOUT)==0) {
		Log( "ERROR: Failed at 0xfe" );
		return FALSE;
	}

	ControlMessageRead(0xff01,(int*)&data.keydata,4,TIMEOUT);
	if (data.keydata == cpu_to_le32(1)) {
		Log( "INFO: Device unlocked!" );
		return TRUE;
	}

	Log( "ERROR: Failed to unlock device" );
	return FALSE;
}


int Unlock (int *CAM_TYPE) {
	static unsigned char challenge[128], response[128];
	int t;
	int x;

	LoadKeys();

	if (*CAM_TYPE == 3) 
          {

            if (!camera_Unlock(*CAM_TYPE))
		{
               		return FALSE;
		}
	    else
                {
			return TRUE;
                }
          }
        else
          {
	     	if (GetKey(challenge,256) == FALSE) {
			Log( "ERROR: Challenge retrieval failed." );
			return FALSE;
		} else {
			Log( "INFO: Challenge retrieved successfully.\n" );
			LogKeyDump(challenge, "C");
		}

		if (GetKey(response,128) == FALSE) {
			Log( "WARNING: Failed to retrieve response from device. Attempting to use challege as the response." );
			memcpy(response, challenge, 128);
		}

		if (response[0] == 0)
		{
			memcpy(response, challenge, 128); // if response=000000, look like 3.62 ?
		}
	
		Log(" ");

		for (t = 0; t < n_keys; ++t) {   

			if (memcmp(challenge,key[t][0],128) == 0) {
				if (PutKey(key[t][1],256) == 0) {
					Log( "ERROR: Failed to unlock camcorder with key %i", t );
					return FALSE;
				} else {
					Log( "INFO: Challenge and response pair being used: %s", (char *)keydesc[t] );
					return TRUE;
				}
			}
		}
  
		Log( "ERROR: Please enter your camcorder's challenge and response keys into your .saturnkeys file." );
		return FALSE;
	}
}


int camera_Unlock(int *CAM_TYPE)
{
	int success = FALSE;
	int camera_write_status =0;
	int camera_read_status = 0;

	static int attempt = 0;

	unsigned char cmd_get_challenge[31] = { 	
	'L','a','M','S',
	0x1d,0xBA,0xAB,0x1D,0x80,0x00,0x00,0x00,0x80,
	0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // Get challange
	unsigned char cmd_send_key[31] = { 	
	'L','a','M','S',
	0x1d,0xBA,0xAB,0x1D,0x80,0x00,0x00,0x00,0x00,
	0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // Send key

	unsigned char challenge[141];    
	unsigned char status[13];


	int i,keyused;

	int received_challenge = FALSE;

	int wrk_x=0;
	double wrk_x1=0;
	int wrk_type =0;
	unsigned char wrk_char[141];

	memset(challenge,0,141);
        wrk_x = usb_resetep(m_p_handle,CAMERA_WRITE_ENDPOINT);

        wrk_x = usb_resetep(m_p_handle,CAMERA_READ_ENDPOINT);

	wrk_x=pv2_bulk_only_mass_storage_reset();
	if (wrk_x<0)
	  {
		Log("Warning: Could not reset device (status:%d)",wrk_x);
		return FALSE;
	  }

	Log("INFO: Requesting challenge from camera");

	camera_write_status = camera_Write(cmd_get_challenge,31,CAM_LTIMEOUT);
	camera_read_status = camera_Read(challenge,141,CAM_STIMEOUT);

	if ( (camera_write_status != 31) || (camera_read_status != 141) )
        {
		Close();
		wrk_x = Init(&wrk_type);
		wrk_x = Open(&wrk_type);
  	        wrk_x = usb_resetep(m_p_handle,CAMERA_WRITE_ENDPOINT);
	        wrk_x = usb_resetep(m_p_handle,CAMERA_READ_ENDPOINT);
		wrk_x=pv2_bulk_only_mass_storage_reset();
		camera_write_status = camera_Write(cmd_get_challenge,31,CAM_LTIMEOUT);
		camera_read_status = camera_Read(challenge,141,CAM_STIMEOUT);
	}

        if ( (camera_write_status == 31) && (camera_read_status == 141) )
	{
		received_challenge = TRUE;
		Log("INFO: Received challenge from camera");

		for(keyused=0;keyused<n_keys;keyused++)	
			{
			strcpy(wrk_char,keydesc[keyused]);
			wrk_x = strlen(wrk_char);
			wrk_char[wrk_x -1] = 0x00;
			Log("INFO: Comparing %s key to challenge",wrk_char);
			if(memcmp(challenge,key[keyused][0],128)==0)
				break;
			}

		if(keyused==n_keys)
		{
			Log("INFO: Challenge not recognized");
			return(FALSE);
		}
                Log("INFO: Challenge matches %s key",wrk_char);
		Log("INFO: Sending response");

		if ((camera_Write(cmd_send_key,31,CAM_STIMEOUT) == 31) && 
		(camera_Write(key[keyused][1],128) == 128) && // Send key
		(camera_Read(status,13,CAM_STIMEOUT) == 13) && // Read status
			(status[12] == 0x00))
		{
			Log("INFO: Succeeded at unlocking camera.");
			if(keyused>2)
				Log("***This key has not been reused before!! Please report this to camerahacking.com!!\n");
			success = TRUE;
		}
		else
		{
			Log( "INFO: Failed to unlock camera.");
		}
	}

	if (success == FALSE)
	{
		if (received_challenge == TRUE)
		{
			Log( "INFO: Key failed for challenge");
			for (i = 0; i < 141; i++)
			{
				Log("%.2X ",challenge[i]);
			}

		}
		else
		{

			Log("INFO: No challenge received.");
			Log("INFO: Will assume camera is permanently unlocked for now....");
			Log("INFO: ...if this is not the case expect the downloads to fail.");
			return TRUE;

		}
	}


	return success;
}

int  pv2_bulk_only_mass_storage_reset()
{
	unsigned char cmd_bomsreset[31] = { 	
	'L','a','M','S',
	0x1d,0xBA,0xAB,0x1D,0x80,0x00,0x00,0x00,0x80,
	0x00,0x00,0x21,0xFF,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // bulk only mass storage reset

	Log("INFO: Sending bulk-only mass storage reset");

	return camera_Write(cmd_bomsreset,31,CAM_LTIMEOUT);

}


