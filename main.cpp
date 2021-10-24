#include "framework.h"
#include "util.h"
#include <iostream>
#include <set>
// For MSVC:
#ifdef _MSC_VER
#include <utility>
#else
#include <bits/stl_pair.h>
#endif
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

// start with '$' disallows ANY RW.
string perm_data_path = "$permission.txt";
string user_data_path = "$users.txt";
string public_file_path = "$public.txt";

char buf[MAX_PATH];
set<string> pub_fn;

pair<string, string> resolveMinorPath(string full) {
	string f2 = full, f3 = "";
	for (size_t i = 0; i < full.length(); i++) {
		if (full[i] == '?') {
			f2 = f2.substr(0, i); // abc? (0,3) -> abc
			f3 = f3.substr(i + 1);
		}
	}
	return make_pair(f2, f3);
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		string it = argv[i];
		if (it == "--default-page") {
			not_found = readAll(argv[i + 1]);
			i++;
		}
		else if (it == "--perm-table") {
			perm_data_path = argv[i + 1];
			i++;
		}
		else if (it == "--users-table") {
			user_data_path = argv[i + 1];
			i++;
		}
		else if (it == "--public-table") {
			public_file_path = argv[i + 1];
			i++;
		}
	}
	FILE *f = fopen(public_file_path.c_str(), "r");
	if (f != NULL) {
		while (!feof(f)) {
			// buf uses begin
			fgets(buf, MAX_PATH, f);
			pub_fn.insert(buf);
			// buf uses end
		}
		fclose(f);
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
		sndinfo.attr["Connection"] = "Keep-Alive";
		sndinfo.content.clear();

		bool flag;

		if (path_pinfo.path.size() == 1) {
			if (path_pinfo.path[0] == "file_operate") {
				// File operation requestion
				string op = path_pinfo.exts["operate"];
				if (op == "open") {
					if (path_pinfo.exts["name"].find('$') != string::npos) {
						sndinfo.codeid = 403;
						sndinfo.code_info = "Forbidden";
					}
					else {
						// Open file, token returns
					// (Here we requires permissions)
						int utoken, rperm = getPermOf(path_pinfo.exts["type"]);
						if (path_pinfo.exts.count("utoken")) {
							utoken = atoi(path_pinfo.exts["utoken"].c_str());
						}
						else {
							utoken = 0;
						}
						// Read for permission information
						int suid = 0;
						if (!uidctrl::vaild(utoken))
							suid = 0;
						else
							suid = uidctrl::uidof(utoken);
						FILE *f = fopen(perm_data_path.c_str(), "r");
						if (f != NULL) {
							int uid, uperm;
							while (!feof(f)) {
								// buf uses begin
								// uperm = -1 for owner information.
								fscanf(f, "%d %s %d", &uid, buf, &uperm);
								if (path_pinfo.exts["name"] == buf && uid == suid && uperm > 0) {
									break;
								}
								// buf uses end
							}
							fclose(f);
							if (!permMatch(rperm, uperm)) {
								sndinfo.codeid = 403;
								sndinfo.code_info = "Forbidden";
							}
							else {
								int tk = findToken();
								file_token[tk] = fopen(path_pinfo.exts["name"].c_str(), path_pinfo.exts["type"].c_str());
								sndinfo.content = to_string(tk);
							}
						}
						// End...
						
					}
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
			else if (path_pinfo.path[0] == "auth_workspace") {
				// Authority spaces:
				// 1. Auth check (and gives 'workspace token').
				// 2. File-in-token access (for giving list)
				// 3. Release.
				// 4. ** Change files' owner and permission.

				// Reserved, to be implemented.
				string op = path_pinfo.exts["operate"];
				// Codes...

				// End of codes
				// Remove that:
				sndinfo.codeid = 501;
				sndinfo.code_info = "Not Implemented";
				sndinfo.content = not_supported;
			}
		}
		else {
			if (hinfo.process == "POST")
				post_info = hinfo.toPost(); // Can be safe only here
			pair<string, string> m = resolveMinorPath(hinfo.path);
			if (pub_fn.count(m.first)) {
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
								// *** To be implemented ***

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
			else {
				sndinfo.codeid = 403;
				sndinfo.code_info = "Forbidden";
				sndinfo.content = no_perm;
			}
		}
			

		s.sends(sndinfo.toSender());
		s.end_accept();
	}

	WSACleanup();
	s.end();
	return 0;
}