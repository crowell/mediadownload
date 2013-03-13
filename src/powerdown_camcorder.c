#include "ops-linux.h"

int Powerdown (void) {
	Log( "INFO: Powering down device..." );

	if (ControlMessageWrite (0x1000, NULL, 0, TIMEOUT) == FALSE)
		Log( "WARNING: Failed to powerdown device." );

	return TRUE;
}
