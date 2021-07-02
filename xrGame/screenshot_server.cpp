#include "stdafx.h"
#include "screenshot_server.h"
#include "xrMessages.h"
#include "Level.h"
#include "xrServer.h"
#include "game_sv_base.h"
#include "game_cl_mp.h"
#include "../xrCore/fastdelegate.h"

BOOL g_sv_mp_save_proxy_screenshots = 1;
BOOL g_sv_mp_save_proxy_configs = 1;

clientdata_proxy::clientdata_proxy(file_transfer::server_site *ft_server) : m_ft_server(ft_server) 
{
}

clientdata_proxy::~clientdata_proxy()
{
	if (is_active())
	{
		if (m_ft_server->is_receiving_active(m_chearer_id))
			m_ft_server->stop_receive_file(m_chearer_id);
		if (m_ft_server->is_transfer_active(m_admin_id, m_chearer_id))
			m_ft_server->stop_transfer_file(std::make_pair(m_admin_id, m_chearer_id));
	}
}

void clientdata_proxy::make_screenshot(ClientID const &admin_id, ClientID const &cheater_id)
{
	m_admin_id = admin_id;
	m_chearer_id = cheater_id;

	xrClientData *tmp_cheater = static_cast<xrClientData *>(Level().Server->ID_to_client(m_chearer_id));
	
	if (!tmp_cheater)
	{
		Msg("! ERROR: SV: client [%u] not found ...", cheater_id.value());
		return;
	}
	
	if (m_ft_server->is_receiving_active(cheater_id))
	{
		Msg("! Receiving from client [%u] already active, please try later", cheater_id.value());
		return;
	}

	m_cheater_digest = "TSMP: here should be digest"; //tmp_cheater->m_cdkey_digest;
	m_cheater_name = tmp_cheater->ps ? tmp_cheater->ps->getName() : "unknown";

	NET_Packet ssr_packet;
	ssr_packet.w_begin(M_GAMEMESSAGE);
	ssr_packet.w_u32(GAME_EVENT_MAKE_DATA);
	ssr_packet.w_u8(e_screenshot_request); //make screenshot

	//alligning size to GAME_EVENT_PLAYER_KILLED message size
	ssr_packet.w_u16(u16(Random.randI(2))); //food for thought for crackers :)
	ssr_packet.w_u16(u16(Random.randI(2)));
	ssr_packet.w_u16(u16(Random.randI(2)));
	ssr_packet.w_u8(u8(Random.randI(2)));

	//Level().Server->SecureSendTo(tmp_cheater, ssr_packet, net_flags(TRUE, TRUE));
	Level().Server->SendTo(cheater_id, ssr_packet, net_flags(TRUE, TRUE));

	file_transfer::receiving_state_callback_t receiving_cb = fastdelegate::MakeDelegate(this, &clientdata_proxy::download_screenshot_callback);
	
	if (my_proxy_mem_file.size())
		my_proxy_mem_file.clear();

	m_first_receive = true;
	m_receiver = m_ft_server->start_receive_file(my_proxy_mem_file, m_chearer_id, receiving_cb);
}

void clientdata_proxy::make_config_dump(ClientID const &admin_id, ClientID const &cheater_id)
{
	m_admin_id = admin_id;
	m_chearer_id = cheater_id;

	xrClientData *tmp_cheater = static_cast<xrClientData *>(Level().Server->ID_to_client(m_chearer_id));
	
	if (!tmp_cheater)
	{
		Msg("! ERROR: SV: client [%u] not found ...", cheater_id.value());
		return;	
	}

	if (m_ft_server->is_receiving_active(cheater_id))
	{
		Msg("! Receiving from client [%u] already active, please try later", cheater_id.value());
		return;
	}

	m_cheater_digest = "TSMP: here must be digest"; //tmp_cheater->m_cdkey_digest;
	m_cheater_name = tmp_cheater->ps ? tmp_cheater->ps->getName() : "unknown";

	NET_Packet ssr_packet;
	ssr_packet.w_begin(M_GAMEMESSAGE);
	ssr_packet.w_u32(GAME_EVENT_MAKE_DATA);
	ssr_packet.w_u8(e_configs_request); //make screenshot

	//alligning size to GAME_EVENT_PLAYER_KILLED message size
	ssr_packet.w_u16(u16(Random.randI(2))); //food for thought for crackers :)
	ssr_packet.w_u16(u16(Random.randI(2)));
	ssr_packet.w_u16(u16(Random.randI(2)));
	ssr_packet.w_u8(u8(Random.randI(2)));

	//Level().Server->SecureSendTo(tmp_cheater, ssr_packet, net_flags(TRUE, TRUE));
	Level().Server->SendTo(cheater_id, ssr_packet, net_flags(TRUE, TRUE));

	file_transfer::receiving_state_callback_t receiving_cb = fastdelegate::MakeDelegate(this, &clientdata_proxy::download_config_callback);
	
	if (my_proxy_mem_file.size())
		my_proxy_mem_file.clear();

	m_first_receive = true;
	m_receiver = m_ft_server->start_receive_file(my_proxy_mem_file, m_chearer_id, receiving_cb);
}

bool clientdata_proxy::is_active()
{
	return (m_ft_server->is_receiving_active(m_chearer_id) || m_ft_server->is_transfer_active(m_admin_id, m_chearer_id));
}

void clientdata_proxy::notify_admin(clientdata_event_t event_for_admin, char const *reason)
{
	NET_Packet ssr_packet;
	ssr_packet.w_begin(M_GAMEMESSAGE);
	ssr_packet.w_u32(GAME_EVENT_MAKE_DATA);
	ssr_packet.w_u8(static_cast<u8>(event_for_admin)); //receive data
	ssr_packet.w_u32(m_chearer_id.value());

	if ((event_for_admin == e_screenshot_response) || (event_for_admin == e_configs_response))	
		ssr_packet.w_stringZ(m_cheater_name);	
	else	
		ssr_packet.w_stringZ(reason ? reason : "failed to download screenshot");
	
	Level().Server->SendTo(m_admin_id, ssr_packet, net_flags(TRUE, TRUE));
}

LPCSTR make_file_name(LPCSTR session_id, string_path& dest)
{
	strcpy_s(dest, sizeof(dest), session_id);
	static const char* denied_symbols = "/\\?%%*:|\"<>.";
	size_t tmp_length = xr_strlen(dest);
	size_t start_pos = 0;
	size_t char_pos;

	while ((char_pos = strcspn(dest + start_pos, denied_symbols)) < (tmp_length - start_pos))
	{
		char_pos += start_pos;
		dest[char_pos] = '_';
		++start_pos;
	}
	return dest;
}

void generate_file_name(
	string_path& file_name,
	LPCSTR file_suffix,
	SYSTEMTIME const& date_time)
{
	sprintf(
		file_name,
		"%02d%02d%02d-%02d%02d%02d_%s",
		date_time.wYear % 100,
		date_time.wMonth,
		date_time.wDay,
		date_time.wHour,
		date_time.wMinute,
		date_time.wSecond,
		file_suffix
	);
}

void clientdata_proxy::save_proxy_screenshot()
{
	game_cl_mp *clgame = smart_cast<game_cl_mp *>(Level().game);
	if (!clgame)
		return;

	string_path screenshot_fn;
	string_path str_digest;

	string128 dest_file_name;
	strcpy(dest_file_name, make_file_name(m_cheater_name.c_str(), screenshot_fn));
	strcat(dest_file_name, "_");
	strcat(dest_file_name, (m_cheater_digest.size() ? make_file_name(m_cheater_digest.c_str(), str_digest) : "nulldigest"));

	SYSTEMTIME date_time;
	GetLocalTime(&date_time);
	generate_file_name(screenshot_fn, dest_file_name, date_time);

	clgame->decompress_and_save_screenshot(
		screenshot_fn,
		my_proxy_mem_file.pointer(),
		my_proxy_mem_file.size(),
		m_receiver->get_user_param());
}

void clientdata_proxy::save_proxy_config()
{
	game_cl_mp *clgame = smart_cast<game_cl_mp *>(Level().game);
	if (!clgame)
		return;

	string_path config_fn;	
	string_path dest_file_name;

	string128 fn_suffix;
	strcpy(fn_suffix, make_file_name(m_cheater_name.c_str(), config_fn));
	strcat(fn_suffix, ".cltx");

	SYSTEMTIME date_time;
	GetLocalTime(&date_time);
	generate_file_name(dest_file_name, fn_suffix, date_time);
	IWriter *tmp_writer = FS.w_open("$screenshots$", dest_file_name);
	if (!tmp_writer)
		return;
	tmp_writer->w_u32(m_receiver->get_user_param()); //unpacked size
	tmp_writer->w(
		my_proxy_mem_file.pointer(),
		my_proxy_mem_file.size());
	FS.w_close(tmp_writer);
}

void clientdata_proxy::download_screenshot_callback(file_transfer::receiving_status_t status,
													u32 downloaded,
													u32 total)
{
	switch (status)
	{
	case file_transfer::receiving_data:
	{
		Msg("* downloaded %d from %d bytes of screenshot from client [%d]", downloaded, total, m_chearer_id);
		if (m_first_receive)
		{
			notify_admin(e_screenshot_response, "prepare for receive...");
			file_transfer::sending_state_callback_t sending_cb =
				fastdelegate::MakeDelegate(
					this, &clientdata_proxy::upload_file_callback);
			m_ft_server->start_transfer_file(
				my_proxy_mem_file,
				total,
				m_admin_id,
				m_chearer_id,
				sending_cb,
				m_receiver->get_user_param());
			m_first_receive = false;
		}
	}
	break;
	case file_transfer::receiving_aborted_by_user:
	{
		FATAL("* download screenshot aborted by user...");
	}
	break;
	case file_transfer::receiving_aborted_by_peer:
	{
		Msg("* download screenshot aborted by peer [%u]", m_chearer_id);
		
		char bufforint[16];
		string128 error_msg;
		strcpy(error_msg, "download screenshot terminated by peer [");
		strcat(error_msg, ultoa(m_chearer_id.value(), bufforint, 10));
		strcat(error_msg, "]");
		notify_admin(e_screenshot_error_notif, error_msg);
	}
	break;
	case file_transfer::receiving_timeout:
	{
		LPCSTR error_msg = "* download screenshot incomplete - timeout";
		Msg(error_msg);
		notify_admin(e_screenshot_error_notif, error_msg);
	}
	break;
	case file_transfer::receiving_complete:
	{
		if (m_first_receive)
		{
			notify_admin(e_screenshot_response, "prepare for receive...");
			file_transfer::sending_state_callback_t sending_cb =
				fastdelegate::MakeDelegate(
					this, &clientdata_proxy::upload_file_callback);
			m_ft_server->start_transfer_file(
				my_proxy_mem_file,
				total,
				m_admin_id,
				m_chearer_id,
				sending_cb,
				m_receiver->get_user_param());
			m_first_receive = false;
		}
		if (g_sv_mp_save_proxy_screenshots)
		{
			save_proxy_screenshot();
		}
	}
	break;
	};
}

void clientdata_proxy::download_config_callback(file_transfer::receiving_status_t status,
												u32 downloaded,
												u32 total)
{
	switch (status)
	{
	case file_transfer::receiving_data:
	{
		Msg("* downloaded %d from %d bytes of config from client [%d]", downloaded, total, m_chearer_id);
		if (m_first_receive)
		{
			notify_admin(e_configs_response, "prepare for receive...");
			file_transfer::sending_state_callback_t sending_cb =
				fastdelegate::MakeDelegate(
					this, &clientdata_proxy::upload_file_callback);
			m_ft_server->start_transfer_file(
				my_proxy_mem_file,
				total,
				m_admin_id,
				m_chearer_id,
				sending_cb,
				m_receiver->get_user_param());
			m_first_receive = false;
		}
	}
	break;
	case file_transfer::receiving_aborted_by_user:
	{
		FATAL("* download config aborted by user...");
	}
	break;
	case file_transfer::receiving_aborted_by_peer:
	{
		Msg("* download config aborted by peer [%u]", m_chearer_id);
		char bufforint[16];

		string128 error_msg;
		strcpy(error_msg, "download config terminated by peer [");
		strcat(error_msg, ultoa(m_chearer_id.value(), bufforint, 10));
		strcat(error_msg, "]");

		notify_admin(e_configs_error_notif, error_msg);
	}
	break;
	case file_transfer::receiving_timeout:
	{
		LPCSTR error_msg = "* download config incomplete - timeout";
		Msg(error_msg);
		notify_admin(e_configs_error_notif, error_msg);
	}
	break;
	case file_transfer::receiving_complete:
	{
		if (m_first_receive)
		{
			notify_admin(e_configs_response, "prepare for receive...");
			file_transfer::sending_state_callback_t sending_cb =
				fastdelegate::MakeDelegate(
					this, &clientdata_proxy::upload_file_callback);
			m_ft_server->start_transfer_file(
				my_proxy_mem_file,
				total,
				m_admin_id,
				m_chearer_id,
				sending_cb,
				m_receiver->get_user_param());
			m_first_receive = false;
		}
		if (g_sv_mp_save_proxy_configs)
		{
			save_proxy_config();
		}
	}
	break;
	};
}

void clientdata_proxy::upload_file_callback(file_transfer::sending_status_t status,
											u32 uploaded,
											u32 total)
{
	switch (status)
	{
	case file_transfer::sending_data:
	{
		Msg("* uploaded %d from %d bytes to client [%d]", uploaded, total, m_admin_id);
	}
	break;
	case file_transfer::sending_aborted_by_user:
	{
		FATAL("* upload file terminated by user ");
	}
	break;
	case file_transfer::sending_rejected_by_peer:
	{
		Msg("* upload file terminated by peer [%d]", m_admin_id);
	}
	break;
	case file_transfer::sending_complete:
	{
		Msg("* upload file to admin [%d] complete !", m_admin_id);
	}
	break;
	};
}