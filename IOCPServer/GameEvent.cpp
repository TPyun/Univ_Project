#include "GameEvent.h"
#include"random"
#include<iostream>

GameEvent::GameEvent(int id)
{
	_id = id;
	ev_type = EV_TYPE::EV_FREE;
}

void GameEvent::random_create()
{
	std::uniform_int_distribution <int>map_uid{ 100, one_side_number - 100 };
	std::default_random_engine dre;
	dre.seed(rand() * _id);

	
	retry:
	_x = map_uid(dre) * 100;
	_y = map_uid(dre) * 100;
	_z = 10;
	for (int p_id = 0; p_id < MAXPLAYER; ++p_id)
	{
		if (!overlap_check(p_id, _id, 10000))
			goto retry;
	}
	ev_type = random_ev[rand() % (static_cast<int>(EV_COUNT) - 1) + 1];
	if (ev_type == EV_FREE || ev_type == EV_COUNT)
		DebugBreak();

}

void GameEvent::check_event(int p_id)
{
	std::cout << p_id << " : check_event" << std::endl;
	sc_packet_eventselect packet;
	int s_option = 1;
	packet.e_id = _id;
	switch (ev_type)
	{
	case GameEvent::EV_FREE:
	{
		DebugBreak();
		break;
	}
	case GameEvent::EV_GETOIL:
	{
		swprintf(packet.summary, L"������ �߰��߽��ϴ�.");
		resource_count[0] = rand() % 30 + 13;
		swprintf(packet.first, L"���� %d �� ȹ���Ͽ����ϴ�", resource_count[0]);
		break;
	}
	case GameEvent::EV_GETWATER:
	{
		swprintf(packet.summary, L"���� �߰��߽��ϴ�.");
		resource_count[1] = rand() % 30 + 13;
		swprintf(packet.first, L"�� %d �� ȹ���Ͽ����ϴ�", resource_count[1]);
		break;
	}
	case GameEvent::EV_GETIRON:
	{
		swprintf(packet.summary, L"ö�� �߰��߽��ϴ�.");
		resource_count[2] = rand() % 30 + 13;
		swprintf(packet.first, L"ö %d �� ȹ���Ͽ����ϴ�", resource_count[2]);
		break;
	}
	case GameEvent::EV_GETFOOD:
	{
		swprintf(packet.summary, L"�ķ��� �߰��߽��ϴ�.");
		resource_count[3] = rand() % 30 + 13;
		swprintf(packet.first, L"�ķ� %d �� ȹ���Ͽ����ϴ�", resource_count[3]);
		break;
	}
	case GameEvent::EV_GETWOOD:
	{
		swprintf(packet.summary, L"������ �߰��߽��ϴ�.");
		resource_count[4] = rand() % 30 + 13;
		swprintf(packet.first, L"���� %d �� ȹ���Ͽ����ϴ�", resource_count[4]);
		break;
	}
	case GameEvent::EV_GETCITIZEN:
	{
		swprintf(packet.summary, L"�ù��� �߰��߽��ϴ�.");
		citizen_count = rand() % 5 + 13;
		resource_count[3] = rand() % 50 + 25;
		resource_count[1] = rand() % 50 + 25;
		swprintf(packet.first, L"�ù� %d���� �������� ȣ���մϴ�.", citizen_count);
		swprintf(packet.second, L"�ķ� %d �� %d �� ��Ż�մϴ�", resource_count[3], resource_count[1]);
		s_option = 2;
		break;
	}
	case GameEvent::EV_COUNT:
	{
		DebugBreak();
		break;
	}
	default:
	{
		DebugBreak();
		break;
	}
	}
	packet.type = SC_PACKET_EVENTSELECT;
	packet.size = sizeof(sc_packet_eventselect);
	packet.s_option = s_option;

	Player* player = reinterpret_cast<Player*>(objects[p_id]);
	player->send_packet(&packet);

	remove_event();
}

void GameEvent::do_event(int select_num)
{

}

void GameEvent::remove_event()
{
	ev_type = EV_FREE;
	sc_packet_removeevent packet;
	packet.type = SC_PACKET_REMOVEEVENT;
	packet.size = sizeof(packet);
	packet.e_id = _id;
	
	for (int p_id = 0; p_id < MAXPLAYER; ++p_id)
	{
		Player* player = reinterpret_cast<Player*>(objects[p_id]);
		if (player->view_list.count(_id) != 0)
		{
			player->send_packet(&packet);
		}
	}
}
