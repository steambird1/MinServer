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
		append = 2
	};

	class file {
	public:
		friend class basic_file_system;

		virtual bool isInvaild() {
			return op_type == invaild;
		}



	protected:
		file_operate op_type;		// 0 - Read, 1 - Override write, 2 - Append
		string file_name;
	};

	class __invaild_file : public file {
	public:
		__invaild_file() {

		}
		virtual bool isInvaild() {
			return true;
		}
	} invaild_file;


	struct file_node {

		file_node(bytes init = "", bool nosync = false) {
			this->data = init;
			this->nosync = nosync;
		}

		bytes data;
		int refcount = 0;
		bool nosync = false;
	};

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
				delete[] tca;
			}
			if (i.second.refcount <= 0 && release) {
				i.second.data.release();
				file_store.erase(i.first);
			}
		}
	}

protected:

	map<string, file_node> file_store;

};