#pragma once

#include "common/common.h"
#include "Program.h"

//#include <opencv2/opencv.hpp>


namespace Festa {
	namespace CV2 {
		enum VideoCaptureAPIs {
			CAP_ANY = 0,            //!< Auto detect == 0
			CAP_VFW = 200,          //!< Video For Windows (obsolete, removed)
			CAP_V4L = 200,          //!< V4L/V4L2 capturing support
			CAP_V4L2 = CAP_V4L,      //!< Same as CAP_V4L
			CAP_FIREWIRE = 300,          //!< IEEE 1394 drivers
			CAP_FIREWARE = CAP_FIREWIRE, //!< Same value as CAP_FIREWIRE
			CAP_IEEE1394 = CAP_FIREWIRE, //!< Same value as CAP_FIREWIRE
			CAP_DC1394 = CAP_FIREWIRE, //!< Same value as CAP_FIREWIRE
			CAP_CMU1394 = CAP_FIREWIRE, //!< Same value as CAP_FIREWIRE
			CAP_QT = 500,          //!< QuickTime (obsolete, removed)
			CAP_UNICAP = 600,          //!< Unicap drivers (obsolete, removed)
			CAP_DSHOW = 700,          //!< DirectShow (via videoInput)
			CAP_PVAPI = 800,          //!< PvAPI, Prosilica GigE SDK
			CAP_OPENNI = 900,          //!< OpenNI (for Kinect)
			CAP_OPENNI_ASUS = 910,          //!< OpenNI (for Asus Xtion)
			CAP_ANDROID = 1000,         //!< Android - not used
			CAP_XIAPI = 1100,         //!< XIMEA Camera API
			CAP_AVFOUNDATION = 1200,         //!< AVFoundation framework for iOS (OS X Lion will have the same API)
			CAP_GIGANETIX = 1300,         //!< Smartek Giganetix GigEVisionSDK
			CAP_MSMF = 1400,         //!< Microsoft Media Foundation (via videoInput)
			CAP_WINRT = 1410,         //!< Microsoft Windows Runtime using Media Foundation
			CAP_INTELPERC = 1500,         //!< RealSense (former Intel Perceptual Computing SDK)
			CAP_REALSENSE = 1500,         //!< Synonym for CAP_INTELPERC
			CAP_OPENNI2 = 1600,         //!< OpenNI2 (for Kinect)
			CAP_OPENNI2_ASUS = 1610,         //!< OpenNI2 (for Asus Xtion and Occipital Structure sensors)
			CAP_GPHOTO2 = 1700,         //!< gPhoto2 connection
			CAP_GSTREAMER = 1800,         //!< GStreamer
			CAP_FFMPEG = 1900,         //!< Open and record video file or stream using the FFMPEG library
			CAP_IMAGES = 2000,         //!< OpenCV Image Sequence (e.g. img_%02d.jpg)
			CAP_ARAVIS = 2100,         //!< Aravis SDK
			CAP_OPENCV_MJPEG = 2200,         //!< Built-in OpenCV MotionJPEG codec
			CAP_INTEL_MFX = 2300,         //!< Intel MediaSDK
			CAP_XINE = 2400,         //!< XINE engine (Linux)
		};
	}
	enum FBOtype {
		FBO_DRAW_BUFFER, FBO_DEPTH_BUFFER
	};

	class FBO {
	public:
		int width, height;
		FBOtype type;
		FBO() { id = rbo = width = height = 0; type = FBO_DRAW_BUFFER; }
		FBO(int width_, int height_, FBOtype type_ = FBO_DRAW_BUFFER) {
			init(width_,height_,type_);
		}
		uint ID()const { return id; }
		void init(int width_, int height_, FBOtype type_ = FBO_DRAW_BUFFER) {
			width = width_, height = height_, type = type_;
			release();
			glGenFramebuffers(1, &id);
			bind();

			if (type == FBO_DRAW_BUFFER) {
				glGenRenderbuffers(1, &rbo);
				glBindRenderbuffer(GL_RENDERBUFFER, rbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
			}
			else if (type == FBO_DEPTH_BUFFER) {
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
			}
			//check();
			//cout << "check1";
			unbind();
		}
		void release() {
			if(id)glDeleteFramebuffers(1, &id);
			if (rbo)glDeleteRenderbuffers(1, &rbo);
		}
		~FBO() {
			release();
		}
		void bind()const {
			glBindFramebuffer(GL_FRAMEBUFFER, id);
		}
		void begin()const {
			bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, width, height);
		}
		static void unbind() {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		static void check() {
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				LOGGER.error("The framebuffer is not complete");
		}
	private:
		uint id, rbo;
	};


	class Image {
	public:
		void* _mat;
		Image();
		Image(const char* file) {
			load(file);
		}
		Image(const std::string& file) {
			load(file);
		}
		/*Image(int x, int y, int w, int h) {
			uchar* data = new uchar[3 * w * h];
			glReadPixels(x, y, w, h, GL_BGR, GL_UNSIGNED_BYTE, data);
			cv::Mat frame(h, w, CV_8UC3, data);
			cv::flip(frame, mat, 0);
		}*/
		Image(void* data, int w, int h, int c) {
			init(data, w, h, c);
		}
		Image(const vec4& color, int w, int h) {
			init(color, w, h);
		}
		Image(const vec3& color, int w, int h) {
			init(color, w, h);
		}
		Image(float color, int w, int h) {
			init(color, w, h);
		}

		void init(void* data, int w, int h, int c);
		void init(const vec4& color, int w, int h);
		void init(const vec3& color, int w, int h);
		void init(float color, int w, int h);

		Image(const Image& x);
		void operator=(const Image& x);
		void release();
		~Image();
		void show(const std::string& title, int delay = 0)const;
		void save(const std::string& path)const;
		int width()const;
		int height()const;
		int channels()const;
		uchar* data()const;
		uchar& get(int x,int y,int c=0)const {
			return data()[ull(channels())*(ull(y)*ull(width())+ull(x))+ull(c)];
		}
		bool empty()const;
		void resetChannels(int value);
		void bgr2rgb();
		void putImage(Image* img, int x, int y) {
			for (int i = 0; i < img->width(); i++) {
				int  u = x + i;
				if (u < 0)continue;
				else if (u >= width())break;
				for (int j = 0; j < img->height(); j++) {
					int v = y + j;
					if (v < 0)continue;
					else if (v >= height())break;
					for (int c = 0; c < img->channels(); c++)
						get(u, v, c) = img->get(i,j,c);
				}
			}
		}
		void resize(float fx, float fy);
		void resize(Image* img, float fx, float fy)const;
		void resize(int w, int h);
		void resize(Image* img, int w, int h)const;
	private:
		void load(const std::string& file);
	};


	class Texture1d {
	public:
		Texture1d() { id = 0; }
		Texture1d(const void* data, uint size, uint format = GL_RGB, uint type = GL_FLOAT) {
			init(data, size, format, type);
		}
		void random(uint size, uint format = GL_RGB, double low = 0.0f, double high = 1.0f) {
			float* data;
			uint channels = 3;
			if (format == GL_RED)channels = 1;
			size *= channels;
			data = new float[size];
			for (uint i = 0; i < size; i++)data[i] = float(randf(low, high));
			init(data, size, format);
		}
		void bind()const {
			glBindTexture(GL_TEXTURE_1D, id);
		}
	private:
		uint id;
		void init(const void* data, uint size, int format = GL_RGB, uint type = GL_FLOAT) {
			glGenTextures(1, &id);
			bind();
			glTexImage1D(GL_TEXTURE_1D, 0, format, size, 0, format, type, data);
			glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);

		}
	};

	class Texture {
	public:
		//static Program program;
		Texture() {
			id = 0;
		}
		Texture(const Image& img) {
			release();
			uint from = GL_BGR, to = GL_RGB;
			if (img.channels() == 4)from = GL_BGRA, to = GL_RGBA;
			else if (img.channels() == 1)from = GL_RED, to = GL_RED;
			generate(img.width(), img.height(), img.data(), from, to);
			parameters();
		}
		void init(const Image& img) {
			release();
			uint from = GL_BGR, to = GL_RGB;
			if (img.channels() == 4)from = GL_BGRA, to = GL_RGBA;
			else if (img.channels() == 1)from = GL_RED, to = GL_RED;
			generate(img.width(), img.height(), img.data(), from, to);
			parameters();
		}
		void init(const FBO& fbo, uint format = GL_RGB, uint filter = GL_LINEAR) {
			fbo.bind();
			uint type = GL_UNSIGNED_BYTE, attachment = GL_COLOR_ATTACHMENT0;
			if (fbo.type == FBO_DEPTH_BUFFER)format = GL_DEPTH_COMPONENT, type = GL_FLOAT, attachment = GL_DEPTH_ATTACHMENT;
			generate(fbo.width, fbo.height, 0, format, format, type);
			minFilter(filter);
			magFilter(filter);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0);
			fbo.check();
			FBO::unbind();
		}
		uint ID()const { return id; }
		Texture(const FBO& fbo, uint format = GL_RGB, uint filter = GL_LINEAR) {
			init(fbo, format, filter);
		}
		void release() {
			if (id) {
				glDeleteTextures(1, &id);
				id = 0;
			}
		}
		~Texture() {
			release();
		}
		void bind()const {
			glBindTexture(GL_TEXTURE_2D, id);
		}
		void bind(uint texture_id)const {
			glActiveTexture(GL_TEXTURE0 + texture_id);
			glBindTexture(GL_TEXTURE_2D, id);
		}
		void bind(const std::string& name,uint texture_id)const {
			if (texture_id <= 31) glActiveTexture(GL_TEXTURE0 + texture_id);
			glBindTexture(GL_TEXTURE_2D, id);
			if (Program::activeProgram)Program::activeProgram->setInt(name,texture_id);
		}
		static void unbind() {
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		static void wrapping(int param = GL_REPEAT) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);
		}
		static void borderColor(const vec4& color = vec4(0.0f, 0.0f, 0.0f, 1.0f)) {
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &color.x);
		}
		static void minFilter(int param = GL_NEAREST) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);
		}
		static void magFilter(int param = GL_LINEAR) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);
		}
		static void parameters(int wrapParam = GL_REPEAT, int minFilterParam = GL_LINEAR_MIPMAP_LINEAR,
			int magFilterParam = GL_LINEAR, const vec4& border = vec4(0.0f, 0.0f, 0.0f, 1.0f)) {
			wrapping(wrapParam);
			borderColor(border);
			minFilter(minFilterParam);
			magFilter(magFilterParam);
			if ((0x2700 <= minFilterParam && minFilterParam <= 0x2703) ||
				(0x2700 <= magFilterParam && magFilterParam <= 0x2703))glGenerateMipmap(GL_TEXTURE_2D);
		}
		void generate(int width, int height, const void* pixels, uint from, uint to, uint type = GL_UNSIGNED_BYTE) {
			glGenTextures(1, &id);
			bind();
			glTexImage2D(GL_TEXTURE_2D, 0, to, width, height, 0, from, type, pixels);
		}
		bool empty()const {
			return !id;
		}
		operator bool()const {
			return id;
		}
	private:
		uint id;
	};

	class Video {
	public:
		Video() {}
		Video(const Path& file) {
			init(file);
		}
		Video(int device) {
			init(device);
		}
		~Video() {
			release();
		}
		void init(const Path& file);
		void init(int device);
		VirtualValue<int> width();
		VirtualValue<int> height();
		VirtualValue<double> fps();
		int frameCount()const;
		double fps()const;
		void posFrames(int pos);
		void release();
		Image* read();
		void operator>>(Image& img)const;
		void operator>>(Texture& texture)const;
		void show(const std::string& title);
	private:
		void* _cap;
		//cv::VideoCapture cap;
	};

	class VideoWriter {
	public:
		VideoWriter() {}
		VideoWriter(const std::string& file, int fps, int width, int height, int fourcc = CV2::CAP_OPENCV_MJPEG) {
			open(file, fps, width, height, fourcc);
		}
		void open(const std::string& file, int fps, int width, int height, int fourcc = CV2::CAP_OPENCV_MJPEG);
		void release();
		~VideoWriter() {
			release();
		}
		void write(const Image& image);
		void write(const Video& video);
	private:
		void* _writer;
		//cv::VideoWriter writer;
	};
}
