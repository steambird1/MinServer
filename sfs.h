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

	void set_case_ignore(bool state) {
		this->isignore = state;
	}

#define process_ignore() do { if (this->isignore) name = sToLower(name); } while (false)

	void auto_init_file(string name) {
		process_ignore();
		if (fileExists(name)) {
			init_file(name);
		}
		else {
			init_ramfile(name);
		}
	}

	void init_file(string name) {
		process_ignore();
		init_ramfile(name, readAll(name));
	}

	void init_ramfile(string name, bytes data = "", bool nosync = false) {
		process_ignore();
		file_store[name] = file_node(data, nosync);
	}

	bool file_exist(string name) {
		return this->file_store.count(name);
	}

	file get_file(string name, file_operate operation) {
		process_ignore();
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
				sync_file(i.first, release);
			}
		}
	}

	void sync_directory(string directory, bool release = false, bool load_ramfile = false) {
		for (auto &i : file_store) {
			if (load_ramfile || (!i.second.nosync)) {
				if (isBeginWith(directory, i.first))
					sync_file(i.first, release);
			}
		}
	}

	void sync_file(string name, bool release = false) {
		if (file_store.count(name)) {
			FILE *f = fopen(name.c_str(), "wb");
			const char *tca = file_store[name].data.toCharArray();
			fwrite(tca, sizeof(char), file_store[name].data.length(), f);
			fclose(f);
			delete[] tca;
		}
		if (file_store[name].refcount <= 0 && release) {
			file_store[name].data.release();
			file_store.erase(name);
		}
	}

	void discard(string name, bool force = false) {
		if (file_store[name].refcount <= 0 || force) {
			auto_init_file(name);
		}
	}

	void discard_all(bool force = false) {
		for (auto &i : file_store) {
			discard(i.first, force);
		}
	}

	// Unit: KB
	size_t usage() {
		size_t o = file_store.size() * 1024;
		for (auto &i : file_store) {
			o += i.second.data.length();
		}
		return o / 1024;
	}

protected:

	fs_type file_store;
	bool isignore = false;

};