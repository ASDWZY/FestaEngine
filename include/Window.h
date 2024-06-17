#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "frame.h"
#include "common/shapes.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "../3rd/GL/glfw3native.h"
#include<MMSystem.h>
#pragma comment(lib, "Winmm.lib")
//#pragma comment(lib,"MMlib.lib")

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../3rd/imgui/imgui.h"
#include "../3rd/imgui/imgui_impl_glfw.h"
#include "../3rd/imgui/imgui_impl_opengl3.h"

#include <mutex>
#include <thread>

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define KEY_UP(vKey) ((GetAsyncKeyState(vKey) & 0x8000) ? 0:1)
typedef std::function<void()> Command_t;
typedef Command_t TaskFunc;


#ifdef HIDE_CONSOLE
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

namespace Festa {

	
	class Timer {
	public:
		Timer() { reset(); }
		void reset() {
			st = getTime();
		}
		double interval()const {
			return getTime() - st;
		}
		double fps()const {
			double i = interval();
			return i == 0.0 ? -1.0 : 1.0 / i;
		}
	private:
		double st = 0.0f;
	};

	class FrameTimer {
	public:
		FrameTimer() {  }
		void reset() {
			st = 0.0;
		}
		double interval() {
			if (st == 0.0) {
				st = getTime();
				return 0.0;
			}
			double ret = getTime() - st;
			st = getTime();
			return ret;
		}
		static double fps(double inter) {
			return inter == 0.0 ? -1.0 : 1.0 / inter;
		}
	private:
		double st = 0.0;
	};

	class TaskTimer {
	public:
		TaskFunc task;
		double interval;
		TaskTimer() { task = 0; timer.reset(); interval = 0.0f; }
		template<typename Fn, typename ...Args>
		TaskTimer(double interval, Fn&& fn, Args&&... args) :interval(interval) {
			reset();
			task = [=] {fn(args...); };
		}
		void reset() {
			timer.reset();
		}
		void checkOnce() {
			if (timer.interval() >= interval)task();
		}
		void check() {
			if (timer.interval() >= interval) {
				task();
				reset();
			}
		}
	private:
		Timer timer;
	};


	class ThreadTimer
	{
	public:
		std::function<void()> task = 0;
		double interval = 0.0;
		ThreadTimer() {}
		template<typename Fn, typename ...Args>
		ThreadTimer(double interval, Fn&& fn, Args&&... args)
			:interval(interval) {
			task = [=] {fn(args...); };
			expired = true; tryToExpire = false;
		}
		ThreadTimer(const ThreadTimer& timer) {
			expired = timer.expired.load();
			tryToExpire = timer.tryToExpire.load();
			task = timer.task; interval = timer.interval;
			expired = true; tryToExpire = false;
		}
		~ThreadTimer() { stop(); }
		void operator=(const ThreadTimer& timer) {
			expired = timer.expired.load();
			tryToExpire = timer.tryToExpire.load();
			task = timer.task; interval = timer.interval;
		}
		void start() {
			if (!expired)return;
			expired = false;
			int inter = int(interval * 1000.0);
			std::thread([this, inter]() {
				while (!tryToExpire) {
					std::this_thread::sleep_for(std::chrono::milliseconds(inter));
					task();
				}
				std::lock_guard<std::mutex> locker(mtx);
				expired = true; tryToExpire = false;
				expiredCond.notify_one();
				}).detach();
		}

		void startOnce()
		{
			int delay = int(interval * 1000.0);
			std::thread([this, delay]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				task();
				}).detach();
		}

		void stop() {
			if (expired || tryToExpire)return;
			tryToExpire = true;
			std::unique_lock<std::mutex> locker(mtx);
			expiredCond.wait(locker, [this] {return expired == true; });
		}
	private:
		std::atomic<bool> expired;
		std::atomic<bool> tryToExpire;
		std::mutex mtx;
		std::condition_variable expiredCond;
	};


	class ButtonStatus {
	public:
		ButtonStatus() {}
		ButtonStatus(bool p) {
			update(p);
		}
		void operator=(const ButtonStatus& b) {
			status = b.status;
		}
		template<typename T>
		void operator=(const T& b) = delete;
		void update(bool p) {
			status >>= 1;
			status |= uchar(p) << 1;
		}
		void clear() {
			status = 0;
		}
		void operator<<(bool p) {
			update(p);
		}
		bool previous()const {
			return status & 1;
		}
		bool pressed()const {
			return status & 2;
		}
		bool down()const {
			return pressed();
		}
		bool up()const {
			return !pressed();
		}
		bool released()const {
			return (!pressed()) && previous();
		}
		bool clicked()const {
			return released();
		}
		bool firstPressed()const {
			return pressed() && (!previous());
		}
		operator bool()const {
			return pressed();
		}
	private:
		uchar status = 0;
	};



	struct Event {
		MSG msg;
		Event() { msg = {}; }
		int get(HWND hwnd = 0) {// 1 if running;0 if quit;-1 if error
			return PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);
		}
		void update() {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		uint message()const {
			return msg.message;
		}
		bool checkEvent(uint ev)const {
			return message() == ev;
		}
		ull wlow()const {
			return LOWORD(msg.wParam);
		}
		ull whigh()const {
			return HIWORD(msg.wParam);
		}
		ull w()const {
			return msg.wParam;
		}
		ull llow()const {
			return LOWORD(msg.lParam);
		}
		ull lhigh()const {
			return HIWORD(msg.lParam);
		}
		ull l()const {
			return msg.lParam;
		}
	};

	class CommandComponent {
	public:
		static std::vector<Command_t> commands;
		static void update(ull id) {
			if (1 <= id && id <= commands.size())commands[id - 1]();
		}
		template<typename Fn, typename ...Args>
		uint addCommand(Fn&& f, Args&&... args) {
			commands.push_back([=] {f(args...); });
			return uint(commands.size());
		}
	};


	Path askOpenFileName(HWND hwnd, const wchar_t* filter, const String& title = String(), const wchar_t* initDir=0);
	Path askSaveFileName(HWND hwnd, const wchar_t* filter, const String& title = String(), const wchar_t* initDir = 0);

	
	enum ImGuiMode {
		IMGUI_NONE=-1, IMGUI_LEFT, IMGUI_RIGHT,IMGUI_TOP,IMGUI_BOTTOM,IMGUI_FULLSCREEN
	};

	class Window {
	public:
		struct IMGUI {
			bool first = true;
			bool focused = false;
			Window* window=0;
			void* dockedWindows[4] = { 0,0,0,0 };
			IMGUI() {}
			IMGUI(Window* window):window(window) {
				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO(); (void)io;
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
				ImGui_ImplGlfw_InitForOpenGL(window->ptr, true);
				std::string f = window->glVersion.glFormat();
				ImGui_ImplOpenGL3_Init(f.c_str());
			}
			void render() {
				focused = false;
				dockedWindows[0] = dockedWindows[1] = 
					dockedWindows[2] = dockedWindows[3] = 0;
				if (!first)ImGui::Render();
				first = false;
				ImDrawData* data = ImGui::GetDrawData();
				if (data)ImGui_ImplOpenGL3_RenderDrawData(data);
			}
			void begin() {
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();
			}
		};
		struct Mouse {
			int scroll=0;
			ivec2 cursorPos;
			ButtonStatus left, right;
			const Window* window = nullptr;
			ivec2 winPos()const {
				ivec2 ret;
				glfwGetWindowPos(window->ptr,&ret.x,&ret.y);
				return cursorPos-ret;
				RECT rect;
				GetClientRect(window->hwnd, &rect);
				return ivec2(cursorPos.x-rect.left,cursorPos.y-rect.top);
			}
			bool inWindow()const {
				ivec2 pos=winPos(),size=window->size();
				return 0 <= pos.x&& pos.x<size.x&&
					0<= pos.y&& pos.y<size.y;
			}
		};

		typedef std::function<void(const Event&)> EventCallback;

		HWND hwnd;
		GLFWwindow* ptr = 0;
		IMGUI* imgui = 0;
		Viewport viewport;
		Mouse mouse;
		Version2 glVersion;
		Window() {
			ptr = 0;
			hwnd = 0;
		}
		Window(int w, int h,const std::string& title,const Version2& glVersion = 4.4, bool resizable = true) {
			init(w, h, title, glVersion, resizable);
		}
		void init(int w, int h,const std::string& title, const Version2& glVersion = 4.4, bool resizable=true);
		Window(GLFWwindow* ptr) :ptr(ptr) {
			mouse.window = this;
			hwnd = glfwGetWin32Window(ptr);
			viewport = Viewport(width(), height());
			viewport.apply();
		}
		void release();
		~Window() {
			release();
		}
		bool isFocused()const {
			if (!imgui)return true;
			else return !(imgui->focused);
		}
		void iconify()const {
			glfwIconifyWindow(ptr);
		}
		void maximize()const {
			glfwMaximizeWindow(ptr);
		}
		static void text(HWND hwnd, const String& str, int x, int y, const vec3& color = vec3(0.0f)) {
			vec3 col = color * 255.0f;
			int r = int(col.x), g = int(col.y), b = int(col.z);
			HDC hdc = GetDC(hwnd);
			SetTextColor(hdc, RGB(r, g, b));
			TextOutW(hdc, x, y, str.towstring().c_str(), int(str.size()));
		}
		void text(const String& str, int x, int y, const vec3& color = vec3(0.0f))const {
			text(hwnd, str, x, y, color);
		}
		void initImgui() {
			imgui = new IMGUI(this);
		}
		bool Imgui()const { return imgui; }
		void setEventCallback(EventCallback callback) {
			eventCallback = callback;
		}
		static int messagebox(HWND hwnd, const String& text, const String& title, uint buttonFlag = MB_OKCANCEL, uint iconFlag = MB_ICONINFORMATION) {
			return MessageBoxW(hwnd, text.towstring().c_str(), title.towstring().c_str(), buttonFlag | iconFlag);
		}
		int messagebox(const String& text, const String& title, uint buttonFlag = MB_OKCANCEL, uint iconFlag = MB_ICONINFORMATION)const {
			return messagebox(hwnd, text, title, buttonFlag, iconFlag);
		}
		void setViewport(const Viewport& v) {
			viewport = v;
			viewport.apply();
		}
		void updateViewport() {
			viewport.x = viewport.y = 0;
			glfwGetWindowSize(ptr, &viewport.w, &viewport.h);
			viewport.apply();
		}
		int updateEvents() {
			timer.reset();
			if (glfwWindowShouldClose(ptr))return 0;
			int state;
			mouse.scroll = 0;
			while (state = ev.get(hwnd)) {
				if (state == -1)return -1;
				if (eventCallback)eventCallback(ev);
				switch (ev.message()) {
				case WM_COMMAND:
					CommandComponent::update(ev.wlow());
					break;
				case WM_MOUSEWHEEL:
					mouse.scroll += sgn(GET_WHEEL_DELTA_WPARAM(ev.w()));
					break;
				}
				ev.update();
			}
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			mouse.cursorPos.x = cursorPos.x; mouse.cursorPos.y = cursorPos.y;
			mouse.left.update(glfwGetMouseButton(ptr, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS);
			mouse.right.update(glfwGetMouseButton(ptr, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
			updateViewport();
			return 1;
		}
		int update() {
			if (imgui)imgui->render();
			glfwSwapBuffers(ptr);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			if (imgui)imgui->begin();
			
			return updateEvents();
		}
		void render() {
			if (imgui)imgui->render();
			glfwSwapBuffers(ptr);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			if (imgui)imgui->begin();
		}
		void setIcon(const Image& icon) {
			Image image=icon;
			image.bgr2rgb();
			image.resetChannels(4);
			GLFWimage img[1];
			img[0].pixels = image.data();
			img[0].height = image.height();
			img[0].width = image.width();
			glfwSetWindowIcon(ptr, 1, img);
		}
		bool shouldClose()const {
			return glfwWindowShouldClose(ptr);
		}
		void setShouldClose(bool val)const {
			glfwSetWindowShouldClose(ptr, val);
		}
		int getKey(int key)const {
			return glfwGetKey(ptr, key);
		}
		const Mouse& getMouse()const {
			return mouse;
		}
		ivec2 pos()const {
			RECT rect;
			GetWindowRect(hwnd, &rect);
			//glfwGetWindowPos();
			return ivec2(rect.left, rect.top);
		}
		ivec2 size()const {
			int w, h;
			glfwGetWindowSize(ptr, &w, &h);
			return ivec2(w, h);
		}
		int width()const {
			int w;
			glfwGetWindowSize(ptr, &w, 0);//Pos
			return w;
		}
		int height()const {
			int h;
			glfwGetWindowSize(ptr, 0, &h);
			return h;
		}
		double interval()const {
			return timer.interval();
		}
	private:
		static bool first;
		Event ev;
		Timer timer;
		EventCallback eventCallback = 0;
	};


	inline ImFont* ImGuiFreetypeFont(const std::string& filename, float pixelSize) {
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF(
			filename.c_str(),
			pixelSize,
			nullptr,
			io.Fonts->GetGlyphRangesChineseFull()
		);
		IM_ASSERT(font);
		return font;
	}

}



