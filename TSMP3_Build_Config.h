#pragma once

#define ALIFE_MP
//#define DEDICATED_SERVER // ���������� ������
#if defined(DEDICATED_BUILD) && !defined(DEDICATED_SERVER)
//�������� ���������� � ����� ������ 
#define DEDICATED_SERVER
#endif
//#define PUBLIC_BUILD // ��������� ������, ��������� ��������� ���������� �������
//#define MP_LOGGING // ����� � ��� ���������� ������� �������
//#define FZ_MOD_CLIENT // ���, ����������� ������ � ������� fz
//#define NO_SINGLE // ��������� ��������� �����
//#define NO_MULTI_INSTANCES // ��������� ��������� ��������� ����������� ����
//#define EVERYBODY_IS_ENEMY // ��� ��� ������� ���� �������
#define TSMP_VERSION "R14_test"

#ifdef PUBLIC_BUILD
#define SEND_ERROR_REPORTS // ���������� ������ � �������
#endif
