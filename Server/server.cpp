//#include "Common.h"
//#include <iostream>
//#include <fstream>
//#include<vector>
//#include<mutex>
//#include<string>
//#include<map>
//#include <chrono>
//#include<shared_mutex>
//#include "terrain.cu"
//#include"global.h"
//
//#define SERVERPORT 9000
//#define BUFSIZE    4096
//using namespace std;
//using namespace chrono;
//
////�Լ����� 
//DWORD WINAPI ProcessClient(LPVOID arg);
//DWORD WINAPI ingame_thread(LPVOID arg);
//
//map<int, players_profile*> players_list; //port, player_info
//
//std::mutex player_cnt_lock;
//shared_mutex player_list_lock;
//float sun_angle;
//
//vector<SOCKET> player_list;
//map <int, Citizen_moving*>citizen_Move;
//map<int, resource_actor*> resource_create_landscape;
//
//Terrain* terrain = new Terrain();
//char** total_terrain = terrain->get_map();
//char** shadow_map = terrain->get_shadow_map();
//unsigned char** temperature_map = terrain->get_temperature_map();
//volatile int player_cnt;
//volatile bool location_set = false;
//int ingame_play = false;
//
//DWORD WINAPI terrain_change(LPVOID arg)
//{
//	/*char** player_sight_terrain = terrain->get_player_sight_map();
//	char** player_sight_temperature = terrain->get_player_temperature_map();
//	terrain->set_city_location(TF{ 20000, 20000 }, 0);*/
//
//	//terrain->show_array(total_terrain, 320);
//	terrain->log_on();
//	int i{};
//	while (1){
//		//clock_t t_0 = clock();
//		cout << endl << i << "��°" << endl;
//
//		terrain->wind_blow({ 1, 0 }, 1);
//		terrain->make_shadow_map(sun_angle);
//		terrain->make_tempertature_map(sun_angle);
//		CC retval = terrain->get_highest_lowest(temperature_map);
//
//		/*if (sun_angle > 360)
//			sun_angle = 0;
//		sun_angle += 6;*/
//		cout << "sun_angle : " << sun_angle << endl;
//		cout << "Temperature Highest: " << (float)retval.x / 4 << ", Lowest" << (float)retval.y / 4 << endl;
//
//		//terrain->show_array(total_terrain, 320);
//		//terrain->show_array(shadow_map, 320);
//		//terrain->show_array(temperature_map, 320);
//
//
//		/*terrain->copy_for_player_map(II{ 200, 200 });
//		terrain->show_array(player_sight_terrain, 120);
//		terrain->show_array(player_sight_temperature, 120);*/
//
//
//		//clock_t t_1 = clock();
//		//cout << "[[[ Loop:" << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec ]] ]" << endl;
//		if (i % 100 == 0 && i != 0) {
//			terrain->save_terrain();
//			cout << "SAVED!!!" << endl;
//		}
//		i++;
//	}
//}
//
//DWORD WINAPI ProcessClient(LPVOID arg)
//{
//	int retval = 0;
//	SOCKET client_sock = (SOCKET)arg;
//	struct sockaddr_in clientaddr;
//	char addr[INET_ADDRSTRLEN];
//	int addrlen;
//	
//	// Ŭ���̾�Ʈ ���� ���
//	addrlen = sizeof(clientaddr);
//	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
//	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
//	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
//	auto start_t = high_resolution_clock::now();
//	
//	ServerStruct1 first_send_server;
//	ServerStruct2 second_send_server;
//	ClientStruct1 first_send_client;
//	//�ش� Ŭ���̾�Ʈ�� port��ȣ map�� ����
//	int port = ntohs(clientaddr.sin_port);
//
//	player_cnt_lock.lock();
//	
//	if (player_cnt >= MAXPLAYER)
//	{
//		closesocket(client_sock);
//		return 0;
//	}
//	
//	players_profile* my_profile = new players_profile;
//	players_list[port] = my_profile;
//	
//	
//	players_list[port]->port = port;
//	int temp = player_cnt++;
//	player_cnt_lock.unlock();
//	
//	Sleep(500);
//
//	//player sight �ּҰ�
//	char** player_sight_terrain = terrain->get_player_sight_map();
//	char** player_sight_temperature = terrain->get_player_temperature_map();
//
//
//	while (!location_set);
//	while (1){
//
//		FirstInit(first_send_server, first_send_client, players_list, player_sight_temperature, port);
//		Secondmemcpy(second_send_server, players_list, resource_create_landscape, port);
//		retval = send(client_sock, (char*)&(first_send_server), (int)sizeof(ServerStruct1), 0);
//		retval = send(client_sock, (char*)&(second_send_server), (int)sizeof(ServerStruct2), 0);
//		break;
//	}
//	Citizen_moving temp_citizen_moving;
//
//	while (1) {
//		auto end_t = high_resolution_clock::now();
//		if (duration_cast<milliseconds>(end_t - start_t).count() > 50)
//		{
//			shared_lock<shared_mutex> lock(player_list_lock);
//			start_t = high_resolution_clock::now();
//			first_send_server.SunAngle = sun_angle;
//
//			Secondmemcpy(second_send_server, players_list, resource_create_landscape, port);
//			retval = send(client_sock, (char*)&(first_send_server), (int)sizeof(ServerStruct1), 0);
//			retval = send(client_sock, (char*)&(second_send_server), (int)sizeof(ServerStruct2), 0);
//
//			////10�� ����ؼ� �ϴ� �׽�Ʈ
//			////cout <<"CAM: " <<  (int)players_list[port]->camera_location.x << ", " << (int)players_list[port]->camera_location.y << endl;
//			
//
//			II player_location{ (int)players_list[port]->curr_location->x / 100, (int)players_list[port]->curr_location->y / 100 };
//			terrain->copy_for_player_map(player_location);
//			for (int i = 0; i < player_sight_size.x; ++i) {
//				retval = send(client_sock, (char*)player_sight_terrain[i], (int)(sizeof(char) * player_sight_size.y), 0);
//			}
//			for (int i = 0; i < player_sight_size.x; ++i) {
//				retval = send(client_sock, (char*)player_sight_temperature[i], (int)(sizeof(char) * player_sight_size.y), 0);
//			}
//
//			int tempsa = recv(client_sock, (char*)&(first_send_client), (int)sizeof(ClientStruct1), MSG_WAITALL);
//			if (tempsa == SOCKET_ERROR)
//			{
//				return 0;
//			}
//			if (first_send_client.connecting == -1)
//			{
//				break;
//			}
//			mouse_input_checking(first_send_client.My_citizen_moving, players_list, port);
//			if (first_send_client.My_UI_input.resource_input.CitizenCountAdd)
//			{
//				Citizen_Work_Add(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
//			}
//			if (first_send_client.My_UI_input.resource_input.CitizenCountSub)
//			{
//				Citizen_Work_Sub(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
//			}
//		}
//		else
//		{
//			Sleep(1);
//		}
//
//	}
//	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",addr, ntohs(clientaddr.sin_port));
//	// ���� �ݱ�
//	{
//		unique_lock<shared_mutex> lock(player_list_lock);
//		cout << "erase" << endl;
//		players_list.erase(port);
//	}
//
//	closesocket(client_sock);
//	return 0;
//}
//
//DWORD WINAPI ingame_thread(LPVOID arg)
//{
//	while (player_cnt != MAXPLAYER);
//
//	player_random_location(players_list, citizen_Move);
//	location_set = create_resource_location(players_list, resource_create_landscape);
//	
//	int player_list_iter{};
//	for (auto& a : players_list) 
//	{
//		terrain->set_city_location(a.second->player_info->location, player_list_iter);
//		player_list_iter++;
//		cout << "��ġ : " << a.second->player_info->location.x << ", " << a.second->player_info->location.y << endl;
//	}
//
//	sun_angle = 0.0f;
//	auto start_game = high_resolution_clock::now();
//	auto start_50 = start_game;
//	auto start_1000 = start_game;
//
//	while (1) {
//		auto end_time = high_resolution_clock::now();
//		//50ms
//		if (duration_cast<milliseconds>(end_time - start_50).count() > 50){
//			auto cycle_time = duration_cast<milliseconds>(end_time - start_50).count();
//			start_50 = high_resolution_clock::now();
//			
//			//rotate sunangle
//			//�¾簢�� 1�ʿ� 2�� ���Ƽ� 180�ʿ� 360�� (3�п� �ѹ���)
//			sun_angle += 2.f * cycle_time / 1000.f;
//			if (sun_angle >= 360.f) {
//				sun_angle -=360.f;
//			}
//			
//			//move camera
//			shared_lock<shared_mutex> lock(player_list_lock);
//			camera_movement(players_list);
//			
//			int connecting_player_count = 0;
//			//move citizen
//			for (auto& a : players_list) {
//				float distance = 0.0f;
//				connecting_player_count++;
//				int cnt = 0;
//				for (auto& b : a.second->player_citizen) {
//					if (b != NULL) {
//						if (a.second->player_citizen_arrival_location[cnt]->team != -1) {
//							if (location_distance(b->location, a.second->player_citizen_arrival_location[cnt]->location) > 10) {
//								Move_Civil(b->location, a.second->player_citizen_arrival_location[cnt]->location);
//							}
//						}
//					}
//					cnt++;
//				}
//			}
//			shared_lock<shared_mutex> unlock(player_list_lock);
//			if (connecting_player_count == 0)
//			{
//				player_cnt = 0;
//				ingame_play = false;
//				resource_create_landscape.clear();
//				location_set = false;
//				cout << "End_ingame_thread" << endl;
//				return 0;
//			}
//		}
//		//1000ms
//		else if (duration_cast<milliseconds>(end_time - start_1000).count() > 1000){
//			start_1000 = high_resolution_clock::now();
//			
//			//set citizen destination
//			shared_lock<shared_mutex> lock(player_list_lock);
//			resource_collect(players_list, resource_create_landscape);
//		}
//		else{
//			Sleep(1);
//		}
//
//	}
//
//	return 0;
//}
//
//int main(int argc, char* argv[])
//{	
//	int retval;
//	// ���� �ʱ�ȭ
//	WSADATA wsa;
//	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
//		return 1;
//
//	// ���� ����
//	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
//	if (listen_sock == INVALID_SOCKET) err_quit("socket()");
//
//	// bind()
//	struct sockaddr_in serveraddr;
//	memset(&serveraddr, 0, sizeof(serveraddr));
//	serveraddr.sin_family = AF_INET;
//	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
//	serveraddr.sin_port = htons(SERVERPORT);
//	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
//	if (retval == SOCKET_ERROR) err_quit("bind()");
//
//	// listen()
//	retval = listen(listen_sock, SOMAXCONN);
//	if (retval == SOCKET_ERROR) err_quit("listen()");
//
//	// ������ ��ſ� ����� ����
//	SOCKET client_sock;
//	struct sockaddr_in clientaddr;
//	int addrlen;
//	HANDLE hThread;
//	hThread = CreateThread(NULL, 0, ingame_thread,0, 0, NULL);
//	ingame_play = true;
//
//	hThread = CreateThread(NULL, 0, terrain_change,0, 0, NULL);
//
//	while (1) {
//		// accept()
//		addrlen = sizeof(clientaddr);
//		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
//		if (client_sock == INVALID_SOCKET) 
//		{
//			err_display("accept()");
//			break;
//		}
//		//ingame thread�� ��������� �ٽ� ����
//		if(ingame_play == false)
//		{
//			cout << "Create_ingame_thread" << endl;
//			hThread = CreateThread(NULL, 0, ingame_thread, 0, 0, NULL);
//			ingame_play = true;
//		}
//
//
//		// ������ Ŭ���̾�Ʈ ���� ���
//		char addr[INET_ADDRSTRLEN];
//		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
//
//		cout << addr << endl;
//
//		player_list.emplace_back(client_sock);
//		// ������ ����
//		hThread = CreateThread(NULL, 0, ProcessClient,
//			(LPVOID)client_sock, 0, NULL);
//		//���� �ݱ�
//		if (hThread == NULL) { closesocket(client_sock); }
//		else { CloseHandle(hThread); }
//	}
//	
//
//	// ���� �ݱ�
//	closesocket(listen_sock);
//
//	// ���� ����
//	WSACleanup();
//	return 0;
//}

#include "Common.h"
#include <iostream>
#include <fstream>
#include<vector>
#include<mutex>
#include<string>
#include <chrono>
#include<shared_mutex>
#include "terrain.cu"

#define SERVERPORT 9000
#define BUFSIZE    4096
using namespace std;
using namespace chrono;

//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);

std::mutex player_cnt_lock;
shared_mutex player_list_lock;

Terrain* terrain = new Terrain();
char** total_terrain = terrain->get_map();
char** shadow_map = terrain->get_shadow_map();
unsigned char** temperature_map = terrain->get_temperature_map();

Game* game = new Game();

DWORD WINAPI terrain_change(LPVOID arg)
{
	/*char** player_sight_terrain = terrain->get_player_sight_map();
	char** player_sight_temperature = terrain->get_player_temperature_map();
	terrain->set_city_location(TF{ 20000, 20000 }, 0);*/

	//terrain->show_array(total_terrain, 320);
	//terrain->log_on();
	int i{};
	while (1) {
		//clock_t t_0 = clock();
		cout << endl << i << "��°" << endl;

		terrain->wind_blow({ 1, 0 }, 1);
		terrain->make_shadow_map(game->SunAngle);
		terrain->make_tempertature_map(game->SunAngle);
		CC retval = terrain->get_highest_lowest(temperature_map);

		/*if (sun_angle > 360)
			sun_angle = 0;
		sun_angle += 6;*/
		cout << "sun_angle : " << game->SunAngle << endl;
		cout << "Temperature Highest: " << (float)retval.x / 4 << ", Lowest" << (float)retval.y / 4 << endl;

		//terrain->show_array(total_terrain, 320);
		//terrain->show_array(shadow_map, 320);
		//terrain->show_array(temperature_map, 320);


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

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	//getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	auto start_t = high_resolution_clock::now();

	//�ش� Ŭ���̾�Ʈ�� port��ȣ map�� ����
	int port = ntohs(clientaddr.sin_port);
	
	game->CurrentPlayerNum++;
	Player* ThisPlayer = new Player();
	game->players[port] = ThisPlayer;

	//player sight �ּҰ�
	char** player_sight_terrain = terrain->get_player_sight_map();
	char** player_sight_temperature = terrain->get_player_temperature_map();

	//���� ������: sunangle, curr location, city location, resource, citizen, building
	const int SendBufferSize = sizeof(float) + sizeof(II) + (sizeof(char) + sizeof(II) + sizeof(Citizen) * 100 + sizeof(Resource) * 5 + sizeof(Building) * 20) * MAXPLAYER;
	char SendBuffer[SendBufferSize];
	const int RecvBufferSize = sizeof(K) + sizeof(UII);
	char RecvBuffer[RecvBufferSize];
	cout << "Recv Buffer Size: " << RecvBufferSize << ", Send Buffer Size: " << SendBufferSize << endl;
	char* RecvBufferPtr = RecvBuffer;
	char* SendBufferPtr = SendBuffer;

	while (1) {
		auto end_t = high_resolution_clock::now();
		if (duration_cast<milliseconds>(end_t - start_t).count() > 50)
		{
			shared_lock<shared_mutex> lock(player_list_lock);
			start_t = high_resolution_clock::now();
			
			char Identity = 1;
			char My = 0;
			int CopyAddress{};
			memcpy(SendBufferPtr + CopyAddress, &game->SunAngle, sizeof(float));
			CopyAddress += sizeof(float);
			memcpy(SendBufferPtr + CopyAddress, &ThisPlayer->CurrentLocation, sizeof(II));
			CopyAddress += sizeof(II);

			cout << "curr location: " << (int)ThisPlayer->CurrentLocation.x << ", " << (int)ThisPlayer->CurrentLocation.y << endl;
			for (auto& player : game->players) {
				if (player.first == port) {
					memcpy(SendBufferPtr + CopyAddress, &My, sizeof(char));
				}
				else {
					memcpy(SendBufferPtr + CopyAddress, &Identity, sizeof(char));
				}
				CopyAddress += sizeof(char);
				memcpy(SendBufferPtr + CopyAddress, &player.second->CityLocation, sizeof(II));
				CopyAddress += sizeof(II);
				memcpy(SendBufferPtr + CopyAddress, player.second->Citizens, sizeof(Citizen) * 100);
				CopyAddress += sizeof(Citizen) * 100;
				memcpy(SendBufferPtr + CopyAddress, player.second->Resources, sizeof(Resource) * 5);
				CopyAddress += sizeof(Resource) * 5;
				memcpy(SendBufferPtr + CopyAddress, player.second->Buildings, sizeof(Building) * 20);
				CopyAddress += sizeof(Building) * 20;
				Identity++;
			}
			if (CopyAddress != SendBufferSize)
				cout << "SendBufferSize Error" << endl;
			retval = send(client_sock, SendBuffer, SendBufferSize, 0);
			if (retval == SOCKET_ERROR) {
				cout << "Send Error" << endl;
				return 0;
			}
			

			//���� ����
			II player_location{ (int)game->players[port]->CurrentLocation.x / 100, (int)game->players[port]->CurrentLocation.y / 100 };
			terrain->copy_for_player_map(player_location);
			for (int i = 0; i < player_sight_size.x; ++i) {
				retval = send(client_sock, (char*)player_sight_terrain[i], (int)(sizeof(char) * player_sight_size.y), 0);
				if (retval == SOCKET_ERROR) {
					cout << "Send Error" << endl;
					return 0;
				}
			}

			////�µ� ����
			for (int i = 0; i < player_sight_size.x; ++i) {
				retval = send(client_sock, (char*)player_sight_temperature[i], (int)(sizeof(char) * player_sight_size.y), 0);
				if (retval == SOCKET_ERROR) {
					cout << "Send Error" << endl;
					return 0;
				}
			}
			
			//����
			int retval = recv(client_sock, RecvBuffer, RecvBufferSize, MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				cout << "recv Error" << endl;
				return 0;
			}

			memcpy(&ThisPlayer->UI, RecvBuffer, sizeof(UII));
			memcpy(&ThisPlayer->KeyInput, RecvBuffer + sizeof(UII), sizeof(K));
			cout << "KeyInput: " << ThisPlayer->KeyInput.W << ", " << ThisPlayer->KeyInput.A << ", " << ThisPlayer->KeyInput.S << ", " << ThisPlayer->KeyInput.D << endl;

			
			//mouse_input_checking(first_send_client.My_citizen_moving, players_list, port);
			//if (first_send_client.My_UI_input.resource_input.CitizenCountAdd)
			//{
			//	Citizen_Work_Add(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
			//}
			//if (first_send_client.My_UI_input.resource_input.CitizenCountSub)
			//{
			//	Citizen_Work_Sub(players_list, resource_create_landscape, port, first_send_client.My_UI_input.resource_input.ResourceNum);
			//}
		}
		else{
			Sleep(1);
		}

	}
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
	// ���� �ݱ�
	{
		unique_lock<shared_mutex> lock(player_list_lock);
		cout << "erase" << endl;
		game->players.erase(port);
	}

	closesocket(client_sock);
	return 0;
}

DWORD WINAPI ingame_thread(LPVOID arg)
{
	while (game->CurrentPlayerNum != MAXPLAYER) {}

	int player_list_iter{};
	for (auto& a : game->players)
	{
		terrain->set_city_location(a.second->CityLocation, player_list_iter);
		player_list_iter++;
		cout << "��ġ : " << a.second->CityLocation.x << ", " << a.second->CityLocation.y << endl;
	}

	game->SunAngle = 0.0f;
	auto start_game = high_resolution_clock::now();
	auto start_50 = start_game;
	auto start_1000 = start_game;

	while (1) {
		auto end_time = high_resolution_clock::now();
		//50ms
		if (duration_cast<milliseconds>(end_time - start_50).count() > 50) {
			auto cycle_time = duration_cast<milliseconds>(end_time - start_50).count();
			start_50 = high_resolution_clock::now();

			//rotate sunangle
			//�¾簢�� 1�ʿ� 2�� ���Ƽ� 180�ʿ� 360�� (3�п� �ѹ���)
			game->SunAngle += 2.f * cycle_time / 1000.f;
			if (game->SunAngle >= 360.f) {
				game->SunAngle -= 360.f;
			}

			//move camera
			shared_lock<shared_mutex> lock(player_list_lock);
			game->MovePlayer();

			//move citizen
			/*for (auto& player : game->players) {
				float distance = 0.0f;
				int cnt = 0;
				for (auto& b : player.second->player_citizen) {
					if (b != NULL) {
						if (a.second->player_citizen_arrival_location[cnt]->team != -1) {
							if (location_distance(b->location, player.second->player_citizen_arrival_location[cnt]->location) > 10) {
								Move_Civil(b->location, player.second->player_citizen_arrival_location[cnt]->location);
							}
						}
					}
					cnt++;
				}
			}*/
			shared_lock<shared_mutex> unlock(player_list_lock);
			if (game->players.size() == 0)
			{
				game->CurrentPlayerNum = 0;
				/*ingame_play = false;
				resource_create_landscape.clear();
				location_set = false;*/
				cout << "End_ingame_thread" << endl;
				return 0;
			}
		}
		//1000ms
		else if (duration_cast<milliseconds>(end_time - start_1000).count() > 1000) {
			start_1000 = high_resolution_clock::now();

			//set citizen destination
			shared_lock<shared_mutex> lock(player_list_lock);
			//resource_collect(players_list, resource_create_landscape);
		}
		else {
			Sleep(1);
		}

	}

	return 0;
}

int main(int argc, char* argv[])
{
	int SendBufferSize = sizeof(float) + sizeof(II) + (sizeof(char) + sizeof(II) + sizeof(Citizen) * 100 + sizeof(Resource) * 5 + sizeof(Building) * 20) * MAXPLAYER;
	char* SendBuffer = new char[SendBufferSize];
	int RecvBufferSize = sizeof(K) + sizeof(UII);
	char* RecvBuffer = new char[RecvBufferSize];
	cout << "Recv Buffer Size: " << RecvBufferSize << ", Send Buffer Size: " << SendBufferSize << endl;

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
	hThread = CreateThread(NULL, 0, ingame_thread, 0, 0, NULL);
	hThread = CreateThread(NULL, 0, terrain_change, 0, 0, NULL);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET){
			err_display("accept()");
			break;
		}
		
		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

		cout << addr << endl;

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
