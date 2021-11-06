#include "framework.h"
#include "util.h"
#include "md5.h"
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
string group_path = "$groups.txt";
int default_join_g = -1;

char buf[MAX_PATH],buf2[100];
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

// The other operations have to be classed like this
class user_groups {
public:
	static void insert(int uid, int gid) {
		FILE *f = fopen(group_path.c_str(), "a+");
		fprintf(f, "%d %d\n", uid, gid);
		fclose(f);
	}
	static void remove(int uid, int gid) {
		string tmp = makeTemp();
		FILE *f = fopen(group_path.c_str(), "r"), *ft = fopen(tmp.c_str(),"w");
		int u, g;
		while (!feof(f)) {
			fscanf(f, "%d%d", &u, &g);
			if (u != uid || g != gid)
				fprintf(ft, "%d %d\n", u, g);
		}
		CopyFile(tmp.c_str(), group_path.c_str(), FALSE);
	}
	static set<int> query(int uid) {
		FILE *f = fopen(group_path.c_str(), "a+");
		int u, g;
		set<int> result;
		while (!feof(f)) {
			fscanf(f, "%d%d", &u, &g);
			if (u == uid)
				result.insert(g);
		}
		return result;
	}
};

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
		else if (it == "--default-join") {
			default_join_g = atoi(argv[i + 1]);
			i++;
		}
		else if (it == "--user-group") {
			// User-group operations
			string op = argv[i + 1];
			if (op == "--insert") {
				// To be implemented
				int uid = atoi(argv[i + 2]);
				int gid = atoi(argv[i + 3]);
				user_groups::insert(uid, gid);
				i += 3;
			}
			else if (op == "--remove") {
				// To be implemented
				int uid = atoi(argv[i + 2]);
				int gid = atoi(argv[i + 3]);
				user_groups::remove(uid, gid);
				i += 3;
			}
			else if (op == "--query") {
				// To be implemented
				int uid = atoi(argv[i + 2]);
				auto gr = user_groups::query(uid);
				for (auto &i : gr) {
					printf("%d\n", i);
				}
				i += 2;
			}
			return 0;	// End of resolving
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
		vector<post_info> post_infolist;			// In file writes WOULD NOT SEND AS POST STANDARD
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
					// Provides:
					// name		- Filename.
					// utoken	- User token logged on.
					// type		- Open-type like "r".
					// @return	- 403 Forbidden / File token

					// As "File-in-token".
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
						set<int> ug = user_groups::query(suid);
						FILE *f = fopen(perm_data_path.c_str(), "r");
						if (f != NULL) {
							int uid, uperm = 10;
							while (!feof(f)) {
								// buf uses begin
								// uperm = -1 for owner information.
								fscanf(f, "%d %s %d", &uid, buf, &uperm);
								if (path_pinfo.exts["name"] == buf && (uid == suid || (uid < 0 && ug.count(uid))) && uperm > 0) {
									break;
								}
								// buf uses end
							}
							fclose(f);
							if (!permMatch(rperm, uperm) && fileExists(path_pinfo.exts["name"])) {
								sndinfo.codeid = 403;
								sndinfo.code_info = "Forbidden";
								
							}
							else {
								if (!fileExists(path_pinfo.exts["name"])) {
									f = fopen(perm_data_path.c_str(), "a+");
									fprintf(f, "%d %s %d\n", suid, path_pinfo.exts["name"].c_str(), -1);
									fprintf(f, "%d %s %d\n", suid, path_pinfo.exts["name"].c_str(), 7);
									fclose(f);
								}
								//							else {
								int tk = findToken();
								file_token[tk] = fopen(path_pinfo.exts["name"].c_str(), path_pinfo.exts["type"].c_str());
								sndinfo.content = to_string(tk);
								//							}
							}	
						}
						else {
							sndinfo.codeid = 403;
							sndinfo.code_info = "Forbidden";
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
				// 2. File-in-token access (for giving list) ** Moved to file operation.
				// 3. Release.
				// 4. ** Change files' owner and permission.

				string op = path_pinfo.exts["operate"];
				// Codes...
				if (op == "check") {
					int uid_got = atoi(path_pinfo.exts["request"].c_str());
					string passwd = path_pinfo.exts["passwd"];
					MD5 pass_m = MD5(passwd);
					FILE *f = fopen(user_data_path.c_str(), "r");
					if (f != NULL) {
						int uid;
						bool flag = false;
						while (!feof(f)) {
							fscanf(f, "%d %s", &uid, buf2);
							if (uid == uid_got) {
								if (pass_m.toString() == buf2) {
									sndinfo.content = to_string(uidctrl::request(uid));
								}
								else {
									sndinfo.codeid = 403;
									sndinfo.code_info = "Forbidden";
								}
								flag = true;
								break;
							}
						}
						fclose(f);
						if (!flag) {
							sndinfo.codeid = 403;
							sndinfo.code_info = "Forbidden";
						}
					}
					else {
						sndinfo.codeid = 403;
						sndinfo.code_info = "Forbidden";
					}
				}
				else if (op == "logout") {
					int token_got = atoi(path_pinfo.exts["token"].c_str());
					uidctrl::release(token_got);
				}
				else if (op == "chown") {
					string filename = path_pinfo.exts["file"];
					int token = atoi(path_pinfo.exts["token"].c_str());
					int chto = atoi(path_pinfo.exts["touid"].c_str());
					FILE *f = fopen(perm_data_path.c_str(), "r");
					string dest = makeTemp();
					FILE *fd = fopen(dest.c_str(), "w");
					if (f != NULL) {
						int uid, uperm, uresult;
						while (!feof(f)) {
							// uperm = -1 for owner information.
							// OWNER CAN ONLY BE A PERSON.
							fscanf(f, "%d %s %d", &uid, buf, &uperm);
							if (uperm == -1 && filename == buf) {
								if (uidctrl::uidof(token) != uid) {
									// Bads
									fprintf(fd, "%d %s %d\n", uid, buf, uperm);
								}
								else {
									fprintf(fd, "%d %s %d\n", chto, buf, uperm);
								}
								//break;
							}
							else {
								fprintf(fd, "%d %s %d\n", uid, buf, uperm);
							}
						}
						fclose(f);
					}
					CopyFile(dest.c_str(), perm_data_path.c_str(), FALSE);
					fclose(fd);
				}
				else if (op == "chperm") {
					string filename = path_pinfo.exts["file"];
					int token = atoi(path_pinfo.exts["token"].c_str());
					int chto = atoi(path_pinfo.exts["toperm"].c_str());
					FILE *f = fopen(perm_data_path.c_str(), "r");
					string dest = makeTemp();
					FILE *fd = fopen(dest.c_str(), "w");
					if (f != NULL) {
						int uid, uperm, uresult;
						while (!feof(f)) {
							// uperm = -1 for owner information.
							fscanf(f, "%d %s %d", &uid, buf, &uperm);
							if (uperm != -1 && filename == buf) {
								if (uidctrl::uidof(token) != uid) {
									// Bads
									fprintf(fd, "%d %s %d\n", uid, buf, uperm);
								}
								else {
									fprintf(fd, "%d %s %d\n", uid, buf, chto);
								}
								//break;
							}
							else {
								fprintf(fd, "%d %s %d\n", uid, buf, uperm);
							}
						}
						fclose(f);
					}
					CopyFile(dest.c_str(), perm_data_path.c_str(), FALSE);
					fclose(fd);
				}
				else if (op == "create") {
					// Create user...
					int chto = atoi(path_pinfo.exts["id"].c_str());
					string upasswd = path_pinfo.exts["passwd"];				// Changing to
					MD5 upass_m = MD5(upasswd);
					bool state = false;
					FILE *fd = fopen(user_data_path.c_str(), "r");
					while (!feof(fd)) {
						int u;
						fscanf(fd, "%d %*s", &u);
						if (u == chto) {
							state = true;
							break;
						}
					}
					fclose(fd);

					if (state) {
						int loggon = uidctrl::uidof(atoi(path_pinfo.exts["token"].c_str()));
						if (loggon == chto) {
							string dest = makeTemp();
							FILE *fd = fopen(user_data_path.c_str(), "r"), *de = fopen(dest.c_str(), "w");
							while (!feof(fd)) {
								// buf uses begin
								int u;
								fscanf(fd, "%d %s", &u, buf);
								if (u == chto) {
									fprintf(de, "%d %s\n", u, upass_m.toString().c_str());
								}
								else {
									fprintf(de, "%d %s\n", u, buf);
								}
								// buf uses end
							}
							fclose(fd);
							fclose(de);
							CopyFile(dest.c_str(), user_data_path.c_str(), FALSE);
						}
					}
					else {
						FILE *f = fopen(user_data_path.c_str(), "a+");
						// Check for user-states and create
						fprintf(f, "%d %s\n", chto, upass_m.toString().c_str());
						fclose(f);
						user_groups::insert(chto, default_join_g);
					}
					// End
					fclose(f);
				}

				/*
				
				Groups-join and Groups-query can't be implemented or
				it'll be meaningless.
				
				*/

				//else if (op == "groups_join") {
				// To be implemented
				//int utoken = atoi(path_pinfo.exts["token"].c_str());
				//}
				//else if (op == "groups_remove") {
				// To be implemented
				//}
				// End of codes
				// Remove that:
				//sndinfo.codeid = 501;
				//sndinfo.code_info = "Not Implemented";
				//sndinfo.content = not_supported;
			}
		}
		else {
			if (hinfo.process == "POST")
				post_infolist = hinfo.toPost(); // Can be safe only here
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
								string ua = "", pa = "", ba = "";
								for (auto &i : path_pinfo.exts) {
									ua += "{key:\"" + i.first + "\",value:\"" + i.second + "\"},\n";
								}
								ua.pop_back(); ua.pop_back();	// ',' and 'LF'.
								t += ua.length();
								// Prepare POST Args
								// ...
								for (auto &i : post_infolist) {
									ua += "{\nattr:[";
									for (auto &j : i.attr) {
										ua += "{key:\"" + j.first + "\",value:\"" + j.second + "\"},\n";
									}
									ua.pop_back();	// ','
									ua += "],content:\"\n";
									// To be implemented ...
									// Should replace something like 'LF' to '\\n'.
								}

								t += pa.length();
								// Prepare BROWSER Args


								t += ba.length();

								// Print
								char *buf = new char[t + 2];
								//      Buffer|Template|Args -->
								sprintf(buf, s.c_str(), hinfo.process.c_str(), hinfo.proto_ver.c_str(), ua.c_str(), pa.c_str());
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