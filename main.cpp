#include "framework.h"
#include "util.h"
#include <iostream>
using namespace std;

map<int, FILE*> file_token;

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		string it = argv[i];
		if (it == "--not-found") {
			not_found = readAll(argv[i + 1]);
			i++;
		}
	}
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
		string path = hinfo.path, rpath;
		auto path_pinfo = hinfo.toPaths();
		vector<post_info> post_info = hinfo.toPost();
		http_send sndinfo;

		sndinfo.codeid = 200;
		sndinfo.code_info = "OK";
		sndinfo.proto_ver = hinfo.proto_ver;

		bool flag;

		if (path_pinfo.path.size() == 1 && path_pinfo.path[0] == "file_operate") {
			// File operation requestion

		}
		else {
			// Get path
			flag = false;
			for (auto &i : defiles) {
				rpath = path + i;
				if (fileExists(rpath)) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				sndinfo.codeid = 404;
				sndinfo.code_info = "Not found";
				sndinfo.content = not_found;
			}

			sndinfo.attr["Connection"] = "keep-alive";
			sndinfo.attr["Content-Type"] = findType(rpath);

			if (sndinfo.attr["Content-Type"] == "text/html") {
				// Insert script
				string dest = makeTemp();
				//CopyFile(rpath.c_str(), dest.c_str(), FALSE);
				// Simply resolve <head> or <body>.
				string qu = "";
				bool mode1 = false;
				FILE *f = fopen(rpath.c_str(), "r"), *fr = fopen(dest.c_str(), "w");
				while (!feof(f)) {
					char c = fgetc(f);
					if (c == '<') {
						qu = "";
						mode1 = true;
					}
					else if (c == '>') {
						mode1 = false;
						if (qu == "head" || qu == "body") {
							// Insert script, now!
							fprintf(fr, "><script>\n");

							// 1. Parameters

							// 2. Post informations

							// 3. Default APIs

							// End
							fprintf(fr, "\n</script>");

							continue;
						}
					}
					else if (mode1) {
						qu += c;
					}
					fputc(c, fr);
				}
				fclose(f);
				fclose(fr);
			}
			else {

			}
		}

		s.sends(sndinfo.toSender());
		s.end_accept();
	}

	WSACleanup();
	s.end();
	return 0;
}