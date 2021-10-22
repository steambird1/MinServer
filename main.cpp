#include "framework.h"
#include "util.h"
#include <iostream>
using namespace std;

int main() {
	ssocket s = ssocket(80);
	if (!s.vaild()) {
		cout << "Can't bind or listen!" << endl;
		exit(1);
	}
	while (true) {
		s.accepts();
		if (!s.accept_vaild()) {
			s.end_accept();
			continue;
		}
		http_recv hinfo = s.receive();


		s.end_accept();
	}

	WSACleanup();
	s.end();
	return 0;
}