#pragma once
class Policy
{
public:
	Policy();
	~Policy();
	
	
	bool police = false;							//�ڰ��
	bool guard_post = false;						//��� �ʼ�
	bool prison = false;							//������
	bool martial_law = false;						//�����
	bool dictator = false;							//������
	
	
	bool funeral = false;							//��ʽ�
	bool memorial = false;							//�߸��
	bool body_storage = false;						//��ü �����
	bool organ_transplant = false;					//����̽�
	bool cannibal = false;							//����
	
	
	bool emergency_shift_work = false;				//��� ���� �ٹ�
	bool overtime = false;							//���� �ٹ�
	bool censor = false;							//������
	bool forced_labor = false;						//���� �뵿
	
	
	bool child_labor = false;						//�Ƶ� �뵿
	bool nursery_school = false;					//������
	bool labor_support = false;						//�۾� ����
	bool student_soldier = false;					//�е��� ¡��
	
	
	bool hearty_meal = false;						//����� �Ļ�
	bool soup = false;								//���� ��
	bool alcohol = false;							//����
	char get_meal_resource_consume();
	char get_meal_satiety();
	char get_meal_alcoholic();
	
	
	bool life_sustaining_treatment = false;			//����ġ��
	bool extreme_drug_prescription = false;			//�ؾ�ó��
	bool overcrowd = false;							//���׼���
	bool additional_serving_for_patient = false;	//ȯ�� �߰� ���
	bool drug_treatment = false;					//���� ġ��
	bool nursing_home = false;						//����
	bool prosthetic_limb = false;					//�Ǽ���
};
