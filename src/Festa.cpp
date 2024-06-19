//#include "3rd/imgui/imgui.cpp"

#include "Festa.hpp"
#include "include/Font.h"
#include "include/animation.h"
#include "include/utils/audio.h"
#include "include/utils/gui.h"
#include "include/utils/game.h"

#define LIB_OPENCV
//#define LIB_IRRKLANG

using namespace Festa;

#ifdef LIB_OPENCV
#include<opencv2/opencv.hpp>
#ifdef _DEBUG
#pragma comment(lib,"libs/opencv_world440d.lib")
#else
#pragma comment(lib,"libs/opencv_world440.lib")
#endif
#define toMat(ptr) (*(cv::Mat*)(ptr))
#define toVideoCapture(ptr) (*(cv::VideoCapture*)(ptr))

Image::Image() {
	_mat = new cv::Mat();
}
Image::~Image() {
	release();
	cv::Mat* tmp = (cv::Mat*)_mat;
	SafeDelete(tmp);
	_mat = 0;
}

void Image::load(const std::string& file) {
	Path path(file);
	path.check(ACCESS_MODE_READ);
	_mat = new cv::Mat;
	toMat(_mat) = cv::imread(path);
	if (toMat(_mat).empty())LOGGER.error("Failed to read the image: " + path.str());
}

void Image::init(void* data, int w, int h, int c) {
	_mat = new cv::Mat(h, w, CV_MAKETYPE(CV_8U, c), data);
}
void Image::init(const vec3& color, int w, int h) {
	_mat = new cv::Mat(h, w, CV_8UC3, cv::Scalar(uchar(color.z * 255.0f),
		uchar(color.y * 255.0f), uchar(color.x * 255.0f)));
}
void Image::init(const vec4& color, int w, int h) {
	_mat = new cv::Mat(h, w, CV_8UC4, cv::Scalar(uchar(color.z * 255.0f),
		uchar(color.y * 255.0f), uchar(color.x * 255.0f), uchar(color.w * 255.0f)));
}
void Image::init(float color, int w, int h) {
	_mat = new cv::Mat(h, w, CV_8UC1, cv::Scalar(uchar(color * 255.0f)));
}
Image::Image(const Image& x) {
	_mat = new cv::Mat();
	toMat(x._mat).copyTo(toMat(_mat));
}
void Image::operator=(const Image& x) {
	if (!_mat)_mat = new cv::Mat();
	toMat(x._mat).copyTo(toMat(_mat));
}
void Image::release() {
	toMat(_mat).release();
}
void Image::show(const std::string& title, int delay)const {
	if (!height() || !width() || !channels())return;
	cv::imshow(title, toMat(_mat));
	cv::waitKey(delay);
}
void Image::save(const std::string& path)const {
	cv::imwrite(path, toMat(_mat));
}
int Image::width()const {
	return toMat(_mat).cols;
}
int Image::height()const {
	return toMat(_mat).rows;
}
int Image::channels()const {
	return toMat(_mat).channels();
}
uchar* Image::data()const {
	return toMat(_mat).data;
}
bool Image::empty()const {
	return toMat(_mat).empty();
}
void Image::resetChannels(int value) {
	int c = channels(), code;
	if (c == value)return;
	if (value == 1) {
		if (c == 3)code = cv::COLOR_BGR2GRAY;
		else code = cv::COLOR_BGRA2GRAY;
	}
	else if (value == 3) {
		if (c == 1)code = cv::COLOR_GRAY2BGR;
		else code = cv::COLOR_BGRA2BGR;
	}
	else if (value == 4) {
		if (c == 1)code = cv::COLOR_GRAY2BGRA;
		else code = cv::COLOR_BGR2BGRA;
	}
	else LOGGER.error("Invaild image channels: " + toString(value));
	cv::cvtColor(toMat(_mat), toMat(_mat), code);
}
void Image::bgr2rgb() {
	int c = channels(), code;
	if (c == 1)return;
	else if (c == 3)code = cv::COLOR_BGR2RGB;
	else code = cv::COLOR_BGRA2RGBA;
	cv::cvtColor(toMat(_mat), toMat(_mat), code);
}
void Image::resize(float fx, float fy) {
	cv::resize(toMat(_mat), toMat(_mat), cv::Size(0, 0), fx, fy);
}
void Image::resize(Image* img, float fx, float fy)const {
	cv::resize(toMat(_mat), toMat(img->_mat), cv::Size(0, 0), fx, fy);
}
void Image::resize(int w, int h) {
	cv::resize(toMat(_mat), toMat(_mat), cv::Size(w, h));
}
void Image::resize(Image* img, int w, int h)const {
	cv::resize(toMat(_mat), toMat(img->_mat), cv::Size(w, h));
}


void VideoWriter::open(const std::string& file, int fps, int width, int height, int fourcc) {
	_writer = new cv::VideoWriter();
	((cv::VideoWriter*)_writer)->open(file, fourcc, fps, cv::Size(width, height));
}
void VideoWriter::release() {
	if (!_writer)return;
	((cv::VideoWriter*)_writer)->release();
	cv::VideoWriter* tmp = (cv::VideoWriter*)_writer;
	SafeDelete(tmp);
	_writer = nullptr;
}
void VideoWriter::write(const Image& image) {
	(*(cv::VideoWriter*)_writer) << toMat(image._mat);
}
void VideoWriter::write(const Video& video) {
	Image frame;
	while (1) {
		video >> frame;
		if (frame.empty())break;
		write(frame);
	}
}

void Video::init(const Path& file) {
	_cap = new cv::VideoCapture();
	toVideoCapture(_cap).open(file.toString());
}
void Video::init(int device) {
	_cap = new cv::VideoCapture();
	toVideoCapture(_cap).open(device);
}
VirtualValue<int> Video::width() {
	return VirtualValue<int>([&](int val) {
		toVideoCapture(_cap).set(cv::CAP_PROP_FRAME_WIDTH, val);
		}, [&] {
			return (int)toVideoCapture(_cap)
				.get(cv::CAP_PROP_FRAME_WIDTH);
			});
}
VirtualValue<int> Video::height() {
	return VirtualValue<int>([&](int val) {
		toVideoCapture(_cap).set(cv::CAP_PROP_FRAME_HEIGHT, val);
		}, [&] {
			return (int)toVideoCapture(_cap)
				.get(cv::CAP_PROP_FRAME_HEIGHT);
			});
}
VirtualValue<double> Video::fps() {
	return VirtualValue<double>([&](double val) {
		toVideoCapture(_cap).set(cv::CAP_PROP_FPS, val);
		}, [&] {
			return toVideoCapture(_cap)
				.get(cv::CAP_PROP_FPS);
			});
}
int Video::frameCount()const {
	return (int)toVideoCapture(_cap).get(cv::CAP_PROP_FRAME_COUNT);
}
void Video::posFrames(int pos) {
	toVideoCapture(_cap).set(cv::CAP_PROP_POS_FRAMES, pos);
}
void Video::release() {
	if (!_cap)return;
	toVideoCapture(_cap).release();
	cv::VideoCapture* tmp = (cv::VideoCapture*)_cap;
	SafeDelete(tmp);
	_cap = nullptr;
}
Image* Video::read() {
	Image* frame = new Image;
	toVideoCapture(_cap) >> toMat(frame->_mat);
	return frame;
}
void Video::operator>>(Image& frame)const {
	toVideoCapture(_cap) >> toMat(frame._mat);
}
void Video::operator>>(Texture& texture)const {
	Image frame; toVideoCapture(_cap) >> toMat(frame._mat);
	texture.init(frame);
}
void Video::show(const std::string& title) {
	Image frame;
	while (1) {
		toVideoCapture(_cap) >> toMat(frame._mat);
		if (frame.empty())break;
		frame.show(title, 1);
	}
}
#endif

#ifdef LIB_IRRKLANG
#pragma comment(lib,"libs/irrKlang.lib")
inline ISoundEngine*& getSoundEngine() {
	if (!Sound::engine) {
		Sound::engine = createIrrKlangDevice();
		if (!Sound::engine)LOGGER.error("Failed to init irrKlang");
	}
	return Sound::engine;
}

void Sound::init(const std::string& _file, bool effects, bool looped) {
	file = _file;
	sound = getSoundEngine()->play2D(file.c_str(), looped, true, true, ESM_AUTO_DETECT, effects);
	if (!sound)LOGGER.error("Failed to create the sound: " + file);
	if (effects) fx = sound->getSoundEffectControl();
	else fx = 0;
	sound3d = false;
}
void Sound::init(const std::string& _file, const vec3& pos, bool effects, bool looped) {
	file = _file;
	sound = getSoundEngine()->play3D(file.c_str(), vec3irr(pos), looped, true, true, ESM_AUTO_DETECT, effects);
	if (!sound)LOGGER.error("Failed to create the sound: " + file);
	if (effects) fx = sound->getSoundEffectControl();
	else fx = 0;
	sound3d = true;
}

void Sound::reload() {
	bool looped = isLooped(), effects = fx;
	vec3 position = pos();
	sound->drop();
	if (sound3d)sound = getSoundEngine()->play3D(file.c_str(), vec3irr(position), looped, true, true, ESM_AUTO_DETECT, effects);
	else sound = getSoundEngine()->play2D(file.c_str(), looped, true, true, ESM_AUTO_DETECT, effects);
	if (!sound)LOGGER.error("Failed to create the sound: " + file);
	if (effects) fx = sound->getSoundEffectControl();
	else fx = 0;
}
void Sound::setListener(const Camera& camera) {
	vec3 pos = camera.pos, front = camera.front();
	getSoundEngine()->setListenerPosition(vec3irr(pos), vec3irr(front));
}
#endif


#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"libs/glew32s.lib")
#pragma comment(lib,"libs/glfw3.lib")

#pragma comment(lib,"libs/assimp-vc140-mt.lib")
#pragma comment(lib,"libs/freetype.lib")

Logger Festa::LOGGER;
Program* Program::activeProgram = 0;
bool Window::first = true;
ISoundEngine* Sound::engine;
uint Joystick::Assign = 0;

PFNWGLSWAPINTERVALFARPROC Festa::wglSwapIntervalEXT = 0;




static void initEngine(const Version2& glVersion,bool resizable) {
	LOGGER.addTask([](const std::string& msg) {
		Window::messagebox(0,msg, "Festa Error", MB_OKCANCEL, MB_ICONERROR); 
		std::cout<<Logger::coloredString(msg,1)<< std::endl;
		}, LOG_LEVEL_ERROR);
	LOGGER.addTask([](const std::string& msg) {
		Window::messagebox(0, msg, "Festa Warning", MB_OKCANCEL, MB_ICONWARNING);
		std::cout << Logger::coloredString(msg, 1) << std::endl;
		}, LOG_LEVEL_WARNING);
	srand((uint)time(0)); 

	if (!glfwInit())LOGGER.error("Failed to init glfw");
	glfwWindowHint(GLFW_RESIZABLE, resizable);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glVersion.major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glVersion.minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

static void initAfterWindow() {
	glewExperimental = GL_TRUE;
	if (glewInit())LOGGER.error("Failed to init glew");

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(0);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icex);
}
static void freeBuffers() {
	static bool done = false;
	if (done)return;
	if (Sound::engine)Sound::engine->drop();
	if (FreetypeFont::ft)FT_Done_FreeType(FreetypeFont::ft);
	glfwTerminate();
	done = true;
}

void Window::init(int w, int h,const std::string& title, const Version2& _glVersion,bool resizable) {
	glVersion = _glVersion;
	if (first)initEngine(glVersion,resizable);
	ptr = glfwCreateWindow(w, h, title.c_str(), NULL, NULL);
	if (!ptr)LOGGER.error("Failed to create the window: " + title);
	glfwMakeContextCurrent(ptr);
	if (first) {
		initAfterWindow();
		first = false;
	}
	viewport = Viewport(w, h);
	viewport.apply();
	hwnd = glfwGetWin32Window(ptr);
	mouse.window = this;
}
void Window::release() {
	glfwDestroyWindow(ptr);
	freeBuffers();
}

FT_Library FreetypeFont::ft=0;
void FreetypeFont::init(const std::string& file, uint height, uint width) {
	if(!ft)if (FT_Init_FreeType(&ft))
		LOGGER.error("Failed to init freetype");
	if (FT_New_Face(ft, file.c_str(), 0, &face))
		LOGGER.error("Failed to load the font:" + file);
	FT_Set_Pixel_Sizes(face, width, height);
}




Path Festa::askOpenFileName(HWND hwnd, const wchar_t* filter, const String& title, const wchar_t* initDir) {
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	wchar_t szFile[256] = { 0 };
	wchar_t titleptr[256] = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = title.towstring().c_str();
	ofn.nMaxFileTitle = title.size() ? 1 : 0;
	ofn.lpstrInitialDir = initDir;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileName(&ofn))return Path(std::wstring(ofn.lpstrFile));
	else return Path();
}

Path Festa::askSaveFileName(HWND hwnd, const wchar_t* filter, const String& title, const wchar_t* initDir) {
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	wchar_t szFile[256] = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = title.towstring().c_str();
	ofn.nMaxFileTitle = title.size() ? 1 : 0;
	ofn.lpstrInitialDir = initDir;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetSaveFileName(&ofn))return Path(std::wstring(ofn.lpstrFile));
	else return Path();
}


std::ostream& operator<< (std::ostream& out,const String& str) {
	out << str.str;
	return out;
}


Mesh Festa::RECT2MESH(std::vector<float>{
	-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,
}, "2");

Mesh Festa::RECT332MESH(std::vector<float>{
	-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}, "332");

Mesh Festa::CUBE332MESH(std::vector<float>{
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
}, "332");
Mesh Festa::CUBELINEMESH(std::vector<float>{
		0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
		0.5f, 0.5f, -0.5f,  -0.5f, 0.5f,- 0.5f,

		0.5f, 0.5f, 0.5f,  0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
		0.5f, 0.5f, -0.5f,  0.5f, -0.5f, -0.5f,

		0.5f, 0.5f, 0.5f,  0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, 0.5f,  0.5f, -0.5f, -0.5f,
}, "3");
VAOSource Festa::RECT2(RECT2MESH), Festa::RECT332(RECT332MESH), 
Festa::CUBE332(CUBE332MESH), Festa::CUBELINE(CUBELINEMESH);

ShaderSource Festa::TEXTURE_VS(GL_VERTEX_SHADER,
	"#version 440 core\n"
	"layout(location=0)in vec2 Pos;"
	"uniform mat4 posTrans;out vec2 texCoord;"
	"void main(){"
	"   gl_Position=posTrans*vec4(Pos,0.0f,1.0f);\n"
	"	texCoord=vec2(Pos.x*0.5f+0.5f,-Pos.y*0.5f+0.5f);\n"
	"}"),
	Festa::TEXTURE_FS(GL_FRAGMENT_SHADER,
		"#version 440 core\n"
		"uniform sampler2D tex;uniform mat4 colorTrans;\n"
		"in vec2 texCoord;\n"
		"out vec4 FragColor;\n"
		"void main(){FragColor=colorTrans*texture(tex,texCoord);}"
	),
	Festa::STANDARD_VS(GL_VERTEX_SHADER,
		"#version 440 core\n"
		"layout(location = 0)in vec3 pos;""layout(location = 1)in vec3 normal;""layout(location = 2)in vec2 texcoord;""layout(location = 3)in vec4 boneids;""layout(location = 4)in vec4 weights;"
"const int MAX_BONES = 100;"
"uniform mat4 boneMatrices[MAX_BONES + 1];uniform mat4 projection;uniform mat4 view;uniform mat4 model;"
"out vec3 Normal;out vec3 FragPos;out vec2 texCoord;"
"void main() {"
	"vec4 position = vec4(0.0f);"
	"for (int i = 0; i < 4; i++) {"
		"if (boneids[i] < 0.0f)continue;"
		"else if (boneids[i] == 0.0f || boneids[i] > MAX_BONES) {"
			"position = vec4(pos, 1.0f);"
			"break;"
		"}"
		"position += boneMatrices[int(boneids[i])] * vec4(pos, 1.0f);"
	"}"
	"gl_Position = projection * view * model * position;"
	"FragPos = (model * vec4(pos, 1.0)).xyz;"
	"Normal = mat3(transpose(inverse(model))) * normal;"
	"texCoord = texcoord;}"), Festa::PICKUP_FS(GL_FRAGMENT_SHADER,
	"#version 330 core\n"
	"uniform vec3 color;"
	"out vec4 FragColor;"
	"void main(){FragColor=vec4(color,1.0f);}"),
	Festa::STANDARD_FS(GL_FRAGMENT_SHADER,
		"#version 330 core\n"
		"struct Material {sampler2D diffuseMap;sampler2D specularMap;"
		"vec3 ambient;vec3 diffuse;vec3 specular;};"

		"uniform Material material;out vec4 FragColor;in vec2 texCoord;"

		"bool eq(vec3 a, vec3 b) {return dot(a, b) > 0.99;}"

		"void main() {float alpha = 1.0f;"
			"vec3 ambient = material.ambient, diffuse = material.diffuse, specular = material.specular;"
			"if (eq(diffuse, vec3(-1.0f))) {"
				"vec4 temp = texture(material.diffuseMap, texCoord);"
				"alpha = temp.w;ambient = temp.xyz;diffuse = vec3(0.0f);specular = vec3(0.0f);}"
			"FragColor = vec4(ambient + diffuse + specular, alpha);}");

ProgramSource Pickup::program(std::vector<ShaderSource>{ STANDARD_VS,PICKUP_FS });
ProgramSource Festa::TEXTURE_PROGRAM(std::vector<ShaderSource>{ TEXTURE_VS,TEXTURE_FS });


void Festa::drawTexture(const Texture& texture,const mat4& posTrans,const mat4& colorTrans) {
	Program& p = TEXTURE_PROGRAM.get();
	p.bind();
	p.setMat4("posTrans", posTrans);
	p.setMat4("colorTrans", colorTrans);
	texture.bind("tex",0);
	RECT2.get()->draw();
}
void Festa::drawTexture3D(const Texture& texture,const Camera& camera, const Transform& transform, const mat4& colorTrans) {
	static ProgramSource program(std::vector<ShaderSource>{ STANDARD_VS,TEXTURE_FS });
	Program& p = program.get();
	p.bind();
	camera.bind();

	p.setMat4("colorTrans", colorTrans);
	//Transform t = transform;
	//mat4 m = translate4(vec3(0.0f, 0.0f, 0.5f)) * scale4(vec3(1.0f, 1.0f, 0.5f));
	p.setMat4("model", transform.toMatrix());
	texture.bind("tex", 0);
	RECT332.get()->draw();
}
void Festa::drawCuboid(const Material& material,const mat4& trans) {
	if (!Program::activeProgram)return;
	Program* program = Program::activeProgram;
	program->setMat4("model", trans);
	material.bind("material");
	RECT332.get()->draw();
}

ShaderSource Festa::SKYBOX_VS(GL_VERTEX_SHADER,
	"#version 330 core\n"
	"layout(location=0)in vec3 pos;\n"
	"uniform mat4 projection;\n"
	"uniform mat4 view;\n"
	"out vec3 texCoord;\n"
	"void main(){\n"
	"   gl_Position=(projection*view*vec4(pos,1.0f)).xyww;\n"
	"	texCoord=pos;\n"
	"}\n\0"),
	Festa::SKYBOX_FS(GL_FRAGMENT_SHADER,
		"#version 330 core\n"
		"uniform samplerCube skybox;\n"
		"in vec3 texCoord;\n"
		"out vec4 FragColor;\n"
		"void main(){\n"
		"   FragColor=texture(skybox,texCoord);\n"
		"}\n\0");

ProgramSource Festa::SKYBOX_PROGRAM(std::vector<ShaderSource>{ SKYBOX_VS,SKYBOX_FS });

inline void setArray(std::vector<float>& arr,uint index,const vec3& v) {
	arr[index] = v.x; arr[index+1] = v.y; arr[index+2] = v.z;
}

inline void setArrayTriangle(std::vector<float>& vertices, uint index, const Triangle& tri,uint stride=6) {
	vec3 normal = tri.normal();

	setArray(vertices, index, tri.a);
	setArray(vertices, index + 3, normal);
	index += stride;

	setArray(vertices, index, tri.b);
	setArray(vertices, index + 3, normal);
	index += stride;

	setArray(vertices, index, tri.c);
	setArray(vertices, index + 3, normal);
}

inline void setArrayCircle(std::vector<float>& vertices,uint i,float a,float y,uint stride,const vec3& normal) {
	float angle = float(i) * a;
	uint index = i * 3 * stride;
	setArray(vertices, index, vec3(0.0f,y,0.0f));
	setArray(vertices, index + 3, normal);
	index += stride;

	setArray(vertices, index, vec3(cosf(angle) * 0.5f, y, sinf(angle) * 0.5f));
	setArray(vertices, index + 3, normal);
	index += stride;

	setArray(vertices, index, vec3(cosf(angle + a) * 0.5f, y, sinf(angle + a) * 0.5f));
	setArray(vertices, index + 3, normal);
}

void Mesh::circle(int n) {
	format = "33";
	float a = PI * 2.0f / float(n);
	vertices.resize((n + 1)*format.stride,0.0f);
	setArray(vertices, n*format.stride+3, vec3(0.0f, 1.0f, 0.0f));
	indices.resize(n*3);
	for (int i = 0; i < n; i++) {
		float angle = float(i)*a;
		uint index = i*3,iv=i*format.stride;
		setArray(vertices, iv, vec3(cosf(angle) * 0.5f, 0.0f, sinf(angle) * 0.5f));
		setArray(vertices, iv + 3, vec3(0.0f, 1.0f, 0.0f));
		indices[index] = n;
		indices[index + 1] = i;
		indices[index+2] = (i + 1)%n;
	}
}

void Mesh::cone(int n) {
	format = "33";
	float a = PI * 2.0f / float(n);
	vertices.resize(n*2*3 * format.stride, 0.0f);
	for (int i = 0; i < n; i++)setArrayCircle(vertices,i,a,0.0f,format.stride,vec3(0.0f,-1.0f,0.0f));
	for (int i = 0; i < n; i++) {
		float angle = float(i) * a;
		Triangle tri(vec3(0.0f,1.0f,0.0f),
			vec3(cosf(angle) * 0.5f,0.0f, sinf(angle) * 0.5f),
			vec3(cosf(angle + a) * 0.5f,0.0f, sinf(angle + a) * 0.5f));
		setArrayTriangle(vertices, (n + i) * 3 * format.stride, tri, format.stride);
	}
}

void Mesh::cylinder(int n) {
	format = "33";
	float a = PI * 2.0f / float(n);
	vertices.resize(n * 4 * 3 * format.stride, 0.0f);
	for (int i = 0; i < n; i++)setArrayCircle(vertices, i, a, 0.0f, format.stride, vec3(0.0f, -1.0f, 0.0f));
	for (int i = 0; i < n; i++)setArrayCircle(vertices, n+i, a, 1.0f, format.stride, vec3(0.0f, 1.0f, 0.0f));
	for (int i = 0; i < n; i++) {
		float angle = float(i) * a;
		vec3 b0(cosf(angle) * 0.5f, 0.0f, sinf(angle) * 0.5f),
			b1(cosf(angle + a) * 0.5f, 0.0f, sinf(angle + a) * 0.5f),t0,t1;
		t0 = b0; t1 = b1;
		t0.y = 1.0f; t1.y = 1.0f;
		Triangle tri1(t1,t0,b1),tri2(b0,b1,t0);
		setArrayTriangle(vertices, (2*n + i) * 3 * format.stride, tri1, format.stride);
		setArrayTriangle(vertices, (3 * n + i) * 3 * format.stride, tri2, format.stride);
	}
}
std::vector<Command_t> CommandComponent::commands;


