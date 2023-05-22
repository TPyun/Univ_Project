#pragma once
#include "Player.h"

class Army : public Object
{
public:
	Army(int i);
	
	~Army();

	enum A_STATE { ST_FREE, ST_TRAINING, ST_CONPLETE };
	int _citizens[5]{};
	A_STATE _a_state;	//-1 : ���� X, 0 : �������, 1 : army
	

	float _arrival_x, _arrival_y, _arrival_z;
	int _playerID;

	void SpawnArmy(float x, float y, float z);
	void set_army_move();
};