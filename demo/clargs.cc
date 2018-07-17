#include <string.h>
#include <cctype>
#include <map>
#include <list>
#include <string>
#include "clargs.h"

using std::map;
using std::list;
using std::string;

class clargs_inst {
public:
	bool parse(int32_t argc, char** argv);

private:
	bool parse_pair(const char* s, size_t l);

	void add_pair(const char* s1, size_t l1, const char* s2, size_t l2, bool force = false);

	void add_pair(string& key, string& val, bool force = false);

	void add_arg(const char* s, size_t l);

	uint32_t parse_single_minus(const char* s, size_t l);

public:
	map<string, string> options;
	list<string> args;
	map<string, string>::iterator opts_it;
	list<string>::iterator args_it;
};

bool clargs_inst::parse(int32_t argc, char** argv) {
	int32_t i;
	size_t len;
	const char* s;
	uint32_t psr;
	char prev_single_opt = '\0';

	for (i = 0; i < argc; ++i) {
		s = argv[i];
		len = strlen(s);
		if (len > 2) {
			// --foo=bar
			// --foo
			if (s[0] == '-' && s[1] == '-') {
				if (parse_pair(s + 2, len - 2)) {
					prev_single_opt = '\0';
					continue;
				}
				goto parse_arg;
			}
		}
		if (len > 1) {
			if (s[0] == '-') {
				// 检测到单个'-'加上单个字母或数字
				// 记录在prev_single_opt变量中
				// 否则清空prev_single_opt变量
				// -abc
				// -a xxoo
				// -b -c
				psr = parse_single_minus(s + 1, len - 1);
				if (psr > 0) {
					prev_single_opt = psr == 1 ? s[1] : '\0';
					continue;
				}
			}
		}

parse_arg:
		if (prev_single_opt) {
			add_pair(&prev_single_opt, 1, s, len, true);
			prev_single_opt = '\0';
		} else {
			add_arg(s, len);
		}
	}
	opts_it = options.begin();
	args_it = args.begin();
	return true;
}

bool clargs_inst::parse_pair(const char* s, size_t l) {
	if (!isalpha(s[0]))
		return false;
	size_t i;
	string key;
	string val;
	for (i = 1; i < l; ++i) {
		if (s[i] == '=') {
			key.assign(s, i);
			val.assign(s + i + 1, l - i - 1);
			break;
		}
	}
	if (i == l)
		key.assign(s, l);
	if (val.length() > 0) {
		// '=' 后面第一个字符不能又是'='
		if (val.data()[0] == '=')
			return false;
	}
	add_pair(key, val);
	return true;
}

uint32_t clargs_inst::parse_single_minus(const char* s, size_t l) {
	size_t i;
	for (i = 0; i < l; ++i) {
		if (!isalnum(s[i]))
			return 0;
	}
	for (i = 0; i < l; ++i) {
		add_pair(s + i, 1, "", 0);
	}
	return l;
}

void clargs_inst::add_pair(const char* s1, size_t l1, const char* s2, size_t l2, bool force) {
	string k, v;
	k.assign(s1, l1);
	v.assign(s2, l2);
	add_pair(k, v, force);
}

void clargs_inst::add_pair(string& key, string& val, bool force) {
	if (force || options.find(key) == options.end())
		options[key] = val;
}

void clargs_inst::add_arg(const char* s, size_t l) {
	string str(s, l);
	args.push_back(str);
}

clargs_h clargs_parse(int32_t argc, char** argv) {
	if (argc <= 1 || argv == nullptr)
		return 0;
	clargs_inst* inst = new clargs_inst();
	if (!inst->parse(argc - 1, argv + 1)) {
		delete inst;
		return 0;
	}
	return reinterpret_cast<clargs_h>(inst);
}

void clargs_destroy(clargs_h handle) {
	if (handle)
		delete reinterpret_cast<clargs_inst*>(handle);
}

int32_t clargs_opt_next(clargs_h handle, const char** key, const char** value) {
	if (handle == 0)
		return -1;
	clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
	if (inst->opts_it == inst->options.end())
		return -2;
	if (key)
		*key = inst->opts_it->first.c_str();
	if (value)
		*value = inst->opts_it->second.c_str();
	++inst->opts_it;
	return 0;
}

int32_t clargs_arg_next(clargs_h handle, const char** arg) {
	if (handle == 0)
		return -1;
	clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
	if (inst->args_it == inst->args.end())
		return -2;
	if (arg)
		*arg= (*inst->args_it).c_str();
	++inst->args_it;
	return 0;
}

const char* clargs_opt_get(clargs_h handle, const char* key) {
	if (handle == 0 || key == NULL)
		return NULL;
	clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
	map<string, string>::iterator it;
	it = inst->options.find(key);
	if (it == inst->options.end())
		return NULL;
	return it->second.c_str();
}

int32_t clargs_opt_has(clargs_h handle, const char* key) {
	if (handle == 0 || key == NULL)
		return 0;
	clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
	map<string, string>::iterator it;
	it = inst->options.find(key);
	if (it == inst->options.end())
		return 0;
	return 1;
}
