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

//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);

typedef struct three_float {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
}TF;

typedef struct transform {
	TF location;
	TF rotation;
}T;

map<int, T*> players_list; //port, player_info
bool game_start = false;
int len = 0;

std::mutex mylock;
TF sun_angle;
T testActor;
vector<SOCKET> player_list;

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];
	
	game_start = true;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",addr, ntohs(clientaddr.sin_port));
	auto start_t = high_resolution_clock::now();
	
	T player_info;
	//�ش� Ŭ���̾�Ʈ�� port��ȣ map�� ����
	int port = ntohs(clientaddr.sin_port);
	players_list[port] = &player_info;
	
	while (1) {
		auto end_t = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50){
			start_t = high_resolution_clock::now();
			retval = send(client_sock, (char*)&sun_angle, (int)sizeof(TF), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			retval = send(client_sock, (char*)&testActor, (int)sizeof(T), 0);
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
	/*sun_angle.x = 0.0f;
	sun_angle.y = 0.0f;
	sun_angle.z = 0.0f;
	testActor.location.x = 0.0f;
	testActor.location.y = 0.0f;
	testActor.location.z = 500.0f;
	testActor.rotation.x = 0.0f;
	testActor.rotation.y = 0.0f;
	testActor.rotation.z = 0.0f;*/
	
	auto sunangle_start_t = high_resolution_clock::now();
	auto actor_move_start_t = high_resolution_clock::now();
	
	while (1){
		auto sunangle_end_t = high_resolution_clock::now();
		auto actor_move_end_t = high_resolution_clock::now();
		
		if (duration_cast<milliseconds>(sunangle_end_t - sunangle_start_t).count() > 50){
			sunangle_start_t = high_resolution_clock::now();
			sun_angle.y += 0.2f;
			if (sun_angle.y >= 180.f) {
				sun_angle.y = -180.f;
			}
			
			testActor.location.x += testActor.rotation.x * 0.5;
			testActor.location.y += testActor.rotation.y * 0.5;
			//cout << testActor.location_x << ", " << testActor.location_y << endl;
		}
		if (duration_cast<milliseconds>(actor_move_end_t - actor_move_start_t).count() > 5000){
			actor_move_start_t = high_resolution_clock::now();
			testActor.rotation.x = rand() % 100;
			testActor.rotation.y = rand() % 100;
			if (rand() % 2 == 1) {
				testActor.rotation.x *= -1;
			}
			if (rand() % 2 == 1) {
				testActor.rotation.y *= -1;
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
