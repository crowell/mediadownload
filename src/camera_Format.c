#include "ops-linux.h"

int camera_Format(void)
{
	int success = FALSE;

	unsigned char cmd_format[31] =  { 
			'L','a','M','S', 
			0x1d, 0xba, 0xab, 0x1d, // dCBWTag
			0x00, 0x00, 0x00, 0x00, // dCBWDataTransferLength Length
			0x00, // bmCBWFlags Write
			0x01, // bCBWLUN LUN=1 for FLASH FS
			0x00, // bCBWCBLength
			0x19, 0x00, // Command
			0x00, 0x00, 0x00, 0x00, 
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	unsigned char status[13];


	Log( "INFO: Attempting camera format...");

	if (camera_Write(cmd_format,31,CAM_STIMEOUT) == 31)
            Log("INFO: format write okay");
        else
            {
		Log("INFO: format command write failed");
                Log("INFO: FORMAT FAILED");
                return FALSE;
            }
        if (camera_Read(status,13,CAM_LTIMEOUT) == 13)
            Log("INFO: format read status okay");
        else
	    {
                Log("INFO: format read status failed");
                Log("INFO: FORMAT FAILED");
                return FALSE;
            }
        if (status[12] == 0x00)
            {
                Log("INFO: Camera format successful.");
                return TRUE;
            }
        else
            {
                Log("INFO: Camera format failed: &x",status[12]);
                return FALSE;
            }

/*	
	if ( (Write(cmd_format,31,SHORT_TIMEOUT) == 31) &&
		(Read(status,13,LONG_TIMEOUT) == 13) &&	// Read status
				(status[12] == 0x00) ) 
	{
		Log( "INFO: Camera format successful.");
		success = TRUE;
	}
	else
	{
		Log( "Format failed.");
		success = FALSE;
	}
	return success;
*/
}

