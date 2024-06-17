#pragma once
#include "app.h"

#define KEY_SHOW_HIDDEN "Show hidden files and directories"
#define KEY_AUTO_SRC "Automatically find source files"
#define KEY_ASSET_CHDIR "Chdir when opening assets"

inline Image extractIcon(const wstring& filePath) {
	SHFILEINFO shfi;
	SHGetFileInfo(filePath.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON);
	HICON fileIcon = shfi.hIcon;
	HDC dc = GetDC(NULL);
	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	HDC iconDC = CreateCompatibleDC(dc);
	HBITMAP hBitmap = CreateCompatibleBitmap(dc, iconWidth, iconHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(iconDC, hBitmap);
	DrawIconEx(iconDC, 0, 0, fileIcon, iconWidth, iconHeight, 0, NULL, DI_NORMAL);

	Image ret(vec4(0.0f), iconWidth, iconHeight);
	GetBitmapBits(hBitmap, iconHeight * iconWidth * 4, ret.data());

	ReleaseDC(NULL, dc);
	SelectObject(iconDC, hOldBitmap);
	DeleteDC(iconDC);
	DestroyIcon(fileIcon);
	DeleteObject(hBitmap);

	return ret;
}

inline void setClipBoard(const string& content) {
	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		HGLOBAL hClipboardData;
		//const char* data = "Hello, this is a text to be copied to clipboard.";
		//int size = strlen(data) + 1; // Include null terminator

		hClipboardData = GlobalAlloc(GMEM_DDESHARE, content.size());
		char* buffer = (char*)GlobalLock(hClipboardData);
		strcpy(buffer, content.c_str());
		GlobalUnlock(hClipboardData);

		SetClipboardData(CF_TEXT, hClipboardData);
		CloseClipboard();
	}
}



struct IconExtractor {
	std::unordered_map<string, Texture*> icons;
	Texture* assetIcon=0;
	IconExtractor() {
		
	}
	~IconExtractor() {
		for (auto& i : icons) {
			SafeDelete(i.second);
		}
		SafeDelete(assetIcon);
	}
	Texture* newTexture(const Path& filePath) {
		SHFILEINFO shfi;
		wstring tmp = string2wstring(filePath.toWindows());
		SHGetFileInfo(tmp.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON);
		HICON fileIcon = shfi.hIcon;
		HDC dc = GetDC(NULL);
		int iconWidth = GetSystemMetrics(SM_CXSMICON);
		int iconHeight = GetSystemMetrics(SM_CYSMICON);
		HDC iconDC = CreateCompatibleDC(dc);
		HBITMAP hBitmap = CreateCompatibleBitmap(dc, iconWidth, iconHeight);
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(iconDC, hBitmap);
		DrawIconEx(iconDC, 0, 0, fileIcon, iconWidth, iconHeight, 0, NULL, DI_NORMAL);

		Image img(vec4(0.0f), iconWidth, iconHeight);
		GetBitmapBits(hBitmap, iconHeight * iconWidth * 4, img.data());

		ReleaseDC(NULL, dc);
		SelectObject(iconDC, hOldBitmap);
		DeleteDC(iconDC);
		DestroyIcon(fileIcon);
		DeleteObject(hBitmap);
		//img.show(filePath.extension());
		return new Texture(img);
	}
	Texture* getAssetIcon() {
		if (assetIcon)return assetIcon;
		else {
			Image img("src/icon/asset.jpg");
			alphaImage(img);
			return assetIcon = new Texture(img);
		}
	}
	Texture* extract(const Path& file) {
		string extension = file.extension();
		Texture*& icon = icons[extension];
		if(icon)return icon;
		else return icon = newTexture(file);
	}
};

IconExtractor ICON_EXTRACTOR;

#define RELOAD_FRENQUECY 0.3f// json
class ImGuiFolderTree:public BasePage {
public:
	struct Node {
		Path path;
		string label;
		map<string, Node> children;
		Node* father = 0;
		LeafType type=NOT_LEAF;
		ButtonStatus openTree;
		bool rename = false,focused=false,hidden=false,src=false;// save tree open in json
		ImGuiInputBox box;
		Texture* icon = 0;
		Node() {}
		~Node() {
			label.clear();
			children.clear();
		}
		void init(App& app,const Path& _path,Node* _father) {
			path = _path; father = _father;
			if (!path.isDirectory()) {
				string p = path.extension();
				if (p == "cpp" || p == "c")type = LEAF_SRC;
				else if (p == "h" || p == "hpp")type = LEAF_HEAD;
				else if (p == "glsl" || p == "vs" || p == "gs" || p == "fs")type = LEAF_GLSL;
				else if (p == "png" || p == "jpg" || p == "bmp")type = LEAF_PIC;
				else if (p == "mp4" || p == "wav")type = LEAF_AUDIO;
				else type = LEAF_TEXT;

				if (!p.size())icon = ICON_EXTRACTOR.icons["txt"];
				else icon = ICON_EXTRACTOR.extract(path);
			}
			else {
				icon = ICON_EXTRACTOR.extract(path);
				build(app);
			}
			label = path.back();
			hidden = label == "CMakeLists.txt" || label[0] == '.';
			src = app.isSourceFile(path);
		}
		bool isLeaf() {
			return type != NOT_LEAF;
		}
		void build(App& app) {
			vector<Path> arr;
			path.glob(arr);
			for (Path& p : arr) {
				if (children.find(p.back().toString()) != children.end())continue;
				if (p.back().toString() == AssetCode) {
					type = LEAF_ASSET;
					icon = ICON_EXTRACTOR.getAssetIcon();
				}
				children[p.back().toString()].init(app,p, this);
			}
			bool changed = false;
			for (auto it = children.begin(); it != children.end(); it++) {
				if (!it->second.path.exists()) {
					changed = true; break;
				}
			}
			if (changed) {
				map<string, Node> tmp = children;
				for (auto it = children.begin(); it != children.end(); it++) {
					if (!it->second.path.exists())tmp.erase(it->first);
				}
				children = tmp;
			}
		}
		void beginRename() {
			focused = true;
			rename = true;
			if (!box.inited)box.init("rename");
			else box.reset();
			box.str() = label;
		}
		void endRename(App& app) {
			rename = false;
			Path newPath = path.previous() + Path(box.str());
			if (!string(box.str()).size() || newPath.exists())return;
			path.rename(newPath);
			renameTabs(app,newPath);
			children.clear();
		}
		void renameTabs(App& app,const Path& newPath) {
			if (isLeaf()) {
				app.renameFile(path,newPath);
				if (type != LEAF_ASSET)return;
			}
			for (auto& i : children)
				i.second.renameTabs(app, newPath + i.first);
		}
		void selectable(App& app,Node*& current) {
			bool tmp = false;
			if (icon) {
				float s = ImGui::GetTextLineHeight();
				ImGui::Image((void*)icon->ID(), ImVec2(s,s)); 
				tmp |= ImGui::IsItemHovered();
				ImGui::SameLine();
			}
			if (src) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.2f, 1.0f));
				ImGui::Text("SRC");
				tmp |= ImGui::IsItemHovered();
				ImGui::PopStyleColor(1);
				ImGui::SameLine();
			}
			if (rename) {
				box.render(false);
				tmp |= ImGui::IsItemHovered();
			}
			else {
				tmp|=ImGui::Selectable(string2u8(label).c_str(),&focused, ImGuiSelectableFlags_SpanAllColumns);
			}
			bool menu;
			if (isLeaf() && this != current)menu = fileMenu(app, current);
			else menu = dirMenu(app, current);
			if (ImGui::IsWindowHovered()&&!menu&&
				(ImGui::IsMouseDown(0) ||ImGui::IsMouseDown(1)))
				focused = tmp;
				
			if (menu)focused = true;
			
			if (rename&&!focused)endRename(app);
		}
		bool treeNode(App& app,Node*& current) {
			if(openTree.firstPressed())
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			bool ret = ImGui::TreeNode(("##" + label).c_str());
			openTree.update(ret);
			ImGui::SameLine(); selectable(app,current);
			if (openTree.firstPressed()||(!rename && ImGui::IsMouseDoubleClicked(0)&&ImGui::IsItemHovered())) {
				openTree.update(true);
			}
			return ret;
		}
		void Chdir(Node*& current) {
			current = this;
			openTree.update(true);
		}
		void deleteNode(App& app) {
			if (src)app.removeSourceFile(path);
			else if (!isLeaf()) {
				for (auto& i:children) {
					i.second.deleteNode(app);
				}
			}
		}
		bool dirMenu(App& app,Node*& current) {
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("New File")) {
					uint i = 0;
					while ((path + Path("NewFile_" + toString(i))).exists())i++;
					Path p = path + Path("NewFile_" + toString(i));
					p.createFile();
					Node& n = children[p.back().toString()];
					n.init(app, p, this);
					n.beginRename();
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("New Directory")) {
					uint i = 0;
					while ((path + Path("NewFolder_" + toString(i))).exists())i++;
					Path p = path + Path("NewFolder_" + toString(i));
					p.createDirectory();
					Node& n = children[p.back().toString()];
					n.init(app, p, this);
					n.beginRename();
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("New Asset")) {
					uint i = 0;
					while ((path + Path("NewAsset_" + toString(i))).exists())i++;
					Path p = path + Path("NewAsset_" + toString(i));
					p.createDirectory();
					(p + Path(AssetCode)).createFile();
					Node& n = children[p.back().toString()];
					n.init(app, p, this);
					n.beginRename();
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("Chdir")) {
					Chdir(current);
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("Rename")) {
					beginRename();
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("Delete")) {
					path.deleteDirectory();
					deleteNode(app);
					ImGui::EndMenu();
					return true;
				}
				ImGui::EndPopup();
				return true;
			}
			return false;
		}

		bool fileMenu(App& app,Node*& current) {
			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Rename")) {
					beginRename();
					ImGui::EndMenu();
					return true;
				}
				if (ImGui::MenuItem("Delete")) {
					if (type == LEAF_ASSET) {
						if (this == current)current = father;
						path.deleteDirectory();
					}
					else path.deleteFile();
					deleteNode(app);
					ImGui::EndMenu();
					return true;
				}
				if (type == LEAF_SRC&&!app.project.cmake.data[KEY_AUTO_SRC].to<int>()) {
					if (src && ImGui::MenuItem("Remove from Source Files")) {
						src = false;
						app.removeSourceFile(path);
						ImGui::EndMenu();
						return true;
					}
					else if (!src && ImGui::MenuItem("Add to Source Files")) {
						src = true;
						app.addSourceFile(path);
						ImGui::EndMenu();
						return true;
					}
				}
				ImGui::EndPopup();
				return true;
			}
			return false;
		}
		void open(App& app) {
			app.openFile(path, type);
		}
		void CV(App& app) {
			if(
				app.window.getKey(GLFW_MOD_CONTROL|GLFW_KEY_V) && OpenClipboard(NULL)) {
				HANDLE hData = GetClipboardData(CF_HDROP);
				if (hData) {
					HDROP hDrop = (HDROP)GlobalLock(hData);
					if (hDrop) {
						UINT numFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
						for (UINT i = 0; i < numFiles; i++) {
							wchar_t filePath[MAX_PATH];
							if (DragQueryFile(hDrop, i, filePath, MAX_PATH)) {
								Path np = path.getDirectory() + Path(filePath).back();
								if (np.exists())continue;
								CopyFile(filePath, string2wstring(np.toWindows()).c_str(), FALSE);
							}
						}
						GlobalUnlock(hDrop);
					}
				}
				CloseClipboard();
			}
		}
		void renderTree(App& app, Node*& current) {
			build(app);
			bool treeOpen = treeNode(app, current);
			if (focused) {
				CV(app);
			}
			if (treeOpen) {
				for (auto& ch : children)
					if(!ch.second.isLeaf())
						ch.second.render(app, current);
				for (auto& ch : children)
					if (ch.second.type==LEAF_ASSET)
						ch.second.render(app, current);
				for (auto& ch : children)
					if (ch.second.isLeaf()&& ch.second.type != LEAF_ASSET)
						ch.second.render(app, current);
				ImGui::TreePop();
				if (!children.size())ImGui::NewLine();
			}else if (focused) {
				CV(app);
			}
		}
		void render(App& app,Node*& current) {
			if (hidden&&
				app.config["Assets"][KEY_SHOW_HIDDEN].to<int>() == 0)return;
			if (isLeaf()) {
				if (type == LEAF_SRC&&!src&&app.project.cmake.data[KEY_AUTO_SRC].to<int>()) {
					src = true;
					app.addSourceFile(path);
				}
				selectable(app, current);
				if (focused) {
					CV(app);
				}
				if (!rename&& ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
					open(app);
					if (type == LEAF_ASSET&&app.config["Assets"][KEY_ASSET_CHDIR].to<int>()) {
						Chdir(current);
					}
				}
				return;
			}
			renderTree(app,current);
		}
		void renderRoot(App& app, Node*& current,Node* root) {
			if (ImGui::Button(" < "))current = father;
			ImGui::SameLine(); if (ImGui::Button("Home"))current = root;
			renderTree(app,current);
		}
	};
	Node root;
	Node* current = 0;
	Path directory;
	App* app=0;
	ImGuiFolderTree() {}
	void init(App& _app,const Path& _directory) {
		app = &_app;
		root = Node();
		directory = _directory;
		root.init(*app,directory,&root);
		root.openTree.update(true);
		current = &root;
	}
	void render() {
		if (current)
			current->renderRoot(*app,current,&root);
	}
};

class AssetsTree:public Plugin {
public:
	ImGuiFolderTree tree;
	AssetsTree() {
		name = "Assets";
	}
	~AssetsTree() {
	}
	void init(App& app) {
		tree.init(app,app.projectDir());
		app.setWindow("Assets",WindowView(app.config,"Assets"), &tree,false);
	}
	void open(App& app) {
		init(app);
	}
	void run(App& app) {
		
	}
};
