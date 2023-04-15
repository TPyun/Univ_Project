//#include <iostream>
//#include <fstream>
//#include<vector>
#include<mutex>
#include<string>
//#include<map>
//#include <chrono>
#include<shared_mutex>
//#include<algorithm>

#include"Network.h"
#include"Player.h"
#include"Citizen.h"
#include"Resource.h"
#include "Building.h"
#include"terrain.cu"

#pragma comment (lib,"WS2_32.lib")
#pragma comment (lib,"MSWSock.lib")

using namespace std;
using namespace chrono;

uniform_int_distribution <int>map_uid{ 1000, one_side_number - 1000 };
//�Լ����� 
DWORD WINAPI ProcessClient(LPVOID arg);
DWORD WINAPI ingame_thread(LPVOID arg);

bool game_start = false;
int len = 0;

std::mutex player_cnt_lock;
shared_mutex player_list_lock;
float sun_angle;
volatile int player_cnt = 0;

Terrain* terrain = new Terrain();
char** total_terrain = terrain->get_map();
char** shadow_map = terrain->get_shadow_map();
unsigned char** temperature_map = terrain->get_temperature_map();
bool Isterrain_change = false;

DWORD WINAPI terrain_change(LPVOID arg)
{
	//terrain->log_on();
	int i{};
	auto terrain_start = std::chrono::system_clock::now();
	while (1){
		auto terrain_end = std::chrono::system_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(terrain_end - terrain_start).count() > 1000)
		{
			terrain_start = std::chrono::system_clock::now();
			cout << endl << i << "��°" << endl;

			terrain->wind_blow({ 1, 0 }, 1);
			terrain->make_shadow_map(sun_angle);
			terrain->make_tempertature_map(sun_angle);
			CC retval = terrain->get_highest_lowest(temperature_map);

			cout << "Temperature Highest: " << (float)retval.x / 4 << ", Lowest" << (float)retval.y / 4 << endl;

			//terrain->show_array(total_terrain, 320);
			//terrain->show_array(shadow_map, 320);
			//terrain->show_array(temperature_map, 320);

			if (i % 100 == 0 && i != 0) {
				terrain->save_terrain();
				cout << "SAVED!!!" << endl;
			}
			i++;
			Isterrain_change = true;
		}
	}
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval = 0;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	
	return 0;
}

DWORD WINAPI ingame_thread(LPVOID arg)
{
	auto Player_Move_Timer_End = std::chrono::system_clock::now();
	auto Citizen_Move_Timer_End = std::chrono::system_clock::now();
	auto Resource_Collect_Timer_End = std::chrono::system_clock::now();
	auto TerrainSend_Timer_End = std::chrono::system_clock::now();
	
	bool IsNight = false;
	bool IsOnceWork = true;
	char** player_terrain = terrain->get_player_sight_map();
	char** player_temperature = terrain->get_player_temperature_map();
	int citizenfoodwatereat = 0;

	for (int i = 0; i < MAXPLAYER; ++i)
	{
		Player* player = reinterpret_cast<Player*>(objects[i]);
		TF player_city{ player->_x, player->_y };
		terrain->set_city_location(TF{player->_x, player->_y}, i);
	}
	

	while (1)
	{
		auto Timer_Start = std::chrono::system_clock::now();

		if (std::chrono::duration_cast<std::chrono::milliseconds>(Timer_Start - Player_Move_Timer_End).count() > 10)
		{
			//sunangle...
			auto cycle_time = duration_cast<milliseconds>(Timer_Start - Player_Move_Timer_End).count();
			Player_Move_Timer_End = std::chrono::system_clock::now();
			//rotate sunangle
			//�¾簢�� 1�ʿ� 2�� ���Ƽ� 180�ʿ� 360�� (3�п� �ѹ���)
			
			sun_angle += 2.f * cycle_time / 1000.f;
			if (sun_angle >= 360.f) 
			{
				sun_angle -= 360.f;
				IsNight = false;
				IsOnceWork = true;
				citizenfoodwatereat = 0;
				for (int i = CITIZENSTART; i < MAXCITIZEN + CITIZENSTART; ++i)
				{
					Citizen* citizen = reinterpret_cast<Citizen*>(objects[i]);
					if (citizen->_Job == 1)
					{
						citizen->_arrival_x = citizen->_job_x;
						citizen->_arrival_y = citizen->_job_y;
					}
				}
			}
			else if (sun_angle >= 180.f)
			{
				IsNight = true;
			}

			if (sun_angle - citizenfoodwatereat > 10)
			{
				citizenfoodwatereat = sun_angle;
				for(int player_id = 0;player_id < MAXPLAYER;++player_id)
				{
					Player* player = reinterpret_cast<Player*>(objects[player_id]);
					int citizencount = player->playercitizencount();
					for (int citizen_id = CITIZENSTART + player_id * PLAYERCITIZENCOUNT; citizen_id < CITIZENSTART + (player_id + 1) * PLAYERCITIZENCOUNT; ++citizen_id)
					{
						Citizen* citizen = reinterpret_cast<Citizen*>(objects[citizen_id]);
						if (citizen->_Job == -1)
							continue;
						citizen->_Satiety -= 1;
						citizen->_thirst -= 1;

						if (citizen->_Satiety == 0)
						{
							if (!citizen->citizen_eat_food())
								citizen->citizen_dead();
						}
						if (citizen->_thirst == 0)
						{
							if (!citizen->citizen_eat_water())
								citizen->citizen_dead();
						}

						if (player->_resource_amount[3] > citizencount)
						{
							if(citizen->_Satiety < 70)
								citizen->citizen_eat_food();
						}
						else
						{
							if (citizen->_Satiety < 30)
								citizen->citizen_eat_food();
						}

						if (player->_resource_amount[1] > citizencount)
						{
							if (citizen->_thirst < 70)
								citizen->citizen_eat_water();
						}
						else
						{
							if (citizen->_thirst < 30)
								citizen->citizen_eat_water();
						}
					}

					player->send_resource_amount();
				}
			}

			for (int i = 0; i < MAXPLAYER; ++i)
			{
				Player* player = reinterpret_cast<Player*>(objects[i]);
				if (player->isconnect)
				{
					char** player_sight_line;
					player_sight_line = terrain->copy_for_player_map_line((int)(player->_x + player->_currentX) /100, (int)(player->_y + player->_currentY) / 100);
					player->key_input(player_sight_line);
					player->send_sunangle(sun_angle);
				}

			}
		}
		if (std::chrono::duration_cast<std::chrono::milliseconds>(Timer_Start - Citizen_Move_Timer_End).count() > 10)
		{
			Citizen_Move_Timer_End = std::chrono::system_clock::now();
			for (int i = CITIZENSTART; i < MAXCITIZEN + CITIZENSTART; ++i)
			{
				Citizen* citizen = reinterpret_cast<Citizen*>(objects[i]);
				if (citizen->_Job == -1)
				{
					continue;
				}
				citizen->set_citizen_move();
			}
		}
		if (std::chrono::duration_cast<std::chrono::milliseconds>(Timer_Start - Resource_Collect_Timer_End).count() > 5000)	//5000
		{	
			Resource_Collect_Timer_End = std::chrono::system_clock::now();
			for (int i = RESOURCESTART; i < RESOURCESTART + MAXRESOURCE; ++i)
			{
				Resource* resource = reinterpret_cast<Resource*>(objects[i]);
				resource->collect_resource();
			}
			for (int i = 0; i < MAXPLAYER; ++i)
			{
				Player* player = reinterpret_cast<Player*>(objects[i]);
				player->send_resource_amount();
			}
		}
		if (Isterrain_change)
		{
			for (int i = 0; i < MAXPLAYER; ++i)
			{
				Player* player = reinterpret_cast<Player*>(objects[i]);
				terrain->copy_for_player_map(II{ (int)(player->_x + player->_currentX) / 100, (int)(player->_y + player->_currentY) / 100});
				if(player->isconnect)
				{
					while(1)
					{
						static unsigned char terrain_x{};
						if (terrain_x == SIGHT_X)
						{
							terrain_x = 0;
							break;
						}
						sc_packet_terrainAll packet;
						packet.size = sizeof(sc_packet_terrainAll);
						packet.type = SC_PACKET_TERRAINALL;
						packet.terrain_X = terrain_x;
						memcpy(packet.terrain_Y, player_terrain[terrain_x], SIGHT_Y);

						player->send_packet(&packet);
						terrain_x++;
					}
					
					while (1)
					{
						static unsigned char terrain_x{};
						if (terrain_x == SIGHT_X)
						{
							terrain_x = 0;
							break;
						}
						sc_packet_temperature packet;
						packet.terrain_X = terrain_x;
						memcpy(packet.terrain_Y, player_temperature[terrain_x], SIGHT_Y);

						player->send_packet(&packet);
						terrain_x++;
					}
					Isterrain_change = false;
				}
			}	
			
		}
		if (IsNight)
		{
			if(IsOnceWork)
			{
				for (int citizen_id = CITIZENSTART; citizen_id < MAXCITIZEN + CITIZENSTART; ++citizen_id)
				{
					int player_id = (citizen_id - 5) / 200;
					Player* player = reinterpret_cast<Player*>(objects[player_id]);
					Citizen* citizen = reinterpret_cast<Citizen*>(objects[citizen_id]);
					if (citizen->_Job == -1)
						continue;
					
					if (citizen->_HouseId == -1)
					{
						bool isHouseStaff = false;
						//���� �� HOUSE�� �������� �ù��� ��ġ���ش�.
						for (int building_id = BUILDINGSTART + player_id * PLAYERBUILDINGCOUNT; building_id < BUILDINGSTART + (player_id + 1) * PLAYERBUILDINGCOUNT; building_id++)
						{

							Building* building = reinterpret_cast<Building*>(objects[building_id]);
							if (building->_type == 1 || building->_type == 2 || building->_type == 3)
							{
								for (auto& a : building->_citizens)
								{
									if (a == nullptr)
									{
										citizen->_HouseId = building->_id;
										a = citizen;
										citizen->_arrival_x = objects[citizen->_HouseId]->_x;
										citizen->_arrival_y = objects[citizen->_HouseId]->_y;
										isHouseStaff = true;
										break;
									}
								}
							}
							if (isHouseStaff)
								break;

						}
						if(!isHouseStaff)
						{
							int rocate = 1;
							if (rand() % 2 == 0)
								rocate *= -1;
							citizen->_arrival_x = player->_x + (rand() % 500) * rocate + 500 * rocate;
							if (rand() % 2 == 0)
								rocate *= -1;
							citizen->_arrival_y = player->_y + rand() % 500 * rocate + 500 * rocate;
						}
					}
					else
					{
						citizen->_arrival_x = objects[citizen->_HouseId]->_x;
						citizen->_arrival_y = objects[citizen->_HouseId]->_y;
					}
				}
				IsOnceWork = false;
			}
		}
		///////////////////////

	}
	return 0;
}

int main(int argc, char* argv[])
{	
	cout << fixed;
	// ���� �ʱ�ȭ
	WSADATA wsa;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa);

	// SOCKET ���� listen & bind
	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN s_address;
	ZeroMemory(&s_address, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(SERVERPORT);
	s_address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	bind(s_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));
	listen(s_socket, SOMAXCONN);

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(s_socket), h_iocp, 99999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	WSA_OVER_EX accept_over;
	ZeroMemory(&accept_over, sizeof(accept_over));
	accept_over.set_accept_over();

	// accept()
	AcceptEx(s_socket, c_socket, accept_over.getbuf(), NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.getWsaOver());

	//��� object �ʱ�ȭ
	for (auto object : objects)
	{
		object = nullptr;
	}

	//�̸� object�� ������ ������
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		objects[i] = new Player(i);
	}
	for (int i = MAXPLAYER; i < MAXCITIZEN + MAXPLAYER; ++i)//5~1004������ Citizen
	{
		objects[i] = new Citizen(i);
	}
	for (int i = RESOURCESTART; i < RESOURCESTART + MAXRESOURCE; ++i)//1005 ~ 1054������ Resource
	{
		objects[i] = new Resource(i ,i % 5);	//i%5 �ؼ� type�� ������ /0 : ����,	1 : ��,		2 : ö,		3 : �ķ�,		4 : ����
	}
	for (int i = BUILDINGSTART; i < BUILDINGSTART + MAXBUILDING; ++i)//1055 ~ 1659������ Building	�δ� 11*11�� = 121��
	{
		objects[i] = new Building(i);
	}
	

	//player, citizen, resource �ʱ� ��ġ ���� �ѹ� �ϰ� �Ⱦ��Ŷ� main�� ��
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		default_random_engine dre2;
		dre2.seed(std::chrono::system_clock::now().time_since_epoch().count());
		retry:		
		float x = map_uid(dre) * UNIT, float y = map_uid(dre) * UNIT, float z = 0;
		for (int j = 0; j < i; ++j)
		{
			if (location_distance(objects[j]->_x, objects[j]->_y, x, y) < 7000)
			{
				goto retry;
			}
		}
		reinterpret_cast<Player*>(objects[i])->set_player_location(x, y, z);
		//reinterpret_cast<Player*>(objects[i])->player_sight_terrain = total_terrain;
		for (int j = 0; j < 10; ++j)
		{
			reinterpret_cast<Citizen*>(objects[MAXPLAYER + i * 200 + j])->set_citizen_spwan_location(x + 2000, y + (j * 500) - 2250,z);
			reinterpret_cast<Resource*>(objects[RESOURCESTART + i * 10 + j])->set_resource_spwan_location(x, y, z);
		}
	}

	//�� �������ְ� thread����
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, terrain_change, 0, 0, NULL);
	hThread = CreateThread(NULL, 0, ingame_thread, 0, 0, NULL);
	int user_id = 0;

	while (1) {

		DWORD io_byte;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		ret = GetQueuedCompletionStatus(h_iocp, &io_byte, &key, &over, INFINITE);
		WSA_OVER_EX* wsa_over_ex = reinterpret_cast<WSA_OVER_EX*>(over);
		if (ret == FALSE)
		{
			if (wsa_over_ex->_iocpop == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client [" << key << "]\n";

				continue;
			}
		}

		user_id = static_cast<int>(key);
		Player* player;

		if (user_id < MAXPLAYER)
		{
			
			player = reinterpret_cast<Player*>(objects[user_id]);
		}
		else if (user_id == 99999)
		{
			cout << "NEW ACEPT" << endl;
		}
		else
		{
			cout << " ACCEPT ERROR " << endl;
			DebugBreak();
		}

		switch (wsa_over_ex->_iocpop)
		{
		case OP_RECV:
		{
			unsigned char* packet_start = wsa_over_ex->_buf;
			int remain_data = io_byte + player->_prev_size;
			//cout << user_id<< "// byte : " << remain_data << endl;
			while (remain_data > 0)
			{
				int packet_size = packet_start[0];
				if (packet_size <= remain_data)
				{
					player->_wsa_recv_over.processpacket(user_id, wsa_over_ex->_buf);
					packet_start += packet_size;
					remain_data -= packet_size;
				}
				else
					break;
			}
			player->_prev_size = remain_data;
			if(remain_data > 0)
			{
				memcpy(wsa_over_ex->_buf, packet_start, remain_data);
			}
			DWORD flags = 0;			
			ZeroMemory(&player->_wsa_recv_over._wsaover, sizeof(player->_wsa_recv_over._wsaover));
			player->_wsa_recv_over._wsabuf.buf = reinterpret_cast<char*>(player->_wsa_recv_over._buf + player->_prev_size);
			player->_wsa_recv_over._wsabuf.len = BUFSIZE - remain_data;
			WSARecv(player->_socket, &player->_wsa_recv_over._wsabuf, 1, NULL, &flags, &player->_wsa_recv_over._wsaover, NULL);
			break;
		}
		case OP_SEND:
		{
			delete wsa_over_ex;
			break;
		}
		case OP_ACCEPT:
		{
			cout << "ACCEPT!" << endl;
			user_id = player_cnt++;
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), h_iocp, user_id, 0);
			Player* n_player = reinterpret_cast<Player*>(objects[user_id]);
			ZeroMemory(&n_player->_wsa_recv_over, sizeof(n_player->_wsa_recv_over));
			n_player->_socket = c_socket;
			n_player->_wsa_recv_over._iocpop = OP_RECV;
			n_player->_prev_size = 0;
			n_player->_id = user_id;
			n_player->_wsa_recv_over._wsabuf.buf = reinterpret_cast<char*>(n_player->_wsa_recv_over._buf);
			n_player->_wsa_recv_over._wsabuf.len = BUFSIZE;
			n_player->isconnect = true;
			//n_player->player_sight_terrain = terrain->get_player_sight_map();
			DWORD flags = 0;
			WSARecv(c_socket, &n_player->_wsa_recv_over._wsabuf, 1, NULL, &flags, &n_player->_wsa_recv_over._wsaover, NULL);

			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&accept_over, sizeof(accept_over));
			accept_over.set_accept_over();

			AcceptEx(s_socket, c_socket, accept_over.getbuf(), NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.getWsaOver());

			break;
		}
		default:
			break;
		}
	}
	return 0;
}
