#pragma once

constexpr int one_side_number = 3200;
constexpr int random_array_size = 10000000;
constexpr int city_size = 100;

#define BUFSIZE						512
#define SERVERPORT					1234

#define MAXNAMESIZE					20
#define MAXOBJECT					5000

#define MAXPLAYER					5
#define ROOMPLAYER					1

#define CITIZENSTART				MAXPLAYER
#define FIRSTCITIZENCREATE			20

#define PLAYERCITIZENCOUNT			200
#define MAXCITIZEN					1000
#define RESOURCESTART				(MAXCITIZEN + CITIZENSTART)
#define MAXRESOURCE					50
#define BUILDINGSTART				(MAXRESOURCE + RESOURCESTART)
#define PLAYERBUILDINGCOUNT			484
#define MAXBUILDING					(PLAYERBUILDINGCOUNT * MAXPLAYER)
#define ARMYSTART					(BUILDINGSTART + MAXBUILDING)
#define PLAYERARMYCOUNT				20
#define ARMYMAX						(PLAYERARMYCOUNT * MAXPLAYER)
#define EVENTSTART					(ARMYSTART + ARMYMAX)
#define EVENTMAX					200


#define SIGHT_X 200
#define SIGHT_Y 100


#define CS_PACKET_LOGIN				1
#define CS_PACKET_MOVE				2
#define CS_PACKET_CITIZENCREATE		3
#define CS_PACKET_CITIZENMOVE		4
#define CS_PACKET_RESOURCECREATE	5
#define CS_PACKET_SUNANGLE			6
#define CS_PACKET_CITIZENREMOVE		7
#define CS_PACKET_MINIMAP			8

#define CS_PACKET_CITIZENPLACEMENT	10
#define CS_PACKET_GAMEOVER			11
#define CS_PACKET_GAMEEND			12
#define CS_PACKET_MATCHING			13

#define CS_PACKET_RESOURCEAMOUNT	20
#define CS_PACKET_PLAYERRESOURCE	21

#define CS_PACKET_TERRAINALL		30
#define CS_PACKET_TERRAINXLOCATION	31
#define CS_PACKET_TERRAINYLOCATION	32

#define CS_PACKET_BUILDABLE			40
#define CS_PACKET_BUILD				41

#define CS_PACKET_TEMPERATURE		50

#define CS_PACKET_ARMYTRAINING		60
#define CS_PACKET_ARMYMOVE			61
#define CS_PACKET_ARMYRETURN		62
#define CS_PACKET_ARMYDISBAND		63
#define CS_PACKET_ARMYSELECT		67

#define CS_PACKET_VIEWEVENT			70
#define CS_PACKET_REMOVEEVENT		71
#define CS_PACKET_EVENTSELECT		72

#define CS_PACKET_POLICY			80


#define CS_PACKET_TRADEREQUEST		90
#define CS_PACKET_TRADEAGREE		91
#define CS_PACKET_TRADERESOURCE		92
#define CS_PACKET_TRADEDEAL			93
#define CS_PACKET_TRADESUCCESS		94

#define CS_PACKET_TECHNOLOGY		100
#define CS_PACKET_TECHPHASE			101

#define CS_PACKET_DECLARATION_WAR	110

#define CS_PACKET_SPRINKLER_STATUS	120
#define CS_PACKET_TESTING			17

#define CS_PACKET_CALL_CHEAT_KEY	255

#define SC_PACKET_LOGIN				1
#define SC_PACKET_MOVE				2
#define SC_PACKET_CITIZENCREATE		3
#define SC_PACKET_CITIZENMOVE		4
#define SC_PACKET_RESOURCECREATE	5
#define SC_PACKET_SUNANGLE			6
#define SC_PACKET_CITIZENREMOVE		7
#define SC_PACKET_MINIMAP			8
#define SC_PACKET_SANDSTORMDAY		9


#define SC_PACKET_CITIZENPLACEMENT	10
#define SC_PACKET_GAMEOVER			11
#define SC_PACKET_GAMEEND			12
#define SC_PACKET_MATCHING			13

#define SC_PACKET_RESOURCEAMOUNT	20
#define SC_PACKET_PLAYERRESOURCE	21

#define SC_PACKET_TERRAINALL		30
#define SC_PACKET_TERRAINXLOCATION	31
#define SC_PACKET_TERRAINYLOCATION	32

#define SC_PACKET_BUILDABLE			40
#define SC_PACKET_BUILD				41
#define SC_PACKET_BUILDSUCCESS		42

#define SC_PACKET_TEMPERATURE		50
#define SC_PACKET_TEMPERATUREX		51
#define SC_PACKET_TEMPERATUREY		52

#define SC_PACKET_ARMYTRAINING		60
#define SC_PACKET_ARMYMOVE			61
#define SC_PACKET_ARMYDISBAND		63
#define SC_PACKET_ARMYCHANGEHP		64
#define SC_PACKET_ARMYDEAD			65
#define SC_PACKET_ARMYATTACK		66
#define SC_PACKET_ARMYSELECT		67


#define SC_PACKET_VIEWEVENT			70
#define SC_PACKET_REMOVEEVENT		71
#define SC_PACKET_EVENTSELECT		72

#define SC_PACKET_POLICY_ACCEPT 	80
#define SC_PACKET_POLICY_TICKET 	81

#define SC_PACKET_TRADEREQUEST		90
#define SC_PACKET_TRADEAGREE		91
#define SC_PACKET_TRADERESOURCE		92
#define SC_PACKET_TRADEDEAL			93
#define SC_PACKET_TRADESUCCESS		94


#define SC_PACKET_TECHNOLOGY		100
#define SC_PACKET_TECHPHASE			101
#define SC_PACKET_TECHTIMER			102

#define SC_PACKET_DECLARATION_WAR	110

#define SC_PACKET_SPRINKLER_OFF		120

#define SC_PACKET_CITIZEN_STATUS	130

#define SC_PACKET_WIND_DIRECTION	140

#define SC_PACKET_DISSATISFACTION	150

#pragma pack (push, 1)
struct cs_packet_login
{
	unsigned char size;
	unsigned char type;
};

struct cs_packet_move
{
	unsigned char size;
	unsigned char type;
	char w, a, s, d;
};

struct cs_packet_citizencreate
{
	unsigned char size;
	unsigned char type;
	//���� ������ citizen ���� ��û�Ҷ� ���� ����
};

struct cs_packet_citizenmove
{
	unsigned char size;
	unsigned char type;
	int citizenid;
	
	float x, y, z;
};

struct cs_packet_resourcecreate
{
	unsigned char size;	
	unsigned char type;
	
	//resource ���� ��û
};

struct cs_packet_citizenplacement
{
	unsigned char size;
	unsigned char type;
	
	int objectid;
	int army_type = 0;
	char isplus;

};

struct cs_packet_buildable
{
	unsigned char size = sizeof(cs_packet_buildable);
	unsigned char type = CS_PACKET_BUILDABLE;
	
	char building_type;
	float x, y;
};

struct cs_packet_build
{
	unsigned char size = sizeof(cs_packet_build);
	unsigned char type = CS_PACKET_BUILD;
	
	char building_type;
	float x, y;
};

struct cs_packet_minimap
{
	unsigned char size;
	unsigned char type;

	float x, y;
};

struct cs_packet_armymove
{
	unsigned char size;
	unsigned char type;

	float x, y;
	int a_id;
};

struct cs_packet_eventselect
{
	unsigned char size;
	unsigned char type;

	int s_option;
	int e_id;
};

struct cs_packet_armyreturn
{
	unsigned char size;
	unsigned char type;

	int a_id;
};

struct cs_packet_armydisband
{
	unsigned char size;
	unsigned char type;

	int a_id;
};

struct cs_packet_traderequest
{
	unsigned char size;
	unsigned char type;

	int request_player;
};

struct cs_packet_policy
{
	unsigned char size = sizeof(cs_packet_policy);
	unsigned char type = CS_PACKET_POLICY;

	char policy_id;
	// 0: 든든한 식사, 1:스프밥, 2: 밀주
	// 3: 연명치료, 4: 극약처방, 5: 과잉수용, 6: 환자추가배식, 7: 마약치료 8: 요양원, 9: 의수족
	// 10: 장례식, 11: 시체저장소, 12: 장기이식, 13: 식인, 14: 추모비
	// 15: 자경단, 16: 경비초소, 17: 교도소, 18: 계엄령, 19: 독재자
	// 20: 비상교대근무, 21: 연장근무, 22: 감찰관, 23: 강제노동
	// 24: 아동노동, 25: 보육원, 26: 학도병징집, 27: 작업지원
};


struct cs_packet_tradeagree
{
	unsigned char size = sizeof(cs_packet_tradeagree);
	unsigned char type = CS_PACKET_TRADEAGREE;

	int isagree;		// 0 : 거래거절, 타임아웃, 1 : 거래동의, 2 : 다른 작업중
	int request_player;
};

struct cs_packet_traderesource {
	unsigned char size = sizeof(cs_packet_traderesource);
	unsigned char type = CS_PACKET_TRADERESOURCE;

	int resource_num;
	int resource_amount;

};

struct cs_packet_tradedeal {
	unsigned char size = sizeof(cs_packet_tradedeal);
	unsigned char type = CS_PACKET_TRADEDEAL;

	int deal_boolean;
};

struct cs_packet_tradesuccess {
	unsigned char size = sizeof(cs_packet_tradesuccess);
	unsigned char type = CS_PACKET_TRADESUCCESS;

	int success_boolean;
};

struct cs_packet_technology
{
	unsigned char size = sizeof(cs_packet_technology);
	unsigned char type = CS_PACKET_TECHNOLOGY;

	int tech_type;
	int tech_level;
};

struct cs_packet_techphase
{
	unsigned char size = sizeof(cs_packet_techphase);
	unsigned char type = CS_PACKET_TECHPHASE;

	int tech_phase;
};

struct cs_packet_declaration_war
{
	unsigned char size = sizeof(cs_packet_declaration_war);
	unsigned char type = CS_PACKET_DECLARATION_WAR;

	int player_num;
};

struct cs_packet_sprinkler_status
{
	unsigned char size = sizeof(cs_packet_sprinkler_status);
	unsigned char type = CS_PACKET_SPRINKLER_STATUS;

	bool status;
	int sprinkler_id;
};

struct cs_packet_armyselect
{
	unsigned char size = sizeof(cs_packet_armyselect);
	unsigned char type = CS_PACKET_ARMYSELECT;
	int select_type = 0;
};

struct cs_packet_cheatkey
{
	unsigned char size = sizeof(cs_packet_cheatkey);
	unsigned char type = CS_PACKET_CALL_CHEAT_KEY;
	int cheat_type;
};

struct cs_packet_testing
{
	unsigned char size = sizeof(cs_packet_testing);
	unsigned char type = CS_PACKET_TESTING;
	int error_num;
};



//////////////////////////////////////////// 서버

struct sc_packet_login
{
	unsigned char size;
	unsigned char type;
	float x, y, z;
	float rx, ry, rz;
	float p1_x, p1_y, p1_z;
	float p2_x, p2_y, p2_z;
	float p3_x, p3_y, p3_z;
	float p4_x, p4_y, p4_z;
	int connectplayer = 0;
	int maxplayer = 0;
	char player_id;
};

struct sc_packet_move
{
	unsigned char size;
	unsigned char type;
	float currentX, currentY, currentZ;
	float currentRX, currentRY, currentRZ;
};

struct sc_packet_citizencreate
{
	unsigned char size;
	unsigned char type;
	int citizenid;
	float x, y, z;
};

struct sc_packet_citizenremove
{
	unsigned char size;
	unsigned char type;
	int citizenid;
};

struct sc_packet_citizenplacement
{
	unsigned char size;
	unsigned char type;

	int object_id;
	char playerjobless;
	char workcitizen;
};

struct sc_packet_citizenmove
{
	unsigned char size;
	unsigned char type;
	int citizenid;
	float x, y, z;
	float rx, ry, rz;
	char citizenstate;
};

struct sc_packet_resourcecreate
{
	unsigned char size;
	unsigned char type;

	int resource_id;
	float x, y, z;
	char resource_type;
	int amount;

};

struct sc_packet_resourceamount
{
	unsigned char size;
	unsigned char type;

	int resourceid;
	int amount;
};

struct sc_packet_playerresource
{
	unsigned char size;
	unsigned char type;

	int resources_acount[5];
};

struct sc_packet_terrainAll
{
	unsigned char size;
	unsigned char type;

	unsigned char terrain_X;
	char terrain_Y[SIGHT_Y];
};

struct sc_packet_terrainXlocation
{
	unsigned char size;
	unsigned char type;

	float terrainX;
	char terrainline_Y[SIGHT_Y];
};

struct sc_packet_terrainYlocation
{
	unsigned char size;
	unsigned char type;

	float terrainY;
	char terrainline_X[SIGHT_X];
};

struct sc_packet_sunangle
{
	unsigned char size;
	unsigned char type;

	float sunangle;
};

struct sc_packet_buildable
{
	unsigned char size = sizeof(sc_packet_buildable);
	unsigned char type = SC_PACKET_BUILDABLE;

	bool buildable;
};

struct sc_packet_build
{
	unsigned char size = sizeof(sc_packet_build);
	unsigned char type = SC_PACKET_BUILD;

	bool do_build;
	char building_type;
	int id;
	float x, y;
};

struct sc_packet_buildsuccess
{
	unsigned char size = sizeof(sc_packet_buildsuccess);
	unsigned char type = SC_PACKET_BUILDSUCCESS;

	int id;
	char building_type;
	float x, y;
};

struct sc_packet_temperature
{
	unsigned char size = sizeof(sc_packet_temperature);
	unsigned char type = SC_PACKET_TEMPERATURE;

	unsigned char terrain_X;
	char terrain_Y[SIGHT_Y];
};

struct sc_packet_temperatureX
{
	unsigned char size = sizeof(sc_packet_temperatureX);
	unsigned char type = SC_PACKET_TEMPERATUREX;

	float terrainX;
	char terrainline_Y[SIGHT_Y];
};

struct sc_packet_temperatureY
{
	unsigned char size = sizeof(sc_packet_temperatureY);
	unsigned char type = SC_PACKET_TEMPERATUREY;

	float terrainY;
	char terrainline_X[SIGHT_X];
};

struct  sc_packet_armytraining
{
	unsigned char size = sizeof(sc_packet_armytraining);
	unsigned char type = SC_PACKET_ARMYTRAINING;

	int c_id1, c_id2, c_id3, c_id4, c_id5;
	float x, y, z;
	int army_id;
	int  _army_type;
	int hp;
};

struct sc_packet_armydisband
{
	unsigned char size;
	unsigned char type;

	int issuccess;
	int a_id;
};

struct sc_packet_armymove
{
	unsigned char size;
	unsigned char type;
	int a_id;
	float x, y, z;
	float rx, ry, rz;
	int  a_state;
};

struct sc_packet_armychangehp
{
	unsigned char size = sizeof(sc_packet_armychangehp);
	unsigned char type = SC_PACKET_ARMYCHANGEHP;
	int army_id;
	int hp;
};
struct sc_packet_armydead
{
	unsigned char size = sizeof(sc_packet_armydead);
	unsigned char type = SC_PACKET_ARMYDEAD;
	int army_id;
};

struct sc_packet_armyattack
{
	unsigned char size = sizeof(sc_packet_armyattack);
	unsigned char type = SC_PACKET_ARMYATTACK;
	int army_id;
	float rx, ry, rz;
	int a_state;
};

struct sc_packet_viewevnet
{
	unsigned char size;
	unsigned char type;

	int e_id;
	float x, y, z;
};

struct sc_packet_removeevent
{
	unsigned char size;
	unsigned char type;

	int e_id;
};

struct sc_packet_eventselect
{
	unsigned char size;
	unsigned char type;

	int s_option;
	wchar_t summary[30]{};
	wchar_t first[20]{}, second[20]{}, third[20]{};
	int e_id;
	int e_type;
};

struct sc_packet_policy_ticket
{
	unsigned char size = sizeof(sc_packet_policy_ticket);
	unsigned char type = SC_PACKET_POLICY_TICKET;

	char ticket;
};

struct sc_packet_policy_accept
{
	unsigned char size = sizeof(sc_packet_policy_accept);
	unsigned char type = SC_PACKET_POLICY_ACCEPT;

	char policy_id;
	bool accept;
	char ticket;
};

struct sc_packet_traderequest
{
	unsigned char size;
	unsigned char type;

	int request_player;
};

struct sc_packet_tradeagree
{
	unsigned char size = sizeof(sc_packet_tradeagree);
	unsigned char type = SC_PACKET_TRADEAGREE;

	int isagree;		// 0 : 거래거절, 타임아웃, 1 : 거래동의, 2 : 다른 작업중
	int request_player;
};

struct sc_packet_traderesource {
	unsigned char size = sizeof(sc_packet_traderesource);
	unsigned char type = SC_PACKET_TRADERESOURCE;

	int resource_num;
	int resource_amount;
};

struct sc_packet_tradedeal {
	unsigned char size = sizeof(sc_packet_tradedeal);
	unsigned char type = SC_PACKET_TRADEDEAL;

	int deal_boolean;
};

struct sc_packet_tradesuccess {
	unsigned char size = sizeof(sc_packet_tradesuccess);
	unsigned char type = SC_PACKET_TRADESUCCESS;

	int success_boolean;
};

struct sc_packet_technology
{
	unsigned char size = sizeof(sc_packet_technology);
	unsigned char type = SC_PACKET_TECHNOLOGY;

	int tech_type;
	int tech_level;
};
struct sc_packet_techphase
{
	unsigned char size = sizeof(sc_packet_techphase);
	unsigned char type = SC_PACKET_TECHPHASE;

	int tech_phase;
};

struct sc_packet_techtimer
{
	unsigned char size = sizeof(sc_packet_techtimer);
	unsigned char type = SC_PACKET_TECHTIMER;

	int tech_timer_second;
};

struct sc_packet_declaration_war
{
	unsigned char size = sizeof(sc_packet_declaration_war);
	unsigned char type = SC_PACKET_DECLARATION_WAR;

	int player_num;
	int is_war;
};

struct sc_packet_sprinkler_off	//스프링클러 다 꺼버리라는 거
{
	unsigned char size = sizeof(sc_packet_sprinkler_off);
	unsigned char type = SC_PACKET_SPRINKLER_OFF;
};

struct sc_packet_sandstormday
{
	unsigned char size = sizeof(sc_packet_sandstormday);
	unsigned char type = SC_PACKET_SANDSTORMDAY;

	int sand_day = 0;
};

struct sc_packet_citizen_status
{
	unsigned char size = sizeof(sc_packet_citizen_status);
	unsigned char type = SC_PACKET_CITIZEN_STATUS;

	int soldier_num{};
	int citizen_num{};
	int citizen_hot{};
	int citizen_hungry{};
	int citizen_thirsty{};
};

struct sc_packet_gameover
{
	unsigned char size = sizeof(sc_packet_gameover);
	unsigned char type = SC_PACKET_GAMEOVER;
};

struct sc_packet_gameend
{
	unsigned char size = sizeof(sc_packet_gameend);
	unsigned char type = SC_PACKET_GAMEEND;

	wchar_t ending_title[20]{};
	int dead_citizen;
	int alive_citizen;
	int kill_citizen;
	int resource;
	int score;
	int rank;
};

struct sc_packet_wind_direction
{
	unsigned char size = sizeof(sc_packet_wind_direction);
	unsigned char type = SC_PACKET_WIND_DIRECTION;

	char wind_x;
	char wind_y;
};

struct sc_packet_dissatisfaction
{
	unsigned char size = sizeof(sc_packet_dissatisfaction);
	unsigned char type = SC_PACKET_DISSATISFACTION;

	float dissatisfaction{};
};

struct sc_packet_matching
{
	unsigned char size = sizeof(sc_packet_matching);
	unsigned char type = SC_PACKET_MATCHING;

	int connectplayer = 0;
	int maxplayer = 0;
};

#pragma pack (pop)