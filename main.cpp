#include "framework.h"
#include "util.h"
#include "md5.h"
#include <iostream>
#include <set>
#include <psapi.h>
#define _CP_DEFINED
#include "c_framework.h"
//#include "c_framework.h"
// For MSVC:
#ifdef _MSC_VER
#include <utility>
#else
#include <bits/stl_pair.h>
#endif
#if MINSERVER_DEBUG == 4
#include <conio.h>
#endif
/*
#ifdef MINSERVER_EXT_DEBUG
#include "test1.h"
#endif
*/
using namespace std;

set<char> roks = { 'r', '+' }, woks = { 'w','a','+' };
// start with '$' disallows ANY RW.
string perm_data_path = "$permission.txt";
string user_data_path = "$users.txt";
string public_file_path = "$public.txt";
string group_path = "$groups.txt";
string assiocate_path = "$assiocate.txt";
string redirect_path = "$redirect.txt";
int default_join_g = -1;

// Allocate ONCE
thread_local char buf4[4096], buf5[4096];

int portz = 80;

map<string, int> visit;

// Uses for MinServer file opener for permission verify
class file_structure {
public:
	file_structure(int nptr = 0) : read_ok(false), write_ok(false) {
		// For map or fclose_m()
	}
	file_structure(const char *filename, const char *operate) {
		size_t sl = strlen(operate);
		for (size_t i = 0; i < sl; i++) {
			if (roks.count(operate[i])) this->read_ok = true;
			if (woks.count(operate[i])) this->write_ok = true;
		}
		this->place = fopen(filename, operate);
		// Make sure no error on RW
		if (this->place == NULL) {
			this->read_ok = false;
			this->write_ok = false;
		}
	}
	file_structure(FILE *obj, bool rok, bool wok) : place(obj), read_ok(rok), write_ok(wok) {}
	operator FILE*&() {
		return this->place;
	}
	bool readable() {
		return this->read_ok;
	}
	bool writeable() {
		return this->write_ok;
	}
private:
	FILE *place;
	bool read_ok, write_ok;
};

// DLL LastError System
int *dll_err;
void setDLLError(int err) {
	(*dll_err) = err;
}

// Preparing for DLL manager.
class memory_manager {
public:
	static void* allocate(size_t size) {
		void *ptr = nullptr;
		try {
			ptr = (void*) new char[size];
		} catch (...) {
			return nullptr;
		}
		
		if (ptr != nullptr) {
			dll_mem += size;
			ptr_mem[ptr] = size;
			memset(ptr, 0, sizeof(char)*size);
		}
		else {
			setDLLError(2);
		}
		return ptr;
	}
	static void release(void *ptr) {
		if (!ptr_mem.count(ptr)) {
			// Bad free
			setDLLError(1);
		}
		else {
			dll_mem -= ptr_mem[ptr];
			ptr_mem.erase(ptr);
			delete[] ptr;
		}
	}
private:
	static size_t dll_mem;
	static map<void*, size_t> ptr_mem;//Count memory usage.
};
// Allocate memory for it
size_t memory_manager::dll_mem;
map<void*, size_t> memory_manager::ptr_mem;

map<int, file_structure> file_token;

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
	while ((c != '\n') && (!feof(f))) {
		tmp += (c = fgetc(f));
	}
	return tmp;
}

thread_local char buf[MAX_PATH],buf2[100];
set<string> pub_fn;

// First: path
// Second: entire args
pair<string, string> resolveMinorPath(string full) {
	string f2 = full, f3 = full;
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
		fclose_m(f);
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
		fclose_m(f);
		fclose_m(ft);
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

// Operation of users
// (Requires check first)
class user_operator {
public:
	// It can only be runned local:
	// (* This is ADD)
	static void adduser(int uid, string passwd) {
		user_groups::insert(uid, default_join_g);
		MD5 m = MD5(passwd);
		string tmps = makeTemp();
		FILE *f = fopen(user_data_path.c_str(), "r"), *ft = fopen(tmps.c_str(), "w");
		if (f != NULL) {
			while (!feof(f)) {
				int uid;
				fscanf(f, "%d %s", &uid, buf2);
				fprintf(ft, "%d %s\n", uid, buf2);
			}
			fclose_m(f);
		}
		fprintf(ft, "%d %s\n", uid, m.toString().c_str());
		fclose_m(ft);
		CopyFile(tmps.c_str(), user_data_path.c_str(), FALSE);
	}

	// (* This is MODIFY)
	static void moduser(int uid, string passwd) {
		MD5 m = MD5(passwd);
		string tmps = makeTemp();
		FILE *f = fopen(user_data_path.c_str(), "r"), *ft = fopen(tmps.c_str(), "w");
		if (f != NULL) {
			while (!feof(f)) {
				int uid2;
				fscanf(f, "%d %s", &uid2, buf2);
				if (uid == uid2) {
					fprintf(ft, "%d %s\n", uid, m.toString().c_str());
				}
				else {
					fprintf(ft, "%d %s\n", uid2, buf2);
				}
			}
		}
		fclose_m(f);
		fclose_m(ft);
		CopyFile(tmps.c_str(), user_data_path.c_str(), FALSE);
	}

	// (* This is QUERY)
	// (Returns: MD5 password (empty for user not exist)
	static string quser(int uid) {
		//MD5 m = MD5(passwd);	// It's not necessery
		FILE *f = fopen(user_data_path.c_str(), "r");
		if (f != NULL) {
			while (!feof(f)) {
				int uid2;
				fscanf(f, "%d %s", &uid2, buf2);
				if (uid == uid2) {
					fclose_m(f);
					return buf2;
				}
			}
			fclose_m(f);
		}
		return "";
	}

	static void chown(string filename, int chto) {
		FILE *f;
		string dest = makeTemp();
		FILE *fd = fopen(dest.c_str(), "w");
		int uid, uperm;
		bool flag = false;
		char buf[1024];
		f = fopen(perm_data_path.c_str(), "r");
		if (f != NULL) {
			while (!feof(f)) {
				// uperm = -1 for owner information.
				// OWNER CAN ONLY BE A PERSON.
				fscanf(f, "%d %s %d", &uid, buf, &uperm);
				if (uperm == -1 && filename == buf) {
//					if (uidctrl::uidof(token) != uid) {
						// Bads
//						fprintf(fd, "%d %s %d\n", uid, buf, uperm);
//					}
//					else {
					
					if (!flag) {
						flag = true;
						fprintf(fd, "%d %s %d\n", chto, buf, uperm);
					}
							// uperm = -1
//					}
					//break;
				}
				else {
					fprintf(fd, "%d %s %d\n", uid, buf, uperm);
				}
			}
			fclose_m(f);
		}
		if (!flag) {
			fprintf(fd, "%d %s -1\n", chto, filename.c_str());
		}
		fclose_m(fd);
		CopyFile(dest.c_str(), perm_data_path.c_str(), FALSE);
		
	}
	// Requires after-auth.
	static void chperm(string filename, int chid, int chto) {
		//	else {
		int uid, uperm;
				FILE *f = fopen(perm_data_path.c_str(), "r");
				string dest = makeTemp();
				FILE *fd = fopen(dest.c_str(), "w");
				bool flag = false;
				if (f != NULL) {
					while (!feof(f)) {
						fscanf(f, "%d %s %d", &uid, buf, &uperm);
						if (uperm != -1 && filename == buf && uid == chid) {
							if (!flag) {
								flag = true;
								fprintf(fd, "%d %s %d\n", chid, buf, chto);
							}
						}
						else {
							fprintf(fd, "%d %s %d\n", uid, buf, uperm);
						}
					}
					fclose_m(f);
				}
				if (!flag) {
					fprintf(fd, "%d %s %d\n", chid, filename.c_str(), chto);
				}
				fclose_m(fd);
				CopyFile(dest.c_str(), perm_data_path.c_str(), FALSE);

		//	}
	}
	static int allocnew(void) {
		int r;
		do {
			r = random();
		} while (quser(r).length());
		return r;
	}
};

// Not includes all
class file_operator {
public:
	static bool release(int tk) {
		if (!file_token.count(tk)) return false;
		fclose_m(file_token[tk]);
		file_token.erase(tk);
		return true;
	}
	const static int perm_denied = -400;	// Permission denied
	const static int op_err = -500;		// Error in operation
	// To abs() for sending code id
	static int open(int suid, string filename, string operate) {
		if (filename[0] == '\\' || filename[0] == '/') filename.erase(filename.begin());
//		filename = sCurrDir(filename);	// Or out of box
		set<int> ug = user_groups::query(suid);
		FILE *f = fopen(perm_data_path.c_str(), "r");
		bool flag3 = false;
		int uid, uperm = 10, rperm = getPermOf(operate);
		if (!fileExists(filename)) {
			flag3 = true;
			if (f != NULL) fclose_m(f);	// Close older one
			f = fopen(perm_data_path.c_str(), "a+");
			fprintf(f, "%d %s %d\n", suid, filename.c_str(), -1);
			fprintf(f, "%d %s %d\n", suid, filename.c_str(), 7);
			fclose_m(f);
		}
		if (f != NULL) {

			while (!feof(f)) {
				// buf uses begin
				// uperm = -1 for owner information.
				int f4 = fscanf(f, "%d %s %d", &uid, buf, &uperm);
				if (f4 != 3) break;	//??
				if (filename == buf && (uid == suid || (uid < 0 && ug.count(uid))) && uperm > 0 && (permMatch(rperm, uperm))) {
					flag3 = true;
					break;
				}
				// buf uses end
			}
			fclose_m(f);
		}
		if (!flag3) {
			return perm_denied;
		}
		else {
			int tk = findToken();
			file_structure fp = file_structure(filename.c_str(), operate.c_str());
			if (fp == NULL) {
				return op_err;
			}
			else {
				file_token[tk] = fp;
				return tk;
			}
		}
	}
};

// C Supportings
extern "C" {
	bool c_user_auth(int uid, cc_str textpwd) {
		MD5 m = MD5(textpwd);
		return (m.toString() == user_operator::quser(uid));
	}
	int c_file_open(int uid, cc_str filename, cc_str method) {
		return file_operator::open(uid, filename, method);
	}
	double c_memory_usage(void) {
		PROCESS_MEMORY_COUNTERS p;
		GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p));
		return p.WorkingSetSize / 1024.0 / 1024.0;
	}
	double c_utoken_usage(void) {
		int ut_use = uidctrl::size();
		//int ft_use = file_token.size();

		// It's in maxinum of rand().
		double ut_free = 100.0 - (ut_use / (double(1<<16) - 1.0));
		//double ft_free = 100.0 - (ft_use / 2.5
		return ut_free;
	}
	double c_ftoken_usage(void) {
		//int ut_use = uidctrl::size();
		int ft_use = file_token.size();
		// It's in maxinum of fopen().
		double ft_free = 100.0 - (ft_use / 2.54);
		return ft_free;
	}
	ip_info c_ip_health(void) {
		ip_info res;
		res.data = (struct _single_ip_info*)calloc(visit.size() + 1, sizeof(struct _single_ip_info));
		res.len = visit.size();
		int it = 0;
		for (auto &i : visit) {
			res.data[it].ip_addr = i.first.c_str();
			res.data[it].ip_vis = i.second;
		}
		return res;
	}
	bool c_ug_query(int uid, int gid) {
		return user_groups::query(uid).count(gid);
	}
	bool c_uo_exists(int uid) {
		return user_operator::quser(uid).length();
	}
	void c_uo_mod(int uid, const char *pwd) {
		if (c_uo_exists(uid)) {
			// Exists
			user_operator::moduser(uid, pwd);
		}
		else {
			user_operator::adduser(uid, pwd);
		}
	}
	void c_uo_chperm(const char *filename, int touid, int toper) {
		if (toper == -1) {
			// Changing ownership
			user_operator::chown(filename, touid);
		}
		else {
			user_operator::chperm(filename, touid, toper);
		}
	}
}

struct vis_info {
	string ip;
	int vis;
};

bool operator < (vis_info x, vis_info y) {
	if (x.vis == y.vis) return x.ip < y.ip;
	return x.vis > y.vis;
}

int aldr = 0;	// Count of assiocation loaded
bool failure = false;
bool downgraded = false, al_cause = false;

void m_stat() {
	system("cls");
	cout << "Server Status" << endl;
	printf("Assiocations loaded: %d\n\n", aldr);

	printf("Memory Usage: %.2lf MB\n\n", c_memory_usage());
#if MINSERVER_DEBUG == 4
	printf("Free times: %d\n\n", bytes::decst);
#endif

	int ut_use = uidctrl::size();
	int ft_use = file_token.size();

	double ut_free = c_utoken_usage();
	double ft_free = c_ftoken_usage();

	if (ut_free < 0.0) ut_free = 0.00;
	if (ft_free < 0.0) ft_free = 0.00;

	printf("User token usage: %d (%.2lf %% Free)\n\n", ut_use, ut_free);
	printf("File token usage: %d (%.2lf %% Free)\n\n", ft_use, ft_free);
	
	if (ut_free == 0.00 || ft_free == 0.00) {
		printf("*** Server downgraded ***\n\n");
	}

	static set<vis_info> vp;
	vp.clear();
	int tot = 0;
	for (auto &i : visit) {
		vp.insert({ i.first, i.second });
		tot += i.second;
	}
	printf("Frequency: \nTotal %d\n\n", tot);
	int lat = 5;
	for (auto &i : vp) {
		if (lat <= 0) break;
		printf("%15s		%4d (%.2lf %%)\n", i.ip.c_str(), i.vis, double(i.vis) / double(tot) * 100.0);
		lat--;
	}
	printf("\n");
	cout << endl << "* Listening at port " << portz << " *" << endl;

if (al_cause) cout << endl << "* Auto-release runned" << endl;
}

bool auto_release = true;

map<string, as_func> acaller;
int max_recesuive = 50;

// To same memory, use once:
http_recv hinfo;
thread_local vector<post_info> post_infolist;			// In file writes WOULD NOT SEND AS POST STANDARD
http_send sndinfo;
const set<string> operations = { "file_operate", "auth_workspace", "uploader", "caller" };
thread_local path_info path_pinfo;

#pragma region(preparations)
const bytes not_found = not_found_c;
const bytes not_supported = not_supported_c;
const bytes no_perm = no_perm_c;
// Also got something as C
cc_str ec403 = no_perm_c.toCharArray();
cc_str ec404 = not_found_c.toCharArray();
cc_str ec501 = not_supported_c.toCharArray();
cc_str ec200_ok = ok.c_str();
cc_str ec200_redirect = redirect.c_str();
#pragma endregion

bytes send_temp = bytes();

http_send normalSender(string path, string external, bytes& prev, int recesuive = 0) {
	if (recesuive > max_recesuive) {
		sndinfo.codeid = 500;
		sndinfo.code_info = "Internal Server Error";
	}
	// As the normal processor
	if (hinfo.process == "POST")
		post_infolist = hinfo.toPost(); // Can be safe only here
	pair<string, string> m;
	m.first = path;
	m.second = external;
	// Or redirection
	// As convert string to wstring take too long
	bool flag = false;
	string rpath = "";
	bool flag2 = false;
	FILE *f = fopen(public_file_path.c_str(), "r");
	cout_d << "Expected main path: " << m.first << endl_d;
	cout_d << "Expected external: " << m.second << endl_d;
	if (m.first.find('$') != string::npos) {
		sndinfo.codeid = 403;
		sndinfo.code_info = "Forbidden";
		sndinfo.content = bytes(no_perm);
		goto sendup;	// As for less jumpers
}
	//heap_test();
	if (f != NULL) {
		while (!feof(f)) {
			// buf uses begin
			fgets(buf, MAX_PATH, f);
			cout_d << "Try: Got: " << buf << endl_d;
			if (m.first == sRemovingEOL(buf)) {	// As there is end of line in BUF
				cout_d << "Equals: " << m.first << endl_d;
				flag2 = true;
				break;
			}
			// buf uses end
		}
		fclose_m(f);
	}
	if (flag2) {
		// Get path
		flag = false;
		string wpath = m.first;
		// -- Is it redirection? --
		FILE *fr = fopen(redirect_path.c_str(), "r");
		if (fr != NULL) {
			while (!feof(fr)) {
				// buf4 uses begin
				fgets(buf4, 4096, fr);
				auto vs = splitLines(buf4, '|', false, '\n');
				bool directz = false;
				if (vs.size() < 2) continue;
				if (wpath == vs[0]) {
					// Got
					// Select 1 or 0
					if (vs.size() >= 3) directz = bool(atoi(vs[2].c_str()));
					if (directz) {
						// Send only OK page, buf4 and buf5 uses begin
						sprintf(buf4, redirect.c_str(), vs[1].c_str(), encodeBytes(vs[1]));
						sprintf(buf5, ok.c_str(), buf4);
						sndinfo.content = buf5;
						goto sendup;
					}
					else {
						// Proceed as another
						http_send s = move(normalSender(vs[1], external, prev, recesuive + 1));
						fclose_m(fr);
						return move(s);
					}
				}
				
				// buf4 uses end
			}
			fclose_m(fr);
		}
		// -- End --
		wpath.erase(wpath.begin());	// As removing '/' at first
		for (auto &i : defiles) {
			rpath = wpath + i;
			if (!rpath.length()) continue;
			if (rpath[0] == '\\') rpath.erase(rpath.begin());	// As removing '\\' at first
			if (fileExists(rpath)) {
				flag = true;
				break;
			}
			rpath = wpath + '\\' + i;
			if (fileExists(rpath)) {
				flag = true;
				break;
			}
		}
		cout_d << "Finally path: " << rpath << endl_d;
		if (!flag) {
			sndinfo.codeid = 404;
			sndinfo.code_info = "Not found";
			sndinfo.content = bytes(not_found);
		}
		else {
			sndinfo.attr["Connection"] = "close";	// It's stupid to keep alive
			sndinfo.attr["Content-Type"] = findType(rpath);

			cout_d << "Sending type: " << sndinfo.attr["Content-Type"] << endl_d;

			// You can see this as a bulitin text/html assiocation
			if (sndinfo.attr["Content-Type"] == "text/html") {
				// Insert script
				cout_d << "Inserting script ..." << endl_d;
				string dest = makeTemp();
				cout_d << "Tempatory file: " << dest << endl_d;
				//CopyFile(rpath.c_str(), dest.c_str(), FALSE);
				// Simply resolve <head> or <body>.
				string qu = "";
				bool mode1 = false, mode2 = false;
				FILE *f = fopen(rpath.c_str(), "r"), *fr = fopen(dest.c_str(), "w");
				while (true) {
					char c = fgetc(f);
					if (feof(f)) break;	 // Preventing EOF remaining
					if (c == '<') {
						qu = "";
						mode1 = true;
					}
					else if (c == '>') {
						cout_d << "Label got: " << c << endl_d;
						mode1 = false;
						if ((sToLower(qu) == "head" || sToLower(qu) == "body") && (!mode2)) {
							// Insert script, now!
							mode2 = true;
							fprintf(fr, "><script>\n");

							// 1. Parameters & Post informations
							string s = readAll("mspara.js").toString();
							auto hp = resolveMinorPath(hinfo.path);
							int t = s.length() + hp.first.length() - 4;			// removing two '%s'
							// Prepare URL Args
							string ua = "", pa = "[", ba = "", ha = "";
							for (auto &i : path_pinfo.exts) {
								ua += "{key:\"" + i.first + "\",value:\"" + i.second + "\"},\n";
							}
							if (ua.length()) {
								ua.pop_back(); ua.pop_back();
							} 	// ',' and 'LF'.
							t += ua.length();
							// Prepare POST Args
							// ...
							for (auto &i : post_infolist) {
								pa += "{\nattr:[";
								bool flag11 = false;
								for (auto &j : i.attr) {
									pa += "{key:\"" + j.first + "\",value:\"" + encodeBytes(j.second) + "\"},\n";
									flag11 = true;
								}
								if (flag11) {
									pa.pop_back();	// 'LF'
									pa.pop_back();	// ','
								}

								pa += "],content:\"" + encodeBytes(i.content) + "\"},";
							}
							pa.pop_back(); // ','
							if (pa.length()) pa += "]";	// As not removed all of things
							t += pa.length();

							// Prepare HEAD-data args (sometimes it'll be useful)
							for (auto &i : hinfo.attr) {
								ha += "{key:\"" + encodeBytes(i.first) + "\",value:\"" + encodeBytes(i.second) + "\"},\n";
							}
							if (ha.length()) {
								ha.pop_back();	// ','
							}

							t += ha.length();

							t += hinfo.process.length();
							t += hinfo.proto_ver.length();

							if (!ua.length()) ua = " ";	// To be not really empty ???
							if (!pa.length()) pa = " ";

							// Print
							cout_d << "Allocated length for buf: " << t + 20 << endl_d;
							char *buf = new char[t + 20];	// Also added ua/pa spaces
							//      Buffer|Template|Args -->
							sprintf(buf, s.c_str(), hinfo.process.c_str(), hinfo.proto_ver.c_str(), hp.first.c_str(), ha.c_str(), ua.c_str(), pa.c_str());	// Fails here, but why?
							//cout << "Builtin scripts: " << endl << buf << endl << "== END ==" << endl;
							fprintf(fr, "// Args\n%s\n", buf);
							// End

							// 2. Default APIs
							// (Copy from javascript)
							fprintf(fr, "// Default MSLIB API\n%s\n", readAll("mslib.js").toCharArray());

							// End
							fprintf(fr, "\n</script>");

							delete[] buf;
							continue;
						}
					}
					else if (mode1) {
						qu += c;
					}
					fputc(c, fr);
				}
				fclose_m(f);
				fclose_m(fr);
				fr = fopen(dest.c_str(), "rb");
				sndinfo.loadContent(fr);	// Here doesn't leak
				fclose_m(fr);
			}
			else {
				// First of all scan for existing assiocation
				string ex = getExt(rpath);	// Get extension, surely contains '.'.
				if (acaller.count(ex)) {
					asdata *s_prep = new asdata;	// Memory leak before here
					// To be updated:
					s_prep->cal_lib = { uidctrl::request, uidctrl::vaild, uidctrl::uidof, uidctrl::release, c_user_auth, file_operator::release, c_file_open, c_memory_usage, c_utoken_usage, c_ftoken_usage, c_ip_health, user_groups::insert, user_groups::remove, c_ug_query, c_uo_mod, c_uo_chperm, c_uo_exists, ec403, ec404, ec501, ec200_ok, ec200_redirect };
					s_prep->mc_lib = { memory_manager::allocate, memory_manager::release };
					s_prep->m_error = dll_err;
					bytes q = prev;
					cc_str stc = q.toCharArray();
					q.release();
					send_info sc;
					sc = acaller[ex](stc, rpath.c_str(), s_prep);
					//delete s_prep;
					delete[] stc;
					//sndinfo.content.add(sc.cdata, sc.len);
					bytes b;
					b.add(sc.cdata, sc.len);
					sndinfo.raw_send = move(b);
					sndinfo.raw_sending = true;
					goto sendup;
					//s.sends(b);
					//b.release();
					//goto after_sentup;
				}
				else {
					FILE *f = fopen(rpath.c_str(), "rb");
					sndinfo.loadContent(f);
					fclose_m(f);
				}
				// End
				// Send directly

			}
		}
	}
	else {
		sndinfo.codeid = 403;
		sndinfo.code_info = "Forbidden";
		sndinfo.content = bytes(no_perm);
	}
sendup: return move(sndinfo);
	// Doesn't need to send in the end
}

int main(int argc, char* argv[]) {
#if MINSERVER_DEBUG == 4
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	HANDLE logHandle = CreateFile(".\\memory.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, logHandle);
#endif
	ts_malloc::init();
	//cout << "Running in directory: " << sCurrDir("example") << endl;
	const char *cbt = new char[60];
	int tbuf = RCV_DEFAULT;
//	cbt = c_boundary("text/html; boundary=------BoundaryInformationDataHereAAABBBCCCDDDEEEFFFGGG");
//	cout_d << "C-Boundary tester:" << cbt << endl_d;
	dll_err = new int;
	
	for (int i = 1; i < argc; i++) {
		string it = argv[i];
		if (it == "--default-page") {
			not_found_c = readAll(argv[i + 1]);
			i++;
		}
		else if (it == "--default-ns") {
			not_supported_c = readAll(argv[i + 1]);
			i++;
		}
		else if (it == "--default-na") {
			no_perm_c = readAll(argv[i + 1]);
			i++;
		}
		else if (it == "--default-ok") {
			ok = readAll(argv[i + 1]).toString();
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
		else if (it == "--groups-table") {
			group_path = argv[i + 1];
			i++;
		}
		else if (it == "--default-join") {
			default_join_g = atoi(argv[i + 1]);
			i++;
		}
		else if (it == "--max-recesuive") {
			max_recesuive = atoi(argv[i + 1]);
			i++;
		}
		else if (it == "--assiocate-table") {
			assiocate_path = argv[i + 1];
			i++;
		}
		else if (it == "--redirect-table") {
			redirect_path = argv[i + 1];
			// Writes as [request]|[redirect_to]|[direct_send (1) or jumper (0)]
			i++;
		}
		else if (it == "--root-file") {
			defiles.push_back(argv[i + 1]);
			i++;
		}
		else if (it == "--no-display") {
			no_data_screen = 1;
		}
		else if (it == "--no-auto-release") {
			auto_release = false;
		}
		else if (it == "--port") {
			portz = atoi(argv[i + 1]);
			i++;
		}
		else if (it == "--buffer") {
			tbuf = atoi(argv[i + 1]);
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
		else if (it == "--user-operate") {
			string op = argv[i + 1];
			if (op == "--add") {
				int ureq = atoi(argv[i + 2]);
				string pwd = argv[i + 3];
				user_operator::adduser(ureq, pwd);
				i += 3;
			}
			else if (op == "--query") {
				int ureq = atoi(argv[i + 2]);
				string res = user_operator::quser(ureq);
				if (res.length()) {
					cout << "Password MD5: " << res << endl;
				}
				else {
					cout << "User not exist" << endl;
				}
				i += 2;
			}
			else if (op == "--set") {
				int ureq = atoi(argv[i + 2]);
				string pwd = argv[i + 3];
				user_operator::moduser(ureq, pwd);
				i += 3;
			}
			else if (op == "--chfown") {
				int uto = atoi(argv[i + 2]);
				string file = argv[i + 3];
				user_operator::chown(file, uto);
				i += 2;
			}
			else if (op == "--chfperm") {
				int uto = atoi(argv[i + 2]);
				int pto = atoi(argv[i + 3]);
				string file = argv[i + 4];
				user_operator::chperm(file, uto, pto);
				i += 3;
			}
			return 0;
		}
		else if (it == "--help") {
		cout << readAll("help.txt").toString() << endl;
		return 0;
		}
	}
	/*
	FILE *f = fopen(public_file_path.c_str(), "r"); 
	if (f != NULL) {
		while (!feof(f)) {
			// buf uses begin
			fgets(buf, MAX_PATH, f);
			pub_fn.insert(buf);
			// buf uses end
		}
		fclose_m(f);
	}
	*/
	ssocket s = ssocket(portz, tbuf);
	if (!s.vaild()) {
		cout << "Can't bind or listen!" << endl;
		exit(1);
	}
	cout << "Loading assiocation..." << endl;
	
	FILE *fa = fopen(assiocate_path.c_str(), "r");
	if (fa != NULL) {
		while (!feof(fa)) {
			// buf, buf2 uses begin
			fscanf(fa, "%s%s", buf, buf2);	// DLLPath Extension
			HINSTANCE h = LoadLibrary(buf);
			if (h == NULL) {
				FreeLibrary(h);
				cout << "Unable to assiocate: " << buf2 << " for " << buf << endl;
				failure = true;
			}
			else {
				as_func df = (as_func)GetProcAddress(h, "AssiocateMain");
				acaller[buf2] = df;
				aldr++;
				// Seemed to be not able to free
			}
			// buf, buf2 uses end
		}
		fclose(fa);
	}
	
	if (failure) {
		system("pause");	// Press any key to continue . . .
	}
	cout << "* Listening started at port " << portz << " *" << endl;
	bytes bs;
	//volatile char leak_detector[] = { "TESTtestTESTtestTESTtestTESTtestTESTtest" };
	
	s.accepts([&](http_recv &hinfo, bytes &raw, const char *p_addr) -> http_send {
		post_infolist.clear();
		visit[s.get_paddr()]++;
		if (c_ftoken_usage() <= 0.0 || c_utoken_usage() <= 0.0) downgraded = true;
		string path = hinfo.path, rpath;
		path_pinfo = hinfo.toPaths();

		sndinfo.codeid = 200;
		sndinfo.code_info = "OK";
		sndinfo.proto_ver = hinfo.proto_ver;
		sndinfo.attr.clear();
		sndinfo.attr["Connection"] = "Keep-Alive";
		sndinfo.content.release();

		send_temp.release();

		bool flag;

		if (downgraded) {
			// A server downgrade is because the limit of fopen().
			if (auto_release) {
				for (auto &i : file_token) {
					fclose_m(i.second);
				}
				file_token.clear();
				downgraded = false;
				al_cause = true;
			}
			else {
				sndinfo.codeid = 500;
				sndinfo.code_info = "Internal Server Error";
				sndinfo.proto_ver = hinfo.proto_ver;
				goto sendup;
			}

		}

		if (path_pinfo.path.size() == 1 && operations.count(path_pinfo.path[0])) {
			if (path_pinfo.path[0] == "file_operate") {
				// It's not necessary to change to command format. Why not directly fopen()?
				// File operation requestion
				string op = path_pinfo.exts["operate"];
				if (op == "open") {
					// Provides:
					// name		- Filename.
					// utoken	- User token logged on.
					// type		- Open-type like "r".
					// @return	- 403 Forbidden / File token

					// As "File-in-token".
					if (resolveMinorPath(hinfo.path).first.find('$') != string::npos) {
						sndinfo.codeid = 403;
						sndinfo.code_info = "Forbidden";
					}
					else {
						// Open file, token returns
					// (Here we requires permissions)
						int utoken;
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
						// Begin of having uid, filename and type
						int tr = file_operator::open(suid, path_pinfo.exts["name"], path_pinfo.exts["type"]);
						if (tr < 0) {
							string msg = "";
							tr = 0 - tr;
							switch (tr) {
							case 400:
								msg = "Permission denied";
								break;
							case 500:
								msg = "Internal server error";
								break;
							default:
								break;
							}
							sndinfo.codeid = tr;
							sndinfo.code_info = msg;
						}
						else {
							sndinfo.content = to_string(tr);
						}
						// End...

					}
				}
				else if (op == "close") {
					// Close file
					// (Release handles)
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (!file_operator::release(tk)) {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
				}
				else if (op == "read") {
					// Read line
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (file_token.count(tk)) {
						file_structure fk = file_token[tk];
						if (!fk.readable()) {
							sndinfo.codeid = 400;
							sndinfo.code_info = "Bad request";
						}
						else try {
							sndinfo.content = CReadLine(fk);
						}
						catch (...) {
							sndinfo.codeid = 500;
							sndinfo.code_info = "Internal server error";
							// At the end of file
							//file_operator::release(tk);
						}

					}
					else {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}

				}
				else if (op == "write") {
					// Write line
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (file_token.count(tk)) {
						if (!file_token[tk].writeable()) {
							sndinfo.codeid = 400;
							sndinfo.code_info = "Bad request";
						}
						else try {
							fputs(hinfo.content.toCharArray(), file_token[tk]);
						}
						catch (...) {
							sndinfo.codeid = 500;
							sndinfo.code_info = "Internal server error";
						}
					}
					else {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}

				}
				else if (op == "eof") {
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (file_token.count(tk)) {
						sndinfo.content = int(feof(file_token[tk])) + '0';
					}
					else {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
				}
				else {
					sndinfo.codeid = 501;
					sndinfo.code_info = "Not Implemented";
					sndinfo.content = bytes(not_supported);
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
						fclose_m(f);
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
					// Should check owner first
					// They're common:
					int uid, uperm, uresult;	// Here might causes warning -- It's ok
					bool flag = false;
					if (f != NULL) {
						while (!feof(f)) {
							fscanf(f, "%d %s %d", &uid, buf, &uperm);
							if (uperm == -1 && filename == buf) {
								if (uidctrl::uidof(token) == uid) {
									flag = true;
									break;
								}
							}
						}
						fclose_m(f);
					}
					if (!flag) {
						sndinfo.codeid = 403;
						sndinfo.code_info = "Forbidden";
					}
					else {
						user_operator::chown(filename, chto);
					}
				}
				else if (op == "chperm") {
					string filename = path_pinfo.exts["file"];
					int token = atoi(path_pinfo.exts["token"].c_str());	// Should be owner
					int chid = atoi(path_pinfo.exts["touid"].c_str());
					int chto = atoi(path_pinfo.exts["toperm"].c_str());
					// Get ownership first
					int uids = uidctrl::uidof(token);
					FILE *fo = fopen(perm_data_path.c_str(), "r");
					set<int> ok_groups = user_groups::query(uids);
					if (fo != NULL) {
						int uid, uperm, uresult;	// Here might causes warning -- It's ok
						bool flag = false;
						while (!feof(fo)) {
							fscanf(fo, "%d %s %d", &uid, buf, &uperm);
							if (uperm == -1) {
								// Ownership information
								if (uids == uid || ok_groups.count(uid)) {
									flag = true;
									break;
								}
							}
						}
						fclose_m(fo);
						if (!flag) {
							sndinfo.codeid = 403;
							sndinfo.code_info = "Forbidden";
						}
						else {
							user_operator::chperm(filename, chid, chto);
						}
						// End
					}
					else {
						sndinfo.codeid = 403;
						sndinfo.code_info = "Forbidden";
					}


				}
				else if (op == "create") {
					// Create user...
					int chto;
					if (path_pinfo.exts.count("id")) {
						chto = atoi(path_pinfo.exts["id"].c_str());
					}
					else {
						chto = user_operator::allocnew();
						sndinfo.content = to_string(chto);
					}

					string upasswd = path_pinfo.exts["passwd"];				// Changing to
					//MD5 upass_m = MD5(upasswd);
					bool state = false;
					if (user_operator::quser(chto).empty()) {
						// Just create
						user_operator::adduser(chto, upasswd);
					}
					else {
						// Modify
						int otoken = atoi(path_pinfo.exts["token"].c_str());
						if (uidctrl::uidof(otoken) != chto) {
							sndinfo.codeid = 403;
							sndinfo.code_info = "Forbidden";
							sndinfo.content.clear();					// Do not send new ID
						}
						else {
							user_operator::moduser(chto, upasswd);
						}
					}
					
				}
				else {
					sndinfo.codeid = 501;
					sndinfo.code_info = "Not Implemented";
					sndinfo.content = bytes(not_supported);
				}
			}
			else if (path_pinfo.path[0] == "uploader") {

				post_infolist = hinfo.toPost();
				int uids = 0;
				if (path_pinfo.exts.count("utoken"))
				{
					uids = uidctrl::uidof(atoi(path_pinfo.exts["utoken"].c_str()));
				}

				bool flag = false;
				for (auto &i : post_infolist) {
					// Knowledges: https://blog.csdn.net/devil_2009/article/details/8013356
					disp_info d = i.toDispInfo();
					int t = file_operator::open(uids, sRemovingQuotes(d.attr["name"]), "wb");
					if (t < 0) {
						sndinfo.codeid = -t;
						//							sndinfo.code_info = "Internal server error";
						flag = true;
						break;
					}
					i.saveContent(file_token[t]);
					file_operator::release(t);
				}

				if (!flag) {
					// buf4 and buf5 uses begin
					memset(buf4, 0, sizeof(buf4));
					if (path_pinfo.exts.count("jumpto")) {
						sprintf(buf4, redirect.c_str(), path_pinfo.exts["jumpto"].c_str(), encodeBytes(path_pinfo.exts["jumpto"]).c_str());
					}
					sprintf(buf5, ok.c_str(), buf4);
					sndinfo.content = buf5;
					// buf4 and buf5 uses end
				}

			}
			else if (path_pinfo.path[0] == "caller") {
				string md = path_pinfo.exts["module"];
				HINSTANCE h = LoadLibrary(sCurrDir(decodeHTMLBytes(md).toString()).c_str());
				d_func df = (d_func)GetProcAddress(h, "ServerMain");	// So uses as: const char* ServerMain(const char *receive)
				if (df == NULL) {
					FreeLibrary(h);
					cout << "Warning: Error " << GetLastError() << " while loading ServerMain() of library " << md << endl;
					sndinfo.codeid = 400;
					sndinfo.code_info = "Bad Request";
				}
				else {
					//sdata *s_prep = new sdata;
					sdata s_prep;
					// To be updated:
					s_prep.cal_lib = { uidctrl::request, uidctrl::vaild, uidctrl::uidof, uidctrl::release, c_user_auth, file_operator::release, c_file_open, c_memory_usage, c_utoken_usage, c_ftoken_usage, c_ip_health, user_groups::insert, user_groups::remove, c_ug_query, c_uo_mod, c_uo_chperm, c_uo_exists, ec403, ec404, ec501, ec200_ok, ec200_redirect };
					s_prep.mc_lib = { memory_manager::allocate, memory_manager::release };
					s_prep.m_error = dll_err;
					bytes b = move(s.get_prev());
					const char *tc = b.toCharArray();	//***Here uses too much memory***
					b.release();
					send_info ds;
					ds = df(tc, &s_prep);
					bytes bq;
					bq.add(ds.cdata, ds.len);	// Leaks somewhere. Probably here.
					cout_d << "Trans back: " << endl_d;
					cout_d << b.toCharArray() << endl_d;
					cout_d << "End" << endl_d;
					//s.sends(bq);
					delete[] tc;
					b.release();
					//bq.release();
					sndinfo.raw_sending = true;
					sndinfo.raw_send = move(bq);
					return move(sndinfo);
				}
			}
		sendup: return move(sndinfo);// s.sends(sndinfo);
		//	bs.release();
		//after_sentup: s.end_accept();
		//	s.release_prev();
		}
		else {
			//...
			auto t = resolveMinorPath(hinfo.path);
			return move(normalSender(t.first, t.second, raw));
		}
		//sndinfo.content.release();
		//hinfo.release();
		/*sendup: bs = sndinfo.toSender();
			s.sends(bs);
			bs.release();
		 after_sentup: s.end_accept();
			s.release_prev();*/
	}, m_stat);

	WSACleanup();
	s.end();

	// View memory state
#if MINSERVER_DEBUG == 4
	system("cls");
	_CrtDumpMemoryLeaks();
	//system("pause");
	CloseHandle(logHandle);
#endif

	return 0;
}
