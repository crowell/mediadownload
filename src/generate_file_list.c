#include "ops-linux.h"
#include <string.h>

int GenerateFileList(void) {
	char logstr[STRINGSIZE];

	char filename[STRINGSIZE];
	char *ptr;
	for (ptr = filename; *ptr; ++ptr);
	int first = TRUE;
	file_info info;
    
        Log("INFO: Generating File List....");

	if ( m_p_handle == NULL )
		return FALSE;
	if ( !ChangePartition(0) ) {
		Log( "INFO: Cannot change to partition 0." );
		return FALSE;
	}

	if ( !ChangeDirectory( "/DCIM/100COACH" ) ) {
		Log( "INFO: Cannot change to directory /DCIM/100COACH." );
		return FALSE;
	}

	Log( "INFO: Searching for media..." );
	while ( GetFileInfo( &info, first )) {
		first = FALSE;
		if ( info.filetype == FIFILE ) {
			strcpy(ptr, info.filename);

			if ( !strcmp( "NO_NAME", info.filename) ) {
				Log( "ERROR: Device appears to be empty..." );
				return FALSE;
			}
			Log( "INFO: Media found: %s (%.0fKB)", info.filename, (float)info.filesize/1024);
		}
	}
	return TRUE;
}
