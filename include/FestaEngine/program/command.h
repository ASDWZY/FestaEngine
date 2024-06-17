#pragma once

#include "../../common/common.h"
#include<queue>
#include <stack>

typedef ull addr_t;



struct DataList {
	struct Reference {
		addr_t len, addr;
		template<typename T>
		T getValue(const DataList& pool) {
			return pool.get<T>(addr);
		}
	};
	vector<uchar> bytes;
	DataList() {

	}
	void erase(addr_t begin, addr_t end) {
		if (begin >= size())return;
		bytes.erase(bytes.begin() + begin, bytes.begin() + end);
	}
	addr_t size()const {
		return addr_t(bytes.size());
	}
	DataList operator+(const DataList& other)const {
		DataList ret;
		ret.bytes = bytes;
		ret.bytes.insert(ret.bytes.end(), other.bytes.begin(), other.bytes.end());
		return ret;
	}
	void operator+=(const DataList& other) {
		bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end());
	}
	void extend(addr_t len) {
		bytes.resize(size() + len, 0);
	}
	void pushByte(uchar byte) {
		bytes.push_back(byte);
	}
	template<typename T>
	void push(const T& value) {
		T number = value;
		const uchar* b = reinterpret_cast<const uchar*>(&number);
		for (addr_t i = 0; i < sizeof(T); i++)bytes.push_back(b[i]);
	}
	template<typename T>
	T get(addr_t ptr)const {
		return *reinterpret_cast<const T*>(&bytes[ptr]);
	}
	template<typename T>
	T set(addr_t ptr, const T& value)const {
		T number = value;
		const uchar* b = reinterpret_cast<const uchar*>(&number);
		for (addr_t i = 0; i < sizeof(T); i++)bytes[ptr + i] = b[i];
	}
	void clear() {
		bytes.clear();
	}
	Reference getReference(uint addr)const {
		return { get<addr_t>(addr),get<addr_t>(addr + sizeof(addr_t)) };
	}
	void pushReference(const Reference& ref) {
		push(ref.len);
		push(ref.addr);
	}
	void erase(addr_t start) {
		bytes.erase(bytes.begin() + start, bytes.end());
	}
};

#define VAR_VALUE 0
#define VAR_REF 1

struct Interface {
	queue<uchar> q;
	Interface() {}
	Interface(const DataList& list) {
		push(list);
	}
	DataList toDataList()const {
		DataList ret;
		const uchar* begin = &q.front();
		ret.bytes = vector<uchar>(begin, begin + q.size());
		return ret;
	}
	addr_t size()const {
		return q.size();
	}
	uchar pop() {
		uchar ret = q.front(); q.pop();
		return ret;
	}
	void push(const DataList& list) {
		for (uchar b : list.bytes)q.push(b);
	}
	template<typename T>
	void push(const T& val) {
		DataList list; list.push(val);
		push(list);
	}
	void clear() {
		while (q.size())q.pop();
	}
	template<typename T>
	T get(addr_t addr) {
		const uchar* begin = &q.front();
		return *reinterpret_cast<const T*>(begin + addr);
	}
};

#define MAX_SPACE 300

class Command {
public:
	struct GlobalMemory {
		DataList pool, retVal;
		Interface in, out;
		vector<stack<addr_t>> spaceHead;
		stack<int> space;
		int exit = 0;
		int exitCode = 0;
		void enterSpace(int s) {
			if (space.size() >= MAX_SPACE) {
				LOGGER.warning("Max space depth reached.");
				//exitCode = -1;
				//exit = int(space.size());
				//return;
			}
			space.push(s);
			spaceHead[s].push(pool.size());
			//cout << "enterspace: " << s << endl;
		}
		void exitSpace() {
			if (!space.size()) {
				LOGGER.error("There hasnot any space to exit");
				return;
			}
			int s = space.top();
			addr_t head = spaceHead[s].top();
			spaceHead[s].pop();
			pool.erase(head);
			space.pop();
			if (exit > 0)exit--;
		}
		void clear() {
			pool.clear(); retVal.clear();
			in.clear(); out.clear();
			for (uint i = 0; i < spaceHead.size(); i++)
				while (spaceHead[i].size())spaceHead[i].pop();
			while (space.size())space.pop();
			exit = exitCode = 0;
		}
	};
	struct FunctionDefination {
		Command* cmd = 0;
		int space = -1;
	};

	typedef void(*PerformFunction)(Command*, GlobalMemory* mem);
	PerformFunction func = 0;
	vector<Command*> commands;
	FunctionDefination** def = 0;
	DataList value;
	int state = 0;
	Command() {}
	Command(PerformFunction func) :func(func) {}
	Command(PerformFunction func, FunctionDefination** def)
		:func(func), def(def) {
	}
	Command(const DataList& value)
		:value(value) {}
	void release() {
		for (uint i = 0; i < commands.size(); i++) {
			if (commands[i]) {
				//commands[i]->release();
				delete commands[i];
			}
		}
	}
	~Command() {
		//release();
		//clear();
	}
	void clear() {
		if (func)value.clear();
		for (uint i = 0; i < size(); i++)
			commands[i]->clear();
	}
	void run(GlobalMemory* mem) {
		if (mem->exit > 0)return;
		if (!value.size())func(this, mem);
	}
	void runCommands(GlobalMemory* mem) {
		for (uint i = 0; i < size(); i++)getInput(i, mem);
	}
	DataList getInput(uint i, GlobalMemory* mem) {
		commands[i]->run(mem);
		DataList ret = commands[i]->value;
		commands[i]->clear();
		return ret;
	}
	template<typename T>
	T input(uint i, GlobalMemory* mem) {
		return getInput(i,mem).get<T>(0);
	}
	uint size()const {
		return uint(commands.size());
	}
};
