#pragma once
class ObjectRespawnClass
{
public:

	// �������� � ������� - ������� �������, � ������� ������� �������� � ����������� �������� � ������� ���.
	static void CheckRespawnObjects();

	// � xrServer::Process_event_destroy
	static int DestroyRespawnID(u16 id);

	// � xrServer::Process_event_destroy
	static int GetRespawnObjectID(u16 id);

	// �������� ������ � ������� xrServer::SLS_Default()
	static void AddObject(shared_str& pSection, shared_str &pCustomData, u16 pID, int pTimeRespawn, Fvector& XYZ);

	// ������� ����� ������� ������ level.spawn xrServer::SLS_Default()
	static void DestroyRespawner();

	u16 id_object;
	int time_respawn;
	int time_tick;
	shared_str section;
	shared_str custom_data;
	Fvector spawn_position;
};