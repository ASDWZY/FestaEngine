#pragma once

#include "Festa.hpp"
#include "include/utils/gui.h"
#include "include/utils/audio.h"
#include "include/FestaEngine/CMake.h"
#include "include/FestaEngine/TextEditor.h"
using namespace std;
using namespace Festa;

enum LeafType {
	NOT_LEAF = -1, LEAF_SRC, LEAF_HEAD, LEAF_GLSL, LEAF_TEXT, LEAF_PIC, LEAF_AUDIO, LEAF_ASSET, LEAF_MAX
};
const Path FESTA_PROJECTS_DIR = FESTA_ROOT + Path("FestaProjects");
const string AssetCode = ".main.festa";

typedef ImGuiTextEditor::EditorParams EditorParams;
namespace Editors {
	EditorParams params;
}
void alphaImage(Image& img, uchar alpha = 255, uchar thres = 180) {
	img.resetChannels(4);
	for (int x = 0; x < img.width(); x++) {
		for (int y = 0; y < img.height(); y++) {
			if (img.get(x, y, 0) > thres &&
				img.get(x, y, 1) > thres &&
				img.get(x, y, 2) > thres)
				img.get(x, y, 3) = 0;
			else img.get(x, y, 3) = alpha;
		}
	}
}

inline LeafType getFileType(const Path& path) {
	if (path.isFile()) {
		string p = path.extension();
		if (p == "cpp" || p == "c")return LEAF_SRC;
		else if (p == "h" || p == "hpp")return LEAF_HEAD;
		else if (p == "glsl" || p == "vs" || p == "gs" || p == "fs")return LEAF_GLSL;
		else if (p == "png" || p == "jpg" || p == "bmp")return LEAF_PIC;
		else if (p == "mp4" || p == "wav")return LEAF_AUDIO;
		else return LEAF_TEXT;
	}
	else return LEAF_ASSET;
}

class ImGuiPage {
public:
	typedef std::function<void(void*)> Func;
	typedef std::function<void(void*,bool)> Releasef;
	void* ptr = 0;
	Func renderf;
	Releasef releasef;
	ImGuiPage() {}
	~ImGuiPage() {
		//release(false);
	}
	template<typename T>
	ImGuiPage(T* page) {
		ptr = page;
		renderf = [](void* ptr) {((T*)ptr)->render(); };
		releasef = [](void* ptr,bool freePtr) {
			((T*)ptr)->release(); 
			if (freePtr) {
				T* tmp = (T*)ptr;
				SafeDelete(tmp);
				ptr = 0;
			}
		};
	}
	void release(bool freePtr) {
		if (ptr) {
			releasef(ptr,freePtr);
		}
	}
	void render() {
		renderf(ptr);
	}
};


class BasePage {
public:
	BasePage() {}
	virtual void render() {}
	virtual void release() {}
};

/*class TextEditor :public BasePage {
public:
	string text;
	Path path;
	char textBuffer[1024] = "";
	TaskTimer saveTimer;
	std::list<string> lines;
	TextEditor() {
		lines.push_back("");
	}
	TextEditor(const Path& _path) :path(_path) {
		saveTimer = TaskTimer(1.0f, [this] {save(); });
		string text; readString(path, text);
		push(text);
		strcpy(textBuffer, text.c_str());
	}
	void save() {
		if (path.empty())return;
		writeString(path, text);
	}
	void release() {
		save();
	}
	void render() {
		ImGui::InputTextMultiline("##TextEditor", textBuffer, sizeof(textBuffer));
		saveTimer.check();
	}
	void push(const string& text) {
		ull tmp = 0;
		for (ull i = 0; i < text.size(); i++) {
			if (text[i] == '\n') {
				buffer.back() += text.substr(tmp, i - tmp);
				buffer.push_back("");
				tmp = i + 1;
			}
		}
		if (tmp < text.size())buffer.back() += text.substr(tmp, text.size() - tmp);
	}
};
*/

class PictureViewer :public BasePage {
public:
	Texture texture;
	ivec2 size;
	PictureViewer() {}
	PictureViewer(const Path& path) {
		Image img(path.toString());
		texture.init(img);
		size = ivec2(img.width(), img.height());
	}
	void render() {
		ImGui::Image((void*)texture.ID(), ImVec2(size.x, size.y));
	}
	void release() {
		texture.release();
	}
};



class TextEditor:public BasePage {
public:
	Path path;
	TaskTimer saveTimer;
	ImGuiTextEditor editor;
	EditorAssistant assistant;
	TextEditor() {
		
	}
	TextEditor(const Path& _path):path(_path) {
		editor.showLineNumber = true;
		editor.params = &Editors::params;
		assistant.init(editor);
		
		saveTimer = TaskTimer(0.7f, [this] {save(); });
		string text; readString(path, text);
		editor.PushText<EditorAssistant>(text);
	}
	void save() {
		if (path.empty())return;
		editor.Save(path);
	}
	void release() {
		save();
	}
	void render() {
		editor.Render<EditorAssistant>();
		saveTimer.check();
	}
};

class CppEditor :public BasePage {
public:
	Path path;
	TaskTimer saveTimer;
	ImGuiTextEditor editor;
	CppAssistant assistant;
	CppEditor() {

	}
	CppEditor(const Path& _path) :path(_path) {
		editor.showLineNumber = true;
		editor.params = &Editors::params;
		assistant.init(editor);
		saveTimer = TaskTimer(0.7f, [this] {save(); });
		string text; readString(path, text);
		editor.PushText<CppAssistant>(text);
	}
	void save() {
		if (path.empty())return;
		editor.Save(path);
	}
	void release() {
		save();
	}
	void render() {
		editor.Render<CppAssistant>();
		saveTimer.check();
	}
};


class AudioPlayer :public BasePage {
public:
	Texture texture;
	Sound sound;
	Video video;
	bool paused = true,firstFrame=true;
	double frequency = 0.0,interval=0.0;
	Timer timer;
	AudioPlayer() {
		
	}
	AudioPlayer(const Path& path) {
		video.init(path);
		paused = false;
		frequency = 1.0 / video.fps();
		interval = 0.0;
		video >> texture;
		firstFrame = true;
	}
	void render() {
		bool tmp = paused;
		ImGui::Checkbox("Paused", &tmp);
		if (!paused && tmp) {
			interval = 0.0;
		}
		else if (paused && !tmp) {
			timer.reset();
		}
		paused = tmp;
		if (firstFrame) {
			firstFrame = false;
			timer.reset();
		}
		if (!paused) {
			interval += timer.interval();
			if (interval >= frequency) {
				video >> texture;
				interval -= frequency;
			}
		}
		//video >> texture;
		timer.reset();
		ImGui::Image((void*)texture.ID(), ImVec2(video.width(),video.height()));
	}
	void release() {
		video.release();
		texture.release();
		sound.release();
	}
};

class AssetEditor : public BasePage {
public:
	AssetEditor() {
		
	}
	AssetEditor(const Path& path) {

	}
};



