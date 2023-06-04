#include "Policy.h"

Policy::Policy()
{
}

Policy::~Policy()
{
}

void Policy::set_policy(int policy_id)
{
	// 0: ����� �Ļ�, 1:������, 2: ����
	// 3: ����ġ��, 4: �ؾ�ó��, 5: ���׼���, 6: ȯ���߰����, 7: ����ġ�� 8: ����, 9: �Ǽ���
	// 10: ��ʽ�, 11: ��ü�����, 12: ����̽�, 13: ����, 14: �߸��
	// 15: �ڰ��, 16: ����ʼ�, 17: ������, 18: �����, 19: ������
	// 20: ��󱳴�ٹ�, 21: ����ٹ�, 22: ������, 23: �����뵿
	// 24: �Ƶ��뵿, 25: ������, 26: �е���¡��, 27: �۾�����
	std::cout << "set_policy " << policy_id << std::endl;
 	switch (policy_id)
	{
	case 0:
		hearty_meal = true;
		break;
	case 1:
		soup = true;
		break;
	case 2:
		alcohol = true;
		break;
	case 3:
		life_sustaining_treatment = true;
		break;
	case 4:
		extreme_drug_prescription = true;
		break;
	case 5:
		overcrowd = true;
		break;
	case 6:
		additional_serving_for_patient = true;
		break;
	case 7:
		drug_treatment = true;
		break;
	case 8:
		nursing_home = true;
		break;
	case 9:
		prosthetic_limb = true;
		break;
	case 10:
		funeral = true;
		break;
	case 11:
		body_storage = true;
		break;
	case 12:
		organ_transplant = true;
		break;
	case 13:
		cannibal = true;
		break;
	case 14:
		memorial = true;
		break;
	case 15:
		police = true;
		break;
	case 16:
		guard_post = true;
		break;
	case 17:
		prison = true;
		break;
	case 18:
		martial_law = true;
		break;
	case 19:
		dictator = true;
		break;
	case 20:
		emergency_shift_work = true;
		break;
	case 21:
		overtime = true;
		break;
	case 22:
		censor = true;
		break;
	case 23:
		forced_labor = true;
		break;
	case 24:
		child_labor = true;
		break;
	case 25:
		nursery_school = true;
		break;
	case 26:
		student_soldier = true;
		break;
	case 27:
		labor_support = true;
		break;
	default:
		break;
	}
}

char Policy::get_meal_resource_consume()
{
	if (hearty_meal)
		return 2;
	else
		return 1;
}

char Policy::get_meal_satiety()
{
	if (hearty_meal)
		return 40;
	else if (soup)
		return 40;
	else
		return 20;
}

char Policy::get_meal_alcoholic()
{
	if (alcohol)
		return 10;
	else
		return 0;
}
