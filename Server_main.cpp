#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include "Security.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")


static char *current_port = NULL; 
static char *current_name = NULL;
static int current_socket = -1;

HANDLE hEvent1,hEvent2;


struct Sockets
{
	int size;
	SOCKET *ClientSendSocket;
	SOCKET *ClientRecvSocket;
	bool *free_sockets;
	Security Sec;
};


void Broadcast(Sockets &Structure)
{
	char *name = new char[30];
	strcpy(name, current_name);
	int socket_number = current_socket;

	// Continue listening
	SetEvent( hEvent1 );

	const int buflen = 512;

	char *buf = new char[buflen];
	char recvbuf[buflen];
	char *token;

	int iResult;

	while( true )
	{
		// Receiving data
		iResult = recv(Structure.ClientRecvSocket[ socket_number ], recvbuf, buflen, 0);
		if ( iResult > 0 )
		{
			token = strtok( recvbuf, "#");

			// Check logging out
			if ( strcmp(token, "*log_out*") == 0 )
			{
				Structure.Sec.Log_Out(name);
				break;
			}

			// Broadcasting data
			for ( int i = 0; i < Structure.size; i++ )
			{
				if( i != socket_number && Structure.free_sockets[ i ] != 0 )
				{
					strcpy(buf,name);
					strcat(buf,": ");
					strcat(buf,token);
					strcat(buf,"#");

					iResult = send( Structure.ClientSendSocket[ i ], buf, (int)strlen(buf), 0 );
					if (iResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						continue;
					}
				}
			}
		}
		else if ( iResult == 0 )
		{
			printf("Connection closed\n");
			closesocket(Structure.ClientSendSocket[ socket_number ]);
			closesocket(Structure.ClientRecvSocket[ socket_number ]);
			Structure.free_sockets[ socket_number ] = 0;
			return;
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(Structure.ClientSendSocket[ socket_number ]);
			closesocket(Structure.ClientRecvSocket[ socket_number ]);
			Structure.free_sockets[ socket_number ] = 0;
			return;
		}

		Sleep(100);
	} 
	closesocket(Structure.ClientSendSocket[ socket_number ]);
	closesocket(Structure.ClientRecvSocket[ socket_number ]);
	Structure.free_sockets[ socket_number ] = 0;
}


void Listner(Sockets &Structure)
{
	current_name = new char[30];
	current_port = new char[10];

	const int buflen = 512;
	char *port = "27000";

	char *buf = new char[80];	
    char *recvbuf= new char[buflen];
    int recvbuflen = buflen;
	char *token;
	char *name = new char[30];
	char *password = new char[30];

	WSADATA wsaData;
	int iResult;
	int iSendResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
    struct addrinfo hints;

	// Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
	{
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(1);
    }

	while ( true )
	{
		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) 
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			continue;
		}

		// Setup the TCP listening socket
		iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			continue;
		}

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) 
		{
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			continue;
		}

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) 
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			continue;
		}

		closesocket(ListenSocket);

		// Receiving name and password
		do 
		{
			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) 
			{
				token = strtok( recvbuf, " " );
				if ( token )
				{
					strcpy(name,token);
					token = strtok( NULL, " " );
				}
				if ( token )
					strcpy(password,token);

				// Verifying name and password
				if(Structure.Sec.Log_In(name, password))
				{
					// Waiting for previous client's connection
					WaitForSingleObject( hEvent1, INFINITE );

					strcpy(current_name, name);
					strcpy(current_port, Structure.Sec.Get_port(name));

					strcpy(buf,current_port);
					strcat( buf, "#");

					// Sending new port to the client
					iSendResult = send( ClientSocket, buf, (int)strlen(buf), 0 );
					if (iSendResult == SOCKET_ERROR) 
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						SetEvent(hEvent1);
						continue;
					}

					// Setting client's connection
					SetEvent( hEvent2 );
				}

				closesocket(ClientSocket);
				break;
			}
			else if (iResult == 0)
			{
				printf("Connection closing...\n");
				closesocket(ClientSocket);
			}
			else  
			{
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
			}

		} 
		while (iResult > 0);

		Sleep(100);
	}
}

void Room(Sockets &Structure)
{
	int max_members = Structure.size;
	int socket_number;

	char *buf = new char[80];

	WSADATA wsaData;
	int iResult;

	HANDLE *hThread = new HANDLE[max_members];
	DWORD *IDThread = new DWORD[max_members];

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
    struct addrinfo hints;

	// Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
	{
        printf("WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	// Adding client to the room
	while ( true )
	{
		// Waiting for new client
		WaitForSingleObject( hEvent2, INFINITE);

		// Resolve the server address and port for receiving
		iResult = getaddrinfo(NULL, current_port, &hints, &result);
		if ( iResult != 0 ) 
		{
			printf("getaddrinfo failed with error: %d\n", iResult);
			SetEvent(hEvent2);
			continue;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) 
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			SetEvent(hEvent2);
			continue;
		}

		// Setup the TCP listening socket
		iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) 
		{
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		// Searching free SOCKET
		for( int i = 0; i < max_members; i++ )
		{
			if ( Structure.free_sockets[ i ] == 0 )
			{
				socket_number = i;
				break;
			}
		}

		// Accept a client socket for receiving
		Structure.ClientRecvSocket[ socket_number ] = accept(ListenSocket, NULL, NULL);
		if (Structure.ClientRecvSocket[ socket_number ] == INVALID_SOCKET) 
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		closesocket(ListenSocket);

		// Resolve the server address and port for sending
		iResult = getaddrinfo(NULL, itoa(atoi(current_port)-1,buf,10), &hints, &result);
		if ( iResult != 0 ) 
		{
			printf("getaddrinfo failed with error: %d\n", iResult);
			SetEvent(hEvent2);
			continue;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) 
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			SetEvent(hEvent2);
			continue;
		}

		// Setup the TCP listening socket
		iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) 
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) 
		{
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		// Accept a client socket for sending
		Structure.ClientSendSocket[ socket_number ] = accept(ListenSocket, NULL, NULL);
		if (Structure.ClientSendSocket[ socket_number ] == INVALID_SOCKET) 
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			SetEvent(hEvent2);
			continue;
		}

		closesocket(ListenSocket);

		current_socket = socket_number;

		hThread[ socket_number ] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Broadcast, (void*)&Structure, 0, &IDThread[ socket_number ]);
		if (hThread[ socket_number ] == NULL)
		{
			SetEvent(hEvent2);
			continue;
		}

		Structure.free_sockets[ socket_number ] = 1;

		Sleep(100);
	}
}

int main(void) 
{

	HANDLE hThread_1, hThread_2;
	DWORD IDThread_1, IDThread_2;

	Sockets Structure;

	fstream in("members.txt");
	Structure.Sec.Read_From_File(in);
	in.close();

	int max_members = Structure.Sec.Get_size();

	Structure.size = max_members;
	Structure.ClientSendSocket = new SOCKET[max_members];
	Structure.ClientRecvSocket = new SOCKET[max_members];
	Structure.free_sockets = new bool[max_members];
	for( int i=0; i<max_members;i++)
	{
		Structure.ClientSendSocket[ i ] = INVALID_SOCKET;
		Structure.ClientRecvSocket[ i ] = INVALID_SOCKET;
		Structure.free_sockets[ i ] = 0;
	}

	hEvent1 = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hEvent1 == NULL)
		return GetLastError();

	hEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent2 == NULL)
		return GetLastError();

	hThread_1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Listner, (void*)&Structure, 0, &IDThread_1);
	if (hThread_1 == NULL)
		return GetLastError();

	hThread_2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Room, (void*)&Structure, 0, &IDThread_2);
	if (hThread_2 == NULL)
		return GetLastError();

	WaitForSingleObject(hThread_1, INFINITE);
	WaitForSingleObject(hThread_2, INFINITE);

	CloseHandle(hThread_1);
	CloseHandle(hThread_2);
	CloseHandle(hEvent1);
	CloseHandle(hEvent2);

    return 0;
}
