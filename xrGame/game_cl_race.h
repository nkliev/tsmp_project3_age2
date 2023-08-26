#pragma once
#include "game_cl_mp.h"

class CUIGameRace;
class CStringTable;

class game_cl_Race : public game_cl_mp
{
	typedef game_cl_mp inherited;
	CUIGameRace* m_game_ui;
	u32 m_ReinforcementTime;
	u16 m_WinnerId;
	bool m_WinnerMessageSet;

	void LoadTeamBaseParticles();
	void UpdateRaceStart();
	void UpdateRaceInProgress();
	void UpdateRaceScores();
	void UpdateCountdownCaption();
	void LoadSounds();

public:
	game_cl_Race();
	~game_cl_Race() override = default;

	CUIGameCustom* createGameUI() override;
	void shedule_Update(u32 dt) override;
	void Init() override;
	void OnRender() override;
	void UpdateMapLocations() override;

	bool OnKeyboardPress(int key) override;
	bool OnKeyboardRelease(int key) override;
	void OnSwitchPhase(u32 oldPhase, u32 newPhase) override;

	void SetUI(CUIGameRace* gameUi);
	void net_import_state(NET_Packet &P) override;
	CUIGameBaseMP* GetMpGameUI() override { return reinterpret_cast<CUIGameBaseMP*>(m_game_ui); }
};
