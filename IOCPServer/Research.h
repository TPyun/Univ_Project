#pragma once




class Research
{
public:
	int tech_phase = 0;
	int lab_count = 0;
	short tech[5]{};		// 5�� ������ ��ﶧ���� 1�� �þ

public:
	void set_tech_upgrade(int type, int level);
	void set_tech_phase(int phase);
}; 