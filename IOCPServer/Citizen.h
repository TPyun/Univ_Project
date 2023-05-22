#pragma once
#include "Player.h"



class Citizen : public Object
{
public:
	Citizen(int id);
	~Citizen();

	char _job;	//	-1 : �����ȵ�, �� ���  // 0 : ���� �� ����  // 1 : �ڿ� ä�� // 2 : �ǹ� ���� // 11 : HunterHouse	// 21 : armycampmove	22 : army
	float _job_x, _job_y, _job_z;
	int _Job_id;
	int _house_id;	// -1 : ������, ������ : �� object id
	char _hp;
	char _citizenstate;		// 0 : idle, 1 : wark, 2 : sleep
	char _satiety;
	char _thirsty;

	float _arrival_x, _arrival_y, _arrival_z;
	int _playerID;
	void set_citizen_spwan_location(float x, float y, float z);
	void set_citizen_arrival_location(float ax, float ay, float az);
	void set_citizen_move();
	void citizen_dead();
	bool citizen_eat_food();
	bool citizen_eat_water();
};

