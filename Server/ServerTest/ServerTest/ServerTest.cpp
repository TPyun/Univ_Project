#include "..\..\Common.h"
#include <iostream>
#include <fstream>
#include<vector>
#include<mutex>
#include<string>
#include<map>
#include <chrono>
#define SERVERPORT 9000
#define BUFSIZE    4096
using namespace std;
using namespace chrono;

int len = 0;
char buffer[BUFSIZE]; // ���� ���� ������
std::mutex mylock;


//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);
 
//���� ����
int hostnum;

//
bool game_start = false;

struct SunAngle {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Actor_location_rotation {
public:
	float location_x, location_y, location_z;
	float rotate_x, rotate_y, rotate_z;

};
SunAngle sun_angle;
vector<SOCKET> player_list;
Actor_location_rotation testActor;

int player_num;

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];
	
	FILE* fp;

	game_start = true;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));
	auto start_t = high_resolution_clock::now();
	while (1) {

		auto end_t = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50)
		{
			start_t = high_resolution_clock::now();
			retval = send(client_sock, (char*)&sun_angle, (int)sizeof(SunAngle), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			retval = send(client_sock, (char*)&testActor, (int)sizeof(Actor_location_rotation), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			//cout << sun_angle.y << endl;
		}


	}

	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));
	// ���� �ݱ�
	closesocket(client_sock);

	return 0;
}
DWORD WINAPI ingame_thread(LPVOID arg)
{
	sun_angle.x = 0.0f;
	sun_angle.y = 0.0f;
	sun_angle.z = 0.0f;
	/*testActor.location_x = 0.0f;
	testActor.location_y = 0.0f;
	testActor.location_z = 0.0f;*/
	testActor.location_x = 0.0f;
	testActor.location_y = 0.0f;
	testActor.location_z = 500.0f;
	testActor.rotate_x = 0.0f;
	testActor.rotate_y = 0.0f;
	testActor.rotate_z = 0.0f;
	auto start_t = high_resolution_clock::now();
	auto actor_move_start= high_resolution_clock::now();
	while (1)
	{
		
		auto end_t = high_resolution_clock::now();
		auto actor_move_end = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50)
		{
			start_t = high_resolution_clock::now();
			sun_angle.y += 0.2f;
			if (sun_angle.y >= 180.f)
				sun_angle.y = -180.f;
			testActor.location_x += testActor.rotate_x * 0.5;
			testActor.location_y += testActor.rotate_y * 0.5;
			cout << testActor.location_x << ", " << testActor.location_y << endl;
		}
		if (duration_cast<milliseconds>(actor_move_end - actor_move_start).count() > 5000)
		{
			actor_move_start = high_resolution_clock::now();
			testActor.rotate_x = rand() % 100;
			testActor.rotate_y = rand() % 100;
			if (rand() % 2 == 1)
			{
				testActor.rotate_x *= -1;
			}
			if (rand() % 2 == 1)
			{
				testActor.rotate_y *= -1;
			}

		}

	}

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, ingame_thread,
		0, 0, NULL);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

		cout << addr << endl;

		player_list.emplace_back(client_sock);
		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		//���� �ݱ�
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }

	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
