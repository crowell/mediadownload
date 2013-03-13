#include "ops-linux.h"
#include <string.h>
#include <time.h> 

int DownloadAllMovies(int *CAM_TYPE) {
	char logstr[STRINGSIZE];

	char foldername[STRINGSIZE];
	char *ptr;
	char buffer[100];
	
	char time_str[32];
	time_t now;
	struct tm *time_now;

	now = time ( NULL );
	time_now = localtime ( &now );

	const char* delimiter = "/";

	char makedir[STRINGSIZE];

	int wrk_x=0;


	if (*CAM_TYPE == 2)
          {
            sprintf(buffer, "%s%s", getenv("HOME"), SATURNDOWNLOAD);
          }
        else if (*CAM_TYPE == 3)
	  {
            sprintf(buffer, "%s%s", getenv("HOME"), PV2_DOWNLOAD);
          }

	strcpy(foldername, buffer);
	strcat(foldername, delimiter);
	sprintf(buffer, "");
	strftime (buffer, 32, "%Y.%m.%d_%I.%M.%S%p", time_now );
	strcat(foldername, buffer );
	strcat(foldername, delimiter);

 	Log( "INFO: Downloading media to %s", foldername );

	sprintf(buffer, "mkdir %s", foldername);	
	if (*CAM_TYPE == 2)
          {
             Log("INFO: Download from camcorder.....");
             system(buffer);
         }
	if (*CAM_TYPE == 3)
          {
	     strcpy(makedir,buffer);
             Log("INFO: Download from camera.....");
	     wrk_x = pv2_download_pictures(&foldername, &makedir);
	     if ( wrk_x == TRUE)
               {
		   Log("INFO: Downloads completed.");
	           return TRUE;  
               }
             else
               {
                   Log("INFO: Downloads failed....");
                   return FALSE;
               }
          }

	for (ptr = foldername; *ptr; ++ptr);
	int first = TRUE;
	file_info info;
	if ( m_p_handle == NULL )
		return FALSE;
	if ( !ChangePartition(0) ) {
		Log( "INFO: Cannot change to partition 0." );
		return FALSE;
	}
	if ( !ChangeDirectory("/DCIM/100COACH") ) {
		Log( "INFO: Cannot change to directory /DCIM/100COACH." );
		return FALSE;
	}
	while ( GetFileInfo( &info, first )) {
		first = FALSE;
		if ( info.filetype == FIFILE ) {
			strcpy(ptr, info.filename);
			if ( !DownloadFile( foldername, info.filename, info.filesize ) ) {
				Log( "ERROR: Cannot download media: %s", info.filename );
				return FALSE;
			}
		}
	}

	time_now = localtime ( &now );
	Log( "INFO: Successfully downloaded all media!");
	return TRUE;
}


