#pragma once

#include "game_sv_base.h"

class xrServer;
class CALifeSimulator;

class game_sv_Single : public game_sv_GameState
{
private:
	typedef game_sv_GameState inherited;

public:
	game_sv_Single();
	virtual ~game_sv_Single();

	virtual LPCSTR type_name() const { return "single"; };
	virtual void Create(shared_str &options);

	virtual void OnCreate(u16 id_who);
	virtual BOOL OnTouch(u16 eid_who, u16 eid_what, BOOL bForced = FALSE);
	virtual void OnDetach(u16 eid_who, u16 eid_what);

	// Main
	virtual void Update();
	virtual ALife::_TIME_ID GetGameTime();
	virtual float GetGameTimeFactor();
	virtual void SetGameTimeFactor(const float fTimeFactor);

	virtual ALife::_TIME_ID GetEnvironmentGameTime();
	virtual float GetEnvironmentGameTimeFactor();
	virtual void SetEnvironmentGameTimeFactor(const float fTimeFactor);

	virtual bool change_level(NET_Packet &net_packet, ClientID const &sender);
	virtual void save_game(NET_Packet &net_packet, ClientID const &sender);
	virtual bool load_game(NET_Packet &net_packet, ClientID const &sender);
	virtual void reload_game(NET_Packet &net_packet, ClientID const &sender);
	virtual void switch_distance(NET_Packet &net_packet, ClientID const &sender);
	virtual BOOL CanHaveFriendlyFire() { return FALSE; }
	virtual void teleport_object(NET_Packet &packet, u16 id);
	virtual void add_restriction(NET_Packet &packet, u16 id);
	virtual void remove_restriction(NET_Packet &packet, u16 id);
	virtual void remove_all_restrictions(NET_Packet &packet, u16 id);
	virtual void sls_default();
	virtual shared_str level_name(const shared_str &server_options) const;
	virtual void on_death(CSE_Abstract *e_dest, CSE_Abstract *e_src);
	void restart_simulator(LPCSTR saved_game_name);
};
