#include "Common.h"
#include"global.h"
#include <iostream>
#include <fstream>
#include<vector>
#include<mutex>
#include<string>
#include<map>
#include <chrono>
#include "terrain.cu"

#define SERVERPORT 9000
#define BUFSIZE    4096
using namespace std;
using namespace chrono;

//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);

map<int, players_profile*> players_list; //port, player_info
bool game_start = false;
int len = 0;

std::mutex player_cnt_lock;
TF sun_angle;

vector<SOCKET> player_list;
map <int, Citizen_moving*>citizen_Move;
map<int, resource_actor*> resource_create_landscape;

Terrain terrain;
char** total_terrain = terrain.get_map();
char** shadow_map = terrain.get_shadow_map();
char** temperature_map = terrain.get_temperature_map();
volatile int player_cnt;
volatile bool location_set = false;

UI_Input UI_input;


DWORD WINAPI terrain_change(LPVOID arg)
{
	terrain.show_array(total_terrain, 320);
	//terrain.log_on();
	int i{};
	while (1){
		//clock_t t_0 = clock();
		cout << endl << i << "��°" << endl;
		terrain.wind_blow({ 1, 0 }, 1);
		//terrain.make_shadow_map(i * 5);
		//terrain.make_tempertature_map(i * 5);

		//terrain.show_array(total_terrain, 320);
		//terrain.show_array(shadow_map, one_side_number);
		//terrain.show_array(temperature_map, one_side_number);
		//clock_t t_1 = clock();
		//cout << "[[[ Loop:" << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec ]] ]" << endl;
		if (i % 100 == 0 && i != 0) {
			terrain.save_terrain();
			cout << "SAVED!!!" << endl;
		}
		i++;
	}
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval = 0;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	FActor testActor;
	UI_Input UI_input;
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

	player_cnt_lock.lock();
	players_profile* my_profile = new players_profile;
	players_list[port] = my_profile;
	player_cnt++;
	players_list[port]->port = port;
	player_cnt_lock.unlock();
	
	//while (!player_location_set);

	Sleep(500);

	//player sight �ּҰ�
	char** player_sight_terrain = terrain.get_player_sight_map();
	char** player_sight_temperature = terrain.get_player_temperature_map(); 

	while (1){
		if (!location_set){
			continue;
		}
		retval = send(client_sock, (char*)&(players_list[port]->player_info), (int)sizeof(FActor), 0);
		for (int i = 0; i < 10; ++i){
			cout << i << "//" << players_list[port]->player_citizen[i]->location.x << ", " << players_list[port]->player_citizen[i]->location.y << endl;
			retval = send(client_sock, (char*)&(*players_list[port]->player_citizen[i]), (int)sizeof(FCitizen_sole), 0);
		}
		for (auto& a : players_list){
			if (port != a.first){
				retval = send(client_sock, (char*)&(a.second->player_info), (int)sizeof(FActor), 0);
				for (int i = 0; i < 10; ++i){
					retval = send(client_sock, (char*)&(*a.second->player_citizen[i]), (int)sizeof(FCitizen_sole), 0);
				}
			}
		}
		for (auto& a : resource_create_landscape) {
			retval = send(client_sock, (char*)&(*a.second), (int)sizeof(resource_actor), 0);
		}

		break;
	}
	Citizen_moving temp_citizen_moving;

	while (1) {
		start_t = high_resolution_clock::now();

		retval = send(client_sock, (char*)&sun_angle, (int)sizeof(TF), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		int playercnts = 0;
		for (int i = 0; i < 10; ++i) {
			retval = send(client_sock, (char*)&(*players_list[port]->player_citizen[i]), (int)sizeof(FCitizen_sole), 0);
		}
		playercnts = 0;
		for (auto& a : players_list) {
			if (a.second->port != port) {
				for (int i = 0; i < 10; ++i) {
					retval = send(client_sock, (char*)&(*a.second->player_citizen[i]), (int)sizeof(FCitizen_sole), 0);
				}
			}
			playercnts++;
		}

		for (auto& a : resource_create_landscape) {
			retval = send(client_sock, (char*)&(*a.second), (int)sizeof(resource_actor), 0);
		}

		retval = send(client_sock, (char*)&(players_list[port]->curr_location), (int)sizeof(TF), 0);
		retval = send(client_sock, (char*)&(players_list[port]->resources), sizeof(int) * 5, 0);

		//10�� ����ؼ� �ϴ� �׽�Ʈ
		//cout <<"CAM: " <<  (int)players_list[port]->camera_location.x << ", " << (int)players_list[port]->camera_location.y << endl;
		II player_location{ (int)players_list[port]->curr_location.x / 100, (int)players_list[port]->curr_location.y / 100 };
		terrain.copy_for_player_map(player_location);
		for (int i = 0; i < player_sight_size.x; ++i) {
			retval = send(client_sock, (char*)player_sight_terrain[i], (int)(sizeof(char) * player_sight_size.y), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
		}
		for (int i = 0; i < player_sight_size.x; ++i) {
			retval = send(client_sock, (char*)player_sight_temperature[i], (int)(sizeof(char) * player_sight_size.y), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
		}

		//Ŭ���̾�Ʈ�κ��� ī�޶� ��ġ �޾ƿ;� ��
		retval = recv(client_sock, (char*)&temp_citizen_moving, (int)sizeof(Citizen_moving), 0);
		mouse_input_checking(temp_citizen_moving, players_list, port);
		retval = recv(client_sock, (char*)&(players_list[port]->my_keyinput), (int)sizeof(keyboard_input), 0);
		retval = recv(client_sock, (char*)&(UI_input), (int)sizeof(UI_Input), 0);
		if (UI_input.resource_input.CitizenCountAdd)
		{
			Citizen_Work_Add(players_list, resource_create_landscape, port, UI_input.resource_input.ResourceNum);
		}
		if (UI_input.resource_input.CitizenCountSub)
		{
			Citizen_Work_Sub(players_list, resource_create_landscape, port, UI_input.resource_input.ResourceNum);
		}
		

	}
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",addr, ntohs(clientaddr.sin_port));
	// ���� �ݱ�
	closesocket(client_sock);
	return 0;
}

DWORD WINAPI ingame_thread(LPVOID arg)
{
	while (player_cnt != MAXPLAYER);

	player_random_location(players_list, citizen_Move);
	location_set = create_resource_location(players_list, resource_create_landscape);
	
	int player_list_iter{};
	for (auto& a : players_list) 
	{
		terrain.set_city_location(a.second->player_info.location, player_list_iter);
		player_list_iter++;
		cout << "��ġ : " << a.second->player_info.location.x << ", " << a.second->player_info.location.y << endl;
	}

	sun_angle.x = 0.0f;
	sun_angle.y = 0.0f;
	sun_angle.z = 0.0f;

	auto sunangle_start_t = high_resolution_clock::now();
	auto actor_move_start_t = high_resolution_clock::now();

	while (1) {
		auto sunangle_end_t = high_resolution_clock::now();
		auto actor_move_end_t = high_resolution_clock::now();
		
		if (duration_cast<milliseconds>(sunangle_end_t - sunangle_start_t).count() > 50) 
		{
			sunangle_start_t = high_resolution_clock::now();
			sun_angle.y += 0.2f;
			if (sun_angle.y >= 180.f) 
			{
				sun_angle.y = -180.f;
			}

			for (auto& a : players_list) 
			{
				float distance = 0.0f;
				int cnt = 0;
				for (auto& b : a.second->player_citizen) 
				{
					if (a.second->player_citizen_arrival_location[cnt]->team != -1) 
					{
						if (location_distance(b->location, a.second->player_citizen_arrival_location[cnt]->location) > 10) {
							Move_Civil(b->location, a.second->player_citizen_arrival_location[cnt]->location);
						}
					}
					cnt++;
				}/*
				cout << a.second->port << " ";
				for (int i = 0; i < 5;i++)
				{
					cout << a.second->resources[i] << " ";
				}
				cout << endl;*/
			}
			camera_movement(players_list);
		}
		if (duration_cast<milliseconds>(actor_move_end_t - actor_move_start_t).count() > 1000) 
		{
			actor_move_start_t = high_resolution_clock::now();
			resource_collect(players_list, resource_create_landscape);
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
	hThread = CreateThread(NULL, 0, ingame_thread,0, 0, NULL);
	hThread = CreateThread(NULL, 0, terrain_change,0, 0, NULL);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) 
		{
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
