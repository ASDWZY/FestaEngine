#pragma once
#include "../Window.h"
#include<stack>
#include <commctrl.h>
#pragma comment(lib,"COMCTL32.lib")





namespace Festa {
	class Menu :public CommandComponent {
	public:
		HMENU hmenu;
		Menu() {
			hmenu = CreatePopupMenu();
		}
		template<typename Fn, typename ...Args>
		void add(const String& name, Fn&& f, Args&&... args) {
			AppendMenu(hmenu, MF_STRING, addCommand(f, args...), name.towstring().c_str());
		}
		~Menu() {
			DestroyMenu(hmenu);
		}
	};

	class MenuBar :public Menu {
	public:
		MenuBar() { hmenu = CreateMenu(); }
		void addMenu(const String& name, const Menu& menu) {
			AppendMenu(hmenu, MF_POPUP, (UINT_PTR)menu.hmenu, name.towstring().c_str());
		}

		void bind(HWND hwnd) {
			SetMenu(hwnd, hmenu);
		}
	};

	class IndMenu :public Menu {
	public:
		struct MenuConfig {
			ull id;
			const wchar_t* name;
			MenuConfig(ull id, const String& name) :id(id), name(name.towstring().c_str()) {}
		};
		IndMenu() {}
		template<typename Fn, typename ...Args>
		void add(const String& name, Fn&& func, Args&&... args) {
			config.push_back(MenuConfig(addCommand(func, args...), name));
		}
		void show(HWND hwnd, int x, int y) {
			hmenu = CreatePopupMenu();
			for (MenuConfig c : config)AppendMenu(hmenu, MF_STRING, c.id, c.name);
			TrackPopupMenu(hmenu, TPM_TOPALIGN | TPM_LEFTALIGN, x, y, 0, hwnd, NULL);
			DestroyMenu(hmenu);
		}
	private:
		std::vector<MenuConfig> config;
	};

	

	class Canvas {
	public:
		Window* window = 0;
		HDC hdc = 0;
		HPEN pen = 0; HBRUSH brush = 0;
		Canvas() {}
		Canvas(Window* window) :window(window) {
			hdc = GetDC(window->hwnd);
		}
		~Canvas() {
			if (pen)DeleteObject(pen);
			if (brush)DeleteObject(brush);
			ReleaseDC(window->hwnd, hdc);
		}
		void createPen(const COLORREF& color, int width, int style = PS_SOLID) {
			if (pen)DeleteObject(pen);
			pen = CreatePen(style, width, color);
			SelectObject(hdc, pen);
		}
		void solidBrush(const COLORREF& color) {
			if (brush)DeleteObject(brush);
			brush = CreateSolidBrush(color);
		}
		void setPixel(int x, int y, const COLORREF& color)const {
			SetPixel(hdc, x, y, color);
		}
		void line(int x1, int y1, int x2, int y2)const {
			MoveToEx(hdc, x1, y1, 0);
			LineTo(hdc, x2, y2);
		}
		void rectangle(int x1, int y1, int x2, int y2)const {
			Rectangle(hdc, x1, y1, x2, y2);
		}
		void circle(int x, int y, int radius)const {
			Ellipse(hdc, x, y, radius, radius);
		}
	};

	class Notebook {
	public:
		HWND hwnd;
		Notebook() {
			hwnd = 0;
		}
		Notebook(HWND hWnd, int w, int h, int x = 0, int y = 0) {
			hwnd = CreateWindowW(WC_TABCONTROL, L"", WS_CHILD | WS_VISIBLE,
				x, y, w, h, hWnd, NULL, GetModuleHandleW(0), NULL);
		}
		void add(const String& name) {
			TCITEM tie;
			tie.mask = TCIF_TEXT;
			wchar_t* tmp = new wchar_t[name.size()];
			lstrcpyW(tmp, name.towstring().c_str());
			tie.pszText = tmp;
			TabCtrl_InsertItem(hwnd, 0, &tie);
		}
	};


	struct ImGuiComponent {
		std::function<void()> func;
		ImGuiComponent() {}
		template <typename Func, typename... Args>
		ImGuiComponent(Func&& f, Args&&... args) {
			func = [=]() { f(args...); };
		}
		void render()const {
			if (func)func();
		}
	};

	class BaseImGuiWindow {
	public:
		std::string title;
		Window* father = 0;
		std::vector<ImGuiComponent> components;
		ImGuiWindowFlags flags = 0;
		virtual void begin() {

		}
		void end() {
			if (!status[0])return;
			status[0] = 0;
			ImGui::End();
		}
		virtual void render() {
			begin(); end();
		}
		VirtualValue<bool> X() {
			return status[2];
		}
		void add(const ImGuiComponent& component) {
			components.push_back(component);
		}
		bool isActivated() {
			return !status[1];
		}
		void activate() {
			status[1] = 0;
		}
		void deactivate() {
			status[1] = 1;
		}
	protected:
		BoolList status = BoolList(3);//0:dur/end,1:ter/run
		bool _begin() {
			if (status[0] || status[1])return true;
			status[0] = 1;
			bool activated = true;
			ImGui::Begin(title.c_str(), X() ? &activated : 0, flags);
			if (!activated) {
				deactivate();
				end();
				return true;
			}
			father->imgui->focused |= ImGui::IsWindowFocused();
			return false;
		}
	};

	class IMGUIWindow:public BaseImGuiWindow {
	public:
		ivec2 size = ivec2(-1), pos = ivec2(-1);
		IMGUIWindow() {}
		IMGUIWindow(Window* _father, const std::string& _title,
			const ivec2& _size = ivec2(-1), const ivec2& _pos = ivec2(-1),bool x=true, ImGuiWindowFlags _flags = 0) {
			init(_father, _title, _size, _pos,x, _flags);
		}
		void init(Window* _father, const std::string& _title,
			const ivec2& _size = ivec2(-1), const ivec2& _pos = ivec2(-1),
			bool x=true,ImGuiWindowFlags _flags = 0) {
			father = _father; title = _title; size = _size; pos = _pos; flags = _flags;
			X() = x;
		}
		void begin() {
			if (_begin())return;
			if (pos.x != -1)ImGui::SetWindowPos(ImVec2(float(pos.x), float(pos.y)));
			if (size.x != -1)ImGui::SetWindowSize(ImVec2(float(size.x), float(size.y)));
			for (uint i = 0; i < components.size(); i++)
				components[i].render();
		}
		void resize(const ivec2& size) {
			bool activated = true;
			ImGui::Begin(title.c_str(), &activated, flags);
			ImGui::SetWindowSize(ImVec2(float(size.x), float(size.y)));
			ImGui::End();
		}
		void setPosition(const ivec2& pos) {
			bool activated = true;
			ImGui::Begin(title.c_str(), &activated, flags);
			ImGui::SetWindowPos(ImVec2(float(pos.x), float(pos.y)));
			ImGui::End();
		}
	};

	class DockedWindow:public BaseImGuiWindow {
	public:
		ImGuiMode mode = IMGUI_NONE;
		int size = -1;
		float dm=0.0f;
		ImVec2 lp = ImVec2(-1.0f, -1.0f), ls = ImVec2(-1.0f, -1.0f);
		DockedWindow() {}
		DockedWindow(Window* _father, const std::string& _title,
			ImGuiMode _mode, int _size = -1,bool x=false, 
			ImGuiWindowFlags _flags = ImGuiWindowFlags_NoCollapse) {
			init(_father, _title, _mode, _size,x, _flags);
		}
		void init(Window* _father, const std::string& _title,
			ImGuiMode _mode, int _size = -1,bool x=false, 
			ImGuiWindowFlags _flags = ImGuiWindowFlags_NoCollapse) {
			father = _father; title = _title; mode = _mode; size = _size, flags = _flags; X() = x;
			if (mode < 0 || mode>4)LOGGER.error("DockedWindow::Invaild mode: " + toString(mode));
		}
		void begin() {
			if (_begin()|| ImGui::IsWindowCollapsed())return;
			const float ww = float(father->viewport.w), wh = float(father->viewport.h),
				wx = float(father->viewport.x), wy = float(father->viewport.y);
			ImVec2 pos = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
			float xoff = size == -1 ? s.x : float(size), yoff = size == -1 ? s.y : float(size);
			float x = wx, y = wy, w = ww, h = wh;
			bool motion = false;
			if (lp.x == -1 || (pos == lp && s == ls)) {
				s = ImVec2(ww, wh);
				switch (mode) {
				case IMGUI_LEFT:
					pos.x = wx, pos.y = wy, s.x = xoff, x += xoff, w -= xoff;
					break;
				case IMGUI_RIGHT:
					pos.x = wx + ww - xoff, pos.y = wy, s.x = xoff, w -= xoff;
					break;
				case IMGUI_TOP:
					pos.x = wx, pos.y = wy, s.y = yoff, y += yoff, h -= yoff;
					break;
				case IMGUI_BOTTOM:
					pos.x = wx, pos.y = wy + wh - yoff, s.y = yoff, h -= yoff;
					break;
				case IMGUI_FULLSCREEN:
					pos.x = wx, pos.y = wy;
					w = h = 0;
					break;
				}
			}
			else motion = true;
			switch (mode) {
			case IMGUI_LEFT:
				s.x += dm, x += dm, w -= dm;
				break;
			case IMGUI_RIGHT:
				pos.x += dm, s.x -= dm, w += dm;
				break;
			case IMGUI_TOP:
				s.y += dm, y += dm, h -= dm;
				break;
			case IMGUI_BOTTOM:
				pos.y += dm, s.y -= dm, h += dm;
				break;
			}
			if (motion)updateWindows(pos, s, father->viewport);
			if (mode != IMGUI_FULLSCREEN)
				father->imgui->dockedWindows[mode] = this;

			normalize(pos.x, pos.y); normalize(s.x, s.y);
			normalize(x, y); normalize(w, h);
			ImGui::SetWindowPos(pos);
			ImGui::SetWindowSize(s);
			lp = pos; ls = s;
			dm = 0.0f;
			//unfolded window
			if (!ImGui::IsWindowCollapsed())
				father->setViewport(Viewport(x, y, w, h));
			for (uint i = 0; i < components.size(); i++)
				components[i].render();
		}
		template<typename T>
		void normalize(T& x, T& y) {
			x = std::max(std::min(x, T(father->width() - 1)), T(0));
			y = std::max(std::min(y, T(father->height() - 1)), T(0));
		}
		void resize(const ivec2& s) {
			ImGui::Begin(title.c_str());
			ImGui::SetWindowSize(ImVec2(float(s.x), float(s.y)));
			ImGui::End();
		}
		void setPos(const ivec2& pos) {
			bool activated = true;
			ImGui::Begin(title.c_str(), &activated);
			ImGui::SetWindowPos(ImVec2(float(pos.x), float(pos.y)));
			ImGui::End();
		}
	private:
		void updateWindows(int m, const ImVec2& p, const ImVec2& s, const Viewport& v) {
			const ImVec2 rp = ImGui::GetWindowPos();

			switch (m) {
			case IMGUI_LEFT:
				if (father->imgui->dockedWindows[IMGUI_LEFT]) {
					DockedWindow& w = *(DockedWindow*)father->imgui->dockedWindows[IMGUI_LEFT];
					if (lp != p && s != p && w.dm == 0)w.dm = p.x - lp.x;
				}
				break;
			case IMGUI_RIGHT:
				if (father->imgui->dockedWindows[IMGUI_RIGHT]) {
					DockedWindow& w = *(DockedWindow*)father->imgui->dockedWindows[IMGUI_RIGHT];
					if (lp == p && s != p && w.dm == 0)w.dm = p.x + s.x - lp.x - ls.x;
				}
				break;
			case IMGUI_TOP:
				if (father->imgui->dockedWindows[IMGUI_TOP]) {
					DockedWindow& w = *(DockedWindow*)father->imgui->dockedWindows[IMGUI_TOP];
					if (lp != p && s != p && w.dm == 0)w.dm = p.y - lp.y;
				}
				break;
			case IMGUI_BOTTOM:
				if (father->imgui->dockedWindows[IMGUI_BOTTOM]) {
					DockedWindow& w = *(DockedWindow*)father->imgui->dockedWindows[IMGUI_BOTTOM];
					if (lp == p && s != p && w.dm == 0)w.dm = p.y + s.y - lp.y - ls.x;
				}
				break;
			}
		}
		void updateWindows(const ImVec2& p, const ImVec2& s, const Viewport& v) {
			switch (mode) {
			case IMGUI_LEFT:
				updateWindows(IMGUI_LEFT, p, s, v);
				updateWindows(IMGUI_TOP, p, s, v);
				updateWindows(IMGUI_BOTTOM, p, s, v);
				break;
			case IMGUI_RIGHT:
				updateWindows(IMGUI_RIGHT, p, s, v);
				updateWindows(IMGUI_TOP, p, s, v);
				updateWindows(IMGUI_BOTTOM, p, s, v);
				break;
			case IMGUI_TOP:
				updateWindows(IMGUI_LEFT, p, s, v);
				updateWindows(IMGUI_RIGHT, p, s, v);
				updateWindows(IMGUI_TOP, p, s, v);
				break;
			case IMGUI_BOTTOM:
				updateWindows(IMGUI_LEFT, p, s, v);
				updateWindows(IMGUI_RIGHT, p, s, v);
				updateWindows(IMGUI_BOTTOM, p, s, v);
				break;
			case IMGUI_FULLSCREEN:
				updateWindows(IMGUI_LEFT, p, s, v);
				updateWindows(IMGUI_RIGHT, p, s, v);
				updateWindows(IMGUI_TOP, p, s, v);
				updateWindows(IMGUI_BOTTOM, p, s, v);
				break;
			}
		}
	};

	inline ImGuiComponent ImGuiSameLine() {
		return ImGuiComponent([]() {ImGui::SameLine(); });
	}

	inline ImGuiComponent ImGuiButton(const std::string& name, Command_t&& command) {
		return ImGuiComponent([name, command]() {
			if (ImGui::Button(name.c_str()))command();
			});
	}
	inline ImGuiComponent ImGuiText(const std::string& text) {
		return ImGuiComponent([text]() {ImGui::Text(text.c_str()); });
	}
	inline ImGuiComponent ImGuiBulletText(const std::string& text) {
		return ImGuiComponent([text]() {ImGui::BulletText(text.c_str()); });
	}



	struct TreeNode {
		std::string name;
		std::vector<TreeNode> children;
		ImGuiComponent component;
		TreeNode() {}
		TreeNode(const std::string& name, bool leaf = true)
			:name(name) {
			pos = leaf ? 0 : 2;
		}
		void render()const {
			if (isLeaf()) {
				component.render();
				return;
			}
			if (pos == 1)component.render();
			if (ImGui::TreeNode(name.c_str())) {
				if (pos == 2)component.render();
				for (uint i = 0; i < children.size(); i++) {
					if (children[i].pos != -1)children[i].render();
				}
				if (pos == 3)component.render();
				ImGui::TreePop();
			}
		}
		void add(TreeNode child) {
			pos = 2;
			children.push_back(child);
		}
		void add(const std::string& treenode) {
			pos = 2;
			children.push_back(TreeNode(treenode));
		}
		bool isLeaf()const {
			return pos == 0;
		}
		void setBrachPos(int position) {
			pos = position;
		}
		void release() {
			if (pos == -1)return;
			name.clear();
			pos = -1;
			children.clear();
		}
		void setPos(int _pos) {
			pos = _pos;
		}
	private:
		int pos = 0;
	};

	struct ImGuiDoubleClick {
		UpdatingValue<float> time;
		bool isClicked = false;
		ImGuiDoubleClick() {
			reset();
		}
		void reset() {
			time = UpdatingValue<float>(-2000.0f, -1000.0f);
			isClicked = false;
		}
		void update(bool x) {
			isClicked = x;
			if (x)time.update(getTime());
		}
		void update() {
			update(ImGui::IsItemClicked());
		}
		bool clicked()const {
			return isClicked;
		}
		bool focused()const {
			return ImGui::IsItemFocused();
		}
		bool get() {
			if (!clicked())return false;
			return (time.now - time.prev) <= ImGui::GetIO().MouseDoubleClickTime;
		}
		operator bool() {
			return get();
		}
	};


	inline std::string ImGuiLeftLabel(const std::string& label) {
		ImGui::Text(label.c_str());
		ImGui::SameLine();
		return "##" + label;
	}



	class ImGuiInputBox {
	public:
		bool inited = false;
		char* buf = 0;
		std::string label;
		uint bufsize = 0;
		ImGuiInputBox() {}
		ImGuiInputBox(const std::string& _label, uint _bufsize = 128) {
			init(_label, _bufsize);
		}
		void init(const std::string& _label, uint _bufsize = 128) {
			label = _label, bufsize = _bufsize;
			buf = new char[bufsize];
			buf[0] = 0;
			inited = true;
		}
		~ImGuiInputBox() {
			//if (inited)delete[] buf;
		}
		void reset() {
			if (!inited)return;
			buf[0] = 0;
		}
		void render(bool showLabel=true) {
			if(showLabel)ImGui::InputText(ImGuiLeftLabel(label).c_str(), buf, bufsize);
			else ImGui::InputText(("##"+label).c_str(), buf, bufsize);
		}
		VirtualValue<std::string> str()const {
			return VirtualValue<std::string>(
				[=](const std::string& t) {strcpy(buf, t.c_str()); },
				[=] {return std::string(buf); }
			);
		}
		ImGuiComponent toComponent()const {
			return ImGuiComponent([this] {
				ImGui::InputText(ImGuiLeftLabel(label).c_str(), buf, bufsize);
				});
		}
		std::string get()const {
			return buf;
		}
	};

	class JsonComponent {
	public:
		typedef std::function<void(JsonData&, JsonData&, const std::string&)> Component;
		std::unordered_map<std::string, Component> components;
		bool firstFrame = true;
		JsonComponent() {
			components["Text"] = [](JsonData& data, JsonData& format, const std::string& label) {
				ImGui::Text((label+": " + data.toStr()).c_str());
				};
			components["Checkbox"] = [](JsonData& data, JsonData& format, const std::string& label) {
				bool t=data.to<int>();
				ImGui::Checkbox(label.c_str(),&t);
				data.to<int>() = t;
				};
			components["RadioList"] = [](JsonData& data, JsonData& format, const std::string& label) {
				ImGui::BulletText(label.c_str());
				int t = data.to<int>();
				for (uint i = 0; i < format[0].size(); i++) {
					ImGui::RadioButton(("##"+label+"-"+format[0][i].toStr()).c_str(), &t, i);
					ImGui::SameLine();
					ImGui::Text(format[0][i].toStr().c_str());
					if (i < format[0].size() - 1 && format[1].to<bool>())
						ImGui::SameLine();
				}
				data.to<int>() = t;
				};
			components["InputInt"] = [](JsonData& data, JsonData& format, const std::string& label) {
				int t = data.to<int>();
				ImGui::InputInt(ImGuiLeftLabel(label).c_str(), &t);
				data.to<int>() = t;
				};
			components["InputFloat"] = [](JsonData& data, JsonData& format, const std::string& label) {
				float t = data.to<float>();
				ImGui::InputFloat(ImGuiLeftLabel(label).c_str(), &t,0.0f,0.0f,format[0].toStr().c_str());
				data.to<float>() = t;
				};
			components["InputBox"] = [](JsonData& data, JsonData& format, const std::string& label) {
				ImGuiInputBox box(label);
				box.str() = data;
				box.render();
				data.toStr() = box.str();
				};
			components["List"]= [](JsonData& data, JsonData& format, const std::string& label) {
				for (auto i : data)
					ImGui::BulletText(i.first.c_str());
				};
		}
		void addCallback(const std::string& name, Component callback) {
			components[name] = callback;
		}
		void render(JsonData& data, JsonData& format, const std::string& label = "",bool root=true) {
			//std::cout << data << "\n\n" << format << "\n\n--\n\n";
			if (format.isList()&&format.size()==2&&format[1].isList()) {
				components[format[0].toStr()](data, format[1], label);
			}
			else if (data.isDict()) {
				for (auto t : data) {
					//std::cout << t.first << ": " << t.second << std::endl;
					if (format.find(t.first)) {
						if (!t.second.isValue()) {
							//ImGui::BulletText(t.first.c_str());
							if (firstFrame)ImGui::SetNextItemOpen(true);
							if (ImGui::TreeNode(t.first.c_str())) {
								render(t.second, format[t.first], t.first,false);
								ImGui::Text("");
								ImGui::TreePop();
							}
						}else
						render(data[t.first], format[t.first], t.first,false);
					}
				}
			}
			else if (data.isList()) {
				for (auto t : data)
					if (format.find(t.first))render(data[t.first], format[t.first], t.first,false);

			}
			if (root)firstFrame = false;
			//exit(0);
		}
	};

	class ImGuiTabs {
	public:
		struct Tab {
			std::string label;
			std::function<void()> f;
		};
		std::string label;
		std::list<Tab> tabs;
		ImGuiTabs() {}
		ImGuiTabs(const std::string& label) :label(label) {}
		void render() {
			ImGui::BeginTabBar(label.c_str());
			for (std::list<Tab>::iterator it = tabs.begin(); it != tabs.end(); it++) {
				bool x = true;
				if (ImGui::BeginTabItem((*it).label.c_str(), &x)) {
					(*it).f();
					ImGui::EndTabItem();
				}
				if (!x) {
					tabs.erase(it);
					ImGui::EndTabBar();
					return;
				}
			}
			ImGui::EndTabBar();
		}
		void add(const Tab& tab) {
			tabs.push_back(tab);
			// ImGui::InputText();
		}
	};

}