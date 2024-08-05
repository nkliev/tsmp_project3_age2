#pragma once

// CL	== client 2 server message
// SV	== server 2 client message

enum
{
	M_UPDATE = 0, // DUAL: Update state
	M_SPAWN,	  // DUAL: Spawning, full state

	M_SV_CONFIG_NEW_CLIENT,
	M_SV_CONFIG_GAME,
	M_SV_CONFIG_FINISHED,

	M_CHAT, // DUAL:

	M_EVENT,	// Game Event
	M_CL_UPDATE,
	M_UPDATE_OBJECTS,
	M_CLIENTREADY, // Client has finished to load level and are ready to play

	M_CHANGE_LEVEL, // changing level
	M_LOAD_GAME,
	M_RELOAD_GAME,
	M_SAVE_GAME,
	M_SAVE_PACKET,

	M_SWITCH_DISTANCE,
	M_GAMEMESSAGE, // Game Message
	M_EVENT_PACK,  // Pack of M_EVENT

	M_GAMESPY_CDKEY_VALIDATION_CHALLENGE,
	M_GAMESPY_CDKEY_VALIDATION_CHALLENGE_RESPOND,
	M_CLIENT_CONNECT_RESULT,
	M_CLIENT_REQUEST_CONNECTION_DATA,

	M_CHAT_MESSAGE,
	M_CLIENT_WARN,
	M_CHANGE_LEVEL_GAME,

	M_CL_PING_CHALLENGE,
	M_CL_PING_CHALLENGE_RESPOND,

	M_AUTH_CHALLENGE,
	M_CL_AUTH,
	M_BULLET_CHECK_RESPOND,

	M_STATISTIC_UPDATE,
	M_STATISTIC_UPDATE_RESPOND,

	M_PLAYER_FIRE,

	M_MOVE_PLAYERS,
	M_MOVE_PLAYERS_RESPOND,

	M_CHANGE_SELF_NAME,
	M_REMOTE_CONTROL_AUTH,
	M_REMOTE_CONTROL_CMD,
	M_MAP_SYNC,
	M_FILE_TRANSFER,
	M_TRANSFER_PATROL_PATHS,
	M_VOICE_MESSAGE,
	M_UID_CHALLENGE,
	M_UID_RESPOND,
	M_UID_ASSIGN,
	M_TASKS_SYNC,
	M_INFOPORTIONS_SYNC,
	M_REQUEST_SAVED_GAMES,
	M_RESPOND_SAVED_GAMES,
	M_CL_SAVE_GAME_COMMAND,
	M_CL_GAME_SAVED_NOTIFY,
	MSG_FORCEDWORD = u32(-1)
};

enum
{
	GE_RESPAWN,
	GE_OWNERSHIP_TAKE, // DUAL: Client request for ownership of an item
	GE_OWNERSHIP_TAKE_MP_FORCED, // �� �� ��� � ����, �� �������� � dm/tdm ��� ������� ������ �������� �� ��������� ������ ������ � ��� �� ������ ��� � �����������
	GE_OWNERSHIP_REJECT, // DUAL: Client request ownership rejection
	GE_TRANSFER_AMMO,	 // DUAL: Take ammo out of weapon for our weapon
	GE_HIT,				 //
	GE_DIE,				 //
	GE_ASSIGN_KILLER,	 //
	GE_DESTROY,			 // authorative client request for entity-destroy
	GE_DESTROY_REJECT,	 // GE_DESTROY + GE_OWNERSHIP_REJECT
	GE_TELEPORT_OBJECT,

	GE_ADD_RESTRICTION,
	GE_REMOVE_RESTRICTION,
	GE_REMOVE_ALL_RESTRICTIONS,

	GE_BUY,

	GE_INFO_TRANSFER, //transfer _new_ info on PDA

	GE_TRADE_SELL,
	GE_TRADE_BUY,

	GE_WPN_AMMO_ADD,
	GE_WPN_STATE_CHANGE,

	GE_ADDON_ATTACH,
	GE_ADDON_DETACH,
	GE_ADDON_CHANGE,

	GE_GRENADE_EXPLODE,
	GE_INV_ACTION, //a action beign taken on inventory

	GE_ZONE_STATE_CHANGE,

	GE_MOVE_ACTOR,	  //move actor to desired position instantly
	
	GE_ACTOR_MAX_POWER=28,

	GE_CHANGE_POS,

	GE_GAME_EVENT,

	GE_CHANGE_VISUAL,
	GE_MONEY,

	GEG_PLAYER_ACTIVATE_SLOT,
	GEG_PLAYER_ITEM2SLOT,
	GEG_PLAYER_ITEM2BELT,
	GEG_PLAYER_ITEM2RUCK,
	GEG_PLAYER_ITEM_EAT,
	GEG_PLAYER_ITEM_SELL,
	GEG_PLAYER_ACTIVATEARTEFACT,

	GEG_PLAYER_WEAPON_HIDE_STATE,

	GEG_PLAYER_ATTACH_HOLDER,
	GEG_PLAYER_DETACH_HOLDER,

	GEG_PLAYER_PLAY_HEADSHOT_PARTICLE,
	GE_HIT_STATISTIC,
	GE_KILL_SOMEONE,

	GE_FREEZE_OBJECT,
	GE_LAUNCH_ROCKET,
	GE_CLIENT_SPAWN, // cl_spawn
	GE_CAR_BEEP,
	GE_BLOODSUCKER_PREDATOR_CHANGE,
	GE_CONTROLLER_PSY_FIRE,
	GE_CLIENT_INV_SPAWN,
	GE_HELI_ROCKET_LAUNCH,
	GE_CALL_HELICOPTER,
	GE_FORCEDWORD = u32(-1)
};

IC const char* GE_ToStr(u16 GE_type)
{
	switch (GE_type)
	{
	case GE_RESPAWN: return "GE_RESPAWN";
	case GE_OWNERSHIP_TAKE: return "GE_OWNERSHIP_TAKE";
	case GE_OWNERSHIP_TAKE_MP_FORCED: return "GE_OWNERSHIP_TAKE_MP_FORCED";
	case GE_OWNERSHIP_REJECT: return "GE_OWNERSHIP_REJECT";
	case GE_TRANSFER_AMMO: return "GE_TRANSFER_AMMO";
	case GE_HIT: return "GE_HIT";
	case GE_DIE: return "GE_DIE";
	case GE_ASSIGN_KILLER: return "GE_ASSIGN_KILLER";
	case GE_DESTROY: return "GE_DESTROY";
	case GE_DESTROY_REJECT: return "GE_DESTROY_REJECT";
	case GE_TELEPORT_OBJECT: return "GE_TELEPORT_OBJECT";
	case GE_ADD_RESTRICTION: return "GE_ADD_RESTRICTION";
	case GE_REMOVE_RESTRICTION: return "GE_REMOVE_RESTRICTION";
	case GE_REMOVE_ALL_RESTRICTIONS: return "GE_REMOVE_ALL_RESTRICTIONS";
	case GE_BUY: return "GE_BUY";
	case GE_INFO_TRANSFER: return "GE_INFO_TRANSFER";
	case GE_TRADE_SELL: return "GE_TRADE_SELL";
	case GE_TRADE_BUY: return "GE_TRADE_BUY";
	case GE_WPN_AMMO_ADD: return "GE_WPN_AMMO_ADD";
	case GE_WPN_STATE_CHANGE: return "GE_WPN_STATE_CHANGE";
	case GE_ADDON_ATTACH: return "GE_ADDON_ATTACH";
	case GE_ADDON_DETACH: return "GE_ADDON_DETACH";
	case GE_ADDON_CHANGE: return "GE_ADDON_CHANGE";
	case GE_GRENADE_EXPLODE: return "GE_GRENADE_EXPLODE";
	case GE_INV_ACTION: return "GE_INV_ACTION";
	case GE_ZONE_STATE_CHANGE: return "GE_ZONE_STATE_CHANGE";
	case GE_MOVE_ACTOR: return "GE_MOVE_ACTOR";
	case GE_ACTOR_MAX_POWER: return "GE_ACTOR_MAX_POWER";
	case GE_CHANGE_POS: return "GE_CHANGE_POS";
	case GE_GAME_EVENT: return "GE_GAME_EVENT";
	case GE_CHANGE_VISUAL: return "GE_CHANGE_VISUAL";
	case GE_MONEY: return "GE_MONEY";
	case GEG_PLAYER_ACTIVATE_SLOT: return "GEG_PLAYER_ACTIVATE_SLOT";
	case GEG_PLAYER_ITEM2SLOT: return "GEG_PLAYER_ITEM2SLOT";
	case GEG_PLAYER_ITEM2BELT: return "GEG_PLAYER_ITEM2BELT";
	case GEG_PLAYER_ITEM2RUCK: return "GEG_PLAYER_ITEM2RUCK";
	case GEG_PLAYER_ITEM_EAT: return "GEG_PLAYER_ITEM_EAT";
	case GEG_PLAYER_ITEM_SELL: return "GEG_PLAYER_ITEM_SELL";
	case GEG_PLAYER_ACTIVATEARTEFACT: return "GEG_PLAYER_ACTIVATEARTEFACT";
	case GEG_PLAYER_WEAPON_HIDE_STATE: return "GEG_PLAYER_WEAPON_HIDE_STATE";
	case GEG_PLAYER_ATTACH_HOLDER: return "GEG_PLAYER_ATTACH_HOLDER";
	case GEG_PLAYER_DETACH_HOLDER: return "GEG_PLAYER_DETACH_HOLDER";
	case GEG_PLAYER_PLAY_HEADSHOT_PARTICLE: return "GEG_PLAYER_PLAY_HEADSHOT_PARTICLE";
	case GE_HIT_STATISTIC: return "GE_HIT_STATISTIC";
	case GE_KILL_SOMEONE: return "GE_KILL_SOMEONE";
	case GE_FREEZE_OBJECT: return "GE_FREEZE_OBJECT";
	case GE_LAUNCH_ROCKET: return "GE_LAUNCH_ROCKET";
	case GE_CLIENT_SPAWN: return "GE_CLIENT_SPAWN";
	case GE_CAR_BEEP: return "GE_CAR_BEEP";
	case GE_BLOODSUCKER_PREDATOR_CHANGE: return "GE_BLOODSUCKER_PREDATOR_CHANGE";
	case GE_CONTROLLER_PSY_FIRE: return "GE_CONTROLLER_PSY_FIRE";
	case GE_CLIENT_INV_SPAWN: return "GE_CLIENT_INV_SPAWN";
	case GE_HELI_ROCKET_LAUNCH: return "GE_HELI_ROCKET_LAUNCH";
	case GE_CALL_HELICOPTER: return "GE_CALL_HELICOPTER";
	default: return "unknown GE";		
	}
}

enum EGameMessages
{ //game_cl <----> game_sv messages
	GAME_EVENT_PLAYER_READY,
	GAME_EVENT_PLAYER_KILL,			//player wants to die
	GAME_EVENT_PLAYER_BUY_FINISHED, //player end to buy items
	GAME_EVENT_PLAYER_BUY_SPAWN,

	GAME_EVENT_PLAYER_GAME_MENU,
	GAME_EVENT_PLAYER_GAME_MENU_RESPOND,

	GAME_EVENT_PLAYER_CONNECTED,
	GAME_EVENT_PLAYER_DISCONNECTED,
	GAME_EVENT_PLAYER_ENTERED_GAME,

	GAME_EVENT_PLAYER_KILLED, //////!!!!!
	GAME_EVENT_PLAYER_HITTED,

	GAME_EVENT_PLAYER_JOIN_TEAM,
	GAME_EVENT_ROUND_STARTED,
	GAME_EVENT_ROUND_END,

	GAME_EVENT_ARTEFACT_SPAWNED,
	GAME_EVENT_ARTEFACT_DESTROYED,
	GAME_EVENT_ARTEFACT_TAKEN,
	GAME_EVENT_ARTEFACT_DROPPED,
	GAME_EVENT_ARTEFACT_ONBASE,

	GAME_EVENT_PLAYER_ENTER_TEAM_BASE,
	GAME_EVENT_PLAYER_LEAVE_TEAM_BASE,

	GAME_EVENT_BUY_MENU_CLOSED,
	GAME_EVENT_TEAM_MENU_CLOSED,
	GAME_EVENT_SKIN_MENU_CLOSED,

	GAME_EVENT_CREATE_CLIENT,
	GAME_EVENT_ON_HIT,
	GAME_EVENT_ON_TOUCH,

	GAME_EVENT_VOTE_START,
	GAME_EVENT_VOTE_YES,
	GAME_EVENT_VOTE_NO,
	GAME_EVENT_VOTE_STOP,
	GAME_EVENT_VOTE_END,

	GAME_EVENT_PLAYER_AUTH,
	GAME_EVENT_PLAYER_NAME,

	GAME_EVENT_SPEECH_MESSAGE,

	GAME_EVENT_PLAYERS_MONEY_CHANGED,
	GAME_EVENT_SERVER_STRING_MESSAGE,
	GAME_EVENT_SERVER_DIALOG_MESSAGE,
	GAME_EVENT_MAKE_DATA,
	GAME_EVENT_PLAYER_AUTH_UID,
	GAME_EVENT_PLAYER_KILLED_NPC,

	GAME_EVENT_CHANGE_VISUAL,

	GAME_EVENT_SCRIPT_BEGINS_FROM, // don't add messages after this
	GAME_EVENT_FORCEDWORD = u32(-1)
};

enum
{
	M_SPAWN_OBJECT_LOCAL = (1 << 0),	 // after spawn it becomes local (authorative)
	M_SPAWN_OBJECT_HASUPDATE = (1 << 2), // after spawn info it has update inside message
	M_SPAWN_OBJECT_ASPLAYER = (1 << 3),	 // after spawn it must become viewable
	M_SPAWN_OBJECT_PHANTOM = (1 << 4),	 // after spawn it must become viewable
	M_SPAWN_VERSION = (1 << 5),			 // control version
	M_SPAWN_UPDATE = (1 << 6),			 // + update packet
	M_SPAWN_TIME = (1 << 7),			 // + spawn time
	M_SPAWN_DENIED = (1 << 8),			 // don't spawn entity with this flag
	M_SPAWN_NO_CLDATA = (1 << 9),		 // don't include client data (wrote in net_Save)

	M_SPAWN_OBJECT_FORCEDWORD = u32(-1)
};
