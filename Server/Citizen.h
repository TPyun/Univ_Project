#pragma once
#include "Resource.h"

class Citizen
{
public:
	Citizen();
	~Citizen();

	TCHAR Name[30];
	II Location;
	int Rotation;
	char resources[5];
	char HP;
	char Job;				/////////////// 0 : ����, 1 : �ڿ� ä��
	char IsJob;
	II JobLocation;

};

