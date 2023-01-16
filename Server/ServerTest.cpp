#include "Common.h"
#include"global.h"
#include <iostream>
#include <fstream>
#include<vector>
#include<mutex>
#include<string>
#include<map>
#include <chrono>
#include "map.cu"

#define SERVERPORT 9000
#define BUFSIZE    4096
using namespace std;
using namespace chrono;

//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);


map<int, FActor*> players_list; //port, player_info
bool game_start = false;
int len = 0;

std::mutex mylock;
TF sun_angle;

vector<SOCKET> player_list;
map <FActor*, location_rotation>my_citizen;

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval = 0;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	FActor testActor;
	game_start = true;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	auto start_t = high_resolution_clock::now();

	FActor player_info;
	//�ش� Ŭ���̾�Ʈ�� port��ȣ map�� ����
	int port = ntohs(clientaddr.sin_port);
	players_list[port] = &player_info;

	int cnt = 0;
	retval = recv(client_sock, (char*)&cnt, sizeof(int), 0);

	if (retval == SOCKET_ERROR) {
		err_display("send()");
		return 0;
	}
	for (int i = 0; i < cnt; ++i)
	{
		bool overlap = false;
		retval = recv(client_sock, (char*)&testActor, sizeof(testActor), 0);
		for (auto& a : my_citizen){
			if (wcscmp(a.first->name, testActor.name) == 0)
			{
				overlap = true;
			}
		}
		if(!overlap){
			FActor* tempActor = new FActor();
			wcscpy(tempActor->name, testActor.name);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				return 0;
			}
			FActor_TF_define(my_citizen[tempActor].location, testActor.location);
			FActor_TF_define(tempActor->location, testActor.location);
			FActor_TF_define(my_citizen[tempActor].rotation, testActor.rotation);
			FActor_TF_define(tempActor->rotation, testActor.rotation);
		}
		/*wcout << testActor.name;
		cout << " : " << testActor.location.x << ", " << testActor.location.y << endl;*/
	}

	while (1) {
		auto end_t = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50) {
			start_t = high_resolution_clock::now();
			retval = recv(client_sock, (char*)&testActor, (int)sizeof(testActor), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			if (wcscmp(testActor.name, L"temp") != 0){
				for (auto& a : my_citizen){{
						FActor_TF_define(a.second.location, testActor.location);	
					}
				}
			}
			retval = send(client_sock, (char*)&sun_angle, (int)sizeof(TF), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			for (auto& a : my_citizen){
				retval = send(client_sock, (char*)&(*a.first), (int)sizeof(testActor), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				wcout << a.first->name;
				cout << " : " << a.second.location.x << ", " << a.second.location.y << endl;
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

	while (1) {
		auto sunangle_end_t = high_resolution_clock::now();
		auto actor_move_end_t = high_resolution_clock::now();

		if (duration_cast<milliseconds>(sunangle_end_t - sunangle_start_t).count() > 50) {
			sunangle_start_t = high_resolution_clock::now();
			sun_angle.y += 0.2f;
			if (sun_angle.y >= 180.f) {
				sun_angle.y = -180.f;
			}
			for (auto& a : my_citizen){
				float distance = 0.0f;
				if (location_distance(a.first->location, a.second.location) > 10){
					Move_Civil(a.first->location, a.second.location);
				}
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	//Make Random Hills Information===================================================
	clock_t t_0 = clock();

	HI* hill_location_host = new HI[4000];
	HI* hill_location_device;
	cudaMalloc((void**)&hill_location_device, 4000 * sizeof(HI));
	int num_of_hills = make_hill_location(hill_location_host);
	int origin_num_of_hills = num_of_hills;
	cudaMemcpy(hill_location_device, hill_location_host, num_of_hills * sizeof(HI), cudaMemcpyHostToDevice); //Memcpy to Device
	printf("Random Hill Info Complete\n");
	for (int i = 0; i < num_of_hills; i++) {
		cout << hill_location_host[i].x << ", " << hill_location_host[i].y << ", " << hill_location_host[i].height << ", " << hill_location_host[i].radius << endl;
	}

	//Terrain Memory Assignement===================================================
	clock_t t_1 = clock();
	
	char** terrain_array_host = new char* [one_side_number];	// 2D array for host
	for (int i = 0; i < one_side_number; i++) {
		terrain_array_host[i] = new char[one_side_number];
	}
	for (int i = 0; i < one_side_number; i++) {
		for (int j = 0; j < one_side_number; j++) {
			terrain_array_host[i][j] = 0;
		}
	}
	char** terrain_array_device;					// 2D array for device
	char* terrain_array_temp[one_side_number];		// 1D array temp
	cudaMalloc((void**)&terrain_array_device, one_side_number * sizeof(char*));
	for (int i = 0; i < one_side_number; i++) {
		cudaMalloc((void**)&terrain_array_temp[i], one_side_number * sizeof(char));
	}
	cudaMemcpy(terrain_array_device, terrain_array_temp, one_side_number * sizeof(char*), cudaMemcpyHostToDevice);
	for (int i = 0; i < one_side_number; i++) {
		cudaMemcpy(terrain_array_temp[i], terrain_array_host[i], one_side_number * sizeof(char), cudaMemcpyHostToDevice);
	}


	//Terrain Memory Assignment For Player's Sight===================================================
	clock_t t_2 = clock();

	char** terrain_player_sight_host = new char* [player_sight_size];	// 2D array for host
	for (int i = 0; i < player_sight_size; i++) {
		terrain_player_sight_host[i] = new char[player_sight_size];
	}
	for (int i = 0; i < player_sight_size; i++) {
		for (int j = 0; j < player_sight_size; j++) {
			terrain_player_sight_host[i][j] = 0;
		}
	}
	char** terrain_player_sight_device;						// 2D array for device
	char* terrain_player_sight_temp[player_sight_size];		// 1D array temp
	cudaMalloc((void**)&terrain_player_sight_device, player_sight_size * sizeof(char*));
	for (int i = 0; i < player_sight_size; i++) {
		cudaMalloc((void**)&terrain_player_sight_temp[i], player_sight_size * sizeof(char));
	}
	cudaMemcpy(terrain_player_sight_device, terrain_player_sight_temp, player_sight_size * sizeof(char*), cudaMemcpyHostToDevice);
	for (int i = 0; i < player_sight_size; i++) {
		cudaMemcpy(terrain_player_sight_temp[i], terrain_player_sight_host[i], player_sight_size * sizeof(char), cudaMemcpyHostToDevice);
	}

	


	//Make Hills===================================================
	clock_t t_3 = clock();
	
	make_hills_cuda << <one_side_number, num_of_hills >> > (terrain_array_device, hill_location_device);
	for (int i = 0; i < one_side_number; i++) {
		cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
	}
	printf("Terrain Generation Complete\n");

	clock_t  t_4 = clock();

	//show_array(terrain_array_host, one_side_number);
	cout << "Terrain size : " << one_side_number << " * " << one_side_number << endl;
	cout << "Terrain Array Size : " << one_side_number * one_side_number * sizeof(char) << " Bytes" << endl;
	cout << "Make Random Hills Information : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "Terrain Memory Assignement : " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "Terrain Memory Assignment For Player's Sight : " << (double)(t_3 - t_2) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "Make Hills : " << (double)(t_4 - t_3) / CLOCKS_PER_SEC << " sec" << endl;


	//Terrain move & Player Sight Update===================================================
	//II player_location = { 0, 0 };		//�̰� ���߿� �߽� �������� �ٲ����
	//int wind_angle = 270;		//����
	//int wind_speed = 50;		//�ִ� ǳ�� 50
	//for (int i = 0; i < 1; i++) {
	//	clock_t t_1 = clock();

	//	//Terrain Move
	//	wind_decide(wind_speed, wind_angle);

	//	FF wind_direction = { cos(wind_angle * PI / 180), sin(wind_angle * PI / 180) };
	//	if (abs(wind_direction.x) < FLT_EPSILON) {
	//		wind_direction.x = 0;
	//	}
	//	if (abs(wind_direction.y) < FLT_EPSILON) {
	//		wind_direction.y = 0;
	//	}

	//	move_terrain(hill_location_host, num_of_hills, wind_direction, wind_speed);
	//	if (num_of_hills < origin_num_of_hills) {
	//		make_new_hills(hill_location_host, num_of_hills, origin_num_of_hills, wind_direction, wind_speed);
	//	}

	//	cudaMemcpy(hill_location_device, hill_location_host, num_of_hills * sizeof(HI), cudaMemcpyHostToDevice); //Memcpy to Device

	//	//Player Sight Update
	//	//player_location.x += 20;
	//	//player_location.y += 20;
	//	//thread must be 1024 for efficiency
	//	player_terrain_update_cuda << <player_sight_size, player_sight_size >> > (terrain_player_sight_device, hill_location_device, num_of_hills, player_location, wind_direction, wind_speed);
	//	for (int i = 0; i < player_sight_size; i++) {
	//		cudaMemcpy(terrain_player_sight_host[i], terrain_player_sight_temp[i], player_sight_size * sizeof(char), cudaMemcpyDeviceToHost);
	//	}
	//	clock_t t_2 = clock();
	//	cout << "Player Sight Update Time : " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " Seconds" << endl;
	//	//show_array(terrain_player_sight_host, player_sight_size);
	//	cout << "==============================" << endl;
	//}
	//=========================================================================================
	
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
