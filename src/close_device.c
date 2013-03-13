#include "ops-linux.h"

int Close (void) {
	Log( "INFO: Closing any opened devices..." );
	if (m_usb_device) {
		if (m_p_handle) {
			usb_release_interface (m_p_handle, DEFAULT_INTERFACE);
			usb_close (m_p_handle);
			m_p_handle = NULL;
		}
	}
	Log ( "INFO: Open devices closed." );

	return TRUE;
}
