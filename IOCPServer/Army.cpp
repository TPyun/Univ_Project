#include "Army.h"
#include "GameEvent.h"
#include"Player.h"
#include"Citizen.h"
#include<iostream>
Army::Army(int i)
{
	_id = i;
	_a_state = A_STATE::ST_FREE;
	_playerID = (i - ARMYSTART) / (ARMYMAX / 5);
	last_attack = std::chrono::system_clock::now();
	last_plunder = std::chrono::system_clock::now();
	for (auto& a : _citizens)
	{
		a = -1;
	}
	_arrival_x = _x = 0;
	_arrival_y = _y = 0;
	_arrival_z = _z = 0;
}

Army::~Army()
{
	_id = -1;
	_a_state = A_STATE::ST_FREE;
	for (auto& a : _citizens)
	{
		a = -1;
	}
}

void Army::SpawnArmy(float x, float y, float z)
{
	_arrival_x = _x = x;
	_arrival_y = _y = y;
	_arrival_z = _z = z;
}

void Army::set_army_move()
{
	if (Is_sand_storm)
	{
		for (int p_id = 0; p_id < MAXPLAYER; ++p_id)
		{
			if (!object_find_check(p_id, _id, 7500))
			{
				set_army_dead();
				return;
			}
		}
	}

	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_attack).count() > 5000)
	{
		for (int a_id = ARMYSTART; a_id < ARMYSTART + ARMYMAX; ++a_id)
		{
			if ((a_id - ARMYSTART) / PLAYERARMYCOUNT == _playerID)
				continue;
			Player* player = reinterpret_cast<Player*>(objects[_playerID]);
			if (player->War_Players[((a_id - ARMYSTART) / PLAYERARMYCOUNT)] == 0)
				continue;

			Army* enemy_army = reinterpret_cast<Army*>(objects[a_id]);
			if (enemy_army->_a_state != ST_CONPLETE)
				continue;
			if (object_find_check(a_id, _id, 5000))
			{
				last_attack = std::chrono::system_clock::now();
				if (object_find_check(_playerID, _id, 7500))
				{
					_arrival_x = _x;
					_arrival_y = _y;
				}
				enemy_army->_hp -= 20 + _army_type * 10;
				if (enemy_army->_hp < 0)
				{
					player->kill_citizen += 5;
					enemy_army->set_army_dead();
					return;
				}
				{
					float distance = sqrt(pow(_x - enemy_army->_x, 2) + pow(_y - enemy_army->_y, 2));

					sc_packet_armyattack packet;
					packet.size = sizeof(packet);
					packet.type = SC_PACKET_ARMYATTACK;
					packet.a_state = 2;
					packet.army_id = _id;
					packet.rx = (enemy_army->_x - _x) / distance;
					packet.ry = (enemy_army->_y - _y) / distance;
					packet.rz = 0;
					all_player_sendpacket(&packet);
				}

				{
					sc_packet_armychangehp packet;
					packet.army_id = enemy_army->_id;
					packet.hp = enemy_army->_hp;
					packet.size = sizeof(packet);
					packet.type = SC_PACKET_ARMYCHANGEHP;
					all_player_sendpacket(&packet);
				}
				return;
			}
		}
	}
	if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_attack).count() > 1000)
	{
		Player* player = reinterpret_cast<Player*>(objects[_playerID]);
		if (_x != _arrival_x || _y != _arrival_y)
		{
			char state = 1;
			float distance = sqrt(pow(_x - _arrival_x, 2) + pow(_y - _arrival_y, 2));
			int testspeed = 1;
			_rx = (_arrival_x - _x) / distance;
			_ry = (_arrival_y - _y) / distance;
			_rz = 0;
			if (distance < 20 * testspeed)
			{
				_x = _arrival_x;
				_y = _arrival_y;
				state = 0;
			}
			bool _isOverlap = false;

			if (!_isOverlap)
			{
				_x += ((_arrival_x - _x) / distance) * 10 * player->_adventure_speed * testspeed;
				_y += ((_arrival_y - _y) / distance) * 10 * player->_adventure_speed * testspeed;
			}
			int _i_x = _x; int _i_y = _y;
			_arrival_z = ((float)object_z[_i_x / 100][_i_y / 100] * (1 - ((_i_x) % 100) / 100) + (float)object_z[(_i_x + 100) / 100][_i_y / 100] * (((_i_x / 100) % 100) / 100) + (float)object_z[_i_x / 100][_i_y / 100] * (1 - ((_i_y) % 100) / 100) + (float)object_z[_i_x / 100][(_i_y + 100) / 100] * ((_i_y) % 100) / 100) * 20;
			//if (abs(_arrival_z - _z) > 10)
			{
				_z = _arrival_z;
			}
			for (int e_id = EVENTSTART; e_id < EVENTSTART + EVENTMAX; ++e_id)
			{
				GameEvent* g_event = reinterpret_cast<GameEvent*>(objects[e_id]);
				if (g_event->ev_type != GameEvent::EV_FREE && g_event->ev_type != GameEvent::EV_COUNT)
				{
					if (object_find_check(e_id, _id, 10000))
					{
						
						if (object_find_check(e_id, _id, 500) && g_event->g_player_id == -1)
						{
							g_event->check_event(_playerID, _id);
						}
						else
							player->find_event(e_id);
					}
					/*if (g_event->ev_type == g_event->EV_GETCITIZEN)
					{
						Player* player = reinterpret_cast<Player*>(objects[0]);
						player->find_event(g_event->_id);
					}*/
				}
			}
			if (object_find_check(_id, _playerID, 5500))
			{

				for (int i = 0; i < 5; ++i)
				{
					if (_resource_amount[i] > 0)
					{
						player->_resource_amount[i] += _resource_amount[i];
						_resource_amount[i] = 0;
					}
				}
				if (_Gypsy_citizen > 0)
				{
					player->create_citizen(_Gypsy_citizen);
					_Gypsy_citizen = 0;
					is_escort = false;
				}
			}

			//_z = (object_z[_i_x / 100][_i_y / 100]) * 50;
			sc_packet_armymove packet;
			packet.x = _x;
			packet.y = _y;
			packet.z = _z;
			packet.rx = _rx;
			packet.ry = _ry;
			packet.rz = 0;
			packet.a_id = _id;
			packet.a_state = state;
			packet.size = sizeof(sc_packet_armymove);
			packet.type = SC_PACKET_ARMYMOVE;

			for (int i = 0; i < MAXPLAYER; ++i)
			{
				reinterpret_cast<Player*>(objects[i])->send_packet(&packet);
			}
		}
		else if (player->enemy_army_list.size() != 0)
		{
			if(object_find_check(_id, _playerID, 10000))
			{
				int near_enemy = -1;
				float min_distance = 50000.0;
				std::set<int> delete_list;
				for (auto a : player->enemy_army_list)
				{
					Army* enemy_army = reinterpret_cast<Army*>(objects[a]);
					if (enemy_army->_a_state != ST_CONPLETE)
					{
						delete_list.insert(a);
						continue;
					}
					if (object_find_check(a, _playerID, 10000))
					{
						float distance = sqrt(pow(_x - objects[_playerID]->_x, 2) + pow(_y - objects[_playerID]->_y, 2));
						if (distance < min_distance)
						{
							min_distance = distance;
							near_enemy = a;
						}
					}
					else
					{
						delete_list.insert(a);
					}
				}
				for (auto a : delete_list)
				{
					if (player->enemy_army_list.count(a) > 0)
					{
						player->enemy_army_list.erase(a);
					}
				}
				if (near_enemy != -1 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_attack).count() > 5500)
				{
					float ar_x = _x;
					float ar_y = _y;
					float distance = sqrt(pow(_x - objects[near_enemy]->_x, 2) + pow(_y - objects[near_enemy]->_y, 2));
					ar_x = objects[near_enemy]->_x;
					ar_y = objects[near_enemy]->_y;
					set_army_arrival_location(ar_x, ar_y);
				}
			}
		}
	}
}

void Army::set_army_arrival_location(float x, float y)
{
	int temp_x = x;
	int temp_y = y;
	_arrival_x = temp_x;
	_arrival_y = temp_y;
	_arrival_z = 0;
}

void Army::set_army_return_home()
{
	float distance = sqrt(pow(_x - objects[_playerID]->_x, 2) + pow(_y - objects[_playerID]->_y, 2));
	float ar_x = objects[_playerID]->_x - ((objects[_playerID]->_x - _x) / distance) * 5000;
	float ar_y = objects[_playerID]->_y - ((objects[_playerID]->_y - _y) / distance) * 5000;
	set_army_arrival_location(ar_x, ar_y);
}

void Army::set_army_disband()
{
	if(!is_escort)
	{
		sc_packet_armydisband packet;
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_ARMYDISBAND;

		packet.issuccess = object_find_check(_id, _playerID, 6500);
		packet.a_id = _id;

		_Gypsy_citizen = 0;
		if (packet.issuccess == false)
		{
			reinterpret_cast<Player*>(objects[_playerID])->send_packet(&packet);
		}
		else
		{
			_a_state = ST_FREE;
			for (int i = 0; i < MAXPLAYER; ++i)
			{
				int x_count = 0;
				for (auto& a : _citizens)
				{
					Player* player = reinterpret_cast<Player*>(objects[i]);
					sc_packet_citizencreate c_packet;
					c_packet.citizenid = a;
					c_packet.size = sizeof(c_packet);
					c_packet.type = SC_PACKET_CITIZENCREATE;

					Citizen* citizen = reinterpret_cast<Citizen*>(objects[a]);
					//citizen->set_citizen_arrival_location(c_packet.x, c_packet.y, 0);
					player->move_citizen_to_tower(a);
					c_packet.x = citizen->_x;
					c_packet.y = citizen->_y;
					c_packet.z = 0;
					citizen->_job = 0;
					x_count++;
					reinterpret_cast<Player*>(objects[i])->send_packet(&c_packet);
				}
				reinterpret_cast<Player*>(objects[i])->send_packet(&packet);
			}
			std::cout << "disband" << std::endl;
		}
	}
}

void Army::set_army_plunder(int p_id)
{
	if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_plunder).count() > 5000)
	{
		if (object_find_check(p_id, _id, 7500))
		{
			last_plunder = std::chrono::system_clock::now();
			Player* enemy_player = reinterpret_cast<Player*>(objects[p_id]);
			Player* player = reinterpret_cast<Player*>(objects[_playerID]);
			if (enemy_player->enemy_army_list.count(_id) == 0)
			{
				enemy_player->enemy_army_list.insert(_id);
			}
			for (int i = 0; i < 5; ++i)
			{
				if (enemy_player->_resource_amount[i] > 10)
				{
					enemy_player->_resource_amount[i] -= 10;
					player->_resource_amount[i] += 10;
					enemy_player->send_resource_amount();
					player->send_resource_amount();
				}
			}
			enemy_player->modify_dissatisfaction(0.01);
		}
	}
}

void Army::set_army_dead()
{
	_a_state = ST_FREE;
	for (auto& a : _citizens)
	{
		Citizen* citizen = reinterpret_cast<Citizen*>(objects[a]);
		citizen->citizen_dead();
	}
	Player* player = reinterpret_cast<Player*>(objects[_playerID]);
	sc_packet_armydead packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ARMYDEAD;
	packet.army_id = _id;
	all_player_sendpacket(&packet);
	std::cout << "army dead" << std::endl;
}
