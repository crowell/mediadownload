#include "ops-linux.h"

static int WaitMediaStatus(void) {
	return TRUE;	//can't seem to get this to ensure media is available
}

typedef struct cvs_dir_entry {
	unsigned char  unknown1[4];  //  0  if 1st byte 0x0f, no more files
	char           filename[12]; //  4  MS-DOS style, but with explicit '.'
	unsigned char  filetype;     // 16  0x00==regular, 0x77==directory
	unsigned char  unknown2;     // 17
	unsigned char  fileattr;     // 18  0x20==regular, 0x10==directory
	unsigned char  unknown3;     // 19
	unsigned       filesize;     // 20  little-endian
	unsigned short modtime;      // 24  crazy DOS time
	unsigned short moddate;      // 26  crazy DOS date
		                     // 28  (this is a 28-byte struct)
} cvs_dir_entry;


int GetFileInfo(file_info* thisfileinfo, int isfirstfile) {
	cvs_dir_entry data;
	int dummy,command,s,t;
	memset(&data, 0, sizeof data);

	if(isfirstfile==TRUE)
		command=0xbc00; //first file
	else
		if(isfirstfile==FALSE)
			command=0xbd00; //next file
  
	//sometimes the first-file read fails after a partition change
	for( t = 0; t < 2; t++ ) {
		if (ControlMessageWrite(command,(char*)&dummy,0,TIMEOUT)==FALSE) {
			Log( "ERROR: Failed at 0xbc (requesting first file info).");
			return FALSE;
		}
		if ( ( (s=Read((char*)&data,28,TIMEOUT) ) ) < 28 ) {
			if ( isfirstfile == FALSE )
				break;
			Log( "ERROR: Failed to bulk read first file info...not retrying" );
			continue;
		}
		break;
	}

	if ( s != 28 ) {
		Log( "ERROR: failed to bulk read file info" );
		return FALSE;
	}

	if ( data.unknown1[0] == 0x0f ) //we've already read the last file
		return FALSE;

	// filename can, oddly, be padded with 0xff
	char *cp = memchr(data.filename, 0xff, sizeof data.filename);
	if ( cp )
		*cp = '\0';
	memcpy(thisfileinfo->filename, data.filename, sizeof data.filename);
	thisfileinfo->filename[sizeof data.filename] = '\0';

	thisfileinfo->filesize = le32_to_cpu(data.filesize);

	switch ( data.fileattr ) {
		case 0x10:
			thisfileinfo->filetype = FIDIR;
			break;
		default:
			Log( "WARNING: unknown file type: 0x%02x attr 0x%02x", data.filetype, data.fileattr);
		case 0x20:
			thisfileinfo->filetype = FIFILE;
			break;
	}

  	return TRUE;
}


int ChangePartition(unsigned int partition) {

	int dummy=0x00;
	unsigned int cmd;
  
	if ( (partition == 1) || (partition > 4) ) {
		Log( "ERROR: Refusing to change to non-filesystem partition.");
		return FALSE;
	}

	cmd=0xbf00 | partition;
  
	ControlMessageWrite(cmd,(char*)&dummy,4,SHORT_TIMEOUT); // This cmd always fails!!

	if(WaitMediaStatus()==FALSE)
		return FALSE;
  
	return TRUE;
}


static int rTrim(char * c, const char e) {  // erase all chars of value e to the right of c
	char* d;
	if (strlen(c) == 0)
		return TRUE;
	d = c;
	while (*d++);
	--d;
	while (d != c) {
		if (e == *d)
			*d = '\0';
		else
			return TRUE;
	}
	return TRUE;
}


int ChangeDirectory(const char* d) {
	char data[LIBUSB_PATH_MAX];
	int c;
	char directory_p[LIBUSB_PATH_MAX], *directory;
	directory = directory_p;
	strncpy(directory, d, LIBUSB_PATH_MAX - 1);
  
	// the following breaks the CD into seperate directory paths...
	// I found the complex path support in the camcorder a bit flakey.
	if((strlen(directory) > 1) && (directory[0] == '/')) {
		if(ChangeDirectory("/")==FALSE)
			return FALSE;

		directory++;
		rTrim(directory, '/');    
	}
  

	while(strlen(directory) > 1 && directory[0]=='/' ) { //FIXME
		char temp[LIBUSB_PATH_MAX];
		strncpy(temp, directory, LIBUSB_PATH_MAX - 1);
		if (directory[0]=='/')
			directory[0] = '\0';    
    
		if(ChangeDirectory(temp)==FALSE) {
			Log( "ERROR: Failed to change directory to parent directory %s", temp );
			return FALSE;
		}

		strncpy(temp, directory, LIBUSB_PATH_MAX - 1);
		directory = temp + 1;
		rTrim(directory, '/');
	}
  
	// We're at a single level directory at this point... lets validate it
	if(strlen(directory) == 0)
		return TRUE;
	
	c = directory[0];
	if(! ( ((c>='a')&&(c<='z')) ||  ((c>='A')&&(c<='Z'))  || ((c>='0')&&(c<='9')) || (c=='_') || c=='/') ) {
		Log("ERROR: Directory formatting abnormal.");
		// the camcorder's CD command created directories if they're not
		// there... best to be prudent and assume it's some kind of error.
		return FALSE;
	}
  
	// validation is done... we can procede with the actual command stuff.
	memset(&data,0x00,LIBUSB_PATH_MAX);
	strcpy(data,directory);
  
	if(ControlMessageWrite(0xb800,data, strlen(data)+1,TIMEOUT)==TRUE)
		return TRUE;

	Log ( "ERROR: Change directory to %s failed", directory );
	return FALSE;
}
