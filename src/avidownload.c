#include "ops-linux.h"
#include <stdio.h>



int main(int argc, char *argv[]) {
	int i, pass;
	int format_dev = FALSE;
	int powerdown_dev = TRUE;
	int CAM_TYPE = 0;
	unsigned pause_loop=0;
	unsigned pause_cnt=0;
	int dev_closed=FALSE;
	double wrk_x=0;
	char wrkx_c;

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-f"))
			format_dev = TRUE;

		if (!strcmp(argv[i], "-p"))
			powerdown_dev = FALSE;
	}

	if (format_dev)
		Log( "NOTICE: Will format device after download!" );

	if (!powerdown_dev)
		Log( "NOTICE: Will NOT powerdown device after download!" );


	for (pass = 0; pass < 1; pass++)
	{
		if (Init(&CAM_TYPE) == FALSE)
		{
			Log(" ");
			Log("******************************************************");
			Log("*  ERROR:  Couldn't initialize device                *");
			Log("*                                                    *");
			Log("*  press Enter to close this window                  *");
                        Log("******************************************************");
			wrkx_c = getchar();
			break;
		}

                if(Open(&CAM_TYPE) == FALSE)
		{
			Log(" ");
			Log("******************************************************");
			Log("*  ERROR:  Couldn't open device                      *");
			Log("*                                                    *");
			Log("*  press Enter to close this window                  *");
                        Log("******************************************************");
			wrkx_c = getchar();
			break;
		}
		if (Unlock(&CAM_TYPE) == FALSE)
		{	
			Log(" ");
			Log("******************************************************");
			Log("*  ERROR:  Couldn't unlock device                    *");
			Log("*                                                    *");
			Log("*  press Enter to close this window                  *");
                        Log("******************************************************");
			wrkx_c = getchar();
			break;
		}

                if (CAM_TYPE ==2)
                  {
  		     if (GenerateFileList() == FALSE)
		     {
			Log(" ");
			Log("******************************************************");
			Log("*  ERROR:  Couldn't find any videos                  *");
			Log("*                                                    *");
			Log("*  press Enter to close this window                  *");
                        Log("******************************************************");
			wrkx_c = getchar();
			break;
		     }
		  }

		if (DownloadAllMovies(&CAM_TYPE) == FALSE)
		{
			Log(" ");
			Log("******************************************************");
			Log("*  ERROR:  Failure downloading video/picture         *");
			Log("*                                                    *");
			Log("*  press Enter to close this window                  *");
                        Log("******************************************************");
			wrkx_c = getchar();
			break;
		}
		if (format_dev)
                  {
                     if (CAM_TYPE ==2)
                       {	
			if (Format() == FALSE)
				break;
                       }
                     else if (CAM_TYPE ==3)
                       {
                        if (camera_Format() == FALSE)
                                break;
                       }
                  }
		if ((powerdown_dev) && (CAM_TYPE == 2))
			if (Powerdown() == FALSE)
				break;
		if (CAM_TYPE==3)
		{
		      pv2_close();
		}		      
		if (Close() == FALSE)
		  {
                      Log("Warning: Couldn't close the camcorder....");
		      break;
                  }
		dev_closed=TRUE;
		Log(" ");
		Log("*******************************************************************");
		Log("*                                                                 *");
		if (CAM_TYPE ==2)
		{
			Log("*                Video Downloads Completed                        *");
		}
		else
		{
			Log("*                Picture Downloads Completed                      *");    
		}
		Log("*                                                                 *");
		Log("* Press Any Key To Close This Window.........                     *");
		Log("*                                                                 *");
		Log("*******************************************************************");
		wrkx_c = getchar();
		return 0;

 	}

	Log("Closing devices");
	dev_closed=Close();
	return 1;

}
