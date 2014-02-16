#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


void Recv(SOCKET &RecvSocket)
{
	int iResult;

	const int recvbuflen = 512;

	char *token;
	char recvbuf[recvbuflen];

	// Receiving data
	do 
	{
        iResult = recv(RecvSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
		{
			token = strtok(recvbuf, "#");
			cout<<endl<<endl<<token<<endl<<endl;
			Sleep(500);
		}
		else if ( iResult == 0 )
		{
            printf("Connection closed\n");
			closesocket(RecvSocket);
			WSACleanup();
			exit(1);
		}
        else
		{
            printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(RecvSocket);
			WSACleanup();
			exit(1);
		}

    } 
	while( iResult > 0 );

}

void Send(SOCKET &SendSocket)
{
	cout<<"System: '#' is reserved symbol"<<endl;
	cout<<"System: Type *log_out* to log out"<<endl;

	int iResult;

	char *buf = new char[500];

	// Sending data
	do
	{
		cin>>buf;
		strcat( buf, "#");

		iResult = send( SendSocket, buf, (int)strlen(buf), 0 );
		if (iResult == SOCKET_ERROR) 
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(SendSocket);
			WSACleanup();
			exit(1);
		}

		Sleep(500);
	}
	while ( strcmp(buf, "*log_out*") != 0 );

}

int main( void )
{
	const int buflen = 512;

	char *addr = "127.0.0.1";
	char *port = "27000";

	char *token;
	char *buf = new char[80];
	char *name = new char[30];
	char *password = new char[30];

	char recvbuf[buflen];
	int recvbuflen = buflen;

	cout<<"Input name:"<<endl;
	cin>>name;
	cout<<"Input password:"<<endl;
	cin>>password;

	strcpy(buf,name);
	strcat( buf, " ");
	strcat( buf, password);
	strcat( buf, " ");

	HANDLE hThread_1, hThread_2;
	DWORD IDThread_1, IDThread_2;

	WSADATA wsaData;

    SOCKET ConnectSocket = INVALID_SOCKET;
	SOCKET RecvSocket = INVALID_SOCKET;
	SOCKET SendSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL, *tmp = NULL, hints;

    int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
	{
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo( addr, port, &hints, &result);
    if ( iResult != 0 ) 
	{
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

	// Attempt to connect to an address until one succeeds
	for(tmp=result; tmp != NULL ;tmp=tmp->ai_next) 
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) 
		{
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

		// Connect to server.
		iResult = connect( ConnectSocket, tmp->ai_addr, (int)tmp->ai_addrlen);
        if (iResult == SOCKET_ERROR) 
		{
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

	 freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) 
	{
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

	// Send name and password
	iResult = send( ConnectSocket, buf, (int)strlen(buf), 0 );
    if (iResult == SOCKET_ERROR) 
	{
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

	iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) 
	{
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

	// Receive new port
	do 
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if ( iResult > 0 )
		{
			closesocket(ConnectSocket);

			token = strtok(recvbuf,"#");

			// Resolve the server address and port for sending
			iResult = getaddrinfo( addr, token, &hints, &result);
			if ( iResult != 0 ) 
			{
				printf("getaddrinfo failed with error: %d\n", iResult);
				WSACleanup();
				return 1;
			}

			// Attempt to connect to an address until one succeeds
			for(tmp=result; tmp != NULL ;tmp=tmp->ai_next) 
			{
				// Create a SOCKET for sending
				SendSocket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
				if (SendSocket == INVALID_SOCKET) 
				{
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}

				// Connect to room
				iResult = connect( SendSocket, tmp->ai_addr, (int)tmp->ai_addrlen);
				if (iResult == SOCKET_ERROR) 
				{
					closesocket(SendSocket);
					SendSocket = INVALID_SOCKET;
					continue;
				}
				break;
			}

			freeaddrinfo(result);

			if (SendSocket == INVALID_SOCKET) 
			{
				printf("Unable to connect to server!\n");
				WSACleanup();
				return 1;
			}

			// Resolve the server address and port for receiving
			iResult = getaddrinfo( addr, itoa(atoi(token)-1,buf,10), &hints, &result);
			if ( iResult != 0 ) 
			{
				printf("getaddrinfo failed with error: %d\n", iResult);
				WSACleanup();
				return 1;
			}

			// Attempt to connect to an address until one succeeds
			for(tmp=result; tmp != NULL ;tmp=tmp->ai_next) 
			{
				// Create a SOCKET for receiving
				RecvSocket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
				if (RecvSocket == INVALID_SOCKET) 
				{
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					return 1;
				}

				// Connect to room
				iResult = connect( RecvSocket, tmp->ai_addr, (int)tmp->ai_addrlen);
				if (iResult == SOCKET_ERROR) 
				{
					closesocket(RecvSocket);
					RecvSocket = INVALID_SOCKET;
					continue;
				}
				break;
			}

			freeaddrinfo(result);

			if (RecvSocket == INVALID_SOCKET) 
			{
				printf("Unable to connect to server!\n");
				WSACleanup();
				return 1;
			}


			hThread_1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Send, (void*)&SendSocket, 0, &IDThread_1);
			if (hThread_1 == NULL)
				return GetLastError();

			hThread_2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Recv, (void*)&RecvSocket, 0, &IDThread_2);
			if (hThread_2 == NULL)
				return GetLastError();

		}
		else if ( iResult == 0 )
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} 
	while( iResult > 0 );


	WaitForSingleObject(hThread_1, INFINITE);


	CloseHandle(hThread_1);
	CloseHandle(hThread_2);

	closesocket(SendSocket);
	closesocket(RecvSocket);
	WSACleanup();

	return 0;
}
