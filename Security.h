#include <string>
#include <iostream>
#include <fstream>
using namespace std;

class Security
{
	char **names;
	char **passwords;
	bool *logged_in;
	int *users_ports;
	bool *free_ports;
	int size;

	

public:

	Security();	
	~Security();

	void Init( int _size );
	bool Log_In( char *name, char *password );
	void Log_Out( char *name );
	int Check( char *name, char *password );
	void Read_From_File( fstream &in );
	int Get_port();
	char* Get_port(char *name);
	void Free_port(int port);
	int Get_size();
};
