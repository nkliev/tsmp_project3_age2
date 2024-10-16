#include "stdafx.h"
#include "game_cl_teamdeathmatch.h"
#include "xrMessages.h"
#include "hudmanager.h"
#include "level.h"
#include "UIGameTDM.h"
#include "xr_level_controller.h"
#include "clsid_game.h"
#include "map_manager.h"
#include "map_location.h"
#include "actor.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPDAWnd.h"
#include "ui/UIMapDesc.h"
#include "game_base_menu_events.h"
#include "ui/TeamInfo.h"
#include "string_table.h"

#define MESSAGE_MENUS "tdm_messages_menu"

#include "game_cl_teamdeathmatch_snd_messages.h"

extern u8 GetRanksCount();

const shared_str game_cl_TeamDeathmatch::GetTeamMenu(s16 team)
{
	switch (team)
	{
	case 0:
		return "teamdeathmatch_team0";
		break;
	case 1:
		return "teamdeathmatch_team1";
		break;
	case 2:
		return "teamdeathmatch_team2";
		break;
	default:
		NODEFAULT;
	}

#ifdef DEBUG
	return NULL;
#endif // DEBUG
}

game_cl_TeamDeathmatch::game_cl_TeamDeathmatch()
{
	PresetItemsTeam1.clear();
	PresetItemsTeam2.clear();

	m_bTeamSelected = FALSE;
	m_game_ui = nullptr;

	m_bShowPlayersNames = true;
	m_bFriendlyIndicators = false;
	m_bFriendlyNames = false;

	LoadSndMessages();
}

void game_cl_TeamDeathmatch::Init()
{
	LoadTeamData(GetTeamMenu(1));
	LoadTeamData(GetTeamMenu(2));
}

game_cl_TeamDeathmatch::~game_cl_TeamDeathmatch()
{
	PresetItemsTeam1.clear();
	PresetItemsTeam2.clear();

	xr_delete(pCurBuyMenu);
	xr_delete(pCurSkinMenu);
}

void game_cl_TeamDeathmatch::net_import_state(NET_Packet &P)
{
	bool teamsEqual = (!m_TeamsScores.empty()) ? (m_TeamsScores[0] == m_TeamsScores[1]) : false;
	inherited::net_import_state(P);
	m_bFriendlyIndicators = !!P.r_u8();
	m_bFriendlyNames = !!P.r_u8();

	if (!m_bFriendlyNames && m_bShowPlayersNames)
		m_bShowPlayersNames = false;

	if (m_TeamsScores.empty())
		return;

	if (teamsEqual)
	{
		if (m_TeamsScores[0] != m_TeamsScores[1] && Level().CurrentViewEntity())
		{
			if (m_TeamsScores[0] > m_TeamsScores[1])
				PlaySndMessage(ID_TEAM1_LEAD);
			else
				PlaySndMessage(ID_TEAM2_LEAD);
		}
	}
	else
	{
		if (m_TeamsScores[0] == m_TeamsScores[1] && Level().CurrentViewEntity())
			PlaySndMessage(ID_TEAMS_EQUAL);
	}
}

void game_cl_TeamDeathmatch::TranslateGameMessage(u32 msg, NET_Packet &P)
{
	CStringTable st;
	string512 Text;
	char Color_Main[] = "%c[255,192,192,192]";

	switch (msg)
	{
	case GAME_EVENT_PLAYER_JOIN_TEAM: //tdm
	{
		string64 PlayerName;
		P.r_stringZ(PlayerName);
		u16 Team;
		P.r_u16(Team);

		sprintf_s(Text, "%s%s %s%s %s%s",
				  "", //no color
				  PlayerName,
				  Color_Main,
				  *st.translate("mp_joined"),
				  CTeamInfo::GetTeam_color_tag(int(Team)),
				  CTeamInfo::GetTeam_name(int(Team)));
		CommonMessageOut(Text);

		Msg("%s %s %s", PlayerName, *st.translate("mp_joined"), CTeamInfo::GetTeam_name(int(Team)));
	}
	break;

	case PLAYER_CHANGE_TEAM: //tdm
	{
		u16 PlayerID, OldTeam, NewTeam;
		P.r_u16(PlayerID);
		P.r_u16(OldTeam);
		P.r_u16(NewTeam);

		game_PlayerState *pPlayer = GetPlayerByGameID(PlayerID);
		if (!pPlayer)
			break;

		sprintf_s(Text, "%s%s %s%s %s%s",
				  CTeamInfo::GetTeam_color_tag(int(OldTeam)),
				  pPlayer->name,
				  Color_Main,
				  *st.translate("mp_switched_to"),
				  CTeamInfo::GetTeam_color_tag(int(NewTeam)),
				  CTeamInfo::GetTeam_name(int(NewTeam)));
		CommonMessageOut(Text);

		Msg("%s *s %s", pPlayer->name, *st.translate("mp_switched_to"), CTeamInfo::GetTeam_name(int(NewTeam)));
	}
	break;

	default:
		inherited::TranslateGameMessage(msg, P);
	}
}

CUIGameCustom *game_cl_TeamDeathmatch::createGameUI()
{
	game_cl_mp::createGameUI();
	CLASS_ID clsid = CLSID_GAME_UI_TEAMDEATHMATCH;
	m_game_ui = smart_cast<CUIGameTDM *>(NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->SetClGame(this);
	m_game_ui->Init();

	LoadMessagesMenu(MESSAGE_MENUS);
	return m_game_ui;
}

void game_cl_TeamDeathmatch::OnMapInfoAccept()
{
	if (CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
}

void game_cl_TeamDeathmatch::OnTeamMenuBack()
{
	if (local_player->testFlag(GAME_PLAYER_FLAG_SPECTATOR))	
		StartStopMenu(m_game_ui->m_pMapDesc, true);	
}

void game_cl_TeamDeathmatch::OnTeamMenu_Cancel()
{
	StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);

	if (!m_bTeamSelected && !m_bSpectatorSelected && CanCallTeamSelectMenu() && !m_game_ui->m_pUITeamSelectWnd->IsShown())
	{
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
		return;
	}

	m_bMenuCalledFromReady = FALSE;
}

void game_cl_TeamDeathmatch::OnSkinMenuBack()
{
	if (CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);
}

void game_cl_TeamDeathmatch::OnSpectatorSelect()
{
	m_bTeamSelected = FALSE;
	inherited::OnSpectatorSelect();
}

void game_cl_TeamDeathmatch::OnTeamSelect(int Team)
{
	bool NeedToSendTeamSelect = true;

	if (Team != -1)
	{
		if (Team + 1 == local_player->team && m_bSkinSelected)
			NeedToSendTeamSelect = false;
		else		
			NeedToSendTeamSelect = true;		
	}

	if (NeedToSendTeamSelect)
	{
		CObject *l_pObj = Level().CurrentEntity();
		CGameObject *l_pPlayer = smart_cast<CGameObject *>(l_pObj);

		if (!l_pPlayer)
			return;

		NET_Packet P;
		l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
		P.w_u16(GAME_EVENT_PLAYER_GAME_MENU);
		P.w_u8(PLAYER_CHANGE_TEAM);

		P.w_s16(s16(Team + 1));
		l_pPlayer->u_EventSend(P);
		m_bSkinSelected = FALSE;
	}

	m_bTeamSelected = TRUE;
}

void game_cl_TeamDeathmatch::SetCurrentBuyMenu()
{
	if (!local_player)
		return;

	if (!local_player->team || local_player->skin == -1)
		return;

	if (!pCurBuyMenu)
	{
		if (local_player->team == 1)
		{
			pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), 1);
			LoadTeamDefaultPresetItems(GetTeamMenu(1), pCurBuyMenu, &PresetItemsTeam1);
			pCurPresetItems = &PresetItemsTeam1;
		}
		else
		{
			pCurBuyMenu = InitBuyMenu(GetBaseCostSect(), 2);
			LoadTeamDefaultPresetItems(GetTeamMenu(2), pCurBuyMenu, &PresetItemsTeam2);
			pCurPresetItems = &PresetItemsTeam2;
		}

		LoadDefItemsForRank(pCurBuyMenu);
	}

	if (!pCurBuyMenu)
		return;

	if (m_cl_dwWarmUp_Time != 0)
		pCurBuyMenu->IgnoreMoneyAndRank(true);
	else
		pCurBuyMenu->IgnoreMoneyAndRank(false);
}

void game_cl_TeamDeathmatch::SetCurrentSkinMenu()
{
	if (!local_player)
		return;

	s16 new_team;

	if (local_player->team == 1)	
		new_team = 1;	
	else
		new_team = 2;
	
	if (pCurSkinMenu && pCurSkinMenu->GetTeam() == new_team)
		return;

	if (pCurSkinMenu && new_team != pCurSkinMenu->GetTeam())
		if (pCurSkinMenu->IsShown())
			StartStopMenu(pCurSkinMenu, true);

	xr_delete(pCurSkinMenu);
	pCurSkinMenu = InitSkinMenu(new_team);
}

bool game_cl_TeamDeathmatch::CanBeReady()
{
	if (!local_player)
		return false;

	m_bMenuCalledFromReady = TRUE;

	if (!m_bTeamSelected)
	{
		m_bMenuCalledFromReady = FALSE;

		if (CanCallTeamSelectMenu())
			StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);

		return false;
	}

	return inherited::CanBeReady();
}

char *game_cl_TeamDeathmatch::getTeamSection(int Team)
{
	switch (Team)
	{
	case 1:	
		return "teamdeathmatch_team1";
		break;

	case 2:	
		return "teamdeathmatch_team2";
		break;

	default:
		return nullptr;
	}
}

#include "string_table.h"
#include "ui/teaminfo.h"

void game_cl_TeamDeathmatch::shedule_Update(u32 dt)
{
	CStringTable st;
	string512 msg;

	if (!m_game_ui && HUD().GetUI())
		m_game_ui = smart_cast<CUIGameTDM *>(HUD().GetUI()->UIGame());

	inherited::shedule_Update(dt);

	if (!m_game_ui)
		return;

	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown() && !CanCallTeamSelectMenu())
		StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);

	switch (m_phase)
	{
	case GAME_PHASE_TEAM1_SCORES:
	{
		sprintf_s(msg, *st.translate("mp_team_wins"), CTeamInfo::GetTeam_name(1));
		m_game_ui->SetRoundResultCaption(msg);

		SetScore();
	}
	break;

	case GAME_PHASE_TEAM2_SCORES:
	{
		sprintf_s(msg, *st.translate("mp_team_wins"), CTeamInfo::GetTeam_name(2));
		m_game_ui->SetRoundResultCaption(msg);

		SetScore();
	}
	break;

	case GAME_PHASE_INPROGRESS:
	{
		if (local_player && !local_player->IsSkip())
		{
			if (Level().CurrentEntity() && Level().CurrentEntity()->CLS_ID == CLSID_SPECTATOR)
			{
				if (!(pCurBuyMenu && pCurBuyMenu->IsShown()) &&
					!(pCurSkinMenu && pCurSkinMenu->IsShown()) &&
					!(m_game_ui->m_pMapDesc && m_game_ui->m_pMapDesc->IsShown()) &&
					(HUD().GetUI() && HUD().GetUI()->GameIndicatorsShown()))
				{
					if (!m_bTeamSelected)
						m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_team");
				}
			}

			SetScore();
		}
	}
	break;
	}
}

void game_cl_TeamDeathmatch::SetScore()
{
	if (local_player && local_player->team >= 0 && m_game_ui)
		m_game_ui->SetScoreCaption(m_TeamsScores[0], m_TeamsScores[1]);
}

bool game_cl_TeamDeathmatch::OnKeyboardPress(int key)
{
	if (kTEAM == key && m_game_ui)
	{
		if (CanCallTeamSelectMenu())		
			StartStopMenu(m_game_ui->m_pUITeamSelectWnd, true);		

		return true;
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_TeamDeathmatch::IsEnemy(game_PlayerState *ps)
{
	if (!local_player)
		return false;

	return local_player->team != ps->team;
}

bool game_cl_TeamDeathmatch::IsEnemy(CEntityAlive *ea1, CEntityAlive *ea2)
{
	return (ea1->g_Team() != ea2->g_Team());
}

#define PLAYER_NAME_COLOR 0xff40ff40

void game_cl_TeamDeathmatch::OnRender()
{
	if (local_player)
	{
		cl_TeamStruct *pTS = &TeamList[ModifyTeam(local_player->team)];

		for(const auto &it: players)
		{
			game_PlayerState *ps = it.second;			

			if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
				continue;

			u16 id = ps->GameID;
			CObject *pObject = Level().Objects.net_Find(id);

			if (!pObject || pObject->CLS_ID != CLSID_OBJECT_ACTOR || IsEnemy(ps) || ps == local_player)
				continue;

			float dup = 0.0f;

			if (m_bShowPlayersNames)
			{
				VERIFY(pObject);
				CActor *pActor = smart_cast<CActor *>(pObject);
				VERIFY(pActor);
				Fvector IPos = pTS->IndicatorPos;
				IPos.y -= pTS->Indicator_r2;
				pActor->RenderText(ps->getName(), IPos, &dup, PLAYER_NAME_COLOR);
			}

			if (m_bFriendlyIndicators)
			{
				VERIFY(pObject);
				CActor *pActor = smart_cast<CActor *>(pObject);
				VERIFY(pActor);
				Fvector IPos = pTS->IndicatorPos;
				IPos.y += dup;
				pActor->RenderIndicator(IPos, pTS->Indicator_r1, pTS->Indicator_r2, pTS->IndicatorShader);
			}
		}
	}

	inherited::OnRender();
}

BOOL game_cl_TeamDeathmatch::CanCallBuyMenu()
{
	if (!m_game_ui)
		return FALSE;

	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;

	if (!m_bTeamSelected || !m_bSkinSelected)
		return FALSE;

	return inherited::CanCallBuyMenu();
}

BOOL game_cl_TeamDeathmatch::CanCallSkinMenu()
{
	if (!m_game_ui)
		return FALSE;

	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;

	if (!m_bTeamSelected)
		return FALSE;

	return inherited::CanCallSkinMenu();
}

BOOL game_cl_TeamDeathmatch::CanCallInventoryMenu()
{
	if (!m_game_ui)
		return FALSE;

	if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
		return FALSE;

	return inherited::CanCallInventoryMenu();
}

BOOL game_cl_TeamDeathmatch::CanCallTeamSelectMenu()
{
	if (Phase() != GAME_PHASE_INPROGRESS || !local_player)
		return false;

	if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown())
		return FALSE;
	
	if ((pCurBuyMenu && pCurBuyMenu->IsShown()) || (pCurSkinMenu && pCurSkinMenu->IsShown()))	
		return FALSE;	

	m_game_ui->m_pUITeamSelectWnd->SetCurTeam(ModifyTeam(local_player->team));
	return TRUE;
}

#define FRIEND_LOCATION "mp_friend_location"

void game_cl_TeamDeathmatch::UpdateMapLocations()
{
	inherited::UpdateMapLocations();

	if (!local_player)
		return;

	for (const auto &it : players)
	{
		game_PlayerState* ps = it.second;
		u16 id = ps->GameID;

		if (ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
		{
			Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
			continue;
		}

		CObject* pObject = Level().Objects.net_Find(id);

		if (!pObject || pObject->CLS_ID != CLSID_OBJECT_ACTOR)
			continue;

		if (IsEnemy(ps))
		{
			if (Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))			
				Level().MapManager().RemoveMapLocation(FRIEND_LOCATION, id);
			
			continue;
		}

		if (!Level().MapManager().HasMapLocation(FRIEND_LOCATION, id))		
			(Level().MapManager().AddMapLocation(FRIEND_LOCATION, id))->EnablePointer();		
	}
}

void game_cl_TeamDeathmatch::LoadSndMessages()
{
	LoadSndMessage("tdm_snd_messages", "team1_win", ID_TEAM1_WIN);
	LoadSndMessage("tdm_snd_messages", "team2_win", ID_TEAM2_WIN);
	LoadSndMessage("tdm_snd_messages", "teams_equal", ID_TEAMS_EQUAL);
	LoadSndMessage("tdm_snd_messages", "team1_lead", ID_TEAM1_LEAD);
	LoadSndMessage("tdm_snd_messages", "team2_lead", ID_TEAM2_LEAD);

	for (u8 i = 1; i < GetRanksCount(); i++)
	{
		string16 lineTeam1, lineTeam2;
		sprintf(lineTeam1, "team1_rank%u", i);
		sprintf(lineTeam2, "team2_rank%u", i);
		LoadSndMessage("tdm_snd_messages", lineTeam1, ID_TEAM1_RANK_1 + i - 1);
		LoadSndMessage("tdm_snd_messages", lineTeam2, ID_TEAM2_RANK_1 + i - 1);
	}
}

void game_cl_TeamDeathmatch::OnSwitchPhase_InProgress()
{
	if (!m_bSkinSelected)
		m_bTeamSelected = FALSE;
}

void game_cl_TeamDeathmatch::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
	inherited::OnSwitchPhase(old_phase, new_phase);

	switch (new_phase)
	{
	case GAME_PHASE_TEAM1_SCORES:
	{
		if (Level().CurrentViewEntity())
			PlaySndMessage(ID_TEAM1_WIN);
	}
	break;

	case GAME_PHASE_TEAM2_SCORES:
	{
		if (Level().CurrentViewEntity())
			PlaySndMessage(ID_TEAM2_WIN);
	}
	break;
	}
}

void game_cl_TeamDeathmatch::OnTeamChanged()
{
	xr_delete(pCurBuyMenu);
	SetCurrentBuyMenu();
	inherited::OnTeamChanged();
}

void game_cl_TeamDeathmatch::PlayRankChangesSndMessage()
{
	if (!local_player)
		return;

	if (!local_player->rank)
		return;

	switch (local_player->team)
	{
	case 1:
		PlaySndMessage(ID_TEAM1_RANK_0 + local_player->rank);
		break;

	case 2:
		PlaySndMessage(ID_TEAM2_RANK_0 + local_player->rank);
		break;
	}
}

void game_cl_TeamDeathmatch::OnGameMenuRespond_ChangeTeam(NET_Packet &P)
{
	s16 OldTeam = local_player->team;
	local_player->team = u8(P.r_s16() & 0x00ff);

	if (OldTeam != local_player->team)
		OnTeamChanged();

	SetCurrentSkinMenu();

	if (pCurSkinMenu)
	{
		pCurSkinMenu->SetCurSkin(local_player->skin);
		if (CanCallSkinMenu())
			StartStopMenu(pCurSkinMenu, true);
	}
}
