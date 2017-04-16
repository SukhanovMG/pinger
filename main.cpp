#include "ping.h"

#include <exception>
#include <vector>
#include <iostream>

using namespace std;

vector<string> addresses = { /*"192.168.0.222",*/ "ya.ru", "google.com", "8.8.8.8" };
unsigned n_ping = 5;

void ping_em_all()
{
	Pinger p(addresses[0]);

	for (const auto& addr : addresses)
	{
		cout << addr << endl;
		p.set_address(addr);
		for (size_t i = 0; i < n_ping; i++)
		{
			try
			{
				p.ping();
			}
			catch (const runtime_error& re)
			{
				cout << re.what() << endl;
			}
		}
	}
}

int main()
{
	try{
		ping_em_all();
	}
	catch(exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	return 0;
}

