#include "stdafx.h"
#include "VoiceChat.h"
#include "Level.h"
#include "../xrSound/voicechat/SoundVoiceChat.h"
#include "game_cl_mp.h"
#include "Actor.h"
#include "Inventory.h"

CVoiceChat::CVoiceChat()
{
	m_pSoundVoiceChat = ::Sound->GetSoundVoiceChat();
	CreateRecorder();
}

CVoiceChat::~CVoiceChat()
{
	xr_delete(m_pSender);

	if (m_pSoundVoiceChat)
	{
		for (auto I = m_soundPlayersMap.begin(); I != m_soundPlayersMap.end(); ++I)
			m_pSoundVoiceChat->DestroySoundPlayer(I->second);
	}

	m_soundPlayersMap.clear();
}

void CVoiceChat::CreateRecorder()
{
	if (m_pSender)
	{
		xr_delete(m_pSender);
		m_pSender = nullptr;
	}

	m_pSender = xr_new<CVoiceSender>();
	m_pSender->SetDistance(10); // default
	m_pRecorder = nullptr;

	if(m_pSoundVoiceChat)
		m_pRecorder = m_pSoundVoiceChat->CreateRecorder((IVoicePacketSender*)m_pSender);
}

void CVoiceChat::Start()
{
	if (m_pRecorder)
		m_pRecorder->Start();
}

void CVoiceChat::Stop()
{
	if (m_pRecorder)
		m_pRecorder->Stop();
}

bool CVoiceChat::IsStarted()
{
	if (m_pRecorder)
		return m_pRecorder->IsStarted();

	return false;
}

u8 CVoiceChat::GetDistance() const
{
	return m_pSender->GetDistance();
}

u8 CVoiceChat::SwitchDistance()
{
	R_ASSERT(m_pSender);

	switch (m_pSender->GetDistance())
	{
	case 5:
		m_pSender->SetDistance(10);
		return 10;
	case 10:
		m_pSender->SetDistance(30);
		return 30;
	default:
		m_pSender->SetDistance(5);
		return 5;
	}
}

void CVoiceChat::Update()
{
	CheckAndClearPlayers(m_soundPlayersMap);
}
//
//void CVoiceChat::OnRender()
//{
//	if (psActorFlags.test(AF_DISPLAY_VOICE_ICON))
//	{
//		constexpr Fvector pos = Fvector{ 0.0, 0.5, 0.0 };
//
//		game_PlayerState* local_player = Game().local_player;
//		auto& players = Game().players;
//
//		for (auto it = players.begin(); it != players.end(); ++it)
//		{
//			game_PlayerState* ps = it->second;
//			u16 id = ps->GameID;
//
//			if (ps == local_player || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || ps->testFlag(GAME_PLAYER_MP_INVIS))
//				continue;
//
//			auto voiceTimeIt = m_voiceTimeMap.find(id);
//			if (voiceTimeIt == m_voiceTimeMap.end())
//				continue;
//
//			auto& voiceIconInfo = voiceTimeIt->second;
//
//			if (voiceIconInfo.time + 200 < GetTickCount())
//				continue;
//
//			CObject* pObject = Level().Objects.net_Find(id);
//			if (!pObject) continue;
//
//			CActor* pActor = smart_cast<CActor*>(pObject);
//			if (!pActor) continue;
//
//			pActor->RenderIndicator(pos, 0.2, 0.2, GetVoiceIndicatorShader());
//		}
//	}
//}

void CVoiceChat::ReceiveMessage(NET_Packet* P)
{
	game_PlayerState* local_player = Game().local_player;

	if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) || !Actor() || !m_pSoundVoiceChat)
		return;

	u8 voiceDistance = P->r_u8();

	u16 clientId = P->r_u16();
	CObject* obj = Level().Objects.net_Find(clientId);

	if (!obj)
		return;

	if (Actor()->Position().distance_to(obj->Position()) > voiceDistance)
		return;

	IStreamPlayer* player = GetStreamPlayer(clientId);
	player->SetPosition(obj->Position());
	player->SetDistance(voiceDistance);
	u8 packetsCount = P->r_u8();

	for (u32 i = 0; i < packetsCount; ++i)
	{
		u32 length = P->r_u32();
		P->r(m_buffer, length);
		player->PushToPlay(m_buffer, length);
	}

	m_voiceTimeMap[clientId] = SVoiceIconInfo(GetTickCount());
}


//const ui_shader& CVoiceChat::GetVoiceIndicatorShader()
//{
//	if (m_voiceIndicatorShader->inited()) return m_voiceIndicatorShader;
//
//	m_voiceIndicatorShader->create("friendly_indicator", "ui\\ui_voice");
//	return m_voiceIndicatorShader;
//}

IStreamPlayer* CVoiceChat::GetStreamPlayer(u16 clientId)
{
	if (IStreamPlayer* player = m_soundPlayersMap[clientId])
		return player;

	IStreamPlayer* player = m_pSoundVoiceChat->CreateStreamPlayer();
	m_soundPlayersMap[clientId] = player;
	return player;
}

void CVoiceChat::CheckAndClearPlayers(SOUND_PLAYERS& players)
{
	for (auto I = players.begin(), E = players.end(); I != E; )
	{
		if (CObject* obj = Level().Objects.net_Find(I->first))
		{
			++I;
			continue;
		}

		auto J = I;
		++I;

		if (auto voiceTimeIt = m_voiceTimeMap.find(J->first) != m_voiceTimeMap.end())
			m_voiceTimeMap.erase(voiceTimeIt);

		if (m_pSoundVoiceChat)
			m_pSoundVoiceChat->DestroySoundPlayer(J->second);

		players.erase(J);
	}
}
