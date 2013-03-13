#include "ops-linux.h"

int Format(void) {
	int data = 0x0000;
	if (ControlMessageWrite(0xb700, (char*)&data, 0, LONG_TIMEOUT)== TRUE) {
		Log( "INFO: Media deleted from the device." );
		return TRUE;
	}
	Log( "INFO: Device format failed...try deleting the media manually." );
	return FALSE;  
}
