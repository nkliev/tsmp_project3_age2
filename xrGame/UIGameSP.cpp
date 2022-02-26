#include "pch_script.h"
#include "uigamesp.h"
#include "actor.h"
#include "level.h"

#include "game_cl_Single.h"
#include "ui/UIPdaAux.h"
#include "xr_level_controller.h"
#include "actorcondition.h"
#include "Console.h"
#include "object_broker.h"
#include "GameTaskManager.h"
#include "GameTask.h"

#include "ui/UIInventoryWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UITalkWnd.h"
#include "ui/UICarBodyWnd.h"
#include "ui/UIMessageBox.h"

CUIGameSP::CUIGameSP()
{
	m_game = nullptr;
	InventoryMenu = xr_new<CUIInventoryWnd>();	
	UIChangeLevelWnd = xr_new<CChangeLevelWnd>();
}

CUIGameSP::~CUIGameSP()
{
	delete_data(InventoryMenu);
	delete_data(UIChangeLevelWnd);
}

bool CUIGameSP::IsAnyWndActive()
{
	return PdaMenu->IsShown()
		|| InventoryMenu->IsShown()
		|| TalkMenu->IsShown()
		|| m_pUICarBodyMenu->IsShown()
		|| UIChangeLevelWnd->IsShown();
}

void CUIGameSP::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
	CActor *pActor = smart_cast<CActor *>(Level().CurrentEntity());

	if (!pActor)
		return;

	if (pActor->g_Alive())
		return;

	HideShownDialogs();
}

void CUIGameSP::HideShownDialogs()
{
	CUIDialogWnd *mir = MainInputReceiver();

	if (mir &&
		(mir == InventoryMenu ||
		 mir == PdaMenu ||
		 mir == TalkMenu ||
		 mir == m_pUICarBodyMenu))
		mir->GetHolder()->StartStopMenu(mir, true);
}

void CUIGameSP::SetClGame(game_cl_GameState *g)
{
	inherited::SetClGame(g);
	m_game = smart_cast<game_cl_Single *>(g);
	R_ASSERT(m_game);
}

bool CUIGameSP::IR_OnKeyboardPress(int dik)
{
	if (inherited::IR_OnKeyboardPress(dik))
		return true;

	if (Device.Paused())
		return false;

	CActor *pActor = smart_cast<CActor *>(Level().CurrentEntity());

	if (!pActor)
		return false;

	if (pActor && !pActor->g_Alive())
		return false;

	switch (get_binded_action(dik))
	{
	case kINVENTORY:
		if (!MainInputReceiver() || MainInputReceiver() == InventoryMenu)
		{
			m_game->StartStopMenu(InventoryMenu, true);
			return true;
		}
		break;

	case kACTIVE_JOBS:
		if (!MainInputReceiver() || MainInputReceiver() == PdaMenu)
		{
			PdaMenu->SetActiveSubdialog(eptQuests);
			m_game->StartStopMenu(PdaMenu, true);
			return true;
		}
		break;

	case kMAP:
		if (!MainInputReceiver() || MainInputReceiver() == PdaMenu)
		{
			PdaMenu->SetActiveSubdialog(eptMap);
			m_game->StartStopMenu(PdaMenu, true);
			return true;
		}
		break;

	case kCONTACTS:
		if (!MainInputReceiver() || MainInputReceiver() == PdaMenu)
		{
			PdaMenu->SetActiveSubdialog(eptContacts);
			m_game->StartStopMenu(PdaMenu, true);
			return true;
		}
		break;
		
	case kSCORES:
		{
			SDrawStaticStruct *ss = AddCustomStatic("main_task", true);
			SGameTaskObjective *o = pActor->GameTaskManager().ActiveObjective();
	
			if (!o)
				ss->m_static->SetTextST("st_no_active_task");
			else
				ss->m_static->SetTextST(*(o->description));
		}
		break;
	}

	return false;
}

bool CUIGameSP::IR_OnKeyboardRelease(int dik)
{
	if (inherited::IR_OnKeyboardRelease(dik))
		return true;

	if (is_binded(kSCORES, dik))
		RemoveCustomStatic("main_task");

	return false;
}

void CUIGameSP::ReInitShownUI()
{
	if (InventoryMenu->IsShown())
		InventoryMenu->InitInventory_delayed();
	else if (m_pUICarBodyMenu->IsShown())
		m_pUICarBodyMenu->UpdateLists_delayed();
}

extern ENGINE_API BOOL bShowPauseString;

void CUIGameSP::ChangeLevel(GameGraph::_GRAPH_ID game_vert_id, u32 level_vert_id, Fvector pos, Fvector ang, Fvector pos2, Fvector ang2, bool b)
{
	if (!MainInputReceiver() || MainInputReceiver() != UIChangeLevelWnd)
	{
		UIChangeLevelWnd->m_game_vertex_id = game_vert_id;
		UIChangeLevelWnd->m_level_vertex_id = level_vert_id;
		UIChangeLevelWnd->m_position = pos;
		UIChangeLevelWnd->m_angles = ang;
		UIChangeLevelWnd->m_position_cancel = pos2;
		UIChangeLevelWnd->m_angles_cancel = ang2;
		UIChangeLevelWnd->m_b_position_cancel = b;
		m_game->StartStopMenu(UIChangeLevelWnd, true);
	}
}

void CUIGameSP::reset_ui()
{
	inherited::reset_ui();
	InventoryMenu->Reset();
	PdaMenu->Reset();
	UIChangeLevelWnd->Reset();
}

CChangeLevelWnd::CChangeLevelWnd()
{
	m_messageBox = xr_new<CUIMessageBox>();
	m_messageBox->SetAutoDelete(true);
	AttachChild(m_messageBox);
	m_messageBox->Init("message_box_change_level");
	SetWndPos(m_messageBox->GetWndPos());
	m_messageBox->SetWndPos(0.0f, 0.0f);
	SetWndSize(m_messageBox->GetWndSize());
}

void CChangeLevelWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (pWnd == m_messageBox)
	{
		switch (msg)
		{
		case MESSAGE_BOX_YES_CLICKED:
			OnOk();
			break;
		case MESSAGE_BOX_NO_CLICKED:
			OnCancel();
			break;
		}		
	}
	else
		inherited::SendMessage(pWnd, msg, pData);
}

void CChangeLevelWnd::OnOk()
{
	Game().StartStopMenu(this, true);
	NET_Packet p;
	p.w_begin(M_CHANGE_LEVEL);
	p.w(&m_game_vertex_id, sizeof(m_game_vertex_id));
	p.w(&m_level_vertex_id, sizeof(m_level_vertex_id));
	p.w_vec3(m_position);
	p.w_vec3(m_angles);

	Level().Send(p, net_flags(TRUE));
}

void CChangeLevelWnd::OnCancel()
{
	Game().StartStopMenu(this, true);

	if (m_b_position_cancel)	
		Actor()->MoveActor(m_position_cancel, m_angles_cancel);	
}

bool CChangeLevelWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (keyboard_action == WINDOW_KEY_PRESSED)
	{
		if (is_binded(kQUIT, dik))
			OnCancel();

		return true;
	}

	return inherited::OnKeyboard(dik, keyboard_action);
}

bool g_block_pause = false;

void CChangeLevelWnd::Show()
{
	g_block_pause = true;
	Device.Pause(TRUE, TRUE, TRUE, "CChangeLevelWnd_show");
	bShowPauseString = FALSE;
}

void CChangeLevelWnd::Hide()
{
	g_block_pause = false;
	Device.Pause(FALSE, TRUE, TRUE, "CChangeLevelWnd_hide");
}
