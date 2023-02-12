#include <iostream>
#include <stdio.h>
#include <random>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <cuda_runtime.h>
#include <cooperative_groups.h>
#include "device_launch_parameters.h"
#include "global.h"
#define PI 3.1415926
using namespace std;

const int one_side_number = 39936;	//39936
const int player_sight_size = 64;	//1024 ������ �ȵ�
const int random_array_size = 150000000;// 150000000;

const int max_height = 8;
const int base_floor = 1;

typedef struct two_int {
	int x;
	int y;
} II;
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
uniform_int_distribution <int>height_uid(4, max_height);

uniform_int_distribution <int>wind_speed_uid(0, 50);
uniform_int_distribution <int>wind_angle_uid(0, 360);

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
			cout << "_Thread_ Random for Scarce: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec\n";
		}
		else {
			Sleep(10);
		}
	}
}

__global__
void make_hills_cuda(char** terrain_array_device, HI* hill_location_device, int num_of_hills)
{
	II terrain;
	terrain.x = blockIdx.x * blockDim.x + threadIdx.x;
	terrain.y = blockIdx.y * blockDim.y + threadIdx.y;
	HI hill;
	int distance;

	if (terrain.x % 5 == 0 && terrain.y % 5 == 0) {
		terrain_array_device[terrain.x][terrain.y] += 4;
	}


	//terrain_array_device[terrain.x][terrain.y] = base_floor;
	//for (int i = 0; i < num_of_hills; i++) {
	//	//���� ��� ä���
	//	hill.x = hill_location_device[i].x;
	//	hill.y = hill_location_device[i].y;
	//	hill.radius = hill_location_device[i].radius;
	//	hill.height = hill_location_device[i].height;
	//	distance = sqrt(pow(terrain.y - hill.y, 2) + pow(terrain.x - hill.x, 2));

	//	if (distance <= hill.radius) {
	//		terrain_array_device[terrain.x][terrain.y] += (hill.height) * (hill.radius - distance) / hill.radius;
	//		return;
	//	}
	//}
}

__global__
void terrain_corrosion_cuda(char** terrain_array_device, int height)
{
	//�ٶ��� �Ҹ� �������� �̵��ϰԲ� �����ؾ� ��
	//�ƴϸ� �ٶ����� ���� �Լ� ���� ���� �ߺ����� ���� �� ��� Ȯ��
	const int block_num = 9;
	II terrain[block_num];
	terrain[0].x = blockIdx.x * blockDim.x + threadIdx.x;	//middle
	terrain[0].y = blockIdx.y * blockDim.y + threadIdx.y;

	if (terrain_array_device[terrain[0].x][terrain[0].y] != height) {	//���ڷ� ���� ���̰� �ƴϸ� ����
		return;
	}
	if (terrain_array_device[terrain[0].x][terrain[0].y] <= base_floor) {	//base floor ���� ������ ���̻� ���߸� �ȵ�
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

	//�����¿� ����
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
	for (int i = 1; i < block_num; i++) {	//�ֺ� ������ ���� ���̸� ����
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] > height_difference) {
			height_difference = terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y];
		}
	}

	if (height_difference <= 0) {	//�ֺ��� �� ���� ���� ������ ����
		return;
	}

	int num_of_lowest{};
	for (int i = 1; i < block_num; i++) {
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {
			num_of_lowest++;
		}
	}

	if (num_of_lowest == 1) {	//���� ���� ���̰� �ϳ��� �ű�� �̵�
		for (int i = 1; i < block_num; i++) {
			if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {
				terrain_array_device[terrain[i].x][terrain[i].y]++;
				terrain_array_device[terrain[0].x][terrain[0].y]--;
				return;
			}
		}
	}
	if (num_of_lowest == 8 && height_difference == 1) {	//�ֺ��� ��� ���̰� ���� ���� ���̰� 1�̸� ����
		return;
	}
	
	char thread_seed = (terrain[0].x + terrain[0].y + abs(terrain[0].x + terrain[0].y * height)) % num_of_lowest;	//random seed made by myself
	int iter = 0;
	for (int i = 0; i < block_num; i++) {
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) {	//���� ���� ���̰� �������� �������� �ϳ� ����
			if (thread_seed == iter) {	//�������� ���õ� ������ �̵�
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
	//wind_direction�� x, y�� �ϳ��� ������ 0�̿��� ��
	II terrain[5];
	terrain[0].x = blockIdx.y * blockDim.y + threadIdx.y;	//����ϰԵ� x,y�°� �ϸ� ���� ���� ��ȭ�ҽ� Ư�� ���̰� ����. �̷��� �ϸ� �Ȼ���. �̷��� �ٲ㵵 �������� ���� ��
	terrain[0].y = blockIdx.x * blockDim.x + threadIdx.x;	//middle

	terrain[1].x = terrain[0].x + wind_direction.x;			//forward
	terrain[1].y = terrain[0].y + wind_direction.y;

	terrain[2].x = terrain[1].x + wind_direction.y;			//forward left
	terrain[2].y = terrain[1].y - wind_direction.x;
	
	terrain[3].x = terrain[1].x - wind_direction.y;			//forward right
	terrain[3].y = terrain[1].y + wind_direction.x;

	terrain[4].x = terrain[0].x - wind_direction.x;			//back
	terrain[4].y = terrain[0].y - wind_direction.y;
	
	//���� �����¿� ����
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
	{	//base floor ���� ������ ���̻� ���߸� �ȵ�
		return;
	}
	//�̰Ŷ����� ������ ������
	if (terrain_array_device[terrain[0].x][terrain[0].y] > base_floor && terrain_array_device[terrain[0].x][terrain[0].y] == terrain_array_device[terrain[4].x][terrain[4].y] && terrain_array_device[terrain[0].x][terrain[0].y] == terrain_array_device[terrain[1].x][terrain[1].y]) {	//��� �þ�� �������� ����
		
	}
	else if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[4].x][terrain[4].y] <= 0) 
	{	//�Ĺ� ���� �� ������ ���ų� ���� ��� �ٶ��� ������ ���� ����
		return;
	}

	int height_difference = -1;
	for (int i = 1; i < 4; i++) 
	{
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y]  > height_difference) 
		{
			height_difference = terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y];	//i���� 0���� ���� ����
		}
	}
	if (height_difference < 0) {	//���� 3�� ������ ���̰� ��� �� ������ ���� ��� ���� �ȹ���
		return;
	}

	int num_of_lowest{};
	for (int i = 1; i < 4; i++) 
	{
		if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) 
		{	//���� ���� ���� �����
			num_of_lowest++;
		}
	}

	if (num_of_lowest == 0)
		return; 
	
	//�������̰� �ϳ��� ��� �׸��� ��
	if (num_of_lowest == 1) 
	{	
		for (int i = 1; i < 4; i++) 
		{
			if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[i].x][terrain[i].y] == height_difference) 
			{
				--terrain_array_device[terrain[0].x][terrain[0].y];
				++terrain_array_device[terrain[i].x][terrain[i].y];
				return;
			}
		}
	}
	
	//�������̰� ������ �� ��, ���� ���� ���� �����̸� �������� ����
	if (terrain_array_device[terrain[0].x][terrain[0].y] - terrain_array_device[terrain[1].x][terrain[1].y] == height_difference) 
	{		
		--terrain_array_device[terrain[0].x][terrain[0].y];
		++terrain_array_device[terrain[1].x][terrain[1].y];
		return;
	}

	int radom_seed = (terrain[0].x + terrain[0].y) % num_of_lowest + 2;
	//�� �� ���� �ϳ��� ���� �̵�
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
void player_terrain_update_cuda(char** terrain_player_sight_device, HI* hill_location_device, int num_of_hills, II player_location, FF wind_direction, int wind_speed)
{
	int x = threadIdx.x;
	int y = blockIdx.x;
	int terrain_x = player_location.x + x;
	int terrain_y = player_location.y + y;

	if (terrain_x >= 0 && terrain_x <= one_side_number && terrain_y >= 0 && terrain_y <= one_side_number) {
		terrain_player_sight_device[x][y] = base_floor;
		for (int i = 0; i < num_of_hills; i++) {
			
			//���� ��� ä���
			int hill_location_x = hill_location_device[i].x;
			int hill_location_y = hill_location_device[i].y;
			int radius = hill_location_device[i].radius;
			int height = hill_location_device[i].height;
			int distance = sqrt(pow(terrain_y - hill_location_y, 2) + pow(terrain_x - hill_location_x, 2));

			if (distance <= radius) {
				terrain_player_sight_device[x][y] += (height) * (radius - distance) / radius;

				//��� ���
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
		//�� ���� ��� 0
		terrain_player_sight_device[x][y] = 0;
	}
}

__global__
void except_city_terrain_cuda(char** terrain_array_device, II* city_location_device, int num_of_city)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;
	int outer_radius = 40;
	int inner_radius = outer_radius - 10;
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
		
		if (distance.x == distance.y && distance.x == 0) 
			terrain_array_device[x][y] -= outer_radius;
		else if (distance.x >= distance.y) 
			terrain_array_device[x][y] += distance.x - outer_radius;
		else if (distance.x < distance.y) 
			terrain_array_device[x][y] += distance.y - outer_radius;
		
		if (terrain_array_device[x][y] < 1) 
			terrain_array_device[x][y] = 1;
	}
}

class Terrain
{
private:
	char** terrain_array_host = new char* [one_side_number];
	char** terrain_array_device;
	char* terrain_array_temp[one_side_number];

	char** terrain_player_sight_host = new char* [player_sight_size];
	unsigned __int64 init_total_hill_height = 0;
	
	II city_location[5];	//���߿� ũ�� MAXPLAYER�� �����ؾ� ��
	II* city_location_device;

	II* random_array = new II[random_array_size];
	II* random_array_device;
	bool random_array_used = true;
	
	
public:
	Terrain()  
	{
		cout << "Generating Terrain Start" << endl;
		
		//�����迭 ���� ������ ����
		thread t1 = thread(make_random_array, random_array, ref(random_array_used));
		t1.detach();
		
		//Make Random Hills Information===================================================
		//clock_t t_0 = clock();

		//HI* hill_location_host;
		//HI* hill_location_device;
		//hill_location_host = new HI[4000];
		//hill_location_device;
		//cudaMalloc((void**)&hill_location_device, 4000 * sizeof(HI));
		//int num_of_hills = make_hill_location(hill_location_host);
		//cudaMemcpy(hill_location_device, hill_location_host, num_of_hills * sizeof(HI), cudaMemcpyHostToDevice); //Memcpy to Device
		//printf("Random Hill Info Complete\n");
		//for (int i = 0; i < num_of_hills; i++) {
		//	cout << hill_location_host[i].x << ", " << hill_location_host[i].y << ", " << hill_location_host[i].height << ", " << hill_location_host[i].radius << endl;
		//}


		//Terrain Memory Assignement===================================================
		clock_t t_1 = clock();
		for (int i = 0; i < one_side_number; i++) {
			terrain_array_host[i] = new char[one_side_number];
		}
		for (int i = 0; i < one_side_number; i++) {
			for (int j = 0; j < one_side_number; j++) {
				terrain_array_host[i][j] = height_uid(dre);			//��� ���� ���ϰ� �������� ����
			}
		}
		cudaMalloc((void**)&terrain_array_device, one_side_number * sizeof(char*));
		for (int i = 0; i < one_side_number; i++) {
			cudaMalloc((void**)&terrain_array_temp[i], one_side_number * sizeof(char));
		}
		cudaMemcpy(terrain_array_device, terrain_array_temp, one_side_number * sizeof(char*), cudaMemcpyHostToDevice);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_temp[i], terrain_array_host[i], one_side_number * sizeof(char), cudaMemcpyHostToDevice);
		}


		//Terrain Memory Assignment For Player's Sight===================================================
		clock_t t_2 = clock();
		for (int i = 0; i < player_sight_size; i++) {
			terrain_player_sight_host[i] = new char[player_sight_size];
		}
		for (int i = 0; i < player_sight_size; i++) {
			for (int j = 0; j < player_sight_size; j++) {
				terrain_player_sight_host[i][j] = 0;
			}
		}


		//Make Hills===================================================
		clock_t t_3 = clock();
		/*dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		make_hills_cuda << <grid, block >> > (terrain_array_device, hill_location_device, num_of_hills);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}*/
		
		clock_t  t_4 = clock();
		init_total_hill_height = add_all();

		cout << "Terrain size : " << one_side_number << " * " << one_side_number << endl;
		cout << "Terrain Array Size : " << one_side_number * one_side_number * sizeof(char) << " Bytes" << endl;
		cout << "Num of Total Blocks: " << init_total_hill_height << endl;
		//cout << "Make Random Hills Information : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Terrain Memory Assignement : " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Terrain Memory Assignment For Player's Sight : " << (double)(t_3 - t_2) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Make Hills (GPU) : " << (double)(t_4 - t_3) / CLOCKS_PER_SEC << " sec" << endl;
		//cout << "Make Hills (CPU) : " << (double)(t_6 - t_5) / CLOCKS_PER_SEC << " sec" << endl;

		cout << "Terrain Completely Generated !" << endl;
		cout << endl;
	}

	~Terrain()
	{
		for (int i = 0; i < one_side_number; i++) {
			delete[] terrain_array_host[i];
		}
		for (int i = 0; i < player_sight_size; i++) {
			delete[] terrain_player_sight_host[i];
		}
		delete[] terrain_array_host;
		delete[] terrain_player_sight_host;
		cudaFree(terrain_array_temp);
		cudaFree(terrain_array_device);
	}
	
	void make_hills_cpu(char** terrain_array_host, HI* hill_location_host, int hill_number)
	{
		II terrain;
		HI hill;
		int distance;
		for (int i = 0; i < one_side_number; i++) {
			terrain.y = i;
			for (int j = 0; j < one_side_number; j++) {
				terrain.x = j;
				
				//printf("%d %d %d\n", terrain.x, terrain.y, i);
				//terrain_array_device[terrain.x][terrain.y] = base_floor;
				for (int k = 0; k < hill_number; k++) {
					//���� ��� ä���
					hill.x = hill_location_host[k].x;
					hill.y = hill_location_host[k].y;
					hill.radius = hill_location_host[k].radius;
					hill.height = hill_location_host[k].height;
					distance = sqrt(pow(terrain.y - hill.y, 2) + pow(terrain.x - hill.x, 2));
					//cout << distance << endl;

					if (distance <= hill.radius) {
						//cout << hill.x << " " << hill.y << endl;

						terrain_array_host[terrain.x][terrain.y] += (hill.height) * (hill.radius - distance) / hill.radius;
					}
				}
			}
		}
	}
	
	CC get_highest_lowest()
	{
		clock_t t_0 = clock();
		char highest = terrain_array_host[0][0];
		char lowest = terrain_array_host[0][0];
		for (int i = 0; i < one_side_number; i++) {
			for (int j = 0; j < one_side_number; j++) {
				if (terrain_array_host[i][j] > highest) {
					highest = terrain_array_host[i][j];
				}
				else if (terrain_array_host[i][j] < lowest) {
					lowest = terrain_array_host[i][j];
				}
			}
		}
		CC value{ highest, lowest };
		clock_t t_1 = clock();
		cout << "Get Highest : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		return value;
	}

	void terrain_corrosion()
	{
		clock_t t_0 = clock();
		CC hi_low = get_highest_lowest();
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
		cout << "Terrain Flatten : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}

	void add_scarce()
	{
		unsigned __int64 scarce_blocks = init_total_hill_height - add_all();
		if (scarce_blocks == 0) {
			return;
		}
		cout<< "scarce_blocks: " << scarce_blocks << endl;

		//==================================================================================
		clock_t t_0 = clock();

		while (random_array_used) {
			cout << "Waiting for Thread\n";
			Sleep(10);
		}
		
		clock_t t_1 = clock();

		cudaMalloc((void**)&random_array_device, random_array_size * sizeof(II));
		cudaMemcpy(random_array_device, random_array, random_array_size * sizeof(II), cudaMemcpyHostToDevice);
		
		int grid, block;
		if (scarce_blocks <= 1024) {
			grid = 1;
			block = scarce_blocks;
		}
		else {
			grid = scarce_blocks / 1024;
			block = 1024;
		}

		if (scarce_blocks > random_array_size) {
			cout << "FATAL ERROR: scarce_blocks is bigger than random_array_size !!!\n";
			return;
		}
		add_scarce_cuda << <grid, block >> > (terrain_array_device, random_array_device, scarce_blocks);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}
		random_array_used = true;
		
		//�޸� ����
		cudaFree(random_array_device);
		//==================================================================================

		//int grid, block;
		//if (scarce_blocks <= 1024) {
		//	grid = 1;
		//	block = scarce_blocks;
		//}
		//else {
		//	grid = scarce_blocks / 1024;
		//	block = 1024;
		//}
		////cout << "Grid * Block: " << grid * block << endl;
		//add_scarce_cuda << <grid, block >> > (terrain_array_device, random_seed_device, scarce_blocks);
		//for (int i = 0; i < one_side_number; i++) {
		//	cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		//}
		////�޸� ����
		//delete[] random_seed;
		//cudaFree(random_seed_device);
		
		clock_t t_2 = clock();
		
		/*scarce_blocks = init_total_hill_height - add_all();
		cout << "after_add_blocks: " << scarce_blocks << endl;*/

		cout << "Waiting Time for Random Thread: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		cout << "Add Scarce Cuda: " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
	}

	void wind_blow(II wind_direction, int wind_speed)
	{
		/*FF wind_direction = { cos(wind_angle * PI / 180), sin(wind_angle * PI / 180) };
		if (abs(wind_direction.x) < FLT_EPSILON) {
			wind_direction.x = 0;
		}
		if (abs(wind_direction.y) < FLT_EPSILON) {
			wind_direction.y = 0;
		}*/
		clock_t t_0, t_1, t_2, t_3;
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		
		t_0 = clock();

		for (int i = 0; i < wind_speed; i++) {
			cout << "__________________________" << endl;
			add_scarce();

			t_1 = clock();

			wind_blow_cuda << <grid, block >> > (terrain_array_device, wind_direction);
			for (int i = 0; i < one_side_number; i++) {
				cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
			}
			t_2 = clock();
			cout << "Only Wind Blow Cuda: " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " sec" << endl;
		}

		t_3 = clock();
		cout << "[Total Wind Blow: " << (double)(t_3 - t_0) / CLOCKS_PER_SEC << " sec]" << endl;
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
		cout << "Terrain Add All : " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
		return all;
	}
	
	void except_city_terrain()
	{
		clock_t t_0 = clock();
		city_location_device = new II[5];
		cudaMalloc((void**)&city_location_device, 5 * sizeof(II));
		cudaMemcpy(city_location_device, city_location, 5 * sizeof(II), cudaMemcpyHostToDevice);
		
		dim3 grid(one_side_number / 32, one_side_number / 32, 1);
		dim3 block(32, 32, 1);
		except_city_terrain_cuda << <grid, block >> > (terrain_array_device, city_location_device, 5);
		for (int i = 0; i < one_side_number; i++) {
			cudaMemcpy(terrain_array_host[i], terrain_array_temp[i], one_side_number * sizeof(char), cudaMemcpyDeviceToHost);
		}
		clock_t t_1 = clock();
		cout << "Except City Terrain: " << (double)(t_1 - t_0) / CLOCKS_PER_SEC << " sec" << endl;
	}
	
	void show_array(char** terrain_array_host, int size)
	{
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				if (terrain_array_host[x][y] > 9) {
					printf("H ");
				}
				else
					printf("%d ", terrain_array_host[x][y]);
			}
			printf("\n");
		}
	}

	void copy_for_player_map(II player_location)
	{
		clock_t start_t, end_t;
		start_t = clock();
		for (int i = 0; i < player_sight_size; i++) {
			for (int j = 0; j < player_sight_size; j++) {
				if (player_location.x - player_sight_size / 2 + i < 0 || player_location.x - player_sight_size / 2 + i >= one_side_number || player_location.y - player_sight_size / 2 + j < 0 || player_location.y - player_sight_size / 2 + j >= one_side_number)
					terrain_player_sight_host[i][j] = 0;
				else
					terrain_player_sight_host[i][j] = terrain_array_host[player_location.x - player_sight_size / 2 + i][player_location.y - player_sight_size / 2 + j];
			}
		}
		end_t = clock();
		cout << "copy_for_player_map : " << double(end_t - start_t) / CLOCKS_PER_SEC << endl;
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
				if (collide_iter > 10) {	//���ѷ��� ���� ���ɼ����� ���� Ƚ�� ����
					return;
				}
				//cout << "���: "<<a << " " << hill_location_host[a].x << " " << hill_location_host[a].y << " " << hill_location_host[a].radius << " " << hill_location_host[a].height << endl;
				//cout << " ��: "<< num_of_hills<< " " << hill_location_host[num_of_hills].x << " " << hill_location_host[num_of_hills ].y << " " << hill_location_host[num_of_hills ].radius << " " << hill_location_host[num_of_hills].height << endl << endl;

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
						cout << "�浹�� ���� �ٲ�" << endl;
						a = -1;
						collide_iter++;
					}
				}
			}
			cout << " ����: " << num_of_hills << " " << hill_location_host[num_of_hills].x << " " << hill_location_host[num_of_hills].y << " " << hill_location_host[num_of_hills].radius << " " << hill_location_host[num_of_hills].height << endl << endl;
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

	void wind_decide(int& wind_speed, int& wind_angle)
	{
		//wind speed 0-50
		//wind angle 0-360
		wind_speed = 50;// wind_speed_uid(dre);
		//wind_angle = 90; // wind_angle_uid(dre);
		wind_angle += 10;
		cout << wind_speed << " " << wind_angle << endl;
		//ǳ���� �������� �ѹ� ������Ʈ �� ������, ǳ���� �������� �ѹ� ������Ʈ �� ������ ȸ�Ǹ� ���� ��������
	}
	
	char** get_map() {
		return terrain_array_host;
	}

	char** get_player_sight_map() {
		return terrain_player_sight_host;
	}

	void set_city_location(TF location, int iter) {
		if (iter > 4) {
			cout << "Error: set_city_location, Bigger than array size" << endl;
		}
		city_location[iter].x = location.x;
		city_location[iter].y = location.y;
		cout << "city_location[" << iter << "] = " << city_location[iter].x << " " << city_location[iter].y << endl;
	}
};
