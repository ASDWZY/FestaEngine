#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <stdio.h>
#include<direct.h>
#include <sys/stat.h>


namespace Festa {
	typedef unsigned int uint;
	typedef unsigned char uchar;
	typedef long long ll;
	typedef unsigned long long ull;

	template<typename T>
	std::string toString(T x) {
		std::ostringstream os;
		os << x;
		return os.str();
	}

	template<typename T>
	T stringTo(const std::string& str) {
		if (str == "true")return true;
		std::stringstream ss;
		ss << str;
		T ret; ss >> ret;
		return ret;
	}

	inline std::wstring string2wstring(const std::string& str)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.size()), 0, 0);
		wchar_t* buffer = new wchar_t[len + 1];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), int(str.size()), buffer, len);
		buffer[len] = '\0';
		std::wstring ret(buffer);
		delete[] buffer;
		return ret;
	}

	inline std::string wstring2string(const std::wstring& wstr)
	{
		int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), 0, 0, 0, 0);
		char* buffer = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.size()), buffer, len, 0, 0);
		buffer[len] = '\0';
		std::string ret(buffer);
		delete[] buffer;
		return ret;
	}

	inline std::string string2u8(const std::string& str){
		int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
		wchar_t* wbuf = new wchar_t[nwLen + 1];
		ZeroMemory(wbuf, nwLen * 2 + 2);
		::MultiByteToWideChar(CP_ACP, 0, str.c_str(), uint(str.length()), wbuf, nwLen);

		int nLen = ::WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, 0, 0, 0, 0);
		char* buf = new char[nLen + 1];
		ZeroMemory(buf, nLen + 1);
		::WideCharToMultiByte(CP_UTF8, 0, wbuf, nwLen, buf, nLen, 0, 0);

		std::string ret(buf);
		delete[]wbuf;
		delete[]buf;
		return ret;
	}

	inline std::string u82string(const std::string& str){
		int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
		wchar_t* pwBuf = new wchar_t[nwLen + 1];
		memset(pwBuf, 0, nwLen * 2 + 2);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), uint(str.length()), pwBuf, nwLen);

		int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, 0, 0, 0, 0);
		char* pBuf = new char[nLen + 1];
		memset(pBuf, 0, nLen + 1);
		WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, 0, 0);

		std::string ret(pBuf);
		delete[]pBuf;
		delete[]pwBuf;
		return ret;
	}

	class String {
	public:
		std::string str;
		String() {}
		String(const char* s) :str(s) {}
		String(const std::string& s) :str(s) {}
		String(const wchar_t* ws) {
			str = wstring2string(ws);
		}
		String(const std::wstring& ws) {
			str = wstring2string(ws);
		}
		String(float number) {
			str = toString(number);
		}
		String(double number) {
			str = toString(number);
		}
		String(ll number) {
			str = toString(number);
		}
		String(ull number) {
			str = toString(number);
		}
		String(int number) {
			str = toString(number);
		}
		String(uint number) {
			str = toString(number);
		}
		friend std::ostream& operator<< (std::ostream& out, const String& str) {
			out << str.str;
			return out;
		}
		String operator+(const String& other)const {
			return String(str + other.str);
		}
		void operator+=(const String& other) {
			str += other.str;
		}
		operator std::string()const {
			return str;
		}
		ull size()const {
			return str.size();
		}
		char operator[](uint pos)const {
			return str[pos];
		}
		const std::wstring towstring()const {
			return string2wstring(str);
		}

	};



#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR 4



	class Logger {
	public:
		typedef std::function<void(const std::string&)> LogFunc;
		std::vector<LogFunc> tasks = { 0,0,0,0 };
		std::string file;
		Logger() {}
		Logger(const std::string& file):file(file) {
			
		}
		void addTask(LogFunc func, int level) {
			tasks[level - 1] = func;
		}
		void log(const std::string& msg, int level) {
			char color; std::string l;
			switch (level) {
			case LOG_LEVEL_DEBUG:
				color = 6, l = "DEBUG";
				break;
			case LOG_LEVEL_INFO:
				color = 2, l = "INFO";
				break;
			case LOG_LEVEL_WARNING:
				color = 3, l = "WARNING";
				break;
			case  LOG_LEVEL_ERROR:
				color = 1, l = "ERROR";
				break;
			default:
				error("Invalid log level: " + toString(level));
				return;
			}
			std::string message = l + ": " + msg;
			if (file.size()) {
				std::ofstream f(file, std::ios::out | std::ios::app);
				if (!f.is_open())
					std::cout<< coloredString("The logger file: " + file + " hasnot the access for writing.",1)<< std::endl;
				f << message << std::endl; f.close();
			}
			if (tasks[level - 1])tasks[level - 1](msg);
			else std::cout << coloredString(message, color) << std::endl;
		}
		char getColor(int level) {
			switch (level) {
			case LOG_LEVEL_DEBUG:
				return 6;
			case LOG_LEVEL_INFO:
				return 2;
			case LOG_LEVEL_WARNING:
				return 3;
			case  LOG_LEVEL_ERROR:
				return 1;
			default:
				error("Invalid log level: " + toString(level));
				return 0;
			}
			
		}
		void debug(const std::string& msg) {
			log(msg, LOG_LEVEL_DEBUG);
		}
		void info(const std::string& msg) {
			log(msg, LOG_LEVEL_INFO);
		}
		void warning(const std::string& msg) {
			log(msg, LOG_LEVEL_WARNING);
		}
		void error(const std::string& msg) {
			log(msg, LOG_LEVEL_ERROR);
		}
		static std::string coloredString(const std::string& msg, char fg, char bg = 0) {
			std::string ret = "\033[3"; ret.push_back(fg + '0');
			ret += ";1m"; ret += msg;
			ret += "\033["; ret.push_back(bg + '0');
			ret.push_back('m');
			return ret;
		}
	};

	extern Logger LOGGER;


	inline void errorlog0(const std::string& msg) {
		Logger().error(msg);
	}

#define ACCESS_MODE_EXISTS 0
#define ACCESS_MODE_WRITE 2
#define ACCESS_MODE_READ 4
#define PATH_DELIMITER '/'

	class Path {
	public:
		Path() {}
		Path(const std::string& path) :path(path) {
			normalize();
		}
		Path(const std::wstring& path) :path(wstring2string(path)) {
			normalize();
		}
		std::string& str() {
			return path;
		}
		std::string toString()const {
			return path;
		}
		std::wstring toWstring()const {
			return string2wstring(path);
		}
		std::string toWindows()const {
			std::string p = path;
			for (char& s : p)
				if (s == PATH_DELIMITER)s = '\\';
			return p;
		}
		ull size()const {
			return path.size();
		}
		bool access(int mode = ACCESS_MODE_EXISTS)const {
			return _waccess(string2wstring(path).c_str(), mode) == 0;
		}
		void check(int mode = ACCESS_MODE_EXISTS)const {
			if (!access(mode))LOGGER.error("The path: " + path + " doesnot exist.");
		}
		bool exists()const {
			return access();
		}
		bool empty()const {
			return !path.size();
		}
		void open(std::ifstream& f)const {
			f.open(path);
			if (!f.is_open())LOGGER.error("Failed to open th file: " + path);
		}
		bool isFile()const {
			struct stat infos;
			stat(path.c_str(), &infos);
			return infos.st_mode & S_IFREG;
		}
		bool isDirectory()const {
			struct stat infos;
			stat(path.c_str(), &infos);
			return infos.st_mode & S_IFDIR;
		}
		Path previous()const {
			ll i;
			for (i = ll(path.size() - 1); i >= 0; i--)if (path[i] == PATH_DELIMITER)break;
			return path.substr(0, i);
		}
		Path back()const {
			ll i;
			for (i = ll(path.size() - 1); i >= 0; i--)if (path[i] == PATH_DELIMITER)break;
			return path.substr(i+1,path.size());
		}
		std::string extension()const {
			ll i;
			for (i = ll(path.size() - 1); i >= 0; i--) {
				if (path[i] == '.')return path.substr(i + 1, path.size());
			}
			return "";
		}
		Path getDirectory()const {
			if (isDirectory())return *this;
			else return previous();
		}
		Path operator+(const Path& p)const {
			std::string tmp = path; tmp.push_back(PATH_DELIMITER); tmp += p.path;
			return Path(tmp);
		}
		void operator+=(const Path& p) {
			path.push_back(PATH_DELIMITER);
			path += p.path;
		}
		Path operator-(const Path& p)const {
			if (p.size()+1 >= size()|| 
				path[p.size()] != '/')return *this;
			for (uint i = 0; i < p.size(); i++)
				if (p.path[i] != path[i])return *this;
			return path.substr(p.size()+1, size());
		}
		void operator-=(const Path& p) {
			*this = *this - p;
		}
		void cd()const {
			chdir(path.c_str());
		}
		void split(std::vector<std::string>& arr)const {
			arr.clear();
			arr.push_back(std::string());
			for (uint i = 0; i < path.size();i++) {
				if (path[i] == PATH_DELIMITER)arr.push_back(std::string());
				else arr.back().push_back(path[i]);
			}
			if (arr.back().size() == 0)arr.pop_back();
		}
		void glob(std::vector<Path>& ret,const std::string& filter="*")const {
			ret.clear();
			if (!isDirectory())return;
			HANDLE hFind;
			WIN32_FIND_DATA findData;
			std::wstring wstrTempDir = toWstring() +string2wstring("\\"+filter);
			hFind = FindFirstFileW(wstrTempDir.c_str(), &findData);
			if (hFind == INVALID_HANDLE_VALUE)return;
			do {
				ret.push_back(Path());
				Path t(findData.cFileName);
				if (t.toString() == "." || t.toString() == "..")ret.pop_back();
				else ret.back() = *this  + t;
			} while (FindNextFile(hFind, &findData) != 0);
		}
		void globRelatively(std::vector<Path>& ret, const std::string& filter = "*")const {
			ret.clear();
			if (!isDirectory())return;
			HANDLE hFind;
			WIN32_FIND_DATA findData;
			std::wstring wstrTempDir = toWstring() + TEXT(PATH_DELIMITER) + string2wstring(filter);
			hFind = FindFirstFile(wstrTempDir.c_str(), &findData);
			if (hFind == INVALID_HANDLE_VALUE)return;
			do {
				ret.push_back(Path());
				Path t(findData.cFileName);
				if (t.toString() == "." || t.toString() == "..")ret.pop_back();
				else ret.back() = t;
			} while (FindNextFile(hFind, &findData) != 0);
		}
		bool createDirectory()const {
			std::vector<std::string> arr; split(arr);
			Path t;
			for (std::string& p:arr) {
				t.path += p;
				if (!t.exists()) {
					if(_mkdir(t.path.c_str()))return false;
				}
				t.path.push_back(PATH_DELIMITER);
			}
			return true;
		}
		void deleteFile() {
			DeleteFile(string2wstring(path).c_str());
		}
		void createFile() {
			if (exists())return;
			std::ofstream f(path, std::ios::out);
			f.close();
		}
		void rename(const Path& newName) {
			std::rename(path.c_str(), newName.toString().c_str());
		}
		void deleteDirectory() {
			if (!exists())return;
			std::vector<Path> arr; glob(arr);
			for (Path& p : arr) {
				if (p.isFile())p.deleteFile();
				else p.deleteDirectory();
			}
			_rmdir(path.c_str());
		}
		friend std::ostream& operator<< (std::ostream& out, const Path& path) {
			out << path.path;
			return out;
		}
		operator std::string()const {
			return path;
		}
	private:
		std::string path;
		void normalize() {
			for (char& s : path)
				if (s == '\\')s = PATH_DELIMITER;
			if (path.back() == PATH_DELIMITER)path.pop_back();
		}
	};



	inline void readString(const Path& file, std::string& source) {
		std::ifstream f;
		try {
			file.open(f);
			std::string line;
			while (std::getline(f, line)) source += line + "\n";
			if (source.size())source.pop_back();
			f.close();
		}
		catch (std::ifstream::failure e) {
			LOGGER.error("Failed to read the file: ");
		}
	}

	//template<typename T>
	inline void writeString(const Path& file,const std::string& source,const
		std::ios_base::openmode mode= std::ios::out) {
		std::ofstream f(file.toString().c_str(), mode);
		try {
			f << source;
			f.close();
		}
		catch (std::ifstream::failure e) {
			LOGGER.error("Failed to write the file: ");
		}
	}

	inline void readLines(const Path& file, std::vector<std::string>& lines) {
		std::ifstream f;
		try {
			file.open(f);
			std::string line;
			while (std::getline(f, line)) lines.push_back(line);
			f.close();
		}
		catch (std::ifstream::failure e) {
			LOGGER.error("Failed to read the file: ");
		}
	}

}


