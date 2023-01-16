#include <iostream>
#include <stdio.h>
#include <random>
#include <windows.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <cuda_runtime.h>
#include <curand.h>
#include <curand_kernel.h>
#include <cooperative_groups.h>
#include "device_launch_parameters.h"
#define PI 3.1415926
using namespace std;

const int one_side_number = 40000;	//39936
const int player_sight_size = 1000;	//1024 ������ �ȵ�

const int max_height = 8;
const int base_floor = 1;

typedef struct two_int {
	int x;
	int y;
} II;

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

random_device rd;
default_random_engine dre(rd());
uniform_int_distribution <int>hills_location(0, one_side_number);
uniform_int_distribution <int>number_of_hills_uid(one_side_number / 10, one_side_number / 10);
uniform_int_distribution <int>hill_size_uid(one_side_number / 20, one_side_number / 10);
uniform_int_distribution <int>height_uid(5, max_height);

uniform_int_distribution <int>wind_speed_uid(0, 50);
uniform_int_distribution <int>wind_angle_uid(0, 360);


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

void show_array(char** terrain_array_host, int size)
{
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			printf("%d ", terrain_array_host[x][y]);
		}
		printf("\n");
	}
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
		hill_location_host[i].x = hills_location(dre);
		hill_location_host[i].y = hills_location(dre);
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
	//for (int i = 0; i < num_of_hills; i++) {
	//	if (hill_location_host[i].x - hill_location_host[i].radius < 0) {
	//		num_of_hills = delete_array(hill_location_host, i, num_of_hills);
	//		--i;
	//		continue;
	//	}
	//	else if (hill_location_host[i].x + hill_location_host[i].radius >= one_side_number) {
	//		num_of_hills = delete_array (hill_location_host, i, num_of_hills);
	//		--i;
	//		continue;
	//	}
	//	else if (hill_location_host[i].y - hill_location_host[i].radius < 0) {
	//		num_of_hills = delete_array(hill_location_host, i, num_of_hills);
	//		--i;
	//		continue;
	//	}
	//	else if (hill_location_host[i].y + hill_location_host[i].radius >= one_side_number) {
	//		num_of_hills = delete_array(hill_location_host, i, num_of_hills);
	//		--i;
	//		continue;
	//	}
	//}
	cout << "real num of hills: " << num_of_hills << endl;
	return num_of_hills;
}

void make_new_hills(HI* hill_location_host, int& num_of_hills, int origin_num_of_hills, FF wind_direction, int wind_speed)
{
	cout << "Wind info: " << wind_direction.x << " " << wind_direction.y << endl;
	if (wind_speed) {
		hill_location_host[num_of_hills].radius = hill_size_uid(dre);
		hill_location_host[num_of_hills].height = height_uid(dre);

		hill_location_host[num_of_hills].x = hills_location(dre);
		hill_location_host[num_of_hills].y = hills_location(dre);

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

					hill_location_host[num_of_hills].x = hills_location(dre);
					hill_location_host[num_of_hills].y = hills_location(dre);

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
//
//int main()
//{
//	//get_device_info();
//
//
//	//ToDo
//	//���� ��ġ ���� ����, ���� ��ġ�� ���� 0�ε� �ֺ� ����� �������� ���̷���, ��� �����̱�
//	// ����� �����̴µ� ȿ������ ���: �÷��̾ ���� �þ߸� ������Ʈ�� �ϰԲ� �ؾ��� 
//	// hill_location ������Ʈ�� �׻� �ϰ�, cuda�� �÷��̾ ���� ������ terrain_array�� ������Ʈ �ؾ���
//	// �÷��̾ ���� ������ 1000*1000�̶� �����ϰ�, �� �κ��� terrain_array�� ������Ʈ �ؾ���
//	// �÷��̾� ���� �°� �÷��̾� ���� 2���� �迭 terrain_array_for_player�� �����Ҵ��Ͽ� �����ؾ���
//	// 
//	// �÷��̾�� �� ������ ������� �� �ñ�: ���� ����, ������ ����� ��, ī�޶� ������ ��
//	// ����: ������ �����϶� terrain_array��ü�� �����ϰ�, �÷��̾�� terrain_array�� ���縸 �ؼ� �Ϻκ��� ��������,
//	// hill_location�� �����ϰ�, �÷��̾�� �����ٶ�, terrain_array�� �ƿ� �׸���. ====�̰� ������...40000*40000 ����°� �����̿���..
//	//
//	// ������ �����̸�, �����̴� ������ �ݴ� �κ��� ��� �ȴ�. ����� ���ٴ��� ���...
//	// ������ �� �������� �����̸� �����������
//	// �׸��� 40000 * 40000 ������ ���� �����  hill_location �迭���� ���������.
//	// 
//	// ����� �����ɶ� ����� ���ĸ� ������ �����غ��� �͵� ������ �ϴ�
//	// 
//	// �縷�� �𷡾�� ����� ������ ���� �ȴٸ� ������~�ʽ´��� ����� ��� �Ǵ°Ͱ���
//	// �ٶ��� ����� ���⿡ ���� �� ����� ���ϴ� ������ �ٲ�����Ѵ�.
//	// �ٶ��� �ѹ��� ���� �ʾҴٸ� �� ����� ��ٴ� �����Ͽ� �ٶ��� �Ҹ�, �ٶ��� �� �������� ����ִ� ����� ������
//	// �ٶ��� ���ʿ��� �дٸ� ����� ���� �κ��� ��� ����� ���������� �̵��ϰԲ� �Ѵ�,
//	// �� �� �ٶ��� ���ʿ��� �дٸ� ��ĭ�� ������ ���Եǰ�, �������� ���̴� �������� ������.
//	// �ٶ��� ������ ��ġ�� ����: ����� �𿴴� �κ��� ä������ ����, ����� ���̴� ����
//	// �ٶ��� ���Ⱑ ��ġ�� ����: ������ �̵��ϴ� �ӵ�, ����� ���̴� �ӵ�
//	// 
//	// //���� ����� ���̴� ����� ����� ���̰� �Ϸ��� ǳ�Ӱ� ǳ���� ȮȮ �ٲٸ� �ȵȴ�.
//	// �ִ� ǳ���� 50�̸� �ٶ��� �Ⱥδ� ���¿��� 10, 20, 30, 40, 50 �̷��� ���� �÷��� Ŭ������ ������� �Ѵ�
//	// �ƴϸ� ����� ����� �ѹ��� Ȯ ���Ѵ�
//	// 
//	// ���� ���� ���� ���� ������ 1:1:0.2�� �ϸ� ��� ������ ��.
//	// ���� 1:1:1�̶�� �پ��ִ� ���� ���� 0, 45�� �ۿ� ����
//	// 
//	// ������ �ٶ��� ���� �̵��ϸ�, �̵��� ������ �ݴ� ���⿡ ����� ���� �����Ǿ�� �Ѵ�.
//	// ó���� ������ ����� ������ ����ϰ�, ����� ������� �Ǹ� �׸�ŭ ���� �����ǰԲ� ����.
//	// �������� �ٶ��� �Ҿ� �����̰� �ִٸ� �����ʿ��� ����, �Ʒ��� �ٶ��� �дٸ� ������ ���� �ϰԲ�.
//	// ����� �پ �������� �ʰԲ� �ؾ��Ѵ�. �߰��� �Ҷ� hill_location �迭�� Ȯ���ؼ� �浹�˻縦 ����.
//	// 
//	// �ٶ��� �δ� �˰����� ��������, ǳ�Ӱ� ǳ���� ���������� �ٲ����Ѵ�.
//	// 
//	// 
//	// 
//	//
//
//
//	//Make Random Hills Information===================================================
//	HI* hill_location_host = new HI[4000];
//	HI* hill_location_device;
//	cudaMalloc((void**)&hill_location_device, 4000 * sizeof(HI));
//	int num_of_hills = make_hill_location(hill_location_host);
//	int origin_num_of_hills = num_of_hills;
//	cudaMemcpy(hill_location_device, hill_location_host, num_of_hills * sizeof(HI), cudaMemcpyHostToDevice); //Memcpy to Device
//	printf("Random Hill Info Complete\n");
//
//
//	//Terrain Memory Assignment For Player's Sight===================================================
//	char** terrain_player_sight_host = new char* [player_sight_size];	// 2D array for host
//	for (int i = 0; i < player_sight_size; i++) {
//		terrain_player_sight_host[i] = new char[player_sight_size];
//	}
//	for (int i = 0; i < player_sight_size; i++) {
//		for (int j = 0; j < player_sight_size; j++) {
//			terrain_player_sight_host[i][j] = 0;
//		}
//	}
//	char** terrain_player_sight_device;						// 2D array for device
//	char* terrain_player_sight_temp[player_sight_size];		// 1D array temp
//	cudaMalloc((void**)&terrain_player_sight_device, player_sight_size * sizeof(char*));
//	for (int i = 0; i < player_sight_size; i++) {
//		cudaMalloc((void**)&terrain_player_sight_temp[i], player_sight_size * sizeof(char));
//	}
//	cudaMemcpy(terrain_player_sight_device, terrain_player_sight_temp, player_sight_size * sizeof(char*), cudaMemcpyHostToDevice);
//	for (int i = 0; i < player_sight_size; i++) {
//		cudaMemcpy(terrain_player_sight_temp[i], terrain_player_sight_host[i], player_sight_size * sizeof(char), cudaMemcpyHostToDevice);
//	}
//
//
//	//Terrain move & Player Sight Update===================================================
//	TI player_location = { 0, 0 };		//�̰� ���߿� �߽� �������� �ٲ����
//	int wind_angle = 270;		//����
//	int wind_speed = 50;		//�ִ� ǳ�� 50
//	for (int i = 0; i < 10; i++) {
//		clock_t t_1 = clock();
//
//		//Terrain Move
//		wind_decide(wind_speed, wind_angle);
//
//		TF wind_direction = { cos(wind_angle * PI / 180), sin(wind_angle * PI / 180) };
//		if (abs(wind_direction.x) < FLT_EPSILON) {
//			wind_direction.x = 0;
//		}
//		if (abs(wind_direction.y) < FLT_EPSILON) {
//			wind_direction.y = 0;
//		}
//
//		move_terrain(hill_location_host, num_of_hills, wind_direction, wind_speed);
//		if (num_of_hills < origin_num_of_hills) {
//			make_new_hills(hill_location_host, num_of_hills, origin_num_of_hills, wind_direction, wind_speed);
//		}
//
//		cudaMemcpy(hill_location_device, hill_location_host, num_of_hills * sizeof(HI), cudaMemcpyHostToDevice); //Memcpy to Device
//
//		//Player Sight Update
//		//player_location.x += 20;
//		//player_location.y += 20;
//		//thread must be 1024 for efficiency
//		player_terrain_update_cuda << <player_sight_size, player_sight_size >> > (terrain_player_sight_device, hill_location_device, num_of_hills, player_location, wind_direction, wind_speed);
//		for (int i = 0; i < player_sight_size; i++) {
//			cudaMemcpy(terrain_player_sight_host[i], terrain_player_sight_temp[i], player_sight_size * sizeof(char), cudaMemcpyDeviceToHost);
//		}
//		clock_t t_2 = clock();
//		cout << "Player Sight Update Time : " << (double)(t_2 - t_1) / CLOCKS_PER_SEC << " Seconds" << endl;
//		//show_array(terrain_player_sight_host, player_sight_size);
//		cout << "==============================" << endl;
//	}
//
//
//
//	//File Save===================================================
//	/*ofstream out{"terrain.txt"};
//
//	for (int i = 0; i < one_side_number; i++) {
//		for (int j = 0; j < one_side_number; j++) {
//			out << terrain_array_host[i][j] << " ";
//		}
//		out << endl;
//	}
//	cout << "File Write Complete" << endl;*/
//
//
//	//Free Memory===================================================
//	delete[] hill_location_host;
//	cudaFree(hill_location_device);
//	for (int i = 0; i < player_sight_size; i++) {
//		delete[] terrain_player_sight_host[i];
//		cudaFree(terrain_player_sight_temp[i]);
//	}
//	delete[] terrain_player_sight_host;
//	cudaFree(terrain_player_sight_device);
//	cudaFree(terrain_player_sight_temp);
//}
