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

Terrain* terrain = new Terrain();
char** total_terrain = terrain->get_map();
char** temperature_map = terrain->get_temperature_map();
volatile int player_cnt;
volatile bool location_set = false;


DWORD WINAPI terrain_change(LPVOID arg)
{
	/*char** player_sight_terrain = terrain->get_player_sight_map();
	char** player_sight_temperature = terrain->get_player_temperature_map();
	terrain->set_city_location(TF{ 20000, 20000 }, 0);*/

	//terrain->show_array(total_terrain, 320);
	//terrain->log_on();
	int i{};
	while (1){
		sun_angle.y += 5;
		//clock_t t_0 = clock();
		//cout << endl << i << "��°" << endl;
		terrain->wind_blow({ 1, 0 }, 10);
		terrain->make_shadow_map(sun_angle.y);
		terrain->make_tempertature_map(sun_angle.y);

		/*terrain->show_array(total_terrain, 320);
		terrain->show_array(temperature_map, 320);*/


		/*terrain->copy_for_player_map(II{ 200, 200 });
		terrain->show_array(player_sight_terrain, 120);
		terrain->show_array(player_sight_temperature, 120);*/

		
		//clock_t t_1 = clock();
		//cout << "[[[ Loop:" << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec ]] ]" << endl;
		if (i % 100 == 0 && i != 0) {
			terrain->save_terrain();
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
	game_start = true;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	auto start_t = high_resolution_clock::now();
	FirstSendServer first_send_server;
	FirstSendClient first_send_client;
	FActor player_info;
	//�ش� Ŭ���̾�Ʈ�� port��ȣ map�� ����
	int port = ntohs(clientaddr.sin_port);

	player_cnt_lock.lock();
	
	if (player_cnt > MAXPLAYER)
	{
		closesocket(client_sock);
		return 0;
	}
	
	players_profile* my_profile = new players_profile;
	players_list[port] = my_profile;
	
	players_list[port]->port = port;
	player_cnt++;
	player_cnt_lock.unlock();
	
	//while (!player_location_set);

	Sleep(500);

	//player sight �ּҰ�
	char** player_sight_terrain = terrain->get_player_sight_map();
	char** player_sight_temperature = terrain->get_player_temperature_map();
	int maxplayer_cnt = 0;
	int trash_value = 0;
	while (1){
		if (!location_set){
			retval = send(client_sock, (char*)&maxplayer_cnt, sizeof(int), 0);
			retval = recv(client_sock, (char*)&trash_value, sizeof(int), 0);
			continue;
		}
		maxplayer_cnt = MAXPLAYER;
		send(client_sock, (char*)&(maxplayer_cnt), sizeof(int), 0);
		retval = recv(client_sock, (char*)&trash_value, sizeof(int), 0);

		
		FirstInit(first_send_server, first_send_client, players_list, resource_create_landscape, player_sight_temperature, port);

		retval = send(client_sock, (char*)&(first_send_server), (int)sizeof(FirstSendServer), 0);
		

		break;
	}
	Citizen_moving temp_citizen_moving;

	while (1) {
		auto end_t = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50)
		{
			start_t = high_resolution_clock::now();
			memcpy(&first_send_server.SunAngle, &sun_angle, sizeof(TF));
			retval = send(client_sock, (char*)&(first_send_server), (int)sizeof(FirstSendServer), 0);
			
			////10�� ����ؼ� �ϴ� �׽�Ʈ
			////cout <<"CAM: " <<  (int)players_list[port]->camera_location.x << ", " << (int)players_list[port]->camera_location.y << endl;
			//II player_location{ (int)players_list[port]->curr_location->x / 100, (int)players_list[port]->curr_location->y / 100 };
			//terrain.copy_for_player_map(player_location);
			/*for (int i = 0; i < player_sight_size.x; ++i) {
				retval = send(client_sock, (char*)player_sight_terrain[i], (int)(sizeof(char) * player_sight_size.y), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}
			for (int i = 0; i < player_sight_size.x; ++i) {
			}
		}
		for (int i = 0; i < player_sight_size.x; ++i) {
			retval = send(client_sock, (char*)player_sight_temperature[i], (int)(sizeof(char) * player_sight_size.y), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}*/

			int tempsa = recv(client_sock, (char*)&(first_send_client), (int)sizeof(FirstSendClient), 0);
			if (tempsa == SOCKET_ERROR)
			{
				return 0;
			}
			mouse_input_checking(first_send_client.My_citizen_moving, players_list, port);
			if (first_send_client.My_UI_input.resource_input.CitizenCountAdd)
			{
				Citizen_Work_Add(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
			}
			if (first_send_client.My_UI_input.resource_input.CitizenCountSub)
			{
				Citizen_Work_Sub(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
			}
		}
		else
		{
			Sleep(1);
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
		terrain->set_city_location(a.second->player_info->location, player_list_iter);
		player_list_iter++;
		cout << "��ġ : " << a.second->player_info->location.x << ", " << a.second->player_info->location.y << endl;
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
					if(b != NULL)
					{
						if (a.second->player_citizen_arrival_location[cnt]->team != -1)
						{
							if (location_distance(b->location, a.second->player_citizen_arrival_location[cnt]->location) > 10) {
								Move_Civil(b->location, a.second->player_citizen_arrival_location[cnt]->location);
							}
						}
					}
					cnt++;
				}
				/*cout << a.second->port << " ";
				for (int i = 0; i < 5;i++)
				{
					cout << a.second->resources[i] << " ";
				}
				cout << endl;*/
			}
			camera_movement(players_list);
		}
		else if (duration_cast<milliseconds>(actor_move_end_t - actor_move_start_t).count() > 1000)
		{
			actor_move_start_t = high_resolution_clock::now();
			resource_collect(players_list, resource_create_landscape);
		}
		else
		{
			Sleep(1);
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
