#pragma once
#include "Player.h"

class Resource : public Object
{
public:
	Resource(int id,char type);
	~Resource();

	char _type{-1};	//0 : ����,		1 : ��,		2 : ö,		3 : �ķ�,	4 : ����
	int _resourceid;
	int _amount{20};
	int _maxamount{};
	int _citizencount{};
	class Citizen* _workcitizen[10]{};
	void set_resource_spwan_location(float player_x, float player_y, float player_z);
	void set_resource_citizen_placement(int client_id, char isplus);
};

