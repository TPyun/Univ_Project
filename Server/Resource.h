#pragma once
#include "Building.h"

class Resource
{
public:
	char Type{};///////////////0 : ����,		1 : ��,		2 : ö,		3 : �ķ�,	4 : ����
	int Amount{};
	II Location{};
	int CitizenCount{};

	Resource();
	~Resource();
};

