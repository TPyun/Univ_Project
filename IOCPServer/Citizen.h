#pragma once
#include "Player.h"



class Citizen : public Object
{
public:
	Citizen(int id);
	~Citizen();

	char _Job;	//	-1 : �����ȵ�, �� ���  // 0 : ���� �� ����  // 1 : �ڿ� ä�� // 2 : ....
	float _job_x, _job_y, _job_z;
	int _HouseId;	// -1 : ������, ������ : �� object id
	char _HP;

	char _Satiety;
	char _thirst;

	float _arrival_x, _arrival_y, _arrival_z;
	int _playerID;
	void set_citizen_spwan_location(float x, float y, float z);
	void set_citizen_arrival_location(float ax, float ay, float az);
	void set_citizen_move();
	void citizen_dead();
	bool citizen_eat_food();
	bool citizen_eat_water();
};

