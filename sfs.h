#pragma once

#include <map>
#include "framework.h"
#include "sfs_util.h"
using namespace std;

class basic_file_system {
public:

	enum file_operate {
		invaild = -1,
		read = 0,
		overriding = 1,
		append = 2,
		ext_overriding = 3,
		ext_append = 4
	};

	struct file_node {

		file_node(bytes init = "", bool nosync = false) {
			this->data = init;
			this->nosync = nosync;
		}

		bytes data;
		int refcount = 0;
		bool nosync = false;
	};

	using fs_type = map<string, file_node>;

	class file {
	public:
		friend class basic_file_system;

		file() {
			this->op_type = invaild;
		}

		virtual bool isInvaild() {
			return op_type == invaild;
		}

		string myname() {
			return this->file_name;
		}

#define check_read() do { if (!readable()) return bytes(); } while (false)

		bool readable() {
			if (this->op_type != 0 && this->op_type != 3 && this->op_type != 4) return false;
			return true;
		}

		bool writeable() {
			if (this->op_type >= 1 && this->op_type <= 4) return true;
			return false;
		}

		bytes readAll() {
			check_read();
			return (*this->buffer)[file_name].data;
		}

		bytes readLine(char eol = '\n', int preallocate = 0) {
			check_read();
			bytes cont;
			if (preallocate > 0) cont.preallocate(preallocate);
			bytes &mycont = (*this->buffer)[file_name].data;
			for (size_t &i = myptr; i < mycont.length() && mycont[i] != eol; i++) {
				cont += mycont[i];
			}
			myptr++;
			return cont;
		}

		bool eof() {
			bytes &mycont = (*this->buffer)[file_name].data;
			return myptr >= mycont.length();
		}

		// Also a way to save memory
		bool write(bytes content) {
			bytes &mycont = (*this->buffer)[file_name].data;
			switch (op_type) {
			case 1: case 3:
				mycont = content;
				break;
			case 2: case 4:
				mycont += content;
				break;
			default:
				return false;
			}
			return true;
		}

	protected:
		file_operate op_type;		// 0 - Read, 1 - Override write, 2 - Append
		string file_name;
		fs_type *buffer;
		size_t myptr = 0;
	};

	class __invaild_file : public file {
	public:
		__invaild_file() {

		}
		virtual bool isInvaild() {
			return true;
		}
	} invaild_file;

	void auto_init_file(string name) {
		if (fileExists(name)) {
			init_file(name);
		}
		else {
			init_ramfile(name);
		}
	}

	void init_file(string name) {
		init_ramfile(name, readAll(name));
	}

	void init_ramfile(string name, bytes data = "", bool nosync = false) {
		file_store[name] = file_node(data, nosync);
	}

	file get_file(string name, file_operate operation) {
		if (!file_store.count(name)) auto_init_file(name);
		if (operation == invaild) return invaild_file;
		if (operation == overriding) {
			file_store[name].data = "";
		}
		file_store[name].refcount++;
		file f;
		f.op_type = operation;
		f.file_name = name;
		f.buffer = &file_store;
		return f;
	}

	void release_file(file &obj) {
		file_store[obj.file_name].refcount--;
		obj.file_name = "";
		obj.op_type = invaild;
	}

	void sync(bool release = false) {
		for (auto &i : file_store) {
			if (!i.second.nosync) {
				FILE *f = fopen(i.first.c_str(), "wb");
				const char *tca = i.second.data.toCharArray();
				fwrite(tca, sizeof(char), i.second.data.length(), f);
				fclose(f);
				delete[] tca;
			}
			if (i.second.refcount <= 0 && release) {
				i.second.data.release();
				file_store.erase(i.first);
			}
		}
	}

protected:

	fs_type file_store;

};