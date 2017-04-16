#include "ping.h"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
	try{
		Pinger p("8.8.8.8");
		p.ping();
	}
	catch(exception& e)
	{
		cout << e.what() << endl;
	}
	return 0;
}

