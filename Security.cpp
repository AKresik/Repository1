#include "Security.h"

Security::Security()
	{
		names = NULL;
		passwords = NULL;
		logged_in = NULL;
		users_ports = NULL;
		free_ports = NULL;
		size = 0;
	}

Security::~Security()
	{
		delete[] names;
		delete[] passwords;
		delete[] logged_in;
		delete[] users_ports;
		delete[] free_ports;
		size = 0;
	}

void Security::Init( int _size )
{
	size = _size;
	names = new char*[ size ];
	passwords = new char*[ size ];
	logged_in = new bool[ size ];
	users_ports = new int[ size ];
	free_ports = new bool[ size ];
	for ( int i = 0; i < size; i++ )
	{
		names[ i ] = new char[30];
		passwords[ i ] = new char[30];
		users_ports[i] = 0;
		logged_in[i] = 0;
		free_ports[i] = 0;
	}
}

bool Security::Log_In( char *name, char *password )
{
	int tmp = Check( name, password );
	if ( tmp >= 0 )
	{
		logged_in[ tmp ] = 1;
		users_ports[ tmp ] = Get_port();
		return 1;
	}
	return 0;
}

void Security::Log_Out( char *name )
{
	for ( int i = 0; i < size; i++ )
		if ( strcmp(names[ i ], name) == 0 )
		{
			logged_in[ i ] = 0;
			Free_port( users_ports[ i ] );
		}
}

int Security::Check( char *name, char *password )
{
	for ( int i = 0; i < size; i++ )
	{
		if ( strcmp(names[ i ], name) == 0 )
		{
			if( strcmp( passwords[ i ], password) == 0 && !logged_in[ i ])
				return i;
			break;
		}
	}
	return -1;
}

void Security::Read_From_File( fstream &in )
{
	char *tmp = new char[ 256 ];
	char *token;
	int i = 0;
	if ( !in.eof() )
	{
		in.getline( tmp, 255 );
		if( strlen( tmp ) )
			Init( atoi( tmp ) );
	}
	while ( !in.eof() )
	{
		in.getline( tmp, 255 );
		if( strlen( tmp ) && i < size )
		{
			token = strtok( tmp, " " );
			if ( token )
			{
				strcpy(names[ i ], token);
				token = strtok( NULL, " " );
			}
			if ( token )
			{
				strcpy(passwords[ i ], token);
				i++;
			}
		}
	}
}

int Security::Get_port()
{
	int port;
	char* buf = new char[80];
	for ( int i = 0; i < size; i++ )
	{
		if ( !free_ports[ i ] )
		{
			free_ports[ i ] = 1;
			port = 27002 + i * 2;
			return port;
		}
	}
	return 0;
}

char* Security::Get_port(char *name)
{
	char *buf = new char[80];
	for ( int i = 0; i < size; i++ )
		if ( strcmp(names[ i ], name) == 0 )
			return itoa( users_ports[ i ], buf, 10);
	return NULL;
}

void Security::Free_port( int port )
{
	free_ports[ (  port  - 27002 ) / 2 ] = 0;
}

int Security::Get_size()
{
	return size;
}
