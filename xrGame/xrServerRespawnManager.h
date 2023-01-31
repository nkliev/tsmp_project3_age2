#pragma once
class ObjectRespawnClass
{
public:

	// �������� � ������� - ������� �������, � ������� ������� �������� � ����������� �������� � ������� ���.
	static void CheckRespawnObjects();

	// � xrServer::Process_event_destroy
	static int DestroyRespawnID(u16 id);

	// �������� ������ � ������� xrServer::SLS_Default()
	static void AddObject(shared_str &pSection, u16 pID, int pTimeRespawn, Fvector &XYZ);

	// ������� ����� ������� ������ level.spawn xrServer::SLS_Default()
	static void DestroyRespawner();

	u16 id_object;
	int time_tick;
	u16 time_respawn;
	Fvector spawn_position;
	shared_str section;
};