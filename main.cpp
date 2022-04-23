#include "framework.h"
#include "util.h"
#include "sfs.h"
#include "md5.h"
#include <memory.h>
#include <iostream>
#include <set>
#include <psapi.h>
#define _CP_DEFINED
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
string dll_path = "$dlls.txt";
string ban_path = "$bans.txt";
string log_path = "$log.txt";
string adm_passwd_path = "$admpwd.txt";
int default_join_g = -1;

// Allocate ONCE
char buf4[4096], buf5[4096], buf6[4096];

int portz = 80;

map<string, int> visit;

// DLL LastError System
int *dll_err;
void setDLLError(int err) {
	(*dll_err) = err;
}

// MD5 supporting.
class quick_md5 {
public:
	static string get(string source) {
		return MD5(source).toString();
	}
};

basic_file_system myfs;
map<int, basic_file_system::file> file_token;

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

char buf[MAX_PATH],buf2[100];
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
	static int query(int utoken) {
		return uidctrl::uidof(utoken);
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
		myfs.release_file(file_token[tk]);
		file_token.erase(tk);
		return true;
	}
	const static int perm_denied = -400;	// Permission denied
	const static int op_err = -500;		// Error in operation

	static basic_file_system::file_operate get_operate(string operate) {
		int d = 0;
		switch (operate.length()) {
		case 0:
			d = -1;
			break;
		case 2:
			if (operate[1] == '+') d += 2;
		case 1:
			switch (operate[0]) {
			case 'r': case 'R':
				break;
			case 'w': case 'W':
				d += 1;
			case 'a': case 'A':
				d += 2;
			}
		}
		return basic_file_system::file_operate(d);
	}

	// Force open without confirming permission, only uses internally
	static int force_open(string filename, string operate, int token = -1) {
		int tk;
		if (token < 0) tk = findToken();
		else tk = token;
		basic_file_system::file f = myfs.get_file(filename, get_operate(operate));
		if (f.isInvaild()) {
			return -1;
		}
		else {
			file_token[tk] = f;
			return tk;
		}
	}
	// To abs() for sending code id
	static int open(int suid, string filename, string operate) {
		if (filename[0] == '\\' || filename[0] == '/') filename.erase(filename.begin());
//		filename = sCurrDir(filename);	// Or out of box
		filename = decodeHTMLBytes(filename);
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
			return force_open(filename, operate);
		}
	}
	// Empty means error
	static string query(int token) {
		if (file_token.count(token)) {
			return file_token[token].myname();
		}
		else {
			return "";
		}
	}
};

// C Supportings
extern "C" {
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
set<string> bans;

void stat() {
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
		printf("%15s		%4d (%.2lf %%)", i.ip.c_str(), i.vis, double(i.vis) / double(tot) * 100.0);
		if (bans.count(i.ip)) {
			printf(" [Banned]");
		}
		printf("\n");
		lat--;
	}
	printf("\n");
}

bool auto_release = true;

map<string, string> acaller;	// acaller symbols VBS path as calling
int max_recesuive = 50;

// To same memory, use once:
http_recv hinfo;
vector<post_info> post_infolist;
http_send sndinfo;
const set<string> operations = { "file_operate", "auth_workspace", "uploader", "caller", "administrative" };
path_info path_pinfo;

#pragma region(preparations)
const bytes not_found = not_found_c;
const bytes not_supported = not_supported_c;
const bytes no_perm = no_perm_c;
#pragma endregion

bytes send_temp = bytes();

string curr_ip = "";
string script_engine = "wscript";

string vbStrAndQuotes(string origin, string replacement = "q") {
	string res = "\"";
	for (auto &i : origin) {
		if (i == '"') {
			res += "\" & " + replacement + " & \"";
		}
		else {
			res += i;
		}
	}
	return res + '"';
}

void appendAllContent(FILE *target, string filename) {
	bytes b;
	b = readAll(filename);
	//fputs(b.toCharArray(), target);
	fprintf(target, "%s", b.toCharArray());	// Or here's another LF
	b.release();
}

// To change settings to this
map<string, string> preferences;

bool always_display_err = false;

struct vbs_result {
	bool hooked = false;	// true if script specifies HOOK
};

vbs_result ProcessVBSCaller(bytes &returned, string script_name) {
	vbs_result ret;
	ret.hooked = false;
	// 1. Prepare Args.
	// To be implemented...
	string vbs_tmp = makeTemp(".vbs");
	string send_tmp = makeTemp();	// To make temp to get information to send
	string cmd_tmp = makeTemp();	// Commands in this directory will be resolved.
	string err_tmp = makeTemp();
	// Simply tell where VBS to save it information, automaticly appends script.
	// Fill "PARA" (it uses before lib)
	
	// Save content IF NECESSARY
	string save_dest = " ";	// To be not empty ??
	if (hinfo.content.length()) {
		save_dest = makeTemp();
		FILE *f = fopen(save_dest.c_str(), "wb");
		auto hct = hinfo.content.toCharArray();
		fwrite(hct, sizeof(char), hinfo.content.length(), f);
		fclose(f);
	}
	
	// Initalize attr
	string para = readAll("mspara.vbs").toString();

	string attcode = "";
	for (auto &i : hinfo.attr) {
		attcode += "attr.add " + vbStrAndQuotes(i.first) + " , " + vbStrAndQuotes(i.second) + "\n";
	}

	string ufcode = "";
	for (auto &i : file_token) {
		ufcode += "ftokens.add " + to_string(i.first) + ", " + vbStrAndQuotes(i.second.myname()) + "\n";
	}
	for (auto &i : uidctrl::getmap()) {
		ufcode += "utokens.add " + to_string(i.first) + "," + to_string(i.second) + "\n";
	}

	string prefcode = "";
	for (auto &i : preferences) {
		prefcode += "srvsetting.add " + vbStrAndQuotes(i.first) + " , " + vbStrAndQuotes(i.second) + "\n";
	}

	FILE *fv = fopen(vbs_tmp.c_str(), "w");
	fprintf(fv, para.c_str(), hinfo.proto_ver.c_str(), hinfo.process.c_str(), vbStrAndQuotes(hinfo.path.toString()).c_str(), save_dest.c_str(), attcode.c_str(), curr_ip.c_str(), ufcode.c_str(), prefcode.c_str(), cmd_tmp.c_str(), err_tmp.c_str());

	// Write down LIB
	appendAllContent(fv, sCurrDir("mslib.vbs"));

	// Write down others
	appendAllContent(fv, script_name);

	// Finish entire program, in mslib.vbs
	fprintf(fv, "\nEndOfProgram()\n");

	fclose(fv);
	
	// 2. Call.
	string cmd = script_engine + " \"" + vbs_tmp + "\" " + send_tmp;
	system(cmd.c_str());

	// 3. Get return values.

	// Proceed on requests
	FILE *fc = fopen(cmd_tmp.c_str(), "r");
	if (fc != NULL) {
		while (!feof(fc)) {
			// buf6 uses begin
			memset(buf6, 0, 4096);
			fgets(buf6, 4096, fc);
			auto spl = splitLines(sRemovingEOL(buf6).c_str(), ' ');
			if (spl.size() < 1) continue;
#define argument_check(num) if (spl.size() < num) goto w_cont
			string &ord = spl[0];
			if (ord == "user_alloc") {
				argument_check(3);
				uidctrl::force_request(atoi(spl[1].c_str()), atoi(spl[2].c_str()));
			}
			else if (ord == "file_alloc") {
				argument_check(4);
				file_operator::force_open(spl[2], spl[3], atoi(spl[1].c_str()));
			}
			else if (ord == "hook") {
				ret.hooked = true;
			}
		w_cont: continue;
#ifdef argument_check
#undef argument_check
#endif
		}
		fclose(fc);
	}
	returned = readAll(send_tmp);
	if (returned.length() == 0) {
		returned = "HTTP/1.1 500 Internal Server Error\nConnection: close\nContent-Length: " + to_string(strlen(no_ret_d)) + "\n\n" + no_ret_d;
	}
	// If here's error
	bytes err_data = readAll(err_tmp);
	if (err_data.length()) {
		char *err_tmp = new char[strlen(err_ret_d) + 10 + max(strlen(err_ret_ndisp),err_data.length())];
		if (always_display_err || curr_ip == "127.0.0.1") {
			sprintf(err_tmp, err_ret_d, err_data.toCharArray());
		}
		else {
			sprintf(err_tmp, err_ret_d, err_ret_ndisp);
		}
		returned = err_tmp;
		err_data.release();
		delete[] err_tmp;
	}
	return move(ret);
}

set<string> ns_hooked;

void normalSender(ssocket &s, string path, string external, int recesuive = 0) {
	if (recesuive > max_recesuive) {
		sndinfo.codeid = 500;
		sndinfo.code_info = "Internal Server Error";
	}
	// As the normal processor
	if (hinfo.process == "POST")
		hinfo.toPost(post_infolist); // Can be safe only here
	pair<string, string> m;
	m.first = path;
	m.second = external;
	// Or redirection
	// As convert string to wstring take too long
	bool flag = false;
	string rpath = "";
	bool flag2 = false;
	// Hook simply before
	bytes hook_prep;
	FILE *f = nullptr;
	for (auto &i : ns_hooked) {
		hook_prep.clear();
		auto res = ProcessVBSCaller(hook_prep, i);
		if (res.hooked) {
			s.sends(hook_prep);
			goto after_sentup;
		}
	}
	f = fopen(public_file_path.c_str(), "r");
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
						normalSender(s, vs[1], external, recesuive + 1);
						fclose_m(fr);
						return;
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
				string dest = makeTemp();
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
							
							t += curr_ip.length();

							// Print
							cout_d << "Allocated length for buf: " << t + 20 << endl_d;
							char *buf = new char[t + 20];	// Also added ua/pa spaces
							//      Buffer|Template|Args -->
							sprintf(buf, s.c_str(), hinfo.process.c_str(), hinfo.proto_ver.c_str(), hp.first.c_str(), ha.c_str(), ua.c_str(), pa.c_str(), curr_ip.c_str());
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
					// To be focus...
					bytes vb_to_send;
					ProcessVBSCaller(vb_to_send, acaller[ex]);
					s.sends(vb_to_send);
					goto after_sentup;
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
sendup: s.sends(sndinfo);
sndinfo.content.release();
after_sentup: s.end_accept();
s.release_prev();
hook_prep.release();
	// Doesn't need to send in the end
}

bool vislog = true;

inline void postClear() {
	for (auto &i : post_infolist) {
		i.content.release();
	}
	post_infolist.clear();
}

string admpwd = "";

int main(int argc, char* argv[]) {
	//cout << "Running in directory: " << sCurrDir("example") << endl;
	const char *cbt = new char[60];
	int tbuf = RCV_DEFAULT;
//	cbt = c_boundary("text/html; boundary=------BoundaryInformationDataHereAAABBBCCCDDDEEEFFFGGG");
//	cout_d << "C-Boundary tester:" << cbt << endl_d;
	dll_err = new int;
	
#pragma region(Preparing Parameters)
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
		else if (it == "--dll-table") {
			dll_path = argv[i + 1];
			i++;
		}
		else if (it == "--ban-table") {
			ban_path = argv[i + 1];
			i++;
		}
		else if (it == "--adm-path") {
			adm_passwd_path = argv[i + 1];
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
		else if (it == "--log-target") {
			log_path = argv[i + 1];
			i++;
		} else if (it == "--no-visit-log") {
			vislog = false;
		}
		else if (it == "--script-engine") {
			script_engine = argv[i + 1];
			i++;
		}
		else if (it == "--always-display-err") {
			always_display_err = true;
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
		else if (it == "--admin") {
			string op = argv[i + 1];
			if (op == "--setpwd") {
				MD5 pwd = MD5(argv[i + 2]);
				FILE *f = fopen(adm_passwd_path.c_str(), "w");
				fprintf(f, "%s\n", pwd.toString().c_str());
				fclose(f);
				i += 1;
			}
			else if (op == "--getpwd") {
				FILE *f = fopen(adm_passwd_path.c_str(), "r");
				if (f == NULL) {
					cout << "Function disabled" << endl;
				}
				else {
					// buf2 uses begin
					fgets(buf2, 100, f);
					if (strlen(buf2) == 0) {
						cout << "Function disabled" << endl;
					}
					else {
						cout << "Administrative Password MD5: " << buf2 << endl;
					}
				}
			}
			else if (op == "--disable") {
				FILE *f = fopen(adm_passwd_path.c_str(), "w");
				fclose(f);	// Clear file data
			}
			i += 1;
			return 0;
		}
		else if (it == "--help") {
		cout << readAll("help.txt").toString() << endl;
		return 0;
		}
	}
#pragma endregion

#pragma region (Preparing Preference Map)
	/*
	string perm_data_path = "$permission.txt";
string user_data_path = "$users.txt";
string public_file_path = "$public.txt";
string group_path = "$groups.txt";
string assiocate_path = "$assiocate.txt";
string redirect_path = "$redirect.txt";
string dll_path = "$dlls.txt";
string ban_path = "$bans.txt";
string log_path = "$log.txt";
string adm_passwd_path = "$admpwd.txt";
int default_join_g = -1;
	*/
#define __preference_str_initalizer(var) preferences[#var] = var

	__preference_str_initalizer(perm_data_path);
	__preference_str_initalizer(user_data_path);
	__preference_str_initalizer(public_file_path);
	__preference_str_initalizer(group_path);
	__preference_str_initalizer(assiocate_path);
	__preference_str_initalizer(redirect_path);
	__preference_str_initalizer(dll_path);
	__preference_str_initalizer(ban_path);
	__preference_str_initalizer(log_path);
	__preference_str_initalizer(adm_passwd_path);

#ifdef __preference_str_initalizer
#undef __preference_str_initalizer
#endif
#pragma endregion

	//_setmaxstdio(50);	// For test only

	ssocket s = ssocket(portz, tbuf);
	if (!s.vaild()) {
		cout << "Can't bind or listen!" << endl;
		exit(1);
	}
	cout << "Loading auth info..." << endl;
	FILE *fd = fopen(adm_passwd_path.c_str(), "r");
	// buf2 uses begin
	if (fd != NULL) {
		fgets(buf2, 100, fd);
		admpwd = sRemovingEOL(buf2);
		fclose(fd);
	}
	// buf2 uses end
	cout << "Loading assiocation..." << endl;
	bool failure = false;
	FILE *fa = fopen(assiocate_path.c_str(), "r");
	if (fa != NULL) {
		while (!feof(fa)) {
			// To be focus, changed as JS
			// buf, buf2 uses begin
			fscanf(fa, "%s%s", buf, buf2);	// With '.'
			if (string(buf2) == "hook") {
				// hook all request (expect reserved)
				ns_hooked.insert(sCurrDir(buf));
			}
			else {
				acaller[buf2] = sCurrDir(buf);
			}
			aldr++;
			// buf, buf2 uses end
		}
		fclose(fa);
	}
	FILE *fb = fopen(ban_path.c_str(), "r");
	if (fb != NULL) {
		while (!feof(fb)) {
			// buf2 uses begin
			fgets(buf2, 100, fb);
			bans.insert(sRemovingEOL(buf2));
			// buf2 uses end
		}
		fclose(fb);
	}
	if (failure) {
		system("pause");	// Press any key to continue . . .
	}
	cout << "* Listening started at port " << portz << " *" << endl;
	bytes bs;
	bool downgraded = false, al_cause = false;
	// End
	if (vislog) {
		FILE *vl = fopen(log_path.c_str(), "a");
		fprintf(vl, "[Server execution begin]\n");
		fclose(vl);
	}
	while (true) {
		if (!no_data_screen) 
		{
			stat();
			cout << endl << "* Listening at port " << portz << " *" << endl;
		}
		if (al_cause) cout << endl << "* Auto-release runned" << endl;
		if (c_ftoken_usage() <= 0.0 || c_utoken_usage() <= 0.0) downgraded = true;
		s.accepts();
		if (!s.accept_vaild()) {
			s.end_accept();
			continue;
		}
		hinfo.content.release();
		s.receive(hinfo);
		postClear();
		string sp = s.get_paddr();
		curr_ip = sp;
		visit[sp]++;
		if (vislog) {
			FILE *vl = fopen(log_path.c_str(), "a");
			fprintf(vl, "[Visit] %s\n", sp.c_str());
			fclose(vl);
		}
		string path = hinfo.path.toString(), rpath;
		path_pinfo = hinfo.toPaths();

		sndinfo.codeid = 200;
		sndinfo.code_info = "OK";
		sndinfo.proto_ver = hinfo.proto_ver;
		sndinfo.attr.clear();
		//sndinfo.attr["Connection"] = "Keep-Alive";
		sndinfo.attr["Connection"] = "Close";	// To be confirmed
		sndinfo.content.release();

		send_temp.release();

		bool flag;

		if (downgraded) {
			// A server downgrade is because the limit of fopen().
			if (auto_release) {
				for (auto &i : file_token) {
					myfs.release_file(i.second);
				}
				myfs.sync(true);
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

		if (bans.count(sp)) {
			sndinfo.codeid = 403;
			sndinfo.code_info = "Forbidden";
			sndinfo.content = no_perm_d;
			goto sendup;
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
					int tk = atoi(decodeHTMLBytes(path_pinfo.exts["token"]).toCharArray());
					if (!file_operator::release(tk)) {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
				}
				else if (op == "read") {
					// Read line
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (file_token.count(tk)) {
						basic_file_system::file& fk = file_token[tk];
						if (!fk.readable()) {
							sndinfo.codeid = 400;
							sndinfo.code_info = "Bad request";
						}
						else try {
							sndinfo.content = fk.readLine();
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
				else if (op == "write") {
					// Write line
					int tk = atoi(path_pinfo.exts["token"].c_str());
					if (file_token.count(tk)) {
						if (!file_token[tk].writeable()) {
							sndinfo.codeid = 400;
							sndinfo.code_info = "Bad request";
						}
						else try {
							file_token[tk].write(hinfo.content);
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
						sndinfo.content = int(file_token[tk].eof()) + '0';
					}
					else {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
				}
				else if (op == "query") {
					string res = file_operator::query(atoi(path_pinfo.exts["token"].c_str()));
					if (res == "") {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
					else {
						sndinfo.content = res;
					}
				}
				else if (op == "list") {
					// List files, instead of FileLister DLL
					set<string> results;
					FILE *f = fopen(perm_data_path.c_str(), "r");
					while (!feof(f)) {
						// buf uses begin
						fscanf(f, "%*d%s%*d", buf);
						results.insert(buf);
					}
					string res = "";
					for (auto &i : results) {
						res += i + "\n";
					}
					sndinfo.content = res;
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
					string passwd = decodeHTMLBytes(path_pinfo.exts["passwd"]).toString();
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
					string filename = decodeHTMLBytes(path_pinfo.exts["file"]).toString();
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
					string filename = decodeHTMLBytes(path_pinfo.exts["file"]).toString();
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
					
					string upasswd = decodeHTMLBytes(path_pinfo.exts["passwd"]).toString();				// Changing to
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
					/*FILE *fd = fopen(user_data_path.c_str(), "r");
					while (!feof(fd)) {
						int u;
						fscanf(fd, "%d %*s", &u);
						if (u == chto) {
							state = true;
							break;
						}
					}
					fclose_m(fd);

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
							fclose_m(fd);
							fclose_m(de);
							CopyFile(dest.c_str(), user_data_path.c_str(), FALSE);
						}
					}
					else {
						FILE *f = fopen(user_data_path.c_str(), "a+");
						// Check for user-states and create
						fprintf(f, "%d %s\n", chto, upass_m.toString().c_str());
						fclose_m(f);
						user_groups::insert(chto, default_join_g);
					}
					// End
					//fclose_m(f);*/
				}
				else if (op == "query") {
					int res = user_operator::query(atoi(path_pinfo.exts["token"].c_str()));
					if (res < 0) {
						sndinfo.codeid = 400;
						sndinfo.code_info = "Bad request";
					}
					else {
						sndinfo.content = to_string(res);
					}
				}
				else {
				sndinfo.codeid = 501;
				sndinfo.code_info = "Not Implemented";
				sndinfo.content = bytes(not_supported);
				}

				/*
				
				Groups-join and Groups-query can't be implemented or
				it'll be meaningless.
				
				*/

				// End of codes
				// Remove that:
				//sndinfo.codeid = 501;
				//sndinfo.code_info = "Not Implemented";
				//sndinfo.content = not_supported;
			}
			else if (path_pinfo.path[0] == "uploader") {

				/*sndinfo.codeid = 501;
				sndinfo.code_info = "Not Implemented";
				sndinfo.content = not_supported;
				*/

				hinfo.toPost(post_infolist);
				int uids = 0;
				if (path_pinfo.exts.count("utoken"))
				{
					uids = uidctrl::uidof(atoi(path_pinfo.exts["utoken"].c_str()));
				}

				bool flag = false;
					for (auto &i : post_infolist) {
						// Knowledges: https://blog.csdn.net/devil_2009/article/details/8013356
						disp_info d = i.toDispInfo();
						int t = file_operator::open(uids,sRemovingQuotes(d.attr["name"]), "wb");
						if (t < 0) {
							sndinfo.codeid = -t;
//							sndinfo.code_info = "Internal server error";
							flag = true;
							break;
						}
						//i.saveContent(file_token[t]);
						file_token[t].write(i.content);
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
			string md = decodeHTMLBytes(path_pinfo.exts["module"]).toString();
			string fs = sCurrDir(md);
			// First of all search
			FILE *fq = fopen(dll_path.c_str(), "r");
			bool flag = false;
			if (fq != NULL) {
				while (!feof(fq)) {
					// buf uses begin
					fgets(buf, MAX_PATH, fq);
					if (sRemovingEOL(buf) == md) {
						flag = true;
						break;
					}
					// buf uses end
				}
				fclose_m(fq);
			}
			if (!flag) {
				sndinfo.codeid = 403;
				sndinfo.code_info = "Forbidden";
				goto sendup;
			}
				// To call VBS here, to focus
			bytes ret;
			ProcessVBSCaller(ret, fs);
			s.sends(ret);
					goto after_sentup;
				
}
			else if (path_pinfo.path[0] == "administrative") {
			string pwd = path_pinfo.exts["password"];
			MD5 m = MD5(pwd);
			if (m.toString() == admpwd) {
				string op = path_pinfo.exts["operate"];
				if (op == "publish") {
					if (path_pinfo.exts.count("file")) {
						FILE *f = fopen(public_file_path.c_str(), "a");
						fprintf(f, "%s\n", decodeHTMLBytes(path_pinfo.exts["file"]).toCharArray());
						fclose(f);
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
			else {
				sndinfo.codeid = 403;
				sndinfo.code_info = "Forbidden";
			}
}
sendup: s.sends(sndinfo);
bs.release();
after_sentup: s.end_accept();
s.release_prev();
		}
		else {
		//...
		auto t = resolveMinorPath(hinfo.path);
		normalSender(s, t.first, t.second);
		}
		sndinfo.content.release();
		hinfo.release();
		hinfo.content.release();
		postClear();
		_fcloseall();
	/*sendup: bs = sndinfo.toSender();
		s.sends(bs);
		bs.release();
	 after_sentup: s.end_accept();
		s.release_prev();*/
	}

	WSACleanup();
	s.end();

	return 0;
}
