#include "Player.h"
#include"Army.h"
#include"Citizen.h"
#include <iostream>
#include <string>
#include <random>

using namespace std;

Player::Player(int id, STATE state)
{
	_x = 0; _y = 0; _z = 0;
	_state = state;
	_currentX = 0; _currentY = 0; _currentZ = 0;
	_terrainX = 0; _terrainY = 0; _terrainZ = 0;
	w = false, a = false, s = false, d = false;
	is_connected = false;
	_id = id;
	_socket={0};
	_research = new Research(id);
	_adventure_efficiency = 1.0;
	_work_efficiency = 1.0;
	_adventure_speed = 1.0;
	_oil_efficiency = 1.0;
	_building_insulation = 0.5;
	army_select_num = ARMYSTART + _id * PLAYERARMYCOUNT;
	for (auto& a : _resource_amount)
	{
		a = 10000;
	}
}

Player::~Player()
{

}

void Player::send_packet(void* packet)
{
	char* buf = reinterpret_cast<char*>(packet);

	WSA_OVER_EX* _wsa_send_over = new WSA_OVER_EX(OP_SEND, buf[0], packet);

	if (_state == STATE::ST_INGAME)
	{
		int ret = WSASend(_socket, &_wsa_send_over->_wsabuf, 1, NULL, 0, &_wsa_send_over->_wsaover, NULL);
	}
}

void Player::set_player_location(float x, float y, float z)
{
	_x = x;
	_y = y;
	_z = 0;
	_currentX = - (SIGHT_X * 100 / 2);
	_currentY = - (SIGHT_Y * 100 / 2);
	_currentZ = 0;
}

void Player::key_input(char** player_sight_terrain_line, char** player_sight_temperature_line)
{ 
	//std::cout << _currentX <<std::endl;
	bool keyinput = false;
	char directionX{};
	char directionY{};
	int speed = 100;

	if (w) {
		directionY = -1;
		keyinput = true;
	}
	if (s) {
		directionY = 1;
		keyinput = true;
	}
	if (a) {
		directionX = -1;
		keyinput = true;
	}
	if (d) {
		directionX = 1;
		keyinput = true;
	}
	if ((_x + _currentX + speed * directionX) / 100 <= 0 || (_x + _currentX + speed * directionX) / 100 >= (one_side_number - SIGHT_X))
	{
		//cout << _x + _currentX + speed * directionX << endl;
		directionX = 0;
	}
	if ((_y + _currentY + speed * directionY) / 100 <= 0 || (_y + _currentY + speed * directionY) / 100 >= (one_side_number - SIGHT_Y))
	{
		//cout << _y + _currentY + speed * directionY << endl;
		directionY = 0;
	}
	if (keyinput)
	{
		_currentX += speed * directionX;
		_currentY += speed * directionY;
		
		sc_packet_move sc_packet_move; 
		sc_packet_move.currentX = _currentX;
		sc_packet_move.currentY = _currentY;
		//cout << "X : " << _currentX + _x << " Y : " << _currentY + _y << endl;
		sc_packet_move.currentZ = _currentZ;
		sc_packet_move.size = sizeof(sc_packet_move);
		sc_packet_move.type = SC_PACKET_MOVE;
		send_packet(&sc_packet_move);
	}

	bool ischangeTerrainX = false;
	bool ischangeTerrainY = false;

	if (abs(abs(_terrainX) - abs(_currentX)) >= 100) {
		ischangeTerrainX = true;
	}
	if (abs(abs(_terrainY) - abs(_currentY)) >= 100) {
		ischangeTerrainY = true;
	}
	if (w && a || w && d || s && a || s && d) {
		if (ischangeTerrainX) {
			ischangeTerrainY = true;
			_currentY = int(_currentY) / 100 * 100 + int(_currentX) % 100;
		}
		if (ischangeTerrainY) {
			ischangeTerrainX = true;
			_currentX = int(_currentX) / 100 * 100 + int(_currentY) % 100;
		}
	}
	
	//std::cout << ischangeTerrainX << " " << ischangeTerrainY << std::endl;
	
	if (ischangeTerrainX)
	{
		//Terrain
		sc_packet_terrainXlocation terrain_packet;
		terrain_packet.size = sizeof(sc_packet_terrainXlocation);
		terrain_packet.type = SC_PACKET_TERRAINXLOCATION;
		terrain_packet.terrainX = _x + _currentX;
		if (_terrainX > _currentX){
			if (ischangeTerrainY){
				if(_terrainY > _currentY){
					memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[2] + 2, SIGHT_Y);
				}
				else{
					memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[2], SIGHT_Y);
				}
			}
			else{
				memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[2] + 1, SIGHT_Y);
			}
		}
		else if (_terrainX < _currentX){
			if (ischangeTerrainY){
				if (_terrainY > _currentY){
					memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[3] + 2, SIGHT_Y);
				}
				else{
					memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[3], SIGHT_Y);
				}
			}
			else{
				memcpy(terrain_packet.terrainline_Y, player_sight_terrain_line[3] + 1, SIGHT_Y);
			}
		}
		send_packet(&terrain_packet);

		
		//Temperature
		sc_packet_temperatureX temperature_packet;
		temperature_packet.terrainX = _currentX;
		if (_terrainX > _currentX) {
			if (ischangeTerrainY) {
				if (_terrainY > _currentY) {
					memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[2] + 2, SIGHT_Y);
				}
				else {
					memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[2], SIGHT_Y);
				}
			}
			else {
				memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[2] + 1, SIGHT_Y);
			}
		}
		else if (_terrainX < _currentX) {
			if (ischangeTerrainY) {
				if (_terrainY > _currentY) {
					memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[3] + 2, SIGHT_Y);
				}
				else {
					memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[3], SIGHT_Y);
				}
			}
			else {
				memcpy(temperature_packet.terrainline_Y, player_sight_temperature_line[3] + 1, SIGHT_Y);
			}
		}
		send_packet(&temperature_packet);
	}
	
	if (ischangeTerrainY)
	{
		//Terrain
		sc_packet_terrainYlocation terrain_packet;
		terrain_packet.size = sizeof(sc_packet_terrainYlocation);
		terrain_packet.type = SC_PACKET_TERRAINYLOCATION;
		terrain_packet.terrainY = _y + _currentY;

		if (_terrainY > _currentY){
			if (ischangeTerrainX){
				if (_terrainX > _currentX){
					memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[0] + 2, SIGHT_X);
				}
				else{
					memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[0], SIGHT_X);
				}
			}
			else{
				memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[0] + 1, SIGHT_X);
			}
		}
		else if (_terrainY < _currentY){
			if (ischangeTerrainX){
				if (_terrainX > _currentX){
					memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[1] + 2, SIGHT_X);
				}
				else{
					memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[1], SIGHT_X);
				}
			}
			else{
				memcpy(terrain_packet.terrainline_X, player_sight_terrain_line[1] + 1, SIGHT_X);
			}
		}
		send_packet(&terrain_packet);

		//Temperature
		sc_packet_temperatureY temperature_packet;
		temperature_packet.terrainY = _currentY;
		if (_terrainY > _currentY) {
			if (ischangeTerrainX) {
				if (_terrainX > _currentX) {
					memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[0] + 2, SIGHT_X);
				}
				else {
					memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[0], SIGHT_X);
				}
			}
			else {
				memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[0] + 1, SIGHT_X);
			}
		}
		else if (_terrainY < _currentY) {
			if (ischangeTerrainX) {
				if (_terrainX > _currentX) {
					memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[1] + 2, SIGHT_X);
				}
				else {
					memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[1], SIGHT_X);
				}
			}
			else {
				memcpy(temperature_packet.terrainline_X, player_sight_temperature_line[1] + 1, SIGHT_X);
			}
		}
		send_packet(&temperature_packet);
	}

	if (abs(abs(_terrainX) - abs(_currentX)) >= 100) {
		_terrainX = _currentX;
	}
	if (abs(abs(_terrainY) - abs(_currentY)) >= 100) {
		_terrainY = _currentY;
	}


	w = false;
	a = false;
	s = false;
	d = false;

	
	/*delete[] player_sight_terrain_line[0];
	delete[] player_sight_terrain_line[1];
	delete[] player_sight_terrain_line[2];
	delete[] player_sight_terrain_line[3];


	delete[] player_sight_temperature_line[0];
	delete[] player_sight_temperature_line[1];
	delete[] player_sight_temperature_line[2];
	delete[] player_sight_temperature_line[3];*/
}

void Player::send_resource_amount()
{
	sc_packet_playerresource packet;
	packet.size = sizeof(sc_packet_playerresource);
	packet.type = SC_PACKET_PLAYERRESOURCE;

	for (int i = 0; i < 5; ++i)
	{
		packet.resources_acount[i] = _resource_amount[i];
	}
	send_packet(&packet);
}

void Player::send_terrain_All()
{

	return;
}

void Player::send_sunangle(const float sunangle)
{
	sc_packet_sunangle packet;
	packet.size = sizeof(sc_packet_sunangle);
	packet.type = SC_PACKET_SUNANGLE;
	packet.sunangle = sunangle;

	send_packet(&packet);
}

int Player::playercitizencount()
{
	int _citizencount = 0;
	for (int citizen_id = CITIZENSTART + _id * PLAYERCITIZENCOUNT; citizen_id < CITIZENSTART + (_id + 1) * PLAYERCITIZENCOUNT; ++citizen_id)
	{
		Citizen* citizen = reinterpret_cast<Citizen*>(objects[citizen_id]);
		if (citizen->_job != -1)
			_citizencount++;

	}
	return _citizencount;
}

int Player::joblesscitizen()
{
	int _citizencount = 0;
	for (int citizen_id = CITIZENSTART + _id * PLAYERCITIZENCOUNT; citizen_id < CITIZENSTART + (_id + 1) * PLAYERCITIZENCOUNT; ++citizen_id)
	{
		Citizen* citizen = reinterpret_cast<Citizen*>(objects[citizen_id]);
		if (citizen->_job == 0)
			_citizencount++;
	}
	return _citizencount;
	return 0;
}

void Player::playerMinimapLocation(float mini_x, float mini_y)
{

	_currentX = (mini_x - _x - SIGHT_X / 2 * 100);
	_currentY = (mini_y - _y - SIGHT_Y / 2 * 100);
	//cout << "=====================================\n";
	//cout << "B playerMinimapLocation: " << _currentX + _x << " " << _currentY + _y << " mini: " << mini_x << " " << mini_y << endl;

	if (_x + _currentX < SIGHT_X / 2 + 100)
		_currentX = SIGHT_X / 2 -_x + 100;
	else if (_x + _currentX >= one_side_number * 100 - SIGHT_X * 100 - 100)
		_currentX = one_side_number * 100 - _x - SIGHT_X * 100 - 100;
	if (_y + _currentY < SIGHT_Y / 2 + 100)
		_currentY = SIGHT_Y / 2 -_y + 100;
	else if (_y + _currentY >= one_side_number * 100 - SIGHT_Y * 100 - 100)
		_currentY = one_side_number * 100 - _y - SIGHT_Y * 100 - 100;

	//cout << "playerMinimapLocation: " << _currentX + _x << " " << _currentY + _y << " mini: " << mini_x << " " << mini_y << endl;
	
retry:
	int cas_bool = _Minimap_terrainsend;
	if (cas_bool != 0)
		goto retry;
	while (CAS(&_Minimap_terrainsend, cas_bool,1)){}
	while (_Minimap_terrainsend == 0) {};

	sc_packet_move sc_packet_move;
	sc_packet_move.currentX = _currentX;
	sc_packet_move.currentY = _currentY;
	sc_packet_move.currentZ = _currentZ;
	sc_packet_move.size = sizeof(sc_packet_move);
	sc_packet_move.type = SC_PACKET_MOVE;
	send_packet(&sc_packet_move);
}

void Player::find_event(int e_id)
{
	if (view_list.count(e_id) != 0)
		return;

	view_list.insert(e_id);


	sc_packet_viewevnet packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_VIEWEVENT;

	packet.e_id = e_id;
	packet.x = objects[e_id]->_x;
	packet.y = objects[e_id]->_y;
	packet.z = objects[e_id]->_z;

	send_packet(&packet);
}

void Player::create_citizen(int num)
{
	int count = 0;
	for (int c_id = CITIZENSTART + PLAYERCITIZENCOUNT * _id; c_id < CITIZENSTART + PLAYERCITIZENCOUNT * (_id + 1); c_id++)
	{
		Citizen* citizen = reinterpret_cast<Citizen*>(objects[c_id]);
		if (citizen->_job == -1)
		{
			citizen->_job = 0;
			move_citizen_to_tower(citizen->_id);
			citizen->_x = citizen->_arrival_x;
			citizen->_y = citizen->_arrival_y;
			
			for (int i = 0; i < MAXPLAYER; ++i)
			{
				sc_packet_citizencreate packet;
				packet.x = citizen->_x;
				packet.y = citizen->_y;
				packet.z = citizen->_z;
				packet.citizenid = citizen->_id;

				packet.size = sizeof(sc_packet_citizencreate);
				packet.type = SC_PACKET_CITIZENCREATE;
				Player* player = reinterpret_cast<Player*>(objects[i]);
				player->send_packet(&packet);
			}
			count++;
		}
		if (num == count)
			break;
	}
}

void Player::Trade_Request(int p_num)
{
	sc_packet_traderequest packet;
	packet.request_player = p_num;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TRADEREQUEST;
	send_packet(&packet);
}

void Player::Trade_Request_Agree(int p_num, int agree)
{
	sc_packet_tradeagree packet;
	packet.isagree = agree;
	packet.request_player = p_num;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TRADEAGREE;
	send_packet(&packet);
}

void Player::change_trade_resource(int trade_resource_num, int resource_amount)
{
	trade_resource[trade_resource_num] = resource_amount;
}

void Player::send_change_trade_resource(int trade_resource_num, int amount)
{
	sc_packet_traderesource packet;
	packet.resource_num = trade_resource_num;
	packet.resource_amount = amount;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TRADERESOURCE;
	send_packet(&packet);
}

void Player::send_declaration_war(int p_num, bool is_war)
{
	sc_packet_declaration_war packet;
	packet.size = sizeof(sc_packet_declaration_war);
	packet.type = SC_PACKET_DECLARATION_WAR;
	packet.player_num = p_num;
	packet.is_war = is_war;

	send_packet(&packet);
}

void Player::trade_clear()
{
	Player* o_player = reinterpret_cast<Player*>(objects[trade_player_id]);
	for (int i = 0; i < 5; ++i)
	{
		o_player->_resource_amount[i] += trade_resource[i];
		_resource_amount[i] -= trade_resource[i];
		trade_resource[i] = 0;
	}
	War_Players[trade_player_id] = false;
	send_declaration_war(trade_player_id, false);

	trade_success = false;
	send_resource_amount();
}

void Player::send_citizen_status()
{
	//더운 사람 수, 굶은 사람 수, 목마른 사람 수
	int soldier_num = 0;
	int citizen_num = 0;
	int hot = 0;
	int hungry = 0;
	int thirsty = 0;
	
	for (int c_id = CITIZENSTART + PLAYERCITIZENCOUNT * _id; c_id < CITIZENSTART + PLAYERCITIZENCOUNT * (_id + 1); c_id++) {
		Citizen* citizen = reinterpret_cast<Citizen*>(objects[c_id]);
		if (citizen->_job == -1 ) continue;
		if (citizen->_job == 22) {
			soldier_num++;
			continue;
		}
		
		if (citizen->_satiety == 0)
			hungry++;
		if (citizen->_thirsty == 0)
			thirsty++;
		if (citizen->_temperature > 40)
			hot++;
		citizen_num++;
		//if (_id == 0)
			//cout << c_id << " " << (int)citizen->_satiety << " " << (int)citizen->_thirsty << " " << (int)citizen->_temperature << endl;
	}
	//cout << "=============================\n";
	total_citizen_num = citizen_num;
	
	//cout << "soldier: " << (int)(soldier_num / 5) << " citizen: " << citizen_num << " hot : " << hot << " hungry : " << hungry << " thirsty : " << thirsty << endl;
	sc_packet_citizen_status packet;
	packet.soldier_num = (int)(soldier_num / 5);
	packet.citizen_num = citizen_num;
	packet.citizen_hot = hot;
	packet.citizen_hungry = hungry;
	packet.citizen_thirsty = thirsty;
	send_packet(&packet);
}

void Player::send_sprinkler_off()
{
	sc_packet_sprinkler_off packet;
	send_packet(&packet);
}

void Player::player_gameover()
{
	sc_packet_gameover packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_GAMEOVER;
	send_packet(&packet);
}

void Player::player_ending()
{
	
	int total_resource = 0;
	for (int i = 0; i < 5; ++i)
	{
		total_resource = _resource_amount[i];
	}

	if (kill_citizen > total_citizen_num)
	{
		swprintf(ending_title, L"무자비한 지배자");
		score += 20000;
	}
	else if (dead_citizen_num == 0 && total_citizen_num > 190 && total_resource > 10000)
	{
		swprintf(ending_title, L"완벽한 지배자");
		score += 50000;
	}
	else if (dead_citizen_num > total_citizen_num)
	{
		swprintf(ending_title, L"무능한 지배자");
		score += 10000;
	}
	else if (kill_citizen == 0)
	{
		swprintf(ending_title, L"평화주의자");
		score += 20000;
	}	
	else if (total_resource > 20000)
	{
		swprintf(ending_title, L"철강왕 지배자");
		score += 20000;
	}
	else
	{
		swprintf(ending_title, L"평범한 지배자");
		score += 10000;
	}
	
}

void Player::set_score()
{
	int total_resource = 0;
	for (int i = 0; i < 5; ++i)
	{
		total_resource = _resource_amount[i];
	}
	all_resource_count = total_resource;
	score += total_resource;
	score -= kill_citizen * 100;
	score -= dead_citizen_num * 100;
	score += total_citizen_num * 100;
	if (score < 0)
		score = 0;
}

void Player::send_ending()
{
	sc_packet_gameend packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_GAMEEND;
	packet.alive_citizen = total_citizen_num;
	packet.dead_citizen = dead_citizen_num;
	swprintf(packet.ending_title, ending_title);
	packet.kill_citizen = kill_citizen;
	packet.resource = all_resource_count;
	packet.rank = rank;
	packet.score = score;
	send_packet(&packet);
}

void Player::move_citizen_to_tower(int citizen_id)
{
	Citizen* citizen = reinterpret_cast<Citizen*>(objects[citizen_id]);
	citizen->stay_tower = true;
	
	int near_town_citizen_num{};
	for (int i = CITIZENSTART + _id * PLAYERCITIZENCOUNT; i < CITIZENSTART + _id * PLAYERCITIZENCOUNT + PLAYERCITIZENCOUNT; ++i)
	{
		Citizen* this_citizen = reinterpret_cast<Citizen*>(objects[i]);
		if (this_citizen->stay_tower) {
			near_town_citizen_num++;
		}
	}
	
	int iter{};
	for (int i = CITIZENSTART + _id * PLAYERCITIZENCOUNT; i < CITIZENSTART + _id * PLAYERCITIZENCOUNT + PLAYERCITIZENCOUNT; ++i)
	{
		Citizen* this_citizen = reinterpret_cast<Citizen*>(objects[i]);
		if (this_citizen->stay_tower) {
			float angle = 3.141592 * 2 / near_town_citizen_num * iter;
			iter++;
			int distance = 600;

			this_citizen->_arrival_x = _x + distance * std::cos(angle);
			this_citizen->_arrival_y = _y + distance * std::sin(angle);
		}
	}
}

void Player::send_wind_direction(int wind_x, int wind_y)
{
	sc_packet_wind_direction packet;
	packet.wind_x = wind_x;
	packet.wind_y = wind_y;
	send_packet(&packet);
}

void Player::send_dissatisfaction()
{
	sc_packet_dissatisfaction packet;
	packet.dissatisfaction = dissatisfaction;
	send_packet(&packet);
}

void Player::modify_dissatisfaction(float amount)
{
	if (dissatisfaction + amount >= 1) dissatisfaction = 1;
	else if (dissatisfaction + amount <= 0) dissatisfaction = 0;
	else dissatisfaction += amount;
	//cout << "dissatisfaction : " << dissatisfaction << endl;
}
void Player::set_army_select(int select_type)
{
	if (Is_sand_storm)
		return;
	
	bool select_fail = true;
	if (select_type == 0)
	{
		army_select_num++;
		if (army_select_num >= ARMYSTART + (_id + 1) * PLAYERARMYCOUNT)
		{
			army_select_num = ARMYSTART + _id * PLAYERARMYCOUNT;
		}
		for (int a_id = army_select_num; a_id < ARMYSTART + (_id + 1) * PLAYERARMYCOUNT; ++a_id)
		{
			army_select_num = a_id;
			Army* army = reinterpret_cast<Army*>(objects[a_id]);
			if (army->_a_state == Army::ST_CONPLETE)
			{
				select_fail = false;
				army_select_num = a_id;
				set_location_input(army->_x, army->_y);
				sc_packet_move sc_packet_move;
				sc_packet_move.currentX = _currentX;
				sc_packet_move.currentY = _currentY;
				sc_packet_move.currentZ = _currentZ;
				sc_packet_move.size = sizeof(sc_packet_move);
				sc_packet_move.type = SC_PACKET_MOVE;
				send_packet(&sc_packet_move);
				break;
			}

		}
	}
	else if (select_type == 1)
	{
		army_select_num--;
		if (army_select_num < ARMYSTART + (_id) * PLAYERARMYCOUNT - 1)
		{
			army_select_num = ARMYSTART + (_id + 1) * PLAYERARMYCOUNT - 1;
		}
		for (int a_id = army_select_num; a_id >= ARMYSTART + (_id)  *PLAYERARMYCOUNT; a_id--)
		{
			army_select_num = a_id;
			Army* army = reinterpret_cast<Army*>(objects[a_id]);
			if (army->_a_state == Army::ST_CONPLETE)
			{
				select_fail = false;
				army_select_num = a_id;
				set_location_input(army->_x, army->_y);
				sc_packet_move sc_packet_move;
				sc_packet_move.currentX = _currentX;
				sc_packet_move.currentY = _currentY;
				sc_packet_move.currentZ = _currentZ;
				sc_packet_move.size = sizeof(sc_packet_move);
				sc_packet_move.type = SC_PACKET_MOVE;
				send_packet(&sc_packet_move);
				break;
			}

		}
	}
	if (select_fail)
	{
		set_location_input(_x, _y);
	}
}

void Player::set_location_input(float set_x, float set_y)
{
	_currentX = (set_x - _x) - (SIGHT_X * 100 / 2);
	_currentY = (set_y - _y) - (SIGHT_Y * 100 / 2);

retry:
	int cas_bool = _Minimap_terrainsend;
	if (cas_bool != 0)
		goto retry;
	while (CAS(&_Minimap_terrainsend, cas_bool, 1)) {}
	while (_Minimap_terrainsend == 0) {};

	sc_packet_move sc_packet_move;
	sc_packet_move.currentX = _currentX;
	sc_packet_move.currentY = _currentY;
	sc_packet_move.currentZ = _currentZ;
	sc_packet_move.size = sizeof(sc_packet_move);
	sc_packet_move.type = SC_PACKET_MOVE;
	send_packet(&sc_packet_move);
}


