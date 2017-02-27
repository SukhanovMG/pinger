#include "ping.h"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
	try{
		Pinger p;
	}
	catch(exception& e)
	{
		cout << e.what() << endl;
	}
	return 0;
}