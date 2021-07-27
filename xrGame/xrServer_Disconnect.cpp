#include "stdafx.h"
#include "xrServer.h"

void xrServer::Disconnect()
{
	if (m_file_transfers)
	{
		deinitialize_screenshot_proxies();
		xr_delete(m_file_transfers);
	}

	IPureServer::Disconnect();
	SLS_Clear();
	xr_delete(game);
}
