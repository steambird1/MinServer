#include "framework.h"
#include <crtdbg.h>

// I love C++ ...
thread_local vector<__t_safe*> __t_safe::tstable;

 bytes::bytes()
{
	clear();
}

 bytes::bytes(string b)
 {
	 clear();
	 //(*this) += b;
	 add(b.c_str(), b.length());
 }

 bytes::bytes(const char * b)
 {
	 clear();
	 add(b, strlen(b));
 }

 bytes::bytes(char b)
 {
	 clear();
	 const char t[1] = { b };
	 add(t, 1);
 }

 bytes::bytes(const bytes & other)
 {
	 this->clear();
	 this->add(other.byte_space, other.len);
 }

 int bytes::decst = 0;

 bytes::~bytes()
 {
	 // Why do I still can't do that ...
	 //release();
	 decst++;
 }

void bytes::release()
 {
	if (this->byte_space != nullptr && this->len) {
		delete[] this->byte_space;
		this->byte_space = nullptr;
	 }
	 this->len = 0;
 }

 void bytes::clear()
{
	this->byte_space = nullptr;
	this->len = 0;
	 //release();
}

 void bytes::fill(char c)
{
	if (!this->len)
		return;
	memset(this->byte_space, c, sizeof(char)*this->len);
}

 void bytes::add(const char * bytes, size_t sz)
{
	size_t tl = this->len;
	this->realloc(tl + sz);
	memcpy(this->byte_space + tl, bytes, sizeof(char)*sz);
}

 void bytes::erase(size_t pos, size_t count)
 {
	 printf("Erase called");	// debugger !!
	 for (size_t i = pos; i < this->length(); i++) {
		 size_t target = pos + count;
		 this->byte_space[i] = byte_space[target];
	 }
	 realloc(length() - count);
 }

 void bytes::pop_back(size_t count)
 {
	 erase(length() - count, count);
 }

 char bytes::front()
 {
	 if (length())
		 return this->byte_space[0];
	 else
		 return 0;
 }

 char bytes::rear()
 {
	 if (length())
		 return byte_space[length() - 1];
	 return 0;
 }

 const char * bytes::toCharArray()
{
	//return this->byte_space;
	 if (this->len <= 0)
		 return "";
	 char *memspec = new char[this->len + 2];
	 memset(memspec, 0, sizeof(char)*this->len);
	 memspec[this->len] = char(0);
	 memcpy(memspec, this->byte_space, sizeof(char)*this->len);
	 return memspec;
}

 string bytes::toString()
{
	 if (this->len <= 0)
		 return "";
	return move(string(toCharArray()));
}

 size_t bytes::length()
{
	return this->len;
}

 void bytes::operator+=(string v)
{
	add(v.c_str(), v.length());
	//return *this;
}

 void bytes::operator+=(const bytes& b)
{
	add(b.byte_space, b.len);
	//return *this;
}

 void bytes::operator+=(char b)
{
	const char c[1] = { b };
	add(c, 1);
	//return *this;
}

 char & bytes::operator[](size_t pos)
 {
	 return this->byte_space[pos];
 }

 bytes::operator string()
 {
	 return toString();
 }

 void bytes::realloc(size_t sz)
{
	if (sz < this->len)
		return;
	char *bs_old = nullptr;
	if (this->len) {
		bs_old = new char[this->len + 1];
		memcpy(bs_old, this->byte_space, sizeof(char)*this->len);
		delete[] this->byte_space;		// Release old pointer, after copied
	}
	//release();
	this->byte_space = new char[sz+2];
	memset(this->byte_space, 0, sizeof(char)*sz);
	if (this->len) {
		memcpy(byte_space, bs_old, sizeof(char)*this->len);
		delete[] bs_old;
	}
	this->byte_space[sz] = char(0);
	this->len = sz;
}

 bytes operator+(const bytes& a, string v)
{
	 bytes ax = bytes(a);
	 ax.add(v.c_str(), v.length());
	 return move(ax);
	//return w += v;
}

 bytes operator+(const bytes& a, const bytes& b)
{
	 bytes ax = bytes(a);
	 ax.add(b.byte_space, b.len);
	 return move(ax);
}

 bytes operator+(const bytes& a, char b)
{
	 const char c[1] = { b };
	 bytes ax = bytes(a);
	 ax.add(c, 1);
	 return move(ax);
}

 bytes operator+(const bytes& a, const char * b)
 {
	 return a + string(b);
 }

 bool operator==(const bytes& a, const bytes& b)
 {
	 if (a.len != b.len)
		 return false;
	 for (size_t i = 0; i < a.len; i++)
		 if (a.byte_space[i] != b.byte_space[i])
			 return false;
	 return true;
 }

 bool operator==(const bytes &a, char b)
 {
	 if (a.len == 0)
		 return false;
	 return a.len == 1 && a.byte_space[0] == b;
 }

 bool operator==(const bytes &a, string b)
 {
	 if (a.len != b.length())
		 return false;
	 return b == a.byte_space;
 }

 WSADATA initalize_socket()
 {
	 WSADATA ws;
	 if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		 throw exception("Cannot start WSA");
	 }
	 return ws;
 }

 bytes resolveHTTPSymbols(string s)
 {
	 bytes b;
	 for (int it = 0; it < s.length(); it++) {
		 char &i = s[it];
		 if (i == '%') {
			 b += char(hex2dec(s.substr(it + 1, 2)));
			 it += 2;
		 }
		 else {
			 b += i;
		 }
	 }
	 return move(b);
 }

 ssocket::ssocket()
 {
	 sock_init();
 }

 ssocket::ssocket(SOCKET s)
 {
	 sock_init();
	 this->s = s;
 }

 ssocket::ssocket(int port, int rcvsz)
 {
	 sock_init(rcvsz);
	 binds(port);
	 listens();
 }

 bool ssocket::binds(int port)
 {
	 sockaddr_in sa;
	 sa.sin_family = AF_INET;
	 sa.sin_port = htons(port);
	 sa.sin_addr.S_un.S_addr = INADDR_ANY;
	 // As we included something special, we have to use this to call C socket binder
	 auto be = ::bind(this->s, (LPSOCKADDR)&sa, sizeof(sa));
	 this->errored = (be == SOCKET_ERROR);
	 return !(be == SOCKET_ERROR);
 }

 bool ssocket::listens(int backlog)
 {
	 auto be = listen(this->s, backlog);
	 this->errored = (be == SOCKET_ERROR);
	 return !(be == SOCKET_ERROR);
 }

 bool ssocket::accepts()
 {
	 int acsz = sizeof(this->acc);
	 this->ace = accept(this->s, (SOCKADDR *)&this->acc, &acsz);
	 this->acc_errored = (this->ace == INVALID_SOCKET);
	 return this->ace != INVALID_SOCKET;
 }

 bool ssocket::accepts(function<http_send(http_recv&, bytes&, const char*)> accept_f, function<void(void)> runner)
 {
	 while (true) {
		 runner();
		 int acsz = sizeof(this->acc);
		 this->ace = accept(this->s, (SOCKADDR*)&this->acc, &acsz);
		 this->acc_errored = (this->ace == INVALID_SOCKET);
		 acceptor p = acceptor(this->ace, this->acc);
		// thread_local http_recv h;
		 //thread_local http_send s;
		 thread t = thread([&]() {
			 http_recv h;
			 p.receive(h);
			 http_send s = move(accept_f(h, p.get_prev(), p.get_paddr()));
			 p.sends(s);
			 h.release();
			 s.content.release();
			 __t_safe::auto_release_thread();
		 });
		 t.detach();
	 }
	 return false;
 }

 bool ssocket::vaild()
 {
	 return (!errored) && this->s != INVALID_SOCKET;
 }

 bool ssocket::accept_vaild()
 {
	 return (!acc_errored) && ace != INVALID_SOCKET;
 }

 http_recv ssocket::receive()
 {
	 this->prev_recv.clear();	// May be unsafe ?
	 bytes b = raw_receive();
	 this->prev_recv += b;
	 //b = bytes();				// Re-initalize
	 http_recv h;
	 // Getting 1st line (surely can use string
	 // that will not contains '\0')
	 string s = b.toString();
	 vector<string> lf = splitLines(s.c_str());
	 if (lf.size() < 1)
		 return http_recv();
	 // 1st line for informations
	 vector<string> firstinf = splitLines(lf[0].c_str(), ' ');
	 // [HTTP/?.?] [GET...] [/]
	 if (firstinf.size() < 3)
		 return http_recv();
	 h.proto_ver = firstinf[2];
	 h.process = firstinf[0];
	 h.path = resolveHTTPSymbols(firstinf[1]);
	 vector<string>::iterator i;
	 size_t pos = lf[0].length();
	 while (b[pos] == '\r' || b[pos] == '\n') pos++;
	 printf_d("\n");
	 for (i = lf.begin() + 1; i != lf.end(); i++) {
		 if (i->empty()) {
			 printf_d("Content receive started from line previous: %s\n", (i - 1)->c_str());
			 break;
		 }
		 vector<string> af = splitLines(i->c_str(), ':', true);
		 if (af.size() < 2)
			 return http_recv();
		 h.attr[af[0]] = af[1];
		 pos += i->length();
		 while (b[pos] == '\r' || b[pos] == '\n') pos++;
		 /*printf("Previous 10 characters are: ");
		 // debuggers
		 for (size_t i = pos - 10; i <= pos; i++) printf("%c", b[i]);
		 printf("\nNext 10 characters' ASCII are: ");
		 for (size_t i = pos; i <= pos + 10; i++) printf("%c", b[i]);
		 printf("\n");
		 // end
		 */
	 }
	 if (!h.attr.count("Content-Length"))
	 {
		 b.release();
		 return move(h);
	 }
	 int l = atoi(h.attr["Content-Length"].c_str());
	 //for (; i != lf.end(); i++) {
		 // Oh! We can't do that.
		 //h.content += ((*i) + '\n');
		 //l -= (i->length() + 1);
	 //}
	 printf_d("Additional receiving:(ASCII=%d, pos=%zd, l=%d)",b[pos],pos,l);				// debuggin
	 printf_d("Contented informations:\n");
	 int lres = 0;	// Might be <0 !!!								// Do NOT use signed int!!!
	 for (size_t i = pos; i < min(l + pos, b.length()); i++) {
		 h.content += b[i];
		 lres++;
		 //printf("{%d->%d}%d ",i,l+pos,b[i]);								// debugging
		 printf_d("%c", b[i]);
	 }

	 printf_d("Another raw receive for contents remaining:\n");
	 // BUT, FOR CONTENT
	 // WE HAVE TO GET MORE
	 printf_d("Less: %d\n", lres);
	// To save memory
	 int r = 0;
	 for (int i = 0; i < l - lres; i += r) {
		 r = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
		 if (r > 0) {
			 h.content.add(this->recv_buf, r);
			 this->prev_recv.add(this->recv_buf, r);
		 }
	 }
	 // As for non-external document promises full
	 /*
	 FILE *f = fopen("promise.gif", "wb");
	 fwrite(h.content.toCharArray(), sizeof(char), h.content.length(), f);
	 fclose(f);*/
	 b.release();
	 return move(h);
 }

 void ssocket::receive(http_recv &h)
 {
	 this->prev_recv.clear();	// May be unsafe ?
	 bytes b = raw_receive();
	 this->prev_recv += b;
	 //b = bytes();				// Re-initalize
	 
	 // Getting 1st line (surely can use string
	 // that will not contains '\0')
	 string s = b.toString();
	 vector<string> lf = splitLines(s.c_str());
	 if (lf.size() < 1)
		 return;
	 // 1st line for informations
	 vector<string> firstinf = splitLines(lf[0].c_str(), ' ');
	 // [HTTP/?.?] [GET...] [/]
	 if (firstinf.size() < 3)
		 return;
	 h.proto_ver = firstinf[2];
	 h.process = firstinf[0];
	 h.path = resolveHTTPSymbols(firstinf[1]);
	 vector<string>::iterator i;
	 size_t pos = lf[0].length();
	 while (b[pos] == '\r' || b[pos] == '\n') pos++;
	 printf_d("\n");
	 for (i = lf.begin() + 1; i != lf.end(); i++) {
		 if (i->empty()) {
			 printf_d("Content receive started from line previous: %s\n", (i - 1)->c_str());
			 break;
		 }
		 vector<string> af = splitLines(i->c_str(), ':', true);
		 if (af.size() < 2)
			 return;
		 h.attr[af[0]] = af[1];
		 pos += i->length();
		 while (b[pos] == '\r' || b[pos] == '\n') pos++;
		 /*printf("Previous 10 characters are: ");
		 // debuggers
		 for (size_t i = pos - 10; i <= pos; i++) printf("%c", b[i]);
		 printf("\nNext 10 characters' ASCII are: ");
		 for (size_t i = pos; i <= pos + 10; i++) printf("%c", b[i]);
		 printf("\n");
		 // end
		 */
	 }
	 if (!h.attr.count("Content-Length"))
	 {
		 b.release();
		 return;
	 }
	 int l = atoi(h.attr["Content-Length"].c_str());
	 //for (; i != lf.end(); i++) {
		 // Oh! We can't do that.
		 //h.content += ((*i) + '\n');
		 //l -= (i->length() + 1);
	 //}
	 printf_d("Additional receiving:(ASCII=%d, pos=%zd, l=%d)", b[pos], pos, l);				// debuggin
	 printf_d("Contented informations:\n");
	 int lres = 0;	
	 for (size_t i = pos; i < min(l + pos, b.length()); i++) {
		 h.content += b[i];
		 lres++;
		 //printf("{%d->%d}%d ",i,l+pos,b[i]);								// debugging
		 printf_d("%c", b[i]);
	 }

	 printf_d("Another raw receive for contents remaining:\n");
	 // BUT, FOR CONTENT
	 // WE HAVE TO GET MORE
	 printf_d("Less: %d\n", lres);
	 // To save memory
	 int r = 0;
	 for (int i = 0; i < l - lres; i += r) {
		 r = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
		 if (r > 0) {
			 h.content.add(this->recv_buf, r);
			 this->prev_recv.add(this->recv_buf, r);
		 }
	 }
	 // As for non-external document promises full
	 /*
	 FILE *f = fopen("promise.gif", "wb");
	 fwrite(h.content.toCharArray(), sizeof(char), h.content.length(), f);
	 fclose(f);*/
	 b.release();
 }

 bool ssocket::sends(bytes& data)
 {
	 const char* dc = data.toCharArray();
	 bool t = (send(this->ace,dc, data.length(), 0) != SOCKET_ERROR);
	 delete[] dc; //?
	 //data.release();	// It's a copy now
	 return t;
 }

 bool ssocket::sends(http_send& sender)
 {
	 if (sender.raw_sending) {
		 return this->sends(sender.raw_send);
	 }
	 bytes b = sender.proto_ver + " " + to_string(sender.codeid) + " " + sender.code_info + "\n";
	 sender.attr["Content-Length"] = to_string(sender.content.length());
	 //	 for (auto i = attr.begin(); i != attr.end(); i++)
	 //		 b += (i->first + ": " + i->second) + "\n";
	 for (auto &i : sender.attr) {
		 b += (i.first + ": " + i.second + "\n");
	 }
	 b += '\n';
	 b += sender.content;
	 const char *d = b.toCharArray();
	 bool t = (send(this->ace, d, b.length(), 0) != SOCKET_ERROR);
	 delete[] d;
	 b.release();
	 return t;
 }

 void ssocket::end_accept()
 {
	 this->acc_errored = true;
	 closesocket(this->ace);
 }

 void ssocket::end()
 {
	 this->errored = true;
	 closesocket(this->s);
 }

 bytes& ssocket::get_prev()
 {
	 return this->prev_recv;
 }

 void ssocket::release_prev()
 {
	 this->prev_recv.release();
 }

 const char * ssocket::get_paddr()
 {
	 return inet_ntoa(this->acc.sin_addr);
 }

 void ssocket::sock_init(int rcvsz)
 {
	 this->prev_recv.clear();
	 this->errored = false;
	 static bool initalized = false;	// Initalize state
	 if (!initalized) {
		 initalize_socket();
		 initalized = true;
	 }
	 this->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	 this->recv_buf = new char[rcvsz];
	 this->rcbsz = rcvsz;
	 memset(this->recv_buf, 0, sizeof(this->recv_buf));
 }

 bytes ssocket::raw_receive()
 {
	 int ret = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
	 this->last_receive = ret;
	 if (ret > 0) {
		 bytes b;
		 b.add(this->recv_buf, ret);
		 printf_d("Receive: Received data:\n%s\n==END==\n", b.toCharArray());
		 return move(b);
	 }
	 else
		 return bytes();
 }

 void http_recv::release()
 {
	 this->path.release();
	 this->content.release();
	 this->process = "";
	 this->proto_ver = "";
	 this->attr.clear();
 }

 path_info http_recv::toPaths()
 {
	 // Please notice that '?' was included in path
	 string tmp = "", tkey = "";
	 int mode = 0; // 'true' to after '?'; '2' to get values
	 vector<string> v;
	 map<string, string> r;
	 string wpath = this->path.toString();
	 wpath.erase(wpath.begin()); // beginning '/'
	 for (auto i = wpath.begin(); i != wpath.end(); i++) {
		 switch (*i) {
		 case '?':
			 if (!mode && tmp.length()) {
				 v.push_back(tmp);
				 tmp = "";
			 }
			 if (!mode) {
				 mode = 1;
				 break;
			 }
			 // Actually continue to push
		 case '/':
			 if (!mode && tmp.length()) {
				 v.push_back(tmp);
				 tmp = "";
			 }
			 else {
				 tmp += (*i);
			 }
			 break;
		 case '=':
			 if (mode) {
				 tkey = tmp;
				 tmp = "";
				 mode = 2;
			 }
			 else {
				 tmp += (*i);
			 }
			 break;
		 case '&':
			 if (mode == 2) {
				 r[tkey] = tmp;
				 tmp = "";
				 mode = 1;
			 }
			 else {
				 tmp += (*i);
			 }
			 break;
		 default:
			 tmp += (*i);
		 }
	 }
	 if (tmp.length()) {
		 if (mode) {
			 r[tkey] = tmp;
		 }
		 else {
			 v.push_back(tmp);
		 }
	 }
	 wpath.insert(wpath.begin(), '/'); // add back, It's not necessary
	 return { v, r };
 }

 content_info http_recv::toCType()
 {
	 if (!this->attr.count("Content-Type"))
		 return content_info();
	 vector<string> s = splitLines(this->attr["Content-Type"].c_str(), ';', false, ' ');
	 // To get boundary...
	 vector<string> bs;
	 string bsz;
	 switch (s.size()) {
	 case 1:
		 return { this->attr["Content-Type"], "" };
	 case 2:
		 //return { s[0], s[1] };
		 bs = splitLines(s[1].c_str(), '=', true);
		 bsz = bs.size() >= 2 ? bs[1] : "";
		 while (bsz.length() && bsz[0] == '-') bsz.erase(bsz.begin());
		 return { s[0], bsz };
	 default:
		 return content_info();
	 }
 }

 vector<post_info> http_recv::toPost()
 {
	 // Files may contains special things...
	 const char *c = this->content.toCharArray();
	 size_t l = this->content.length();
	 string ba = toCType().boundary;
	 if (ba == "") {
		 return vector<post_info>();
	 }
	 bytes tmp;
	 int state = 0;	// 0 -- Normal data.
					// 1 -- Args.
	
	 post_info p;
	 vector<post_info> t;
	 bool flag = true;
	 for (size_t i = 0; i < l; i++) {
		 if (c[i] == '\n') {
			 // debug
			 printf_d("Debugger: state=%d, tmp: %s\n", state, tmp.toCharArray());
			 // end
			 bool wflag = false;
			 string s = tmp.toString();
			 //if (!s.empty()) s.pop_back();	// Here wasn't this kind of thing ('r').
			 while (s.length() && (s[s.length() - 1] == '\n' || s[s.length() - 1] == '\r')) s.pop_back();
			 while (s.length() && s[0] == '-') s.erase(s.begin());
//			 while (s.length() && s[s.length() - 1] == '-') s.pop_back();
			 if (s.length() > 2 && s.substr(s.length() - 2) == "--") {
				 printf_d("EOB Checking...\n");
				 s = s.substr(0, s.length() - 2);
				 printf_d("EOB Info: \"%s\"\nEO Bound: \"%s\"", s.c_str(), ba.c_str());
				 if (s == ba) break;	// End of processing already
			 }
			 //printf_d("Debugger: s=\"%s\"\nDebugger:ba=\"%s\"\n", s.c_str(),ba.c_str());
			 if (s == ba && state == 0) {
				 // Start boundary execution
				 state = 1;
				 // Clear too much end-lines
				 //if (p.content.length()) p.content.pop_back();
				 t.push_back(p);
				 p = post_info();
				 p.boundary = tmp.toString();
				 tmp.clear();
				 continue;
			 }
			 else if (state == 1) {
				 if (s == "") {
					 state = 0;
					 tmp.clear();
					 continue;
				 }
				 vector<string> s = splitLines(tmp.toCharArray(), ':', true, ' ');
				 if (s.size() < 2)
					 continue;				// Probably 'r'
				 p.attr[s[0]] = s[1];
				 tmp.clear();
				 continue;
			 }
			 else {
				 p.content += (tmp + '\n');
				 tmp.clear();
				 continue;
			 }
		 }
		 else tmp += c[i];
	 }
	 t.erase(t.begin());		// Erase first unused information
	 //p.content += tmp;
	 t.push_back(p);
	 return t;
 }

 map<string, string> contentTypes()
 {
	 static map<string, string> ret = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };
	 return ret;
 }

 string searchTypes(string extension, string def)
 {
	 static map<string, string> ct = contentTypes();
	 if (!ct.count(extension))
		 return def;
	 return ct[extension];
 }

 long getFileLength(FILE * f)
 {
	 if (f == NULL) return 0;
	 long fb = ftell(f);
	 fseek(f, 0, SEEK_END);
	 long res = ftell(f);
	 fseek(f, fb, SEEK_SET);	// Recover
	 return res;
 }

 vector<string> splitLines(const char *data, char spl, bool firstonly, char filter) {
	 size_t len = strlen(data);
	 string tmp = "";
	 vector<string> res;
	 bool flag = false;
	 for (size_t i = 0; i < len; i++) {
		 if (data[i] == spl && !(firstonly && flag)) {
			 flag = true;
			 res.push_back(tmp);
			 tmp = "";
		 }
		 else if (data[i] != filter) {
			 tmp += data[i];
		 }
	 }
	 if (tmp.length()) res.push_back(tmp);
	 return move(res);
 }

 // It requires open in write/append binary.
 void post_info::saveContent(FILE * hnd)
 {
	 fwrite(this->content.toCharArray(), sizeof(char), this->content.length(), hnd);
 }

 disp_info post_info::toDispInfo()
 {
	 // Content-Disposition
	 // processing
	 if (!this->attr.count("Content-Disposition")) {
		 return disp_info();
	 }
	 vector<string> s = splitLines(this->attr["Content-Disposition"].c_str(), ';', false, ' ');
	 if (s.size() < 2) {
		 if (s.size() < 1) {
			 return disp_info();
		 }
		 return { s[0], {} };
	 }
	 map<string, string> atz;
	 for (auto i = s.begin() + 1; i != s.end(); i++) {
		 vector<string> sa = splitLines(i->c_str(), '=');
		 if (sa.size() != 2)
			 continue;
		 atz[sa[0]] = sa[1];
	 }
	 return { s[0], atz };
 }

 void http_send::loadContent(FILE *hnd)
 {
	 long len = getFileLength(hnd);
	 char *c = new char[len + 1];
	 fread(c, sizeof(char), len, hnd);
	 this->content.clear();
	 this->content.add(c, len);
	 delete[] c;
 }

 int hex2dec(string s) {
	 int t = 1, u = 0;
	 while (s.length()) {
		 char c = s[s.length() - 1];
		 s.pop_back();
		 if (c >= 'a' && c <= 'z') c = toupper(c);
		 if (c >= 'A' && c <= 'F') u += (10 + (c - 'A')) * t;
		 else u += (c - '0') * t;
		 t *= 16;
	 }
	 return u;
 }

 ssocket::acceptor::acceptor(SOCKET a, sockaddr_in sa, int recv_buf)
 {
	 this->ace = a;
	 this->acc = sa;
	 this->acc_errored = (a == INVALID_SOCKET);
	 this->rcbsz = recv_buf;
	 this->recv_buf = new char[recv_buf];
 }

 void ssocket::acceptor::receive(http_recv & h)
 {
	 this->prev_recv.clear();	// May be unsafe ?
	 bytes b = raw_receive();
	 this->prev_recv += b;
	 //b = bytes();				// Re-initalize

	 // Getting 1st line (surely can use string
	 // that will not contains '\0')
	 string s = b.toString();
	 vector<string> lf = splitLines(s.c_str());
	 if (lf.size() < 1)
		 return;
	 // 1st line for informations
	 vector<string> firstinf = splitLines(lf[0].c_str(), ' ');
	 // [HTTP/?.?] [GET...] [/]
	 if (firstinf.size() < 3)
		 return;
	 h.proto_ver = firstinf[2];
	 h.process = firstinf[0];
	 h.path = resolveHTTPSymbols(firstinf[1]);
	 vector<string>::iterator i;
	 size_t pos = lf[0].length();
	 while (b[pos] == '\r' || b[pos] == '\n') pos++;
	 printf_d("\n");
	 for (i = lf.begin() + 1; i != lf.end(); i++) {
		 if (i->empty()) {
			 printf_d("Content receive started from line previous: %s\n", (i - 1)->c_str());
			 break;
		 }
		 vector<string> af = splitLines(i->c_str(), ':', true);
		 if (af.size() < 2)
			 return;
		 h.attr[af[0]] = af[1];
		 pos += i->length();
		 while (b[pos] == '\r' || b[pos] == '\n') pos++;
	 }
	 if (!h.attr.count("Content-Length"))
	 {
		 b.release();
		 return;
	 }
	 int l = atoi(h.attr["Content-Length"].c_str());
	 int lres = 0;
	 for (size_t i = pos; i < min(l + pos, b.length()); i++) {
		 h.content += b[i];
		 lres++;
		 //printf("{%d->%d}%d ",i,l+pos,b[i]);								// debugging
		 printf_d("%c", b[i]);
	 }
	 // To save memory
	 int r = 0;
	 for (int i = 0; i < l - lres; i += r) {
		 r = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
		 if (r > 0) {
			 h.content.add(this->recv_buf, r);
			 this->prev_recv.add(this->recv_buf, r);
		 }
	 }
	 // As for non-external document promises full
	 /*
	 FILE *f = fopen("promise.gif", "wb");
	 fwrite(h.content.toCharArray(), sizeof(char), h.content.length(), f);
	 fclose(f);*/
	 b.release();
 }

 bool ssocket::acceptor::sends(bytes & sender)
 {
	 const char* dc = sender.toCharArray();
	 bool t = (send(this->ace, dc, sender.length(), 0) != SOCKET_ERROR);
	 delete[] dc; //?
	 return t;
 }

 bool ssocket::acceptor::sends(http_send & sender)
 {
	 if (sender.raw_sending) {
		 return this->sends(sender.raw_send);
	 }
	 bytes b = sender.proto_ver + " " + to_string(sender.codeid) + " " + sender.code_info + "\n";
	 sender.attr["Content-Length"] = to_string(sender.content.length());
	 //	 for (auto i = attr.begin(); i != attr.end(); i++)
	 //		 b += (i->first + ": " + i->second) + "\n";
	 for (auto &i : sender.attr) {
		 b += (i.first + ": " + i.second + "\n");
	 }
	 b += '\n';
	 b += sender.content;
	 const char *d = b.toCharArray();
	 bool t = (send(this->ace, d, b.length(), 0) != SOCKET_ERROR);
	 delete[] d;
	 b.release();
	 return t;
 }

 void ssocket::acceptor::end_accept()
 {
	 this->acc_errored = true;
	 closesocket(this->ace);
 }

 bool ssocket::acceptor::accept_vaild()
 {
	 return !this->acc_errored;
 }

 bytes & ssocket::acceptor::get_prev()
 {
	 return this->prev_recv;
 }

 void ssocket::acceptor::release_prev()
 {
	 this->prev_recv.release();
 }

 const char * ssocket::acceptor::get_paddr()
 {
	 return inet_ntoa(this->acc.sin_addr);
 }

 bytes ssocket::acceptor::raw_receive()
 {
	 int ret = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
	 this->last_receive = ret;
	 if (ret > 0) {
		 bytes b;
		 b.add(this->recv_buf, ret);
		 printf_d("Receive: Received data:\n%s\n==END==\n", b.toCharArray());
		 return move(b);
	 }
	 else
		 return bytes();
 }

 void __t_safe::lock()
 {
		 tstable.push_back(this);
		 m.lock();
 }

 bool __t_safe::try_lock()
 {
	 return m.try_lock();
 }

 void __t_safe::unlock()
 {
	 m.unlock();
 }

 void __t_safe::auto_release_thread()
 {
	 for (auto &i : tstable) {
		 i->unlock();
	 }
 }
