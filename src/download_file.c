#include "ops-linux.h"
#include <time.h>

int DownloadFile(char* saveto, char* filename, int size) {
	int count, round = 0, rate = 1, seconds = 0;	
	FILE* file = NULL;
	char sfilename[256];
	char buffer[BUFSIZE];
	float dwe_sofar=0;
	float dwe_done=0;

	time_t now, saved;

	double dif;

	Log( "INFO: Downloading %s to %s ...", filename, saveto );
	strcpy(sfilename, filename);
	file = fopen(saveto, "wb");
	if (file == NULL) {
		Log( "ERROR: Trouble opening %s", saveto );
		return FALSE;
	}

	memset(sfilename, '\0', sizeof sfilename);
	strcpy(sfilename,filename);

	if (ControlMessageWrite(0xb101, sfilename, strlen(sfilename) + 1, TIMEOUT) == FALSE)
		return FALSE;
	if (ControlMessageWrite(0x9301, NULL, 0, TIMEOUT) == FALSE)
		return FALSE;

        time ( &saved );

	while (TRUE) {
		round++;

	        time ( &now );

		dif = difftime (now, saved);

		if ((int)dif >= 1) {
			seconds = seconds + 1;
			rate =  ( ( round * 4 ) - rate ) / seconds;
		        time ( &saved );
		}		

		count = Read(buffer, LINUX_BUFSIZE, TIMEOUT);
		fwrite(buffer, 1, count, file);

		if (count < LINUX_BUFSIZE) {
			Log( "\nINFO: Success retrieving file %s", filename );
			break;
		}

		printf( "\r \r" );
		fflush(stdout);
		dwe_sofar = dwe_sofar + count;
		dwe_done = (dwe_sofar / size) * 100;
		printf ( "INFO: Downloaded %000000.fKB of %dKB (%000.f%% complete)...",(dwe_sofar / 1024), size / 1024, dwe_done );
		fflush(stdout);
	}

	fclose(file);

	return TRUE;
}

