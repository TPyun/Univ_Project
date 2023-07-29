
#include "Network.h"
#include "Player.h"
#include "Citizen.h"
#include"Resource.h"
#include "Building.h"
#include"Army.h"
#include"GameEvent.h"
#include<iostream>

std::array<Object*, MAXOBJECT> objects;
HANDLE h_iocp;
bool IsNight;
char** object_z;
int survil_day = 1;
std::set<int> sand_storm_day;
bool Is_sand_storm = false;
volatile int room_player_cnt = 0;

WSA_OVER_EX::WSA_OVER_EX()
{
	return;
}

WSA_OVER_EX::WSA_OVER_EX(IOCPOP iocpop, unsigned char byte, void* buf)
{
	ZeroMemory(&_wsaover, sizeof(_wsaover));
	_iocpop = iocpop;
	_wsabuf.buf = reinterpret_cast<char*>(buf);
	_wsabuf.len = byte;
}

void WSA_OVER_EX::processpacket(int client_id, unsigned char* pk)
{
	unsigned char packet_type = pk[1];
	Player* player = reinterpret_cast<Player*>(objects[client_id]);

	switch (packet_type)
	{
	case CS_PACKET_LOGIN:
	{
		{
			std::cout << "login player : " << client_id << std::endl;
			cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(pk);
			send_login_ok_packet(client_id);
			send_citizen_First_create_packet(client_id);
			send_resource_First_create_packet(client_id);
			player->send_terrain_All();
		}
		{
			sc_packet_matching packet;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_MATCHING;
			packet.connectplayer = room_player_cnt;	
			packet.maxplayer = ROOMPLAYER;
			all_player_sendpacket(&packet);
		}

		break;
	}
	case CS_PACKET_MOVE:
	{
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(pk);
		player->w = packet->w;
		player->a = packet->a;
		player->s = packet->s;
		player->d = packet->d;
		break;
	}
	case CS_PACKET_CITIZENPLACEMENT:
	{
		//추후 건물도 똑같이 작동할거니깐 건물도 추가해주면 좋음

		cs_packet_citizenplacement* packet = reinterpret_cast<cs_packet_citizenplacement*>(pk);
		if (packet->objectid >= RESOURCESTART && packet->objectid < RESOURCESTART + MAXRESOURCE) 
		{
			Resource* resource = reinterpret_cast<Resource*> (objects[packet->objectid]);
			resource->set_resource_citizen_placement(client_id, packet->isplus);
		}
		else if (packet->objectid >= BUILDINGSTART && packet->objectid < BUILDINGSTART + MAXBUILDING)
		{
			Building* building = reinterpret_cast<Building*>(objects[packet->objectid]);
			building->set_building_citizen_placement(packet->isplus, packet->army_type);
		}

		break;
	}
	case CS_PACKET_BUILDABLE:
	{
		cs_packet_buildable* cs_packet = reinterpret_cast<cs_packet_buildable*>(pk);
		//std::cout << "buildable" << std::endl;
		bool location_buildable = true;
		bool worker_builable = false;
		if (!IsNight) {
			for (auto& obj : objects) {
				if (obj == nullptr)
					continue;
				if (obj->_x == 0) {
					continue;
				}
				if (obj->_id >= BUILDINGSTART && obj->_id < BUILDINGSTART + MAXBUILDING || obj->_id >= RESOURCESTART && obj->_id < RESOURCESTART + MAXRESOURCE || obj->_id >= 0 && obj->_id < MAXPLAYER) {
					if (obj->_x < cs_packet->x + 500 && obj->_x > cs_packet->x - 500 && obj->_y < cs_packet->y + 500 && obj->_y > cs_packet->y - 500) {		// test 원래는 800
						location_buildable = false;
						break;
					}
				}
			}
			for (int i = CITIZENSTART + PLAYERCITIZENCOUNT * player->_id; i < CITIZENSTART + PLAYERCITIZENCOUNT * player->_id + PLAYERCITIZENCOUNT; ++i) {
				Citizen* citizen = reinterpret_cast<Citizen*>(objects[i]);
				if (citizen->_job == 0) {
					worker_builable = true;
				}
			}
		}
		sc_packet_buildable sc_packet;
		if (location_buildable && worker_builable && !IsNight)
			sc_packet.buildable = true;
		else
			sc_packet.buildable = false;
		player->send_packet(&sc_packet);
		break;
	}
	case CS_PACKET_BUILD:
	{
		cs_packet_build* cs_packet = reinterpret_cast<cs_packet_build*>(pk);
		sc_packet_build sc_packet;

		for (int i = BUILDINGSTART + PLAYERBUILDINGCOUNT * player->_id; i < BUILDINGSTART + PLAYERBUILDINGCOUNT * player->_id + PLAYERBUILDINGCOUNT; ++i) {
			Building* building = reinterpret_cast<Building*>(objects[i]);
			if (building->_type != -1)
				continue;
			sc_packet.id = building->_id;
			sc_packet.x = cs_packet->x;
			sc_packet.y = cs_packet->y;
			sc_packet.building_type = cs_packet->building_type;
			sc_packet.do_build = building->_create_building(cs_packet->x, cs_packet->y, cs_packet->building_type, i);
			break;
		}
		
		for (int player_num = 0; player_num < MAXPLAYER; player_num++) {	//모든 플레이어들에게 전송
			Player* this_player = reinterpret_cast<Player*>(objects[player_num]);
			this_player->send_packet(&sc_packet);
		}
		break;
	}
	case CS_PACKET_MINIMAP:
	{
		cs_packet_minimap* packet= reinterpret_cast<cs_packet_minimap*>(pk);
		if(!Is_sand_storm)
			player->playerMinimapLocation(packet->x, packet->y);
		break;
	}
	case CS_PACKET_ARMYMOVE:
	{
		cs_packet_armymove* packet = reinterpret_cast<cs_packet_armymove*>(pk);
		Army* army = reinterpret_cast<Army*>(objects[packet->a_id]);
		if (!army->is_escort)	//  시민 호위중엔 마을로만 직진
			army->set_army_arrival_location(packet->x, packet->y);
		break;
	}
	case CS_PACKET_EVENTSELECT:
	{
		cs_packet_eventselect* packet = reinterpret_cast<cs_packet_eventselect*>(pk);
		GameEvent* g_event = reinterpret_cast<GameEvent*>(objects[packet->e_id]);
		g_event->do_event(packet->s_option);

		break;
	}
	case CS_PACKET_ARMYRETURN:
	{
		cs_packet_armyreturn* packet = reinterpret_cast<cs_packet_armyreturn*>(pk);
		Army* army = reinterpret_cast<Army*>(objects[packet->a_id]);
		army->set_army_return_home();
		break;
	}
	case CS_PACKET_ARMYDISBAND:
	{
		cs_packet_armydisband* packet = reinterpret_cast<cs_packet_armydisband*>(pk);
		Army* army = reinterpret_cast<Army*>(objects[packet->a_id]);
		army->set_army_disband();
		break;
	}
	case CS_PACKET_POLICY:
	{
		cs_packet_policy* cs_packet = reinterpret_cast<cs_packet_policy*>(pk);
		sc_packet_policy_accept sc_packet;
		if (player->_policy.policy_ticket > 0) {
			int duplication = player->_policy.set_policy(cs_packet->policy_id);
			if (duplication == 1) {
				std::cout << "정책 중복\n";
				sc_packet.accept = false;
			}
			else if (duplication == 2) {
				std::cout << "정책 순서 안됨\n";
				sc_packet.accept = false;
			}
			else if (duplication == 3) {
				std::cout << "정책 둘중 하나만 가능\n";
				sc_packet.accept = false;
			}
			else {
				std::cout << "정책 적용\n";
				player->_policy.policy_ticket--;
				sc_packet.accept = true;
			}
		}
		else {
			std::cout << "정책 티켓 부족\n";
			sc_packet.accept = false;
		}
		
		sc_packet.policy_id = cs_packet->policy_id;
		sc_packet.ticket = player->_policy.policy_ticket;
		player->send_packet(&sc_packet);

		std::cout << "정책 티켓 : " << player->_policy.policy_ticket << std::endl;
		break;
	}
	case CS_PACKET_TRADEREQUEST:
	{
		cs_packet_traderequest* packet = reinterpret_cast<cs_packet_traderequest*>(pk);
		Player* request_player = reinterpret_cast<Player*>(objects[packet->request_player]);
		player->trade_player_id = packet->request_player;
		request_player->Trade_Request(client_id);
		break;
	}
	case CS_PACKET_TRADEAGREE:
	{
		cs_packet_tradeagree* packet = reinterpret_cast<cs_packet_tradeagree*>(pk);
		Player* request_player = reinterpret_cast<Player*>(objects[packet->request_player]);
		if (packet->isagree == 1)
		{
			
			player->trade_player_id = packet->request_player;
			break;
		}
		else
		{
			request_player->Trade_Request_Agree(client_id, packet->isagree);
		}
		break;
	}
	case CS_PACKET_TRADERESOURCE:
	{
		cs_packet_traderesource* packet = reinterpret_cast<cs_packet_traderesource*>(pk);
		Player* request_player = reinterpret_cast<Player*>(objects[player->trade_player_id]);
		if (packet->resource_num < 5)
		{
			player->change_trade_resource(packet->resource_num, packet->resource_amount);
			request_player->send_change_trade_resource(packet->resource_num + 5, packet->resource_amount);
		}
		else if (packet->resource_num < 10)
		{
			request_player->change_trade_resource(packet->resource_num - 5, packet->resource_amount);
			request_player->send_change_trade_resource(packet->resource_num - 5, packet->resource_amount);
		}
		break;
	}
	case CS_PACKET_TRADEDEAL:
	{
		cs_packet_tradedeal* packet = reinterpret_cast<cs_packet_tradedeal*>(pk);
		Player* request_player = reinterpret_cast<Player*>(objects[player->trade_player_id]);

		sc_packet_tradedeal s_packet;
		s_packet.deal_boolean = packet->deal_boolean;
		s_packet.size = sizeof(s_packet);
		s_packet.type = SC_PACKET_TRADEDEAL;

		request_player->send_packet(&s_packet);

		break;
	}
	case CS_PACKET_TRADESUCCESS:
	{
		cs_packet_tradesuccess* packet = reinterpret_cast<cs_packet_tradesuccess*>(pk);
		Player* request_player = reinterpret_cast<Player*>(objects[player->trade_player_id]);

		sc_packet_tradesuccess s_packet;
		s_packet.success_boolean = packet->success_boolean;
		s_packet.size = sizeof(s_packet);
		s_packet.type = SC_PACKET_TRADESUCCESS;
		request_player->send_packet(&s_packet);
		player->trade_success = packet->success_boolean;
		if (request_player->trade_success && player->trade_success)
		{
			player->trade_clear();
			request_player->trade_clear();
		}
		break;
	}
	case CS_PACKET_TECHNOLOGY:
	{
		cs_packet_technology* packet = reinterpret_cast<cs_packet_technology*>(pk);
		player->_research->set_tech_upgrade(packet->tech_type, packet->tech_level);
		sc_packet_technology s_packet;
		s_packet.size = sizeof(s_packet);
		s_packet.type = SC_PACKET_TECHNOLOGY;
		s_packet.tech_level = player->_research->tech[packet->tech_type];
		s_packet.tech_type = packet->tech_type;
		player->send_packet(&s_packet);
		break;
	}
	case CS_PACKET_TECHPHASE:
	{
		cs_packet_techphase* packet = reinterpret_cast<cs_packet_techphase*>(pk);
		player->_research->set_tech_phase(packet->tech_phase);
		std::cout << "tech_phase : " << player->_research->tech_phase << "\n";
		sc_packet_techphase s_packet;
		s_packet.size = sizeof(s_packet);
		s_packet.type = SC_PACKET_TECHPHASE;
		s_packet.tech_phase = player->_research->tech_phase;
		player->send_packet(&s_packet);
		break;
	}
	case CS_PACKET_DECLARATION_WAR:
	{
		cs_packet_declaration_war* packet = reinterpret_cast<cs_packet_declaration_war*>(pk);
		std::cout << player->_id << " is War : " << packet->player_num << "\n";
		player->War_Players[packet->player_num] = true;
		Player* war_player = reinterpret_cast<Player*>(objects[packet->player_num]);
		war_player->War_Players[client_id] = true;
		player->send_declaration_war(packet->player_num, true);
		war_player->send_declaration_war(client_id, true);
		break;
	}
	case CS_PACKET_SPRINKLER_STATUS:
	{
		cs_packet_sprinkler_status* packet = reinterpret_cast<cs_packet_sprinkler_status*>(pk);
		Building* building = reinterpret_cast<Building*>(objects[packet->sprinkler_id + BUILDINGSTART]);
		building->activated = packet->status;
		std::cout << "sprinkler status : " << packet->sprinkler_id << " " << packet->status << "\n";
		break;
	}
	case CS_PACKET_ARMYSELECT:
	{
		cs_packet_armyselect* packet = reinterpret_cast <cs_packet_armyselect*> (pk);
		player->set_army_select(packet->select_type);
		break;
	}
	case CS_PACKET_CALL_CHEAT_KEY:
	{
		cs_packet_cheatkey* packet = reinterpret_cast <cs_packet_cheatkey*> (pk);
		if (packet->cheat_type == 9) // win, GameEnd
		{
			player->set_score();
			player->player_ending();
			player->rank = 1;
			player->send_ending();
		}
		else if (packet->cheat_type == 8)	// lose, GameOver
		{
			player->player_gameover();
		}
		break;
	}
	case CS_PACKET_TESTING:
	{
		cs_packet_testing* packet = reinterpret_cast <cs_packet_testing*> (pk);
		std::cout << "error code : " << packet->error_num << std::endl;
		break;
	}
	default:
	{
		closesocket(reinterpret_cast<Player*>(objects[client_id])->_socket);
		DebugBreak();
		break;
	}
	}
}

void WSA_OVER_EX::send_login_ok_packet(int client_id)
{
	sc_packet_login packet;
	Player* player = reinterpret_cast<Player*>(objects[client_id]);
	
	packet.x = player->_x;
	packet.y = player->_y;
	packet.z = player->_z;
	packet.player_id = player->_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN;
	packet.maxplayer = ROOMPLAYER;
	packet.connectplayer = 0;
	int pl = 0;
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		if(i != player->_id)
		{
			if(pl == 0)
			{
				packet.p1_x = objects[i]->_x;
				packet.p1_y = objects[i]->_y;
				packet.p1_z = objects[i]->_z;
			}
			if (pl == 1)
			{
				packet.p2_x = objects[i]->_x;
				packet.p2_y = objects[i]->_y;
				packet.p2_z = objects[i]->_z;
			}
			if (pl == 2)
			{
				packet.p3_x = objects[i]->_x;
				packet.p3_y = objects[i]->_y;
				packet.p3_z = objects[i]->_z;
			}
			if (pl == 3)
			{
				packet.p4_x = objects[i]->_x;
				packet.p4_y = objects[i]->_y;
				packet.p4_z = objects[i]->_z;
			}
			pl++;
		}
	}

	player->send_packet(&packet);
}

void WSA_OVER_EX::send_citizen_First_create_packet(int client_id)
{
	
	Player* player = reinterpret_cast<Player*>(objects[client_id]);

	for (int i = 0; i < MAXPLAYER; ++i)
	{
		for (int j = 0; j < FIRSTCITIZENCREATE; ++j)
		{
			sc_packet_citizencreate packet;
			packet.x = objects[i * 200 + j + 5]->_x;
			packet.y = objects[i * 200 + j + 5]->_y;
			packet.z = objects[i * 200 + j + 5]->_z;
			packet.citizenid = i * 200 + j + 5;

			packet.size = sizeof(sc_packet_citizencreate);
			packet.type = SC_PACKET_CITIZENCREATE;

			player->send_packet(&packet);
		}
	}
}

void WSA_OVER_EX::send_resource_First_create_packet(int client_id)
{
	Player* player = reinterpret_cast<Player*>(objects[client_id]);
	for (int i = RESOURCESTART; i < MAXRESOURCE + RESOURCESTART; ++i)
	{
		Resource* resource = reinterpret_cast<Resource*>(objects[i]);
		sc_packet_resourcecreate packet;

		packet.x = resource->_x;
		packet.y = resource->_y;
		packet.z = resource->_z;
		packet.resource_type = resource->_type;
		packet.resource_id = i;
		packet.amount = resource->_amount;

		packet.size = sizeof(sc_packet_resourcecreate);
		packet.type = SC_PACKET_RESOURCECREATE;

		player->send_packet(&packet);
	}
}

void WSA_OVER_EX::set_accept_over()
{
	_iocpop = IOCPOP::OP_ACCEPT;
}


bool CAS(volatile int* addr, int expected, int update)
{
	return std::atomic_compare_exchange_strong(reinterpret_cast <volatile std::atomic_int*>(addr), &expected, update);
}

void all_player_sendpacket(void* packet)
{
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		Player* player = reinterpret_cast<Player*>(objects[i]);
		if (player->_state != STATE::ST_INGAME) continue;
		player->send_packet(packet);
	}
}

bool overlap_check(int id1, int id2, float range)
{
	if (abs(objects[id1]->_x - objects[id2]->_x) <  range) return false;
	if(abs(objects[id1]->_y - objects[id2]->_y) < range) return false;
	return true;
}

bool object_find_check(int id1, int id2, float range)
{
	if (abs(objects[id1]->_x - objects[id2]->_x) > range) return false;
	if (abs(objects[id1]->_y - objects[id2]->_y) > range) return false;
	return true;
}
