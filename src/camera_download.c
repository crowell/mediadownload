/* pv2.c - pv2 picture download and manipulation routines */
/* Copyright 2004,2005 Forkboy, BillW, Drmn4ea. Licensed under the GPL */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include "ops-linux.h"


/**** Official API functions ****/
int  pv2_clear_cam(void);
int  pv2_download_pictures(char *foldername, char * makedir);


/**** unofficial local functions ****/
int  pv2_getpiclist(void); //fills the fileinfo structures in the finfos array with the info for Raws on the cam
int  pv2_readsector(unsigned int sector, unsigned char* buffer); //read a sector from flash
int  pv2_changedirectory(char *directory);
int  pv2_download_picture(char *filename,int length,unsigned char *buffer);
void pv2_log(char *logmessage, ...); 						// **better to use a global function
int  pv2_bulkwrite(unsigned char* p_buffer, unsigned int length, int timeout); 	// **better to use a global function
int  pv2_bulkread(unsigned char* p_buffer, unsigned int length, int timeout);  	// **better to use a global function
int  pv2_savetofile(unsigned char *filename,int size,unsigned char *buffer);  // **better to use a global function


typedef struct
{
	char picname[1024];
	int filesize;
} fileinfo;

fileinfo finfos[256];

//our module's global pointer to the dev handed to us from the first call of the pv2_open function.
struct usb_device* pv2_dev=NULL; 


int pv2_download_pictures(char *foldername, char *makedir)
{
	int t,good,bad;
	unsigned char *buffer;
	char value[1024];
	char savedirectory[1024]; 
	int processraws;
	int deleteraws;
	int i;
	int cout;

	i = system("convert *.ppm *.jpg");

	strcpy(savedirectory, foldername);
	for(t=0;t<256;t++)
		finfos[t].picname[0]=0; //make sure it's a zero terminated list

	//First make sure we're in the right directory...
	Log("INFO: Checking directory location..");

 	if(pv2_changedirectory("/")==FALSE)
          {
		Log("ERROR: Can't CD to root folder");
		return(FALSE);
          }
	if(pv2_changedirectory("DCIM")==FALSE)
          {
		Log("ERROR: Can't CD to DCIM folder");
		return(FALSE);
          }
	if(pv2_changedirectory("RAW")==FALSE)
          {
		Log("ERROR: Can't CD to RAW folder");
		return(FALSE);
          }
	Log("INFO: Checking picture count..");

	//TODO: Get FAT and figure out what the names of the RAW files are
	if(pv2_getpiclist()==0)
          {
                Log("WARNING:  No pictures to download..");
		return(1);
          }

        system(makedir);

	buffer=(unsigned char *)malloc(1024); //just priming the pump for the realloc()
	if(buffer==NULL)
	{
		Log("ERROR: Couldn't allocate memory for picture download");
		return(FALSE);
	}


	good=0; bad=0;
	for(t=0;finfos[t].picname[0]!=0;t++)
	{
		buffer=(unsigned char *)realloc(buffer,finfos[t].filesize);
		if(buffer==NULL)
		{
			Log("ERROR: Couldn't allocate memory for download of picture %s",finfos[t].picname);
			return(FALSE);
		}
		if(pv2_download_picture(finfos[t].picname,finfos[t].filesize,buffer)!=TRUE)
		{
			bad++;
			Log("ERROR: Trouble downloading picture %s",finfos[t].picname);
			//we try to continue, even if we can't download this particular pic.
		}
		else
		{
			//TODO: Here's where we can do any picture processing and conversion!!!

			//buffer = in-memory RAW file
			//finfos[t].filesize = size of in-memory RAW
			//finfos[t].picname = original picture name
			if(savedirectory[0]!=0)
			{
				snprintf(value,1024,"%s%s",savedirectory,finfos[t].picname);
				value[1023]=0;
				strcpy(finfos[t].picname,value);
			}

			if(pv2_savetofile(finfos[t].picname,finfos[t].filesize,buffer)==TRUE)
			{
				good++;
				processraws=1;

				if(processraws==1)
					{
					dcraw2jpg(finfos[t].picname);
					chdir(savedirectory);
					i = system("convert *.ppm pv2.jpg");
					
//					if(deleteraws==1)
//						DeleteFile(finfos[t].picname);
					}
			}
			else
				bad++;
		}
	}
	free(buffer);
	if((good>0)&&(bad>0))
	{
		Log("Warning: only some files were downloaded.");
		return(1); //gotta return failure so the pictures aren't erased.
	}
	if(bad>0)
	{
		Log("Warning: No files were retrieved.");
		return(1);
	}
	if(good>0)
	{
		Log("INFO: All pictures downloaded successfuly");
		return(TRUE);
	}
	return(1);
}


int pv2_clear_cam()
{
	unsigned char cmd_format[31] =  { 
			'L','a','M','S', 
			0x1d, 0xba, 0xab, 0x1d, /* dCBWTag */
			0x00, 0x00, 0x00, 0x00, /* dCBWDataTransferLength Length */
			0x00, /* bmCBWFlags Write */
			0x01, /* bCBWLUN LUN=1 for FLASH FS */
			0x00, /* bCBWCBLength */
			0x19, 0x00, /* Command */
			0x00, 0x00, 0x00, 0x00, 
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	unsigned char status[13];

	Log( "INFO: Formatting storage media...");
	if ( (pv2_bulkwrite(cmd_format,31,SHORT_TIMEOUT) == 31) &&
	(pv2_bulkread(status,13,LONG_TIMEOUT) == 13) &&	// Read status
				(status[12] == 0x00) ) 
	{
		Log( "INFO: Format successful.");
		return(0);
	}

	Log( "INFO: Format failed.");
	return(1);
}


/**** unofficial local functions ****/

int pv2_bulkread(unsigned char* p_buffer, unsigned int length, int timeout)
{
	int bytes_read = -1;

	if (m_p_handle)
	{
		bytes_read = usb_bulk_read(m_p_handle,CAMERA_READ_ENDPOINT,(char*)p_buffer,length,timeout);
	}

	return bytes_read;
}

int pv2_bulkwrite(unsigned char* p_buffer, unsigned int length, int timeout)
{
	int bytes_written = -1;

	if (m_p_handle)
	{
		bytes_written = usb_bulk_write(m_p_handle,CAMERA_WRITE_ENDPOINT,(char*)p_buffer,length,timeout);						

	}

	return bytes_written;
}


int pv2_download_picture(char *filename,int length,unsigned char* buffer)
{
	//this function assumes that you've already changed the directory to "/DCIM/RAW"

	unsigned char cmd_downloadfile [31] = {		
		'L','a','M','S', 
		0x1d, 0xba, 0xab, 0x1d, // dCBWTag
		0x00, 0x00, 0x00, 0x00, // dCBWDataTransferLength Length
		0x80, // bmCBWFlags Read
		0x01, // bCBWLUN LUN=1 for FLASH FS
		0x00, // bCBWCBLength
		0x54, 0x00, // Command
		0x00, 0x00, 0x00, 0x00, 
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char status[13];
	unsigned char logstr[2048];


	//Put the filename in the command...
	strncpy((char *)cmd_downloadfile+17,filename,12);

	//Put the length in the command
	*(int*)&cmd_downloadfile[8]=length;

	// then lets try to read it
	if ( (pv2_bulkwrite(cmd_downloadfile,sizeof(cmd_downloadfile),CAM_STIMEOUT) == 31) &&
		(pv2_bulkread(buffer,length,CAM_STIMEOUT)==length) && 
		(pv2_bulkread(status,13,CAM_STIMEOUT)==13) && (status[12]==0x00) )
	{
		Log("INFO: Succeeded at downloading %s",filename);

		return(TRUE);
	}
	Log("ERROR: Failed at downloading %s",filename);
	return(FALSE);
}

int pv2_changedirectory(char *directory)
{

	unsigned char cmd_cd[31] =  {
		// Change directory
		'L','a','M','S', 
		0x1d, 0xba, 0xab, 0x1d, // dCBWTag
		0x00, 0x00, 0x00, 0x00, // dCBWDataTransferLength Length
		0x00, // bmCBWFlags Write
		0x01, // bCBWLUN LUN=1 for FLASH FS
		0x00, // bCBWCBLength
		0x58, 0x00, // Command
		0x00, 0x00, 0x00, 0x00, 
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	unsigned char status[13];

	strcpy((char *)cmd_cd+17,directory);

	if (pv2_bulkwrite(cmd_cd,31,SHORT_TIMEOUT) == 31) // Send cd command
	{
		if ( (pv2_bulkread(status,13,SHORT_TIMEOUT) == 13) &&	// Read status
				(status[12] == 0x00)) 
		{

			return(TRUE);
		}
	}
	Log("ERROR: Failed during change directory to %s",directory);
	return(FALSE);

}

int pv2_readsector(unsigned int sector, unsigned char* buffer)
{
	//Read a raw sector from the flash. We need this to get the FAT.

	unsigned char cmd[31] = {'L','a','M','S',0x1d,0xBA,0xAB,0x1D,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x52,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // Endpoint 80
	unsigned char status[13];

	*(int*)&cmd[8] = 512; // Size
	*(int*)&cmd[17] = sector; // Start

	if ((pv2_bulkwrite(cmd,31,TIMEOUT) == 31) &&		// Send command
	(pv2_bulkread(buffer,512,TIMEOUT) == 512) &&		// Read block
	(pv2_bulkread(status,13,TIMEOUT) == 13) &&		// Read status
	(status[12] == 0x00))
	{
		return TRUE;
	}
	return FALSE;
}

int pv2_getpiclist()
{
	DirEntry dir[1024];
	int i,j,k,n,t;
	char *p;
	int entry=0;
	int fatsector,sect;
	char filename[9],suffix[4];
	int recurselevel=0;

	memset(filename,0,sizeof(filename));
	memset(suffix,0,sizeof(suffix));

	//This command follows the MBR to the FAT and attempts to find the names
	if (pv2_readsector(0,(unsigned char*)&g_MBR))
	{
		if (pv2_readsector(g_MBR.Partiton[0].RelativeSectors,(unsigned char*)&g_Boot))
		{
			for(t=0;t<g_Boot.FATSize;t++)
			{
				pv2_readsector(g_MBR.Partiton[0].RelativeSectors+1+t,&g_fat[t*512]);
			}


			fatsector=g_MBR.Partiton[0].RelativeSectors+1+(2*g_Boot.FATSize);
			for(sect=0;sect<128;sect++)
			{
				if (pv2_readsector(sect+fatsector,(unsigned char*)dir))
				{
					for (i=0;i<(512/32);i++)
					{
						strncpy(filename,(char*)dir[i].Name,8);
						strncpy(suffix,(char*)dir[i].Ext,3);

						//fprintf(stderr,"Studying %s.%s\n",filename,suffix); //debug

						if (filename[0] == '.')
							continue;

						if((dir[i].Attributes.VolumeID)&&
							(dir[i].Attributes.System)&&
							(dir[i].Attributes.Hidden)&&
							(dir[i].Attributes.ReadOnly))
							continue; //It's a long-filename entry

						if(filename[0]==0xe5)
							continue; //it's a deleted file

						if (filename[0] == 0)
						{
							//fprintf(stderr,"zero filename\n");//debug
							sect=256; //break outer loop
							break;
						}


						p = (char*)dir[i].Name;

						//find our way into /DCIM/RAW following the FAT
						if (dir[i].Attributes.Directory)
						{
							if ( ((strcmp(filename,"DCIM    ")==0)&&(recurselevel==0)) ||
							((strcmp(filename,"RAW     ")==0)&&(recurselevel==1)) )
							{
								recurselevel++;
															fatsector=(((dir[i].EntryCluster-2)*32*512)+0xC000)/512;
								sect=-1; //will become 0 after break;
								break;
							}
						}

						// Check for illegal filenames
						for (j = 0; j < 11; j++)
						{
							if (strrchr("ABCDEFGHIJKLMNOPQRSTUVWXYZZ01234567899_^$~!#%&-{}()@'` ",*p++) == NULL)
								break;
						}
						if(j<11)
							continue; //it's a malformed filename.  Skip it.

						//Check if it's a RAW
						if(strncmp(suffix,"RAW",3)!=0) //not a RAW. Skip it.
							continue;

						//It's a RAW. Go ahead and log the name
						sprintf(finfos[entry].picname,"%s.%s",filename,suffix);
						finfos[entry].filesize=dir[i].Size;
						entry++;
					}

				}
			}
		}

	}
	else
	{
		Log("ERROR: Read FAT failed.");
		return(FALSE);
	}
	if(entry==0)
		Log("Warning: FAT scan succeeded but found 0 picture files");
	else
		Log("INFO: Found %d pictures",entry);
	return(entry);
}

int pv2_savetofile(unsigned char *filename,int size,unsigned char *buffer)
{
	FILE *out, *in;
	int written;
        int t;

	static unsigned char newfilename[1024],*dot;


	in=fopen(filename,"rb"); //check if the file already exists
	if(in!=NULL)
	{
		fclose(in);
		dot=strrchr(filename,'.');
		if(dot==NULL)
			return(FALSE);
		*dot=0;
		dot++;
		for(t=1;t<1000;t++)
		{
			snprintf(newfilename,1023,"%s(%04d).%s",filename,t,dot);
			in=fopen(newfilename,"rb");
			if(in==NULL)
				break;
			fclose(in);
		}
		strncpy(filename,newfilename,1024);
	}
	out=fopen(filename,"wb");
	if(out==NULL)
	{
		Log("ERROR: Couldn't create file %s",filename);
		return(FALSE);
	}
	written=fwrite(buffer,1,size,out);
	fclose(out);
	if(written<size)
	{
		Log("ERROR: Couldn't write to file %s",filename);
		return(FALSE);
	}
	Log("INFO: Wrote %s",filename);
	return(TRUE);
}


void pv2_close()
{
	//The pv2 doesn't like to be closed and reopened for some reason,
	// so we send the camera a "reset program" instead.

	unsigned char cmd_upload [31] = {'L','a','M','S',
                0x1d, 0xba, 0xab, 0x1d, // dCBWTag
                0x03, 0x00, 0x00, 0x00, // dCBWDataTransferLength Length
                0x00, 0x00, 0x00, // bCBWCBLength
                0x26, 0x00,	// Command
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00};

	unsigned char resetcode [3] = { 0xc8, 0x15, 0xf7 };

	pv2_bulkwrite(cmd_upload,sizeof(cmd_upload),SHORT_TIMEOUT);
	pv2_bulkwrite(resetcode,3,SHORT_TIMEOUT);
	return;

}

