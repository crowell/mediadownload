#include "ops-linux.h"

int ControlMessageRead(u16 command, char *data, int size, int timeout) {
	int x = usb_control_msg(m_p_handle,
			  USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			  
			  0x01,
			  command,
			  0x0000,
			  data,
			  size,
			  timeout );
	if(x < size)
		return FALSE;

	return TRUE;
}

int ControlMessageWrite(u16 command, char * data, int size, int timeout) {
	int x=usb_control_msg(m_p_handle,
			USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			0x01,
			command,
			0x0101,
			data,
			size,
			timeout);
	/*  data can't be made const since usb_control_msg won't handle const char */
	if(x < size)
		return FALSE;

	return TRUE;
}


int Read(char *p_buffer, unsigned int length, int timeout)
{
	int bytes_read = -1;
  
	if (m_p_handle)
		bytes_read = usb_bulk_read(m_p_handle,READ_ENDPOINT,p_buffer,length,timeout);

	return bytes_read;
}

int Write(char * p_buffer, unsigned int length, int timeout)
{
	int bytes_written = -1;

	if (m_p_handle)
		bytes_written = usb_bulk_write(m_p_handle,WRITE_ENDPOINT,p_buffer,length,timeout);

	return bytes_written;
}

int camera_Read( char* p_buffer, unsigned int length, int timeout /* = TIMEOUT */)
{
	int bytes_read = -1;

	if (m_p_handle)
	{
		bytes_read = usb_bulk_read(m_p_handle,CAMERA_READ_ENDPOINT,p_buffer,length,timeout);
	}

	return bytes_read;
}

int camera_Write(char* p_buffer, unsigned int length, int timeout /* = TIMEOUT */)
{
	int bytes_written = -1;

	if (m_p_handle)
	{
		bytes_written = usb_bulk_write(m_p_handle,CAMERA_WRITE_ENDPOINT,p_buffer,length,timeout);						

	}

	return bytes_written;
}

