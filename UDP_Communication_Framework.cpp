// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//
//dskfkfl

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>

// Enter the RECEIVER IP ADRESS (same on both devices)
#define TARGET_IP	"127.0.0.1"
#define BUFFERS_LEN 1024
#define CRC32_LEN 4


#define RECEIVER



#ifdef SENDER
#define TARGET_PORT 8888
#define LOCAL_PORT 5555
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 5555
#define LOCAL_PORT 8888
#endif // RECEIVER

void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

uint32_t countCRC(char buffer[], size_t length);
//**********************************************************************
int main()
{
	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local {};
	struct sockaddr_in from {};

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;


	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar(); //wait for press Enter
		return 1;
	}

	int i = 1;
	//**********************************************************************


#ifdef SENDER
	char buffer_tx[BUFFERS_LEN];
	sockaddr_in addrDest{};
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);

	char filename[50];
	char filepath[100];
	printf("Enter the filename to send: ");
	scanf("%s", filename);
	strcpy(filepath, "D:\\");
	strcat(filepath, filename);

	FILE* file = fopen(filepath, "rb");
	if (!file) {
		printf("Error opening file!\n");
		return 1;
	}

	strncpy(buffer_tx, "start", BUFFERS_LEN);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	Sleep(10000);


	size_t bytes_read;

	// my goals for now: ensure that the image YES and crc will be delevired
	while ((bytes_read = fread(buffer_tx, 1, BUFFERS_LEN, file)) > 0) {
		// create new buffer for value: buffer + counted_crc
		char crc_buffer_s[BUFFERS_LEN + CRC32_LEN];
		uint32_t crc_initial = countCRC(buffer_tx, bytes_read);

		// add buffer and crc to crc_buffer_s
		crc_initial = htonl(crc_initial);
		memcpy(crc_buffer_s, buffer_tx, bytes_read);
		memcpy(crc_buffer_s + bytes_read, &crc_initial, CRC32_LEN);

		int total_bytes = bytes_read + CRC32_LEN;
		sendto(socketS, crc_buffer_s, total_bytes, 0, (sockaddr*)&addrDest, sizeof(addrDest));

		// implemantation of receiving answear
		// 
		// 
		// 

		printf("Packet number %d was sent and received\n", i);
		i++;
		// sleep пока не получили подтверждение, если OK то континью, если НЕ ОК, то бафер сохраняем до темп переменной и заново отправляем
		Sleep(30);
	}

	strcpy(buffer_tx, "end");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	printf("END!\n");

	fclose(file);


	closesocket(socketS);

#endif // SENDER

#ifdef RECEIVER
	int test[10000];
	int len = 0;
	char buffer_rx[BUFFERS_LEN];
	strncpy(buffer_rx, "No data is received.\n", BUFFERS_LEN);
	printf("Waiting for the datagram...\n");
	if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
		printf("Socket error!\n");
		getchar();
		return 1;
	}
	else {
		if (strncmp(buffer_rx, "start", 5) == 0) {
			char filename[50];
			char filepath[100];
			printf("Enter the filename to receive: ");
			scanf("%s", filename);
			strcpy(filepath, "D:\\");
			strcat(filepath, filename);
			FILE* file = fopen(filepath, "wb");
			printf("File %s is opened for writing.\n", filename);
			int keep_sending = 1;
			while (keep_sending) {
				char crc_buffer_r[BUFFERS_LEN + CRC32_LEN];

				char crc_received[CRC32_LEN];

				int total_bytes = BUFFERS_LEN + CRC32_LEN;

				int bytes_received = recvfrom(socketS, crc_buffer_r, total_bytes, 0, (sockaddr*)&from, &fromlen);
				int buffer_len = bytes_received - CRC32_LEN;

				// separate line to buffer (0 to BUFFER_LEN-1) and crc (BUFFER_LEN to BUFFER_LEN + CRC32_LEN - 1)
				if (bytes_received > CRC32_LEN) {
					memcpy(buffer_rx, crc_buffer_r, buffer_len);
					memcpy(crc_received, crc_buffer_r + buffer_len, CRC32_LEN);
				}

				if (strncmp(crc_buffer_r, "end", 3) == 0) {
					keep_sending = 0;
					printf("End\n");
					break;
				}

				// counting actual crc
				uint32_t crc_actual = countCRC(buffer_rx, buffer_len);

				// counting expected crc (type uint32_t)
				uint32_t crc_expected;
				memcpy(&crc_expected, crc_received, CRC32_LEN);
				crc_expected = ntohl(crc_expected);

				if (crc_actual != crc_expected) {
					printf("CRC Error in the packet %d\n", i);
					// test[len] = i;
					// len++;
				}

				fwrite(buffer_rx, 1, bytes_received - CRC32_LEN, file);
				printf("Packet %d was received succesfully! (%d bytes)\n", i, bytes_received);
				i++;
			}

			// test CRC
			// 
			// printf("CRC error on lines: ");
			// for (int t = 0; t < len; t++) {
			//	printf("%d ", test[t]);
			// }
			fclose(file);
			closesocket(socketS);
		}
	}
#endif
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}


// counting crc32, return value: uin32_t counted_crc
uint32_t countCRC(char buffer[], size_t length) {
	uint32_t crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (const Bytef*)buffer, length);
	return crc;
}
