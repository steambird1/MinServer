#include "framework.h"
#include "util.h"
#include <iostream>
using namespace std;

map<int, FILE*> file_token;

int findToken() {
	int r;
	do {
		r = random();
	} while (file_token.count(r));
	return r;
}

// By this way we don't care buffer size.
string CReadLine(FILE *f) {
	char c = '\0';
	string tmp = "";
	while (c == '\n' || feof(f)) {
		tmp += (c = fgetc(f));
	}
	return tmp;
}

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
		vector<post_info> post_info;			// In file writes WOULD NOT SEND AS POST STANDARD
		http_send sndinfo;

		sndinfo.codeid = 200;
		sndinfo.code_info = "OK";
		sndinfo.proto_ver = hinfo.proto_ver;

		bool flag;

		if (path_pinfo.path.size() == 1 && path_pinfo.path[0] == "file_operate") {
			// File operation requestion
			string op = path_pinfo.exts["operate"];
			if (op == "open") {
				// Open file, token returns
				int tk = findToken();
				file_token[tk] = fopen(path_pinfo.exts["name"].c_str(), path_pinfo.exts["type"].c_str());
				sndinfo.content = to_string(tk);
			}
			else if (op == "close") {
				// Close file
				// (Release handles)
				int tk = atoi(path_pinfo.exts["token"].c_str());
				fclose(file_token[tk]);
				file_token.erase(tk);
			}
			else if (op == "read") {
				// Read line
				int tk = atoi(path_pinfo.exts["token"].c_str());
				sndinfo.content = CReadLine(file_token[tk]);
			}
			else if (op == "write") {
				// Write line
				int tk = atoi(path_pinfo.exts["token"].c_str());
				fputs(hinfo.content.toCharArray(), file_token[tk]);
			}
		}
		else {
			post_info = hinfo.toPost(); // Can be safe only here
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

							// 1. Parameters & Post informations
							string s = readAll("mspara.js").toString();
							int t = s.length() - 4;			// removing two '%s'
							// Prepare URL Args
							string ua = "", pa = "";
							for (auto &i : path_pinfo.exts) {
								ua += "{key:\"" + i.first + "\",value:\"" + i.second + "\"}\n";
							}
							t += ua.length();
							// Prepare POST Args
							// ...


							t += pa.length();
							// Print
							char *buf = new char[t + 2];
							sprintf(buf, s.c_str(), ua.c_str(), pa.c_str());
							fprintf(fr, "// Args\n%s\n", buf);
							// End

							// 2. Default APIs
							// (Copy from javascript)
							fprintf(fr, "// Default MSLIB API\n%s\n", readAll("mslib.js").toCharArray());

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
				// Send directly
				FILE *f = fopen(rpath.c_str(), "rb");
				sndinfo.loadContent(f);
				fclose(f);
			}
		}

		s.sends(sndinfo.toSender());
		s.end_accept();
	}

	WSACleanup();
	s.end();
	return 0;
}