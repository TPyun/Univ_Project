#pragma once
#include "Player.h"



class Citizen : public Object
{
public:
	Citizen(int id);
	~Citizen();

	char _job;	//	-1 : 생성안됨, 및 사망  // 0 : 생성 및 무직  // 1 : 자원 채취 or 건물에서 업무 // 2 : 건물 짓기  21 : armycampmove	22 : army
	float _job_x, _job_y, _job_z;
	int _Job_id;
	int _house_id;	// -1 : 집없음, 나머지 : 집 object id
	char _citizenstate;		// 0 : idle, 1 : walk, 2 : sleep

	bool stay_tower;
	
	char _satiety;	//0~100
	char _thirsty;	//0~100
	char _temperature;
	
	char _hp;			//0~100
	char _alcoholic;	//0~100
	bool _disabled;
	char _dissatisfaction; //불만 0~100

	float _arrival_x, _arrival_y, _arrival_z;
	int _playerID;
	void set_citizen_spwan_location(float x, float y, float z);
	void set_citizen_arrival_location(float ax, float ay, float az);
	void set_citizen_move();
	void citizen_dead();
	void citizen_eat_food();
	void citizen_drink_water();
	void modify_hp(int amount);
	void modify_satiety(int amount);
	void modify_thirsty(int amount);
	void make_random_round_position(float& x, float& y, float distance, int max_citizen, int iter);
};

