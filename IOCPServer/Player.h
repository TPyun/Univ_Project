#pragma once
#include"protocol.h"
#include"Network.h"
#include "Policy.h"
#include"research.h"

class Player : public Object
{
public:
	STATE _state;
	SOCKET _socket;
	WSA_OVER_EX _wsa_recv_over;
	int _prev_size{};
	char _packet_buf[BUFSIZE]{};
	char is_connected;
	char w, a, s, d;

	Policy _policy;
	Research* _research;
	std::set<int> view_list;
	int army_select_num = -1;

	int total_citizen_num;
	int around_tower_citizen_num{};

	//ending시 사용
	int dead_citizen_num = 0;
	int kill_citizen = 0;

	float _resource_amount[5] = {};
	float _adventure_efficiency = {1.0};
	float _work_efficiency = {1.0};
	float _adventure_speed = { 1.0 };
	float _oil_efficiency = { 1.0 };
	float _building_insulation = { 0.5 };

	float _currentX, _currentY, _currentZ;

	char** player_sight_terrain;
	float _terrainX, _terrainY, _terrainZ;

	volatile int _Minimap_terrainsend = 0;
	std::mutex _t_l;

	int trade_player_id{};
	int trade_resource[5]{};			//0,1,2,3,4
	bool trade_success = false;
	std::set<int> enemy_army_list;
	//�Ҹ� ���
	float dissatisfaction = 0.0;

	int score = 0;
	int rank = 0;
	wchar_t ending_title[30];
	int all_resource_count = 0;

	bool War_Players[5] = {};			//false = Peace, true = War

	Player(int id, STATE state = STATE::ST_FREE);
	~Player();

	void send_packet(void * packet);
	void set_player_location(float x, float y, float z);
	void key_input(char** player_sight_terrain_line, char** player_sight_temperature_line);
	void send_resource_amount();
	void send_terrain_All();
	void send_sunangle(const float sunangle);
	int playercitizencount();
	int joblesscitizen();
	void playerMinimapLocation(float mini_x, float mini_y);
	void find_event(int e_id);

	void create_citizen(int num);
	void Trade_Request(int p_num);
	void Trade_Request_Agree(int p_num, int agree);
	void change_trade_resource(int trade_resource_num, int resource_amount);
	void send_change_trade_resource(int trade_resource_num, int amount);
	void send_declaration_war(int p_num, bool is_war);
	void trade_clear();
	void set_army_select(int select_type);
	void set_location_input(float mini_x, float mini_y);
	void modify_dissatisfaction(float amount);
	void send_citizen_status();
	void send_sprinkler_off();
	void player_gameover();
	void player_ending();
	void set_score();
	void player_ending(int rank, int score);
	void send_ending();
	void move_citizen_to_tower( int citizen_id);
	void send_wind_direction(int wind_x, int wind_y);
	void send_dissatisfaction();
};
