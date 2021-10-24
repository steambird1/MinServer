#include "framework.h"

 bytes::bytes()
{
	clear();
}

 bytes::bytes(string b)
 {
	 clear();
	 (*this) += b;
 }

 bytes::bytes(const char * b)
 {
	 (*this) += string(b);
 }

 bytes::bytes(char b)
 {
	 clear();
	 (*this) += b;
 }

 bytes::bytes(const bytes & other)
 {
	 this->clear();
	 this->add(other.byte_space, other.len);
 }

 void bytes::clear()
{
	this->byte_space = nullptr;
	this->len = 0;
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
	 char *memspec = new char[this->len + 1];
	 memset(memspec, 0, sizeof(char)*this->len + sizeof(char));
	 memcpy(memspec, this->byte_space, sizeof(char)*this->len);
	 return memspec;
}

 string bytes::toString()
{
	 if (this->len <= 0)
		 return "";
	return string(toCharArray());
}

 size_t bytes::length()
{
	return this->len;
}

 bytes bytes::operator+=(string v)
{
	add(v.c_str(), v.length());
	return *this;
}

 bytes bytes::operator+=(bytes b)
{
	add(b.byte_space, b.len);
	return *this;
}

 bytes bytes::operator+=(char b)
{
	const char c[1] = { b };
	add(c, 1);
	return *this;
}

 char & bytes::operator[](size_t pos)
 {
	 return this->byte_space[pos];
 }

 void bytes::realloc(size_t sz)
{
//	if (sz < this->len)
//		return;
	char *bs_old = new char[this->len + 1];
	if (this->len) {
		memcpy(bs_old, this->byte_space, sizeof(char)*this->len);
	}
	this->byte_space = new char[sz];
	memset(this->byte_space, 0, sizeof(char)*sz);
	if (this->len) {
		memcpy(byte_space, bs_old, sizeof(char)*this->len);
	}
	this->len = sz;
}

 bytes operator+(bytes a, string v)
{
	bytes w = a;
	return w += v;
}

 bytes operator+(bytes a, bytes b)
{
	bytes w = a;
	return w += b;
}

 bytes operator+(bytes a, char b)
{
	bytes w = a;
	return w += b;
}

 bytes operator+(bytes a, const char * b)
 {
	 return a + string(b);
 }

 bool operator==(bytes a, bytes b)
 {
	 if (a.length() != b.length())
		 return false;
	 for (size_t i = 0; i < a.length(); i++)
		 if (a[i] != b[i])
			 return false;
	 return true;
 }

 bool operator==(bytes a, char b)
 {
	 if (a.length() == 0)
		 return false;
	 return a.length() == 1 && a[0] == b;
 }

 bool operator==(bytes a, string b)
 {
	 return a.toString() == b;
 }

 WSADATA initalize_socket()
 {
	 WSADATA ws;
	 if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		 throw exception("Cannot start WSA");
	 }
	 return ws;
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
	 auto be = bind(this->s, (LPSOCKADDR)&sa, sizeof(sa));
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
	 bytes b = raw_receive();
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
	 h.path = firstinf[1];
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
		 return h;
	 int l = atoi(h.attr["Content-Length"].c_str());
	 //for (; i != lf.end(); i++) {
		 // Oh! We can't do that.
		 //h.content += ((*i) + '\n');
		 //l -= (i->length() + 1);
	 //}
	 printf_d("Additional receiving:(ASCII=%d, pos=%zd, l=%d)",b[pos],pos,l);				// debuggin
	 printf_d("Contented informations:\n");
	 for (size_t i = pos; i < l + pos; i++) {
		 h.content += b[i];
		 //printf("{%d->%d}%d ",i,l+pos,b[i]);								// debugging
		 printf_d("%c", b[i]);
	 }
	 printf_d("Another raw receive for contents remaining:\n");
	 // BUT, FOR CONTENT
	 // WE HAVE TO GET MORE
	 for (int i = 0; i < l; i += this->rcbsz) {
		 h.content += raw_receive();
	 }
	 printf_d("End\n");										// debugging
	 return h;
 }

 bool ssocket::sends(bytes data)
 {
	 return send(this->ace, data.toCharArray(), data.length(), 0) != SOCKET_ERROR;
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

 void ssocket::sock_init(int rcvsz)
 {
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
	 if (ret > 0) {
		 bytes b;
		 b.add(this->recv_buf, ret);
		 printf_d("Receive: Received data:\n%s\n==END==\n", b.toCharArray());
		 return b;
	 }
	 else
		 return bytes();
 }

 path_info http_recv::toPaths()
 {
	 // Please notice that '?' was included in path
	 string tmp = "", tkey = "";
	 int mode = 0; // 'true' to after '?'; '2' to get values
	 vector<string> v;
	 map<string, string> r;
	 path.erase(path.begin()); // beginning '/'
	 for (auto i = path.begin(); i != path.end(); i++) {
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
	 path.insert(path.begin(), '/'); // add back
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
			 while (s.length() && s[0] == '-') s.erase(s.begin());
			 while (s.length() && s[s.length() - 1] == '-') s.pop_back();
			 //printf("Debugger: s=\"%s\"\nDebugger:ba=\"%s\"\n", s.c_str(),ba.c_str());
			 if (s == ba && state == 0) {
				 // Start boundary execution
				 state = 1;
				 // Clear too much end-lines
				 if (p.content.length()) p.content.pop_back();
				 t.push_back(p);
				 p = post_info();
				 p.boundary = tmp.toString();
				 tmp.clear();
				 continue;
			 }
			 else if (state == 1) {
				 if (tmp.toString() == "") {
					 state = 0;
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
		 else if (c[i] == '\r') {
			 // Ignoring this kind of thing
			 continue;
		 }
		 tmp += c[i];
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
	 return res;
 }

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
 }

 bytes http_send::toSender(bool autolen)
 {
	 bytes b = proto_ver + " " + to_string(codeid) + " " + code_info + "\n";
	 if (autolen) attr["Content-Length"] = to_string(this->content.length());
	 for (auto i = attr.begin(); i != attr.end(); i++)
		 b += (i->first + ": " + i->second) + "\n";
	 b += string("\n");
	 b += content;
	 return b;
 }