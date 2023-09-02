#pragma once
#include <iostream>
#include <stdio.h>
#include <random>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include "cuda.h"
#include <cuda_runtime.h>
#include <cooperative_groups.h>
#include "device_launch_parameters.h"
#include "global.h"
#include"Network.h"
#include"Player.h"
#include"Citizen.h"
#include"Resource.h"
#include "Building.h"
#include"Army.h"
#include "GameEvent.h"

#define PI 3.1415926

using namespace std;

const int min_height = 1;
const int init_max_height = 10;
const int max_height = 30;
const int base_floor = 1;
const int temperature_divide = 4;

typedef struct two_char {
	int x;
	int y;
} CC;
typedef struct two_float {
	float x;
	float y;
} FF;
typedef struct hill_info {
	int x;
	int y;
	int radius;
	int height;
} HI;
typedef struct compare_block {
	II middle;
	II up;
	II down;
	II left;
	II right;
}CB;

random_device rd;
default_random_engine dre(rd());
uniform_int_distribution <int>terrain_distance(0, one_side_number - 1);
uniform_int_distribution <int>number_of_hills_uid(one_side_number / 10, one_side_number / 10);
uniform_int_distribution <int>hill_size_uid(one_side_number / 20, one_side_number / 10);
uniform_int_distribution <int>height_uid(min_height, init_max_height);
uniform_int_distribution <int>wind_direction_uid(0, 3);

void make_random_array(II* random_array, bool& random_array_used)
{
	while (1) {
		if (random_array_used) {
			clock_t t_0 = clock();
			for (int i = 0; i < random_array_size; i++) {
				random_array[i].x = terrain_distance(dre);
				random_array[i].y = terrain_distance(dre);
			}
			random_array_used = false;
			clock_t t_1 = clock();
			//cout << "_Thread_ Random for Scarce: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec\n";
		}
		else {
			Sleep(10);
		}
	}
}

__global__
void terrain_corrosion_cuda(char** terrain_array_device, int height)
{
	//바람이 불면 그쪽으로 이동하게끔 수정해야 함
	//아니면 바람으로 인한 함수 따로 만들어서 중복으로 했을 때 어떤지 확인
	const int block_num = 9;
	II terrain[block_num];
	terrain[0].x = blockIdx.x * blockDim.x + threadIdx.x;	//middle
	terrain[0].y = blockIdx.y * blockDim.y + threadIdx.y;

	if (terrain_array_device[terrain[0].x][terrain[0].y] != height) {	//인자로 들어온 높이가 아니면 리턴
		return;
	}
	if (terrain_array_device[terrain[0].x][terrain[0].y] <= base_floor) {	//base floor 보다 작으면 더이상 낮추면 안됨
		return;
	}
	
	terrain[1] = { terrain[0].x, terrain[0].y - 1 };		//up
	terrain[2] = { terrain[0].x, terrain[0].y + 1 };		//down
	terrain[3] = { terrain[0].x - 1, terrain[0].y };		//left
	terrain[4] = { terrain[0].x + 1, terrain[0].y };		//right
	terrain[5] = { terrain[0].x - 1, terrain[0].y - 1 };	//up left
	terrain[6] = { terrain[0].x - 1, terrain[0].y + 1 };	//down left
	terrain[7] = { terrain[0].x + 1, terrain[0].y - 1 };	//up right
	terrain[8] = { terrain[0].x + 1, terrain[0].y + 1 };	//down right

	//상하좌우 연결
	for (int i = 0; i < block_num; i++) {
		if (terrain[i].x < 0) {
			terrain[i].x += one_side_number;
		}
		if (terrain[i].x >= one_side_number) {
			terrain[i].x -= one_side_number;
		}
		if (terrain[i].y < 0) {
			terrain[i].y += one_side_number;
		}
		if (terrain[i].y >= one_side_number) {
			terrain[i].y -= one_side_number;
		}
	}

	int height_difference = 0;
	for (int i = 1; i < block_num; i++) {	//주변 블럭들의 높이 차이를 구함
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] > height_difference) {
			height_difference = terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y];
		}
	}

	if (height_difference <= 0) {	//주변에 더 낮은 블럭이 없으면 리턴
		return;
	}

	int num_of_lowest{};
	for (int i = 1; i < block_num; i++) {
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {
			num_of_lowest++;
		}
	}

	if (num_of_lowest == 1) {	//가장 낮은 높이가 하나면 거기로 이동
		for (int i = 1; i < block_num; i++) {
			if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {
				terrain_array_device[terrain[i].x][terrain[i].y]++;
				terrain_array_device[terrain[0].x][terrain[0].y]--;
				return;
			}
		}
	}
	if (num_of_lowest == 8 && height_difference == 1) {	//주변이 모두 높이가 같고 높이 차이가 1이면 리턴
		return;
	}
	
	char thread_seed = (terrain[0].x + terrain[0].y + abs(terrain[0].x + terrain[0].y * height)) % num_of_lowest;	//random seed made by myself
	int iter = 0;
	for (int i = 0; i < block_num; i++) {
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {	//가장 낮은 높이가 여러개면 랜덤으로 하나 선택
			if (thread_seed == iter) {	//랜덤으로 선택된 블럭으로 이동
				terrain_array_device[terrain[i].x][terrain[i].y]++;
				terrain_array_device[terrain[0].x][terrain[0].y]--;
				return;
			}
			iter++;
		}
	}
}

__global__
void wind_blow_cuda(char** terrain_array_device, II wind_direction)
{
	//wind_direction은 x, y중 하나는 무조건 0이여야 함
	II terrain[5];
	terrain[0].x = blockIdx.y * blockDim.y + threadIdx.y;	//희안하게도 x,y맞게 하면 지형 많이 변화할시 특정 무늬가 생김. 이렇게 하면 안생김. 이렇게 바꿔도 로직에는 영향 무
	terrain[0].y = blockIdx.x * blockDim.x + threadIdx.x;	//middle

	terrain[1].x = terrain[0].x + wind_direction.x;			//forward
	terrain[1].y = terrain[0].y + wind_direction.y;

	terrain[2].x = terrain[1].x + wind_direction.y;			//forward left
	terrain[2].y = terrain[1].y - wind_direction.x;
	
	terrain[3].x = terrain[1].x - wind_direction.y;			//forward right
	terrain[3].y = terrain[1].y + wind_direction.x;

	terrain[4].x = terrain[0].x - wind_direction.x;			//back
	terrain[4].y = terrain[0].y - wind_direction.y;
	
	//지형 상하좌우 연결
	for (int i = 0; i < 5; i++) 
	{
		if (terrain[i].x < 0 ) {
			terrain[i].x += one_side_number;
		}
		if (terrain[i].x >= one_side_number) {
			terrain[i].x -= one_side_number;
		}
		if (terrain[i].y < 0) {
			terrain[i].y += one_side_number;
		}
		if (terrain[i].y >= one_side_number) {
			terrain[i].y -= one_side_number;
		}
	}

	if (terrain_array_device[terrain[0].x][terrain[0].y] <= base_floor) 
	{	//base floor 보다 작으면 더이상 낮추면 안됨
		return;
	}
	//이거때문에 간격이 좁아짐
	if (terrain_array_device[terrain[0].x][terrain[0].y] > base_floor && terrain_array_device[terrain[0].x][terrain[0].y] == terrain_array_device[terrain[4].x][terrain[4].y] && terrain_array_device[terrain[0].x][terrain[0].y] == terrain_array_device[terrain[1].x][terrain[1].y]) {	//길게 늘어나는 교착상태 제거
		
	}
	else if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[4].x][terrain[4].y] <= 0) 
	{	//후방 블럭이 현 블럭보다 같거나 높을 경우 바람의 영향을 박지 않음
		return;
	}

	int height_difference = -1;
	for (int i = 1; i < 4; i++) 
	{
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y]  > height_difference) 
		{
			height_difference = terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y];	//i블럭와 0블럭의 높이 차이
		}
	}
	if (height_difference < 0) {	//전방 3개 블럭들의 높이가 모두 현 블럭보다 높을 경우 영향 안받음
		return;
	}

	int num_of_lowest{};
	for (int i = 1; i < 4; i++) 
	{
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) 
		{	//최저 높이 동일 몇개인지
			num_of_lowest++;
		}
	}

	if (num_of_lowest == 0)
		return; 
	
	//최저높이가 하나일 경우 그리로 감
	if (num_of_lowest == 1) 
	{	
		for (int i = 1; i < 4; i++) 
		{
			if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) 
			{
				/*atomic_fetch_add(&terrain_array_device[terrain[i].x][terrain[i].y], -1);
				atomic_fetch_add(&terrain_array_device[terrain[0].x][terrain[0].y], 1);*/
				--terrain_array_device[terrain[0].x][terrain[0].y];
				++terrain_array_device[terrain[i].x][terrain[i].y];
				return;
			}
		}
	}
	
	//최저높이가 여러개 일 때, 전방 블럭이 최저 높이이면 전방으로 보냄
	if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[1].x][terrain[1].y] == height_difference) 
	{		
		/*atomicAdd(&terrain_array_device[terrain[1].x][terrain[1].y], -1);
		atomicAdd(&terrain_array_device[terrain[0].x][terrain[0].y], 1);*/
		--terrain_array_device[terrain[0].x][terrain[0].y];
		++terrain_array_device[terrain[1].x][terrain[1].y];
		return;
	}

	int radom_seed = (terrain[0].x + terrain[0].y) % num_of_lowest + 2;
	//왼 오 둘중 하나로 랜덤 이동
	/*atomicAdd(&terrain_array_device[terrain[radom_seed].x][terrain[radom_seed].y], -1);
	atomicAdd(&terrain_array_device[terrain[0].x][terrain[0].y], 1);*/
	--terrain_array_device[terrain[0].x][terrain[0].y];
	++terrain_array_device[terrain[radom_seed].x][terrain[radom_seed].y];
	return;
}

__global__
void add_scarce_cuda(char** terrain_array_device, II* random_seed_device, int size) {
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	II my_seed;
	my_seed.x = random_seed_device[id].x;
	my_seed.y = random_seed_device[id].y;

	terrain_array_device[my_seed.x][my_seed.y]++;
}

__global__
void sub_scarce_cuda(char** terrain_array_device, II* random_seed_device, int size) {
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	II my_seed;
	my_seed.x = random_seed_device[id].x;
	my_seed.y = random_seed_device[id].y;

	terrain_array_device[my_seed.x][my_seed.y]--;
}

__global__
void player_terrain_update_cuda(char** terrain_player_sight_device, HI* hill_location_device, int num_of_hills, II player_location, FF wind_direction, int wind_speed)
{
	int x = threadIdx.x;
	int y = blockIdx.x;
	int terrain_x = player_location.x + x;
	int terrain_y = player_location.y + y;

	if (terrain_x >= 0 && terrain_x <= one_side_number && terrain_y >= 0 && terrain_y <= one_side_number) {
		terrain_player_sight_device[x][y] = base_floor;
		for (int i = 0; i < num_of_hills; i++) {
			
			//원래 언덕 채우기
			int hill_location_x = hill_location_device[i].x;
			int hill_location_y = hill_location_device[i].y;
			int radius = hill_location_device[i].radius;
			int height = hill_location_device[i].height;
			int distance = sqrt(pow(terrain_y - hill_location_y, 2) + pow(terrain_x - hill_location_x, 2));

			if (distance <= radius) {
				terrain_player_sight_device[x][y] += (height) * (radius - distance) / radius;

				//언덕 깎기
				hill_location_x = hill_location_device[i].x - radius * wind_direction.x * (100 - wind_speed) / 50;
				hill_location_y = hill_location_device[i].y - radius * wind_direction.y * (100 - wind_speed) / 50;
				distance = sqrt(pow(terrain_y - hill_location_y, 2) + pow(terrain_x - hill_location_x, 2));

				if (distance <= radius) {
					terrain_player_sight_device[x][y] -= height * (radius - distance) / radius + base_floor;
					if (terrain_player_sight_device[x][y] > max_height) {
						terrain_player_sight_device[x][y] = max_height;
					}
					else if (terrain_player_sight_device[x][y] < base_floor) {
						terrain_player_sight_device[x][y] = base_floor;
					}
				}
			}
		}
	}
	else {
		//맵 밖의 경우 0
		terrain_player_sight_device[x][y] = 0;
	}
}

__global__
void except_city_terrain_cuda(char** terrain_array_device, II* city_location_device, int num_of_city)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;
	int inner_radius = (city_size + 20) / 2;
	int outer_radius = inner_radius + 30;
	II distance;

	for (int i = 0; i < num_of_city; i++) {
		if (city_location_device[i].x == 0 && city_location_device[i].y == 0) 
			continue;
		if (abs(x - city_location_device[i].x) <= inner_radius && abs(y - city_location_device[i].y) <= inner_radius) {
			terrain_array_device[x][y] = 1;
			return;
		}
		
		distance.x = abs(x - city_location_device[i].x);
		distance.y = abs(y - city_location_device[i].y);
		//printf("%d, %d\n", distance.x, distance.y);
		if (distance.x > outer_radius || distance.y > outer_radius)
			continue;
		
		/*if (distance.x == distance.y && distance.x == 0) 
			terrain_array_device[x][y] -= outer_radius;
		else*/ 
		if (distance.x >= distance.y) {
			if (terrain_array_device[x][y] > distance.x - inner_radius)
				terrain_array_device[x][y] = distance.x - inner_radius;
		}
		else {
			if (terrain_array_device[x][y] > distance.y - inner_radius)
				terrain_array_device[x][y] = distance.y - inner_radius;
		}
			
		
		if (terrain_array_device[x][y] < base_floor)
			terrain_array_device[x][y] = base_floor;
	}
}

__global__
void make_shadow_map_cuda(char** terrain_array_device, char** shadow_map_device, int sun_angle)
{
	II coo;
	coo.y = blockIdx.y * blockDim.y + threadIdx.y;
	coo.x = blockIdx.x * blockDim.x + threadIdx.x;
	
	int height = terrain_array_device[coo.x][coo.y];
	float ratio = tanf(sun_angle * PI / 180);

	if (abs(ratio) < FLT_EPSILON) {
		ratio = 0;
	}
	int distance = height / ratio * -1;

	if (distance < 0) {
		for (int i = -1; i >= distance; i--) {
			//printf("%d, %d\n", i, distance);
			if (coo.x + i < 0) {
				break;
			}
			if (terrain_array_device[coo.x + i][coo.y] >= height) {
				break;
			}
			shadow_map_device[coo.x + i][coo.y] = 0;
		}
	}
	else if (distance > 0) {
		for (int i = 1; i <= distance; i++) {
			if (coo.x + i >= one_side_number) {
				break;
			}
			if (terrain_array_device[coo.x + i][coo.y] >= height) {
				break;
			}
			shadow_map_device[coo.x + i][coo.y] = 0;
		}
	}
}

__global__
void make_temperature_map_cuda(char** terrain_array_device, char** shadow_map_device, unsigned char** temperature_map_device, int sun_angle)
{
	II coo;
	coo.x = blockIdx.y * blockDim.y + threadIdx.y;
	coo.y = blockIdx.x * blockDim.x + threadIdx.x;

	char height[3];
	height[1] = terrain_array_device[coo.x][coo.y];	//중심

	if (coo.x - 1 < 0) {
		height[0] = height[1];
	}
	else {
		height[0] = terrain_array_device[coo.x - 1][coo.y];
	}
	if (coo.x + 1 >= one_side_number) {
		height[2] = height[1];
	}
	else {
		height[2] = terrain_array_device[coo.x + 1][coo.y];
	}
	//온도 unsigned char로 해서 256까진데 0.25단위로 클라랑 차이내서 클라에서 받으면 곱하기 0.25해서 받을거임 그래서 최대 온도 64임
	int temperature{};
	//낮일 때
	if (sun_angle >= 0 && sun_angle <= 180){
		//햇빛 드는 곳
		if (shadow_map_device[coo.x][coo.y] == 1) {
			int ground_angle{};
			if (height[0] > height[2]) {
				ground_angle = (atanf(abs(height[0] - height[2])) * 180 / PI);
			}
			else if (height[0] < height[2]) {
				ground_angle = (atanf(abs(height[0] - height[2])) * 180 / PI) + 90;
			}
			else {
				ground_angle = 90;
			}
			int angle_difference = abs(ground_angle - sun_angle);	//0 ~ 180 

			//각도차 90도 넘어서 햇빛 영향 없음
			if (angle_difference >= 90) {
				temperature = -1;
			}
			//각도차 90도 이하로 햇빛 영향 있음
			else {
				temperature = (90 - angle_difference) / 10;
				//printf("%d %d\n", angle_difference, temperature);
			}
		}
		//햇빛 안드는 곳
		else {
			temperature = -1;
		}
	}
	//밤일 때
	else {
		temperature = -2;
	}
	
	temperature_map_device[coo.x][coo.y] += temperature;
	//최고 최저 온도 설정
	temperature_map_device[coo.x][coo.y] = max(temperature_map_device[coo.x][coo.y], 20 * temperature_divide);
	temperature_map_device[coo.x][coo.y] = min(temperature_map_device[coo.x][coo.y], 60 * temperature_divide);
}

__global__
void round_shaped_cool_cuda(unsigned char** temperature_map_device, II object_pos, float efficiency)
{
	II coo;
	coo.x = blockIdx.y * blockDim.y + threadIdx.y;
	coo.y = blockIdx.x * blockDim.x + threadIdx.x;

	int distance = sqrt(pow(coo.x - (int)blockDim.x / 2, 2) + pow(coo.y - (int)blockDim.y / 2, 2));
	if (distance <= blockDim.x / 2) {
		//회당 1도씩 낮춤
		temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] -= (int)(efficiency * temperature_divide);
		//최고 최저 온도 설정
		temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] = max(temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2], 20 * temperature_divide);
		temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] = min(temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2], 60 * temperature_divide);
		//printf("%d\n", temperature_map_device[springkler_pos.x + threadIdx.x - blockDim.x / 2][springkler_pos.y + threadIdx.y - blockDim.y / 2]);
	}
}

__global__
void rectangle_shaped_cool_cuda(unsigned char** temperature_map_device, II object_pos, float efficiency)
{
	//회당 1도씩 낮춤
	temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] -= (int)(efficiency * temperature_divide);
	//최고 최저 온도 설정
	temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] = max(temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2], 20 * temperature_divide);
	temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2] = min(temperature_map_device[object_pos.x + threadIdx.x - blockDim.x / 2][object_pos.y + threadIdx.y - blockDim.y / 2], 60 * temperature_divide);
	//printf("%d\n", temperature_map_device[springkler_pos.x + threadIdx.x - blockDim.x / 2][springkler_pos.y + threadIdx.y - blockDim.y / 2]);
}

__global__
void add_object_height_cuda(char** terrain_array_device, II building_pos, int building_height)
{
	terrain_array_device[building_pos.x + threadIdx.x - blockDim.x / 2][building_pos.y + threadIdx.y - blockDim.y / 2] += building_height;
	//printf("%d %d\n", building_pos.y, building_pos.x);
	//printf("%d %d %d\n", building_pos.y + threadIdx.y - blockDim.y / 2, building_pos.x + threadIdx.x - blockDim.x / 2, building_height);
}

__global__
void heat_conduction_cuda(unsigned char** temperature_map_device)
{
	II coo;
	coo.x = blockIdx.y * blockDim.y + threadIdx.y;
	coo.y = blockIdx.x * blockDim.x + threadIdx.x;

	// 0 1 2
	// 3 4 5
	// 6 7 8
	char temperature[9];
	temperature[4] = temperature_map_device[coo.x][coo.y];	//중심
	if (coo.x - 1 >= 0) {
		temperature[0] = temperature_map_device[coo.x - 1][coo.y - 1];
		temperature[3] = temperature_map_device[coo.x - 1][coo.y];
		temperature[6] = temperature_map_device[coo.x - 1][coo.y + 1];
	}
	else {
		temperature[0] = temperature[4];
		temperature[3] = temperature[4];
		temperature[6] = temperature[4];
	}
	
	if (coo.x + 1 < one_side_number){
		temperature[2] = temperature_map_device[coo.x + 1][coo.y - 1];
		temperature[5] = temperature_map_device[coo.x + 1][coo.y];
		temperature[8] = temperature_map_device[coo.x + 1][coo.y + 1];
	}
	else {
		temperature[2] = temperature[4];
		temperature[5] = temperature[4];
		temperature[8] = temperature[4];
	}
	
	if (coo.y - 1 >= 0) {
		temperature[0] = temperature_map_device[coo.x - 1][coo.y - 1];
		temperature[1] = temperature_map_device[coo.x][coo.y - 1];
		temperature[2] = temperature_map_device[coo.x + 1][coo.y - 1];
	}
	else {
		temperature[0] = temperature[4];
		temperature[1] = temperature[4];
		temperature[2] = temperature[4];
	}
	
	if (coo.y + 1 < one_side_number) {
		temperature[6] = temperature_map_device[coo.x - 1][coo.y + 1];
		temperature[7] = temperature_map_device[coo.x][coo.y + 1];
		temperature[8] = temperature_map_device[coo.x + 1][coo.y + 1];
	}
	else {
		temperature[6] = temperature[4];
		temperature[7] = temperature[4];
		temperature[8] = temperature[4];
	}

	int sum{};
	for (int i = 0; i < 9; i++) {
		sum += temperature[i];
	}
	int average = sum / 9;
	temperature_map_device[coo.x][coo.y] = average;
	//9개 평균내면 온도가 안올라감
	//printf("%d\n", sum);

	//temperature_map_device[coo.x][coo.y] = sum / 9;
}

__global__
void make_big_hill_cuda(char** terrain_array_device, II hill_pos, int hill_size)
{
	II coo;
	coo.x = blockIdx.y * blockDim.y + threadIdx.y;
	coo.y = blockIdx.x * blockDim.x + threadIdx.x;

	//if (coo.x < hill_pos.x - hill_size || coo.x > hill_pos.x + hill_size || coo.y < hill_pos.y - hill_size || coo.y > hill_pos.y + hill_size)
	//	return;

	int distance = sqrt(pow(coo.y - hill_pos.y, 2) + pow(coo.x - hill_pos.x, 2));
	if (distance <= hill_size / 2) {
		terrain_array_device[coo.x][coo.y] += hill_size * (hill_size / 2 - distance) / hill_size / 2;
		if (terrain_array_device[coo.x][coo.y] > max_height) {
			terrain_array_device[coo.x][coo.y] = max_height;
		}
		else if (terrain_array_device[coo.x][coo.y] < base_floor) {
			terrain_array_device[coo.x][coo.y] = base_floor;
		}
	}
}

class Terrain
{
private:
	char** terrain_array_host = new char * [one_side_number];
	char** terrain_array_device;
	char* terrain_array_temp[one_side_number];

	char** shadow_map_host = new char* [one_side_number];
	char** shadow_map_device;
	char* shadow_map_temp[one_side_number];
	
	unsigned char** temperature_map_host = new unsigned char* [one_side_number];
	unsigned char** temperature_map_device;
	unsigned char* temperature_map_temp[one_side_number];
	
	char** terrain_player_sight_host = new char* [SIGHT_X];
	char** temperature_player_sight = new char* [SIGHT_X];

	unsigned __int64 init_total_hill_height = (unsigned __int64)one_side_number * (unsigned __int64)one_side_number * (unsigned __int64)(init_max_height + min_height) / 2;
	
	II city_location[5];	//나중에 크기 MAXPLAYER로 수정해야 함
	II* city_location_device;
	II* building_location_device;

	II* random_array = new II[random_array_size];
	II* random_array_device;
	bool random_array_used = true;
	
	bool log = false;
	bool can_make_big_hill = false;

	II wind_direction{};
	int wind_speed = 2;
	
public:
	Terrain()  
	{
		cout << "Generating Terrain Start" << endl;
		
		//랜덤배열 제작 쓰레드 가동===================================================
		thread t1 = thread(make_random_array, random_array, ref(random_array_used));
		t1.detach();

		//Terrain Memory Assignement===================================================
		clock_t t_1 = clock();
		for (int i = 0; i < one_side_number; i++) {
			terrain_array_host[i] = new char[one_side_number];
			shadow_map_host[i] = new char[one_side_number];
			temperature_map_host[i] = new unsigned char[one_side_number];
		}
		load_terrain();
		
		cudaMalloc((void**)&terrain_array_device, one_side_number * sizeof(char*));
		cudaMalloc((void**)&shadow_map_device, one_side_number * sizeof(char*));
		cudaMalloc((void**)&temperature_map_device, one_side_number * sizeof(char*));
		for (int i = 0; i < one_side_number; i++) {
			cudaMalloc((void**)&terrain_array_temp[i], one_side_number * sizeof(char));
			cudaMalloc((void**)&shadow_map_temp[i], one_side_number * sizeof(char));
			cudaMalloc((void**)&temperature_map_temp[i], one_side_number * sizeof(char));
		}
		cudaMemcpy(terrain_array_device, terrain_array_temp, one_side_number * sizeof(char*), cudaMemcpyHostToDevice);
		cudaMemcpy(shadow_map_device, shadow_map_temp, one_side_number * sizeof(char*), cudaMemcpyHostToDevice);
		cudaMemcpy(temperature_map_device, temperature_map_temp, one_side_number * sizeof(char*), cudaMemcpyHostToDevice);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_temp[i], terrain_array_host[i], one_side_number * sizeof(char), cudaMemcpyHostToDevice);
			cudaMemcpy(shadow_map_temp[i], shadow_map_host[i], one_side_number * sizeof(char), cudaMemcpyHostToDevice);
			cudaMemcpy(temperature_map_temp[i], temperature_map_host[i], one_side_number * sizeof(char), cudaMemcpyHostToDevice);
		}

		cudaMalloc((void**)&city_location_device, 5 * sizeof(II));
		cudaMalloc((void**)&building_location_device, MAXBUILDING * sizeof(II));

		//Terrain Memory Assignment For Player's Sight===================================================
		clock_t t_2 = clock();
		for (int i = 0; i < SIGHT_X; i++) {
			terrain_player_sight_host[i] = new char[SIGHT_Y];
			temperature_player_sight[i] = new char[SIGHT_Y];
		}
		for (int i = 0; i < SIGHT_X; i++) {
			for (int j = 0; j < SIGHT_Y; j++) {
				terrain_player_sight_host[i][j] = 0;
				temperature_player_sight[i][j] = 0;
			}
		}

		//Make Hills===================================================
		clock_t t_3 = clock();
		
		cout << "Terrain size : " << one_side_number << " * " << one_side_number << endl;
		cout << "Terrain Array Size : " << one_side_number * one_side_number * sizeof(char) << " Bytes" << endl;
		cout << "Num of Total Blocks: " << init_total_hill_height << endl;
		cout << "Terrain Memory Assignement : " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Terrain Memory Assignment For Player's Sight : " << (double)(t_3 - t_2) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Terrain Completely Generated !" << endl;
		cout << endl;
	}

	~Terrain()
	{
		for (int i = 0; i < one_side_number; i++) {
			delete[] terrain_array_host[i];
			delete[] shadow_map_host[i];
			delete[] temperature_map_host[i];
		}
		for (int i = 0; i < SIGHT_X; i++) {
			delete[] terrain_player_sight_host[i];
		}
		delete[] terrain_array_host;
		delete[] shadow_map_host;
		delete[] temperature_map_host;
		delete[] terrain_player_sight_host;
		
		cudaFree(terrain_array_temp);
		cudaFree(shadow_map_temp);
		cudaFree(temperature_map_temp);
		
		cudaFree(terrain_array_device);
		cudaFree(shadow_map_device);
		cudaFree(temperature_map_device);
	}
	
	CC get_highest_lowest(char** array_host)
	{
		clock_t t_0 = clock();
		char highest = array_host[0][0];
		char lowest = array_host[0][0];
		for (int i = 0; i < one_side_number; i++) {
			for (int j = 0; j < one_side_number; j++) {
				if (array_host[i][j] > highest) {
					highest = array_host[i][j];
				}
				else if (array_host[i][j] < lowest) {
					lowest = array_host[i][j];
				}
			}
		}
		CC value{ highest, lowest };
		clock_t t_1 = clock();
		cout << "Get Highest : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		return value;
	}
	
	CC get_highest_lowest(unsigned char** array_host)
	{
		clock_t t_0 = clock();
		unsigned char highest = array_host[0][0];
		unsigned char lowest = array_host[0][0];
		for (int i = 0; i < one_side_number; i++) {
			for (int j = 0; j < one_side_number; j++) {
				if (array_host[i][j] > highest) {
					highest = array_host[i][j];
				}
				else if (array_host[i][j] < lowest) {
					lowest = array_host[i][j];
				}
			}
		}
		CC value{ highest, lowest };
		clock_t t_1 = clock();
		if(log)
		{
			cout << "Get Highest : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		}
		return value;
	}

	void make_shadow_map(int sun_angle) {
		clock_t t_0 = clock();
		
		if (sun_angle <= 0 || sun_angle >= 180) {
			if (log)
			{
				cout << "Shadow Sun Angle is not valid: " << sun_angle << endl;
			}
			return;
		}
		
		for (int i = 0; i < one_side_number; i++) {
			cudaMemset(shadow_map_temp[i], 1, one_side_number * sizeof(char));
		}
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		make_shadow_map_cuda << <grid, block >> > (terrain_array_device, shadow_map_device, sun_angle);
		cudaDeviceSynchronize();
		/*for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(shadow_map_host[i], shadow_map_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}*/
		
		clock_t t_1 = clock();
		if (log) {
			cout << "Sun Angle: " << sun_angle << endl;
			cout << "Make Shadow Map : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		}
	}

	void make_tempertature_map(int sun_angle)
	{
		clock_t t_0 = clock();
			
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		make_temperature_map_cuda << <grid, block >> > (terrain_array_device, shadow_map_device, temperature_map_device, sun_angle);
		cudaDeviceSynchronize();
		/*for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(temperature_map_host[i], temperature_map_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}*/
		
		clock_t t_1 = clock();
		if(log)
			cout << "Make Temperature Map : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}

	void cooling_system()
	{
		for (int i = 0; i < MAXPLAYER; i++) {	//타워 원모양으로 쿨링
			int size = 21;	//사이즈 홀수로 해야 함
			dim3 block(size, size, 1);
			round_shaped_cool_cuda << <1, block >> > (temperature_map_device, { (int)objects[i]->_x / 100, (int)objects[i]->_y / 100 }, 0.5f);
			cudaDeviceSynchronize();
		}
		for (int i = BUILDINGSTART; i < BUILDINGSTART + MAXBUILDING; ++i) {		
			Building* building = reinterpret_cast<Building*>(objects[i]);
			
			if (building->_type == 8) {							//스프링클러 cooling
				if (building->activated == false) continue;	//작동중일 때만
				int springkler_size = 21;	//사이즈 홀수로 해야 함
				dim3 block(springkler_size, springkler_size, 1);
				round_shaped_cool_cuda << <1, block >> > (temperature_map_device, { (int)building->_x / 100, (int)building->_y / 100 }, 1.f);
			}
			else {												//건물 단열화
				Player* player = reinterpret_cast<Player*>(objects[building->_client_id]);
				dim3 block(building->size_x, building->size_y, 1);
				rectangle_shaped_cool_cuda << <1, block >> > (temperature_map_device, { (int)building->_x / 100, (int)building->_y / 100 }, player->_building_insulation);
			}
			cudaDeviceSynchronize();
		}
		
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(temperature_map_host[i], temperature_map_temp[i], one_side_number * sizeof(unsigned char), cudaMemcpyDeviceToHost);
		}
	}

	void terrain_corrosion()
	{
		clock_t t_0 = clock();
		CC hi_low = get_highest_lowest(terrain_array_host);
		cout << hi_low.x << " " << hi_low.y << endl;
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		for (int height = hi_low.x; height > hi_low.y; height--) {
			terrain_corrosion_cuda << <grid, block >> > (terrain_array_device, height);
		}
		
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}
		
		clock_t t_1 = clock();
		if (log)
			cout << "Terrain Flatten : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}

	void make_big_hill()
	{
		if (!can_make_big_hill)
			return;
		
		clock_t t_0 = clock();
		uniform_int_distribution <int>random_pos(0, one_side_number);
		uniform_int_distribution <int>random_size(100, 200);
		
		II hill_pos = { random_pos(dre), random_pos(dre) };
		int hill_size = random_size(dre);
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		make_big_hill_cuda <<<grid, block >>> (terrain_array_device, hill_pos, hill_size);
		cudaDeviceSynchronize();
		/*for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}*/
		clock_t t_1 = clock();
		if (log) {
			cout << "Big Hill Location: " << hill_pos.x << " " << hill_pos.y << " Size: " << hill_size << endl;
			cout << "Make Big Hills : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		}
	}

	void adjust_num_blocks()
	{
		unsigned __int64 curr_total = add_all();
		__int64 scarce_blocks = init_total_hill_height - curr_total;
		
		if (scarce_blocks <= 0) {	//부족하거나 많은게 없다면
			if (log)
				cout << "No scarce_blocks" << endl;
			can_make_big_hill = false;
			return;
		}
		
		if (log)
			cout<< "Scarce_blocks: " << scarce_blocks << endl;

		clock_t t_0 = clock();

		while (random_array_used) {
			//cout << "Waiting for Thread\n";
			Sleep(10);
		}
		
		clock_t t_1 = clock();

		cudaMalloc((void**)&random_array_device, random_array_size * sizeof(II));
		cudaMemcpy(random_array_device, random_array, random_array_size * sizeof(II), cudaMemcpyHostToDevice);

		int grid, block;
		if (abs(scarce_blocks) <= 1024) {	//절댓값이 1024보다 작다면
			grid = 1;
			block = abs(scarce_blocks);
		}
		else {									//절댓값이 1024보다 크다면
			grid = abs(scarce_blocks) / 1024;
			block = 1024;
		}

		if (scarce_blocks > random_array_size) {
			cout << "FATAL ERROR: scarce_blocks is bigger than random_array_size !!!\n";
			scarce_blocks = random_array_size;
		}

		if (scarce_blocks > 0)
			add_scarce_cuda << <grid, block >> > (terrain_array_device, random_array_device, scarce_blocks);
		else
			sub_scarce_cuda << <grid, block >> > (terrain_array_device, random_array_device, scarce_blocks);

		cudaDeviceSynchronize();
		
		random_array_used = true;
		can_make_big_hill = true;
		cudaFree(random_array_device);
		clock_t t_2 = clock();

		if (log) {
			cout << "Waiting Time for Random Thread: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
			cout << "Add Scarce Cuda: " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
		}
	}

	void wind_blow()
	{
		clock_t t_0, t_1, t_2, t_3, t_4, t_5;
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		
		t_0 = clock();
		
		for (int i = 0; i < wind_speed; i++) {
			//cout << "__________________________" << endl;
			t_1 = clock();

			adjust_num_blocks();
			make_big_hill();

			t_2 = clock();
			wind_blow_cuda << <grid, block >> > (terrain_array_device, wind_direction);
			cudaDeviceSynchronize();
			/*for (int i = 0; i < one_side_number; i++) {
				cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
			}*/
			
			t_3 = clock();
			if (log)
				cout << "Wind Blow Cuda: " << (double)(t_3 - t_2) / CLOCKS_PER_SEC << " sec" << endl;
			
			except_city_terrain();

			t_4 = clock();
			if (log)
				cout << "=> Once Wind Blow: " << (double)(t_4 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
		}
		t_5 = clock();
		if (log)
			cout << "Total Wind Blow: " << (double)(t_5 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}
	
	unsigned __int64 add_all()
	{
		clock_t t_0 = clock();
		unsigned __int64 all = 0;
		for (int i = 0; i < one_side_number; i++) {
			for (int j = 0; j < one_side_number; j++) {
				all += terrain_array_host[i][j];
			}
		}
		clock_t t_1 = clock();
		if (log)
			cout << "Terrain Add All : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		return all;
	}
	
	void add_object_height()
	{
		for (int i = 0; i < MAXPLAYER; ++i) {
			Player* player = reinterpret_cast<Player*>(objects[i]);
			int well_pump_height = 14;
			int well_pump_size = 6;
			dim3 block(well_pump_size, well_pump_size, 1);
			add_object_height_cuda << <1, block >> > (terrain_array_device, { (int)player->_x / 100, (int)player->_y / 100 }, well_pump_height);
			cudaDeviceSynchronize();
		}
		for (int i = RESOURCESTART; i < RESOURCESTART + MAXRESOURCE; ++i) {
			Resource* resource = reinterpret_cast<Resource*>(objects[i]);
			if (resource->_type == -1)
				continue;
			int resource_size;
			int resource_height;
			switch (resource->_type)
			{
			case 0: {	//석유
				resource_size = 3;
				resource_height = 2;
				break;
			}
			case 1: {	//물
				resource_size = 3;
				resource_height = 2;
				break;
			}
			case 2: {	//철
				resource_size = 5;
				resource_height = 1;
				break;
			}
			case 3: {	//식량
				resource_size = 3;
				resource_height = 2;
				break;
			}
			case 4: {	//나무
				resource_size = 3;
				resource_height = 2;
				break;
			}
			}
			dim3 block(resource_size, resource_size, 1);
			add_object_height_cuda << <1, block >> > (terrain_array_device, { (int)resource->_x / 100, (int)resource->_y / 100 }, resource_height);
			cudaDeviceSynchronize();
		}
		for (int i = BUILDINGSTART; i < BUILDINGSTART + MAXBUILDING; ++i) {
			Building* building = reinterpret_cast<Building*>(objects[i]);
			if (building->_type == -1) 
				continue;
			dim3 block(building->size_x, building->size_y, 1);
			add_object_height_cuda << <1, block >> > (terrain_array_device, { (int)building->_x / 100, (int)building->_y / 100 }, building->height);
			cudaDeviceSynchronize();
		}
	}
	
	void citizen_hot()
	{
		//모든 citizen 들의 위치를 가지로 해당 위치의 terrain의 온도를 확인하고 citizen의 더위를 조정(40도를 넘으면 5도마다 더위를 증가)
		for (int i = CITIZENSTART; i < CITIZENSTART + MAXCITIZEN; ++i) {
			Citizen* citizen = reinterpret_cast<Citizen*>(objects[i]);
			if (citizen->_job == -1 || citizen->_job == 22) continue;
			int temperature = temperature_map_host[(int)citizen->_x / 100][(int)citizen->_y / 100] / temperature_divide;
			
			int player_id = (i - CITIZENSTART) / 200;
			Player* player = reinterpret_cast<Player*>(objects[player_id]);
			
			citizen->_temperature = temperature;
			if (temperature > 45) {
				citizen->modify_hp(-(int)((temperature - 40) / 2));
				player->modify_dissatisfaction(0.002);
			}
			else if (temperature < 35) {
				player->modify_dissatisfaction(-0.001);
			}
			//if(player_id == 0)
				//cout << "Citizen " << i << " is hot. Temperature : " << temperature << endl;
		}
		//cout << "==================================\n";
	}

	void except_city_terrain()
	{
		/*for (int i = 0; i < 5; i++) {
			cout << city_location[i].x << " " << city_location[i].y << endl;
		}*/
		if (city_location[0].x == 0 && city_location[0].y == 0) {
			for (int i = 0; i < one_side_number; i++) {
				cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
			}
			if (log) {
				cout << "No exeption\n";
			}
			return;
		}
		clock_t t_0 = clock();
		cudaMemcpy(city_location_device, city_location, 5 * sizeof(II), cudaMemcpyHostToDevice);
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		except_city_terrain_cuda << <grid, block >> > (terrain_array_device, city_location_device, 5);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}
		clock_t t_1 = clock();
		if (log)
			cout << "Except City Terrain: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}
	
	void show_array(char** terrain_array_host, int size)
	{
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				if (terrain_array_host[x][y] > 9) {
					printf("%d_", terrain_array_host[x][y] / 10);
				}
				else if (terrain_array_host[x][y] < 0) {
					printf("%d-", abs(terrain_array_host[x][y]) / 10);
				}
				else
					//printf("%4d", terrain_array_host[x][y]);
					printf("%d ", terrain_array_host[x][y]);
			}
			printf("\n");
		}
	}

	void copy_for_player_map(II player_location)
	{
		clock_t start_t, end_t;
		start_t = clock();
		for (int i = 0; i < SIGHT_X; i++) {
			for (int j = 0; j < SIGHT_Y; j++) {
				if (player_location.x + i < 0 || player_location.x  + i >= one_side_number || player_location.y + j < 0 || player_location.y + j >= one_side_number) {
					terrain_player_sight_host[i][j] = 0;
					temperature_player_sight[i][j] = 0;
				}
				else {
					terrain_player_sight_host[i][j] = terrain_array_host[player_location.x + i][player_location.y  + j];
					temperature_player_sight[i][j] = temperature_map_host[player_location.x  + i][player_location.y + j];
				}
			}
		}
		end_t = clock();
		//if (log)
			//cout << "copy_for_player_map : " << double(end_t - start_t) / CLOCKS_PER_SEC << endl;
	}
	
	char** copy_for_player_map_line(int player_x, int player_y)
	{
		char* terrain_line_x = new char[SIGHT_X + 2];
		char* terrain_line_x2 = new char[SIGHT_X + 2];
		char* terrain_line_y = new char[SIGHT_Y + 2];
		char* terrain_line_y2 = new char[SIGHT_Y + 2];
		char* returnvalue[4] = {};
		for (int x = -1; x < SIGHT_X + 1; x++) {
			terrain_line_x[x + 1] = terrain_array_host[player_x + x][player_y];
			terrain_line_x2[x + 1] = terrain_array_host[player_x + x][player_y + SIGHT_Y];
		}

		memcpy(terrain_line_y, &terrain_array_host[player_x][player_y - 1], SIGHT_Y + 2);
		memcpy(terrain_line_y2, &terrain_array_host[player_x + SIGHT_X][player_y - 1], SIGHT_Y + 2);

		returnvalue[0] = terrain_line_x;
		returnvalue[1] = terrain_line_x2;
		returnvalue[2] = terrain_line_y;
		returnvalue[3] = terrain_line_y2;
		return returnvalue;
	}

	char** copy_for_player_temperature_line(int player_x, int player_y)
	{
		char* terrain_line_x = new char[SIGHT_X + 2];
		char* terrain_line_x2 = new char[SIGHT_X + 2];
		char* terrain_line_y = new char[SIGHT_Y + 2];
		char* terrain_line_y2 = new char[SIGHT_Y + 2];
		char* returnvalue[4] = {};
		for (int x = -1; x < SIGHT_X + 1; x++) {
			terrain_line_x[x + 1] = temperature_map_host[player_x + x][player_y];
			terrain_line_x2[x + 1] = temperature_map_host[player_x + x][player_y + SIGHT_Y];
		}

		memcpy(terrain_line_y, &temperature_map_host[player_x][player_y - 1], SIGHT_Y + 2);
		memcpy(terrain_line_y2, &temperature_map_host[player_x + SIGHT_X][player_y - 1], SIGHT_Y + 2);

		returnvalue[0] = terrain_line_x;
		returnvalue[1] = terrain_line_x2;
		returnvalue[2] = terrain_line_y;
		returnvalue[3] = terrain_line_y2;
		return returnvalue;
	}
		
	char** get_map() 
	{
		return terrain_array_host;
	}

	char** get_shadow_map() 
	{
		return shadow_map_host;
	}
	
	unsigned char** get_temperature_map()
	{
		return temperature_map_host;
	}

	char** get_player_sight_map() {
		return terrain_player_sight_host;
	}
	
	char** get_player_temperature_map()
	{
		return temperature_player_sight;
	}

	void set_city_location(TF location, int iter) {
		if (iter > 4) {
			cout << "Error: set_city_location, Bigger than array size" << endl;
		}
		city_location[iter].x = location.x / 100;
		city_location[iter].y = location.y / 100;
		cout << "city_location[" << iter << "] = " << city_location[iter].x << " " << city_location[iter].y << endl;
	}

	void log_on()
	{
		log = true;
	}

	void save_terrain()
	{
		clock_t t_0 = clock();
		
		vector<string> filenames;
		const int ROWS = one_side_number;
		const int COLS = one_side_number;
		const int CHUNK_SIZE = 1024;
		// write chunks of the 2D array to separate binary files
		for (int i = 0; i < ROWS; i += CHUNK_SIZE) {
			for (int j = 0; j < COLS; j += CHUNK_SIZE) {
				// create a filename for the chunk
				string filename = "TerrainFile/data_" + to_string(i) + "_" + to_string(j) + ".bin";
				filenames.push_back(filename);

				// create a binary file for the chunk
				ofstream file(filename, ios::out | ios::binary);

				// write the chunk to the file
				for (int k = i; k < min(i + CHUNK_SIZE, ROWS); k++) {
					file.write(reinterpret_cast<char*>(terrain_array_host[k]) + j, (min(CHUNK_SIZE, COLS - j)));
				}
			}
		}
		clock_t t_1 = clock();
		cout << "Save File: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;

	}
	
	void load_terrain()
	{
		clock_t t_0 = clock();
		ifstream fin;
		fin.open("TerrainFile/data_0_0.bin");
		if (fin) {			
			fin.close();

			const int ROWS = one_side_number;
			const int COLS = one_side_number;
			const int CHUNK_SIZE = 1024;
			for (int i = 0; i < ROWS; i += CHUNK_SIZE) {
				for (int j = 0; j < COLS; j += CHUNK_SIZE) {
					// find the filename for the chunk
					string filename = "TerrainFile/data_" + to_string(i) + "_" + to_string(j) + ".bin";

					// open the binary file for the chunk
					ifstream file(filename, ios::in | ios::binary);

					// read the chunk from the file into the array
					for (int k = i; k < min(i + CHUNK_SIZE, ROWS); k++) {
						file.read(reinterpret_cast<char*>(terrain_array_host[k]) + j, (min(CHUNK_SIZE, COLS - j)));
					}
				}
			}

			for (int i = 0; i < one_side_number; i++) {
				for (int j = 0; j < one_side_number; j++) {
					shadow_map_host[i][j] = 0;
					temperature_map_host[i][j] = 30 * temperature_divide;
				}
			}
			cout << "Terrain Loaded From File" << endl;
		}
		else {
			for (int i = 0; i < one_side_number; i++) {
				for (int j = 0; j < one_side_number; j++) {
					terrain_array_host[i][j] = height_uid(dre);			//언덕 생성 안하고 랜덤으로 생성
					shadow_map_host[i][j] = 0;
					temperature_map_host[i][j] = 20 * temperature_divide;
				}
			}
			cout << "Terrain File Not Exist" << endl;
		}
		clock_t t_1 = clock();
		cout << "Load File: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}
	
	

	void get_device_info()
	{
		cudaDeviceProp  prop;

		int count;
		cudaGetDeviceCount(&count);

		for (int i = 0; i < count; i++) {
			cudaGetDeviceProperties(&prop, i);
			printf("   --- General Information for device %d ---\n", i);
			printf("Name:  %s\n", prop.name);
			printf("Compute capability:  %d.%d\n", prop.major, prop.minor);
			printf("Clock rate:  %d\n", prop.clockRate);
			printf("Device copy overlap:  ");
			if (prop.deviceOverlap)
				printf("Enabled\n");
			else
				printf("Disabled\n");
			printf("Kernel execution timeout :  ");
			if (prop.kernelExecTimeoutEnabled)
				printf("Enabled\n");
			else
				printf("Disabled\n");
			printf("\n");

			printf("   --- Memory Information for device %d ---\n", i);
			printf("Total global mem:  %ld\n", prop.totalGlobalMem);
			printf("Total constant Mem:  %ld\n", prop.totalConstMem);
			printf("Max mem pitch:  %ld\n", prop.memPitch);
			printf("Texture Alignment:  %ld\n", prop.textureAlignment);
			printf("\n");

			printf("   --- MP Information for device %d ---\n", i);
			printf("Multiprocessor count:  %d\n", prop.multiProcessorCount);
			printf("Shared mem per mp:  %ld\n", prop.sharedMemPerBlock);
			printf("Registers per mp:  %d\n", prop.regsPerBlock);
			printf("Threads in warp:  %d\n", prop.warpSize);
			printf("Max threads per block:  %d\n", prop.maxThreadsPerBlock);
			printf("Max thread dimensions:  (%d, %d, %d)\n", prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
			printf("Max grid dimensions:  (%d, %d, %d)\n", prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
			printf("\n");
		}
	}
	
	template <typename T>
	int delete_array(T* array, int i, int size)
	{
		for (int j = i; j < size; j++) {
			array[j] = array[j + 1];
		}
		size -= 1;
		return size;
	}

	int make_hill_location(HI* hill_location_host)
	{
		int num_of_hills = number_of_hills_uid(dre);
		cout << "expected num of hills: " << num_of_hills << endl;

		for (int i = 0; i < num_of_hills; i++) {
			hill_location_host[i].x = terrain_distance(dre);
			hill_location_host[i].y = terrain_distance(dre);
			hill_location_host[i].radius = hill_size_uid(dre);
			hill_location_host[i].height = height_uid(dre);
		}
		//sort(&hill_location_host[0], &hill_location_host[num_of_hills], [](const HI& a, const HI& b) { return a.y < b.y; });
		for (int a = 0; a < num_of_hills; a++) {
			for (int b = 0; b < num_of_hills; b++) {
				if (a != b) {
					if (pow(hill_location_host[a].x - hill_location_host[b].x, 2) - pow(hill_location_host[a].radius + hill_location_host[b].radius, 2) < 0) {
						if (pow(hill_location_host[a].y - hill_location_host[b].y, 2) - pow(hill_location_host[a].radius + hill_location_host[b].radius, 2) < 0) {
							num_of_hills = delete_array(hill_location_host, b, num_of_hills);
							b--;
						}
					}
				}
			}
		}
		/*for (int i = 0; i < num_of_hills; i++) {
			if (hill_location_host[i].x - hill_location_host[i].radius < 0) {
				num_of_hills = delete_array(hill_location_host, i, num_of_hills);
				--i;
				continue;
			}
			else if (hill_location_host[i].x + hill_location_host[i].radius >= one_side_number) {
				num_of_hills = delete_array (hill_location_host, i, num_of_hills);
				--i;
				continue;
			}
			else if (hill_location_host[i].y - hill_location_host[i].radius < 0) {
				num_of_hills = delete_array(hill_location_host, i, num_of_hills);
				--i;
				continue;
			}
			else if (hill_location_host[i].y + hill_location_host[i].radius >= one_side_number) {
				num_of_hills = delete_array(hill_location_host, i, num_of_hills);
				--i;
				continue;
			}
		}*/
		cout << "real num of hills: " << num_of_hills << endl;
		return num_of_hills;
	}

	void make_new_hills(HI* hill_location_host, int& num_of_hills, int origin_num_of_hills, FF wind_direction, int wind_speed)
	{
		cout << "Wind info: " << wind_direction.x << " " << wind_direction.y << endl;
		if (wind_speed) {
			hill_location_host[num_of_hills].radius = hill_size_uid(dre);
			hill_location_host[num_of_hills].height = height_uid(dre);

			hill_location_host[num_of_hills].x = terrain_distance(dre);
			hill_location_host[num_of_hills].y = terrain_distance(dre);

			while (1) {
				hill_location_host[num_of_hills].x -= wind_direction.x * wind_speed;
				hill_location_host[num_of_hills].y -= wind_direction.y * wind_speed;
				if (hill_location_host[num_of_hills].x - hill_location_host[num_of_hills].radius > one_side_number) {
					break;
				}
				if (hill_location_host[num_of_hills].x + hill_location_host[num_of_hills].radius < 0) {
					break;
				}
				if (hill_location_host[num_of_hills].y - hill_location_host[num_of_hills].radius > one_side_number) {
					break;
				}
				if (hill_location_host[num_of_hills].y + hill_location_host[num_of_hills].radius < 0) {
					break;
				}
			}

			int collide_iter{};
			for (int a = 0; a < num_of_hills; a++) {
				if (collide_iter > 10) {	//무한루프 빠질 가능성으로 인해 횟수 제한
					return;
				}
				//cout << "대상: "<<a << " " << hill_location_host[a].x << " " << hill_location_host[a].y << " " << hill_location_host[a].radius << " " << hill_location_host[a].height << endl;
				//cout << " 나: "<< num_of_hills<< " " << hill_location_host[num_of_hills].x << " " << hill_location_host[num_of_hills ].y << " " << hill_location_host[num_of_hills ].radius << " " << hill_location_host[num_of_hills].height << endl << endl;

				if (pow(hill_location_host[a].x - hill_location_host[num_of_hills].x, 2) - pow(hill_location_host[a].radius + hill_location_host[num_of_hills].radius, 2) <= 0) {
					if (pow(hill_location_host[a].y - hill_location_host[num_of_hills].y, 2) - pow(hill_location_host[a].radius + hill_location_host[num_of_hills].radius, 2) <= 0) {

						hill_location_host[num_of_hills].radius = hill_size_uid(dre);
						hill_location_host[num_of_hills].height = height_uid(dre);

						hill_location_host[num_of_hills].x = terrain_distance(dre);
						hill_location_host[num_of_hills].y = terrain_distance(dre);

						while (1) {
							hill_location_host[num_of_hills].x -= wind_direction.x * wind_speed;
							hill_location_host[num_of_hills].y -= wind_direction.y * wind_speed;
							if (hill_location_host[num_of_hills].x - hill_location_host[num_of_hills].radius > one_side_number) {
								break;
							}
							if (hill_location_host[num_of_hills].x + hill_location_host[num_of_hills].radius < 0) {
								break;
							}
							if (hill_location_host[num_of_hills].y - hill_location_host[num_of_hills].radius > one_side_number) {
								break;
							}
							if (hill_location_host[num_of_hills].y + hill_location_host[num_of_hills].radius < 0) {
								break;
							}
						}
						cout << "충돌로 인해 바꿈" << endl;
						a = -1;
						collide_iter++;
					}
				}
			}
			cout << " 최종: " << num_of_hills << " " << hill_location_host[num_of_hills].x << " " << hill_location_host[num_of_hills].y << " " << hill_location_host[num_of_hills].radius << " " << hill_location_host[num_of_hills].height << endl << endl;
			num_of_hills++;
		}
	}

	void move_terrain(HI* hill_location_host, int& num_of_hills, FF wind_direction, int wind_speed)
	{
		if (wind_speed) {
			int wind_move_x = wind_speed * wind_direction.x;
			int wind_move_y = wind_speed * wind_direction.y;
			cout << "Wind == " << "X: " << wind_move_x << " " << "Y: " << wind_move_y << endl;
			for (int i = 0; i < num_of_hills; i++) {
				hill_location_host[i].x += wind_move_x;
				hill_location_host[i].y += wind_move_y;

				if (hill_location_host[i].x - hill_location_host[i].radius > one_side_number) {
					num_of_hills = delete_array(hill_location_host, i, num_of_hills);
					--i;
					continue;
				}
				if (hill_location_host[i].x + hill_location_host[i].radius < 0) {
					num_of_hills = delete_array(hill_location_host, i, num_of_hills);
					--i;
					continue;
				}
				if (hill_location_host[i].y - hill_location_host[i].radius > one_side_number) {
					num_of_hills = delete_array(hill_location_host, i, num_of_hills);
					--i;
					continue;
				}
				if (hill_location_host[i].y + hill_location_host[i].radius < 0) {
					num_of_hills = delete_array(hill_location_host, i, num_of_hills);
					--i;
					continue;
				}
			}
			/*for (int i = 0; i < num_of_hills; i++) {
				cout << hill_location_host[i].x << " " << hill_location_host[i].y << endl;
			}
			cout << endl;*/
		}
	}

	void wind_direction_decide()
	{
		switch (wind_direction_uid(dre)) {
		case 0:
			wind_direction.x = 0;
			wind_direction.y = 1;
			break;
		case 1:
			wind_direction.x = 0;
			wind_direction.y = -1;
			break;
		case 2:
			wind_direction.x = 1;
			wind_direction.y = 0;
			break;
		case 3:
			wind_direction.x = -1;
			wind_direction.y = 0;
			break;
		}
		cout << "Wind Direcyion Decide: " << "X: " << wind_direction.x << " " << "Y: " << wind_direction.y << endl;
		for (int i = 0; i < MAXPLAYER; i++)
		{
			Player* player = reinterpret_cast<Player*>(objects[i]);
			player->send_wind_direction(wind_direction.x, wind_direction.y);
		}
	}

	void modify_wind_speed(int additional_speed)
	{
		if (wind_speed + additional_speed > 0)
			wind_speed += additional_speed;
		cout << "Wind Speed Modified: " << wind_speed << " Additional Speed: " << additional_speed << endl;
	}
};
