#pragma once
#include"Network.h"



class Citizen : public Object
{
public:
	Citizen(int id);
	~Citizen();

	char _Job;	//	-1 : �����ȵ�, �� ���, 0 : ����, 1 : �ڿ� ä��, 2 : ....
	float _job_x, _job_y, _job_z;
	char _HP;

	void set_citizen_location(float x, float y, float z);
};

