#include "..\..\Common.h"
#include <iostream>
#include <fstream>
#include<vector>
#include<mutex>
#include<string>
#define SERVERPORT 9000
#define BUFSIZE    4096
using namespace std;
int len = 0;
char buffer[BUFSIZE]; // ���� ���� ������
std::mutex mylock;
//���� ����
int hostnum;
struct SunAngle {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};
DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];
	SunAngle sun_angle;
	FILE* fp;

	sun_angle.x = 0.0f;
	sun_angle.y = 0.0f;
	sun_angle.z = 0.0f;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));

	while (1) {

		retval = send(client_sock, (char*)&sun_angle, (int)sizeof(SunAngle), 0);
		sun_angle.y += 0.2f;
		if (sun_angle.y >= 180.f)
			sun_angle.y = -180.f;
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

	}

	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));
	// ���� �ݱ�
	closesocket(client_sock);

	return 0;
}



int main(int argc, char* argv[])
{
	int retval;
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;
	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

		cout << addr << endl;


		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		//���� �ݱ�
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }

	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
