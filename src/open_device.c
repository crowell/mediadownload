#include "ops-linux.h"
#include <string.h>

static unsigned short m_vendor_id;
static unsigned short m_product_id;
static char m_manufacturer[STRINGSIZE];
static char m_product[STRINGSIZE];
struct usb_device* m_usb_device;

#define Log(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)


int Open (int *CAM_TYPE) {
	int success = FALSE;
	int wrk_x=0;
  
	if (m_usb_device) {

		if (m_p_handle)
		  {
			usb_release_interface (m_p_handle, DEFAULT_INTERFACE);
			usb_close (m_p_handle);
			m_p_handle = NULL;
		  }

		
		m_p_handle = usb_open (m_usb_device);


   
		if (m_p_handle)
		{
			wrk_x = usb_set_configuration (m_p_handle, DEFAULT_CONFIGURATION);
			if (wrk_x !=0)
			{ 
			     Log("Warning: Unable to set usb configuration (%d)",wrk_x);
                             return FALSE;
                        }
                        wrk_x =usb_claim_interface (m_p_handle, DEFAULT_INTERFACE);
                        if (wrk_x !=0)
                        {
                             Log("Warning: Unable to claim usb interface (%d)",wrk_x);
                             return FALSE;
                        }
			if (*CAM_TYPE==2)
 			{
			      if (usb_set_altinterface (m_p_handle, DEFAULT_ALT_INTERFACE) >= 0)
			      {
			  	   Log( "INFO: Device successfully opened." );
				   return TRUE;
                              }
 			
			      else 
                              {
				usb_release_interface (m_p_handle, DEFAULT_INTERFACE);
				usb_close (m_p_handle);
				Log( "ERROR: Device could not be opened. Unplug and try again." );
				return FALSE;
			      }
                        }
                        Log("INFO: USB device configured and interface claimed");
			return TRUE;
			}
		else
                  {
			Log("Warning: Unable to open usb for m_p_handle (%s)",m_p_handle);
			return FALSE;
                  }
	}
	Log("Warning: No usb device.....(%s)",m_usb_device);
	return FALSE;
}

static void reset_values(void) {
	m_usb_device = NULL;
	m_p_handle = NULL;
}


int Init (int *CAM_TYPE) {
	int success = FALSE;
	char tmp[256];
	int bus_no = 0;
	struct usb_bus *p_bus = NULL;

	reset_values();  
	usb_init ();
  
	if (usb_find_busses () < 0)
		Log ( "ERROR: Couldn't find USB bus." );
  
	if (usb_find_devices () < 0) 
		Log ( "ERROR: Couldn't find any USB devices." );

	Close ();
		  

/*	m_usb_device = NULL;
*/
  
	for (p_bus = usb_get_busses(); p_bus != NULL; p_bus = p_bus->next, ++bus_no) {
		struct usb_device *p_device;// = p_bus->devices;
		Log( "INFO: Scanning USB bus %d ...", bus_no );

		for (p_device = p_bus->devices; p_device != NULL; p_device = p_device->next) {
			if (p_device->descriptor.idVendor != 0 && p_device->descriptor.idProduct != 0)
				Log( "INFO: USB device with VID: 0x%04x, PID: 0x%04x on bus %d", p_device->descriptor.idVendor, p_device->descriptor.idProduct, bus_no );

			if (p_device->descriptor.idVendor == VENDOROLD)
			{
				Log( "ERROR: Unsupported testmarket camcorder found. Exiting..." );
				return FALSE;
			}

			if ((p_device->descriptor.idVendor == CAMCORDER_VENDOR)||(p_device->descriptor.idVendor == CAMERA_VENDOR))
			{
				usb_dev_handle *udev = usb_open (p_device);
				if (udev) {
					m_usb_device = p_device;

					m_vendor_id = p_device->descriptor.idVendor;
					m_product_id = p_device->descriptor.idProduct;

					if (p_device->descriptor.iManufacturer)
						if (usb_get_string_simple (udev, p_device->descriptor.iManufacturer, tmp, sizeof (tmp)) > 0)
							strncpy (m_manufacturer, tmp, STRINGSIZE - 1);

					if (p_device->descriptor.idProduct)
						if (usb_get_string_simple (udev, p_device->descriptor.idProduct, tmp, sizeof (tmp)) > 0)
							strncpy (m_product, tmp, STRINGSIZE - 1);

					Log( "INFO: Device found: %s %s VID: 0x%.4X, PID: 0x%.4X", m_manufacturer, m_product, m_vendor_id, m_product_id );
	  				success = TRUE; 
					if (p_device->descriptor.idVendor == CAMCORDER_VENDOR)
					   {
						Log("INFO:  Device is a camcorder");
						*CAM_TYPE=2;
						return TRUE;
					   }
					else if (p_device->descriptor.idVendor == CAMERA_VENDOR)
					   {
						Log("INFO:  Device is a camera");
						*CAM_TYPE=3;
						return TRUE;
					   }
					
					break;
				} else {
					Log ( "ERROR: Found the device but failed to open. Unplug the device and try again" );
					return FALSE;
				}

 			}
		}

/*		if (m_usb_device)
			return TRUE;
*/
	}
	
	return FALSE;
}
