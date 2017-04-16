#include "ping.h"

#include <exception>
#include <iostream>

using namespace std;

int main()
{
	try{
		Pinger p("ya.ru");
		p.ping();
		p.set_address("google.com");
		p.ping();
	}
	catch(exception& e)
	{
		cout << e.what() << endl;
	}
	return 0;
}

