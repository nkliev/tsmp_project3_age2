#pragma once
#include <string>
using namespace std;

class ObjectRespawnClass
{
public:

	// �������� � ������� - ������� �������, � ������� ������� �������� � ����������� �������� � ������� ���.
	void CheckRespawnObjects();

	// � xrServer::Process_event_destroy
	int DestroyRespawnID(int id);

	// �������� ������ � ������� xrServer::SLS_Default()
	void AddObject(string _section, int _id, int _time_respawn, float _pos_x, float _pos_y, float _pos_z);

	// ������� ����� ������� ������ level.spawn xrServer::SLS_Default()
	void DestroyRespawner();


	int id_object;
	int time_tick;
	int time_respawn;
	float pos_x;
	float pos_y;
	float pos_z;
	string section;
};

/*
	� xrServer::SLS_Default()
	1) ����� ������� level spawn ����� �������� ������
	ServerRespawnManager.DestroyRespawner();

	2) � ����� ���� ������ Process_spawn(packet, clientID); ��
	CSE_Abstract* E = Process_spawn(packet, clientID);
	ServerRespawnManager.AddObject(E->s_name.c_str(), E->ID, E->RespawnTime, E->o_Position.x, E->o_Position.y, E->o_Position.z);


	1) � xrServer::Process_event_destroy
	��������� ServerRespawnManager.DestroyRespawnID(id_dest);


	�� ������� ��� � ������� ��� � 10 (����� ����� ��������  ��     respawn->time_tick +=10) ������ �������� ������� CheckRespawnObjects
*/
