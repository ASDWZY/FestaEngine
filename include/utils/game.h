#pragma once

#include "../Window.h"


namespace Festa {
    template<typename T>
    struct Coord2 {
        T x = 0, y = 0, prex = 0, prey = 0;
        void update(T x_, T y_) {
            prex = x; prey = y;
            x = x_; y = y_;
        }
        vec2 dir()const {
            return vec2(float(x - prex), float(y - prey));
        }
    };

    inline void KeyboardPosition(const Window& win, vec3& x, float vel,
        const vec3& right = VEC3X, const vec3& front = vec3(0.0f, 0.0f, -1.0f)) {
        if (win.getKey(GLFW_KEY_A) == GLFW_PRESS)x -= vel * right;
        if (win.getKey(GLFW_KEY_D) == GLFW_PRESS)x += vel * right;
        if (win.getKey(GLFW_KEY_W) == GLFW_PRESS)x += vel * front;
        if (win.getKey(GLFW_KEY_S) == GLFW_PRESS)x -= vel * front;
        if (win.getKey(GLFW_KEY_E) == GLFW_PRESS)x.y += vel;
        if (win.getKey(GLFW_KEY_C) == GLFW_PRESS)x.y -= vel;
    }

    struct FrameVel {
        Timer timer;
        float low = 0.0f, high = 0.0f, dt = 0.0f;
        bool first = true;
        FrameVel() {}
        FrameVel(float low, float high) :low(low), high(high) {
            timer.reset();
        }
        float velocity(bool highMode) {
            if (first) {
                first = false;
                timer.reset();
                dt = 0.0f;
                return 0.0f;
            }
            dt = float(timer.interval());
            timer.reset();
            if (highMode)return high * dt;
            else return low * dt;
        }
    };

    struct KeyboardAccelerator :public FrameVel {
        int key = -1;
        KeyboardAccelerator() {}
        KeyboardAccelerator(float low_)
        {
            low = low_;
            timer.reset();
        }
        void setAccel(float accelSpeed, int key_ = GLFW_KEY_V) {
            high = accelSpeed;
            key = key_;
        }
        float velocity(const Window& win) {
            if (first) {
                first = false;
                timer.reset();
                return dt = 0.0f;
            }
            dt = float(timer.interval());
            timer.reset();
            if (key != -1 && win.getKey(key) == GLFW_PRESS)return high * dt;
            else return low * dt;
        }
    };
    struct WASDCameraController {
        Camera* camera = 0;
        KeyboardAccelerator accelerator;
        WASDCameraController() {}
        WASDCameraController(Camera* camera, float speed = 5.0f) :camera(camera) {
            accelerator = KeyboardAccelerator(speed);
        }
        void apply(const Window& win) {
            if (!win.isFocused())return;
            float vel = accelerator.velocity(win);
            vec3 right = camera->right(), front = camera->front();
            right.y = front.y = 0.0f; right = normalize(right), front = normalize(front);
            if (win.getKey(GLFW_KEY_A) == GLFW_PRESS)camera->pos -= right * vel;
            if (win.getKey(GLFW_KEY_D) == GLFW_PRESS)camera->pos += right * vel;
            if (win.getKey(GLFW_KEY_W) == GLFW_PRESS)camera->pos += front * vel;
            if (win.getKey(GLFW_KEY_S) == GLFW_PRESS)camera->pos -= front * vel;
        }
    };
    struct MouseDragging {
        Coord2<int> mouse;
        bool init = true;
        bool update(const Window& window) {
            if (window.getMouse().left.up()||!window.getMouse().inWindow()) {
                init = true;
                mouse = Coord2<int>();
                return false;
            }
            mouse.update(window.getMouse().winPos().x, window.getMouse().winPos().y);
            if (init) {
                init = false;
                return false;
            }
            return window.isFocused();
        }
        vec2 dir()const {
            return mouse.dir();
        }
        vec2 begin()const {
            return vec2(float(mouse.prex), float(mouse.prey));
        }
        vec2 end()const {
            return vec2(float(mouse.x), float(mouse.y));
        }
    };
    struct DraggingCameraController {
        Camera* camera = 0;
        float sensitivity = 0.0f;
        MouseDragging dragging;
        DraggingCameraController() {}
        DraggingCameraController(Camera* camera, float sensitivity = 0.1f) :camera(camera), sensitivity(sensitivity) {

        }
        void apply(const Window& window) {
            if (dragging.update(window)) {
                vec2 dir = dragging.dir() * sensitivity;
                camera->roll += glm::radians(dir.y);
                camera->pitch += glm::radians(dir.x);
            }
        }

    };

    struct KeyboardCameraController {
        float sensitivity = 0.1f;
        WASDCameraController* base = 0;
        KeyboardCameraController() {}
        KeyboardCameraController(WASDCameraController* base, float sensitivity = 1.0f) :base(base), sensitivity(sensitivity) {}
        void apply(const Window& win) {
            if (!win.isFocused())return;
            float deltaTime = base->accelerator.dt;
            float vel = base->accelerator.velocity(win);
            vec3 right = base->camera->right(), front = base->camera->front();
            right.y = front.y = 0.0f;
            right = normalize(right), front = normalize(front);
            KeyboardPosition(win, base->camera->pos, vel, right, front);

            if (KEY_DOWN(VK_UP))base->camera->roll += sensitivity * deltaTime;
            if (KEY_DOWN(VK_DOWN))base->camera->roll -= sensitivity * deltaTime;
            if (KEY_DOWN(VK_LEFT))base->camera->pitch += sensitivity * deltaTime;
            if (KEY_DOWN(VK_RIGHT))base->camera->pitch -= sensitivity * deltaTime;
        }
    };

    struct CameraController1 {
        Window* window = 0;
        Camera* camera = 0;
        WASDCameraController wasd;
        DraggingCameraController dcc;
        KeyboardCameraController kcc;
        CameraController1() {}
        CameraController1(Window* window, Camera* camera) :window(window), camera(camera) {
            dcc = DraggingCameraController(camera);
            wasd = WASDCameraController(camera);
            wasd.accelerator.setAccel(10.0f);
            kcc = KeyboardCameraController(&wasd);
        }
        void update() {
            dcc.apply(*window);
            kcc.apply(*window);
        }
    };


    class Joystick {
    public:
        std::vector<ButtonStatus> buttons;
        std::unordered_map<std::string, uint> buttonMap;
        Joystick() {
            ID = Assign++;
            joyinfoex.dwSize = sizeof(JOYINFOEX);
            joyinfoex.dwFlags = JOY_RETURNALL;
            init();
        }
        void update() {
            state = joyGetPosEx(ID, &joyinfoex);
            for (uint i = 0; i < buttons.size(); i++) {
                buttons[i].update(joyinfoex.dwButtons & (1 << i));
            }
        }
        std::string name()const {
            return wstring2string(caps.szPname);
        }
        bool unplugged() {
            return state != 0;
            //return state == JOYERR_PARMS || state == JOYERR_UNPLUGGED;
        }
        static uint numJoysticks() {
            return Assign;
        }
        void init() {
            buttons.clear();
            state = joyGetDevCaps(ID, &caps, sizeof(caps));
            if (state != JOYERR_PARMS && state != JOYERR_UNPLUGGED)
                switch (joyGetDevCaps(ID, &caps, sizeof(caps))) {
                case 0:
                    break;
                case MMSYSERR_NODRIVER:
                    LOGGER.error("No Diver for Joystick");
                    break;
                default:
                    LOGGER.error("Unknown Joystick Error: " + toString(state));
                    break;
                }
            //cout << "mid " << caps.wMid << endl;
            //cout << "pid " << caps.wPid << endl;
            //cout << "name " << name() << endl;
            buttons.resize(caps.wNumButtons, ButtonStatus());
        }
        double X()const {
            return double(joyinfoex.dwXpos - caps.wXmin)
                / double(caps.wXmax - caps.wXmin) * 2.0f - 1.0f;
        }
        double Y()const {
            return -double(joyinfoex.dwYpos - caps.wYmin)
                / double(caps.wYmax - caps.wYmin) * 2.0f + 1.0f;
        }
        double Z()const {
            return double(joyinfoex.dwZpos - caps.wZmin)
                / double(caps.wZmax - caps.wZmin) * 2.0f - 1.0f;
        }
        double U()const {
            return double(joyinfoex.dwUpos - caps.wUmin)
                / double(caps.wUmax - caps.wUmin) * 2.0f - 1.0f;
        }
        double R()const {
            return -double(joyinfoex.dwRpos - caps.wRmin)
                / double(caps.wRmax - caps.wRmin) * 2.0f + 1.0f;
        }
        double V()const {
            return double(joyinfoex.dwVpos - caps.wVmin)
                / double(caps.wVmax - caps.wVmin) * 2.0f - 1.0f;
        }
        uint POV()const {
            return joyinfoex.dwPOV;
        }
        static vec2 circle(vec2 dir) {
            const float len = length(dir);
            if (len > 1.0f)dir /= len;
            return dir;
        }
        vec2 XY()const {
            return vec2(float(X()), float(Y()));
        }
        vec2 UR()const {
            return vec2(float(U()), float(R()));
        }
        vec2 circleXY()const {
            return circle(XY());
        }
        vec2 circleUR()const {
            return circle(UR());
        }
        const ButtonStatus& button(uint button) {
            return buttons[button];
        }
        const ButtonStatus& button(const std::string& button) {
            return buttons[buttonMap[button]];
        }
    private:
        static uint Assign;
        JOYINFOEX joyinfoex;
        uint ID = 0;
        JOYCAPS caps;
        MMRESULT state = 0;
    };

    struct JoystickCameraController {
        Camera* camera = 0;
        float sensitivity = 0.1f;
        FrameVel frame;
        JoystickCameraController() {}
        JoystickCameraController(Camera* camera, float speed = 10.0f, float sensitivity = 1.0f)
            :camera(camera), sensitivity(sensitivity) {
            frame = FrameVel(speed, 0);
        }
        void apply(const Window& win, Joystick& joystick) {
            if (!win.isFocused())return;
            joystick.update();
            if (joystick.unplugged())return;
            float vel = frame.velocity(false);
            if (joystick.button(2).pressed())vel *= 2;
            vec3 right = camera->right(), front = camera->front();
            right.y = front.y = 0.0f; right = normalize(right), front = normalize(front);
            vec2 dir = joystick.circleXY() * vel;
            camera->pos += right * dir.x + front * dir.y;
            camera->pos.y += vel * float(joystick.Z());
            float sen = sensitivity * frame.dt;
            camera->roll += float(joystick.R()) * sen;
            camera->pitch -= float(joystick.U()) * sen;
        }
    };

    struct Pickup {
        static ProgramSource program;
    };
    struct Pickup1:public Pickup {
        Camera* camera = nullptr;
        uint T,off;
        Pickup1() {}
        Pickup1(uint T,uint off,Camera* camera)
            :T(T),off(off),camera(camera) {
            
        }
        vec3 toColor(uint id) {
            //cout << "id " << id << " color: ";
            id++;
            vec3 ret;
            ret.z = float(id % T) / float(T - 1); id /= T;
            ret.y = float(id % T) / float(T - 1); id /= T;
            ret.x = float(id % T) / float(T - 1); id /= T;
            ret /= float(T - 1);
            //printvec3(ret);
            return ret;
        }
        int toID(const vec3& color) {
            float tmp = T - off,t=T;
            std::cout << (color.x + color.y * t + color.z * t * t) * float(T - 1) << std::endl;
            return int((color.x + color.y * t + color.z * t * t)* float(T - 1))-1;
        }
        uint maxID()const {
            return T * T * T - off-1;
        }
        void start(uint id) {
            program.get().bind();
            camera->bind();
            program.get().setVec3("color", toColor(id));
        }
        void render(uint id,std::function<void()>& renderFunc) {
            start(id);
            renderFunc();
        }
        void render(uint id,const Model& model) {
            start(id);
            model.draw();
        }
        static void clear() {
            glClearColor(0.0f,0.0f,0.0f,0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
        int getID(int x,int y) {
            Viewport viewport; viewport.load();
            //vec3 color;
            uchar data[3] = {0,0,0};
            glReadPixels(x,viewport.h-y,1,1,GL_RGB,GL_UNSIGNED_BYTE,data);
            vec3 color=vec3(float(data[0]),float(data[1]),float(data[2]));
            color /= 255.0f;
            std::cout << int(data[0]) << "," << int(data[1]) << "," << int(data[2]) << std::endl;
            //cout << "color ";
            //printvec3(color);
            return toID(color);
            std::cout << x << "," << y << std::endl;
            int t = T;
            return (int(data[0]) + int(data[1]) * t + int(data[2]) * t * t) * (T - 1)/255 - 1;
        }
    };

    struct Pickup2 :public Pickup {
        Camera* camera = nullptr;
        uint maxID;
        Pickup2() {}
        Pickup2(uint maxID,Camera* camera)
            :maxID(maxID),camera(camera) {

        }
        void start(uint id) {
            program.get().bind();
            camera->bind();
            program.get().setVec3("color", vec3(float(id+1)/float(maxID), 0.0f, 0.0f));
        }
        void render(uint id, std::function<void()>& renderFunc) {
            start(id);
            renderFunc();
        }
        void render(uint id, const Model& model) {
            start(id);
            model.draw();
        }
        static void clear() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
        int getID(int x, int y) {
            Viewport viewport; viewport.load();
            uchar data[3] = { 0,0,0 };
            glReadPixels(x, viewport.h - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
            return int(float(data[0])/255.0f*float(maxID)+0.5) - 1;
        }
    };

    inline void rotateCamera(Window& window, Camera& camera, 
        const vec3& ori,const vec3& to, 
        float time, const std::function<void()>& renderScene) {
        //window.update();
        const vec3 st=camera.getEulerAngle();
        const vec3 t=to-st,v = t / time,old=camera.pos;
        float dur = 0.0f;
        FrameTimer timer;
        while (dur<time) {
            if (window.update() != 1)return;
            const vec3 delta = v * dur;
            camera.setEulerAngle(st+delta);
            camera.pos = rotateOri(old,ori,Rotation(delta.x,delta.y,delta.z));
            dur += float(timer.interval());
            renderScene();
            
        }
        camera.setEulerAngle(to);
        camera.pos = rotateOri(old, ori, Rotation(t.x,t.y,t.z));
        renderScene();
    }

    struct ImageButton {
        uchar st = 0;
        ButtonStatus state;
        Texture texture;
        vec2 size,pos;
        ImageButton() {}
        ImageButton(const Image& img, const vec2& _pos){
            init(img, _pos);
        }
        ImageButton(const Image& img, const vec2& _size, const vec2& _pos) {
            init(img,_size, _pos);
        }
        void init(const Image& img, const vec2& _pos) {
            size = vec2(img.width(), img.height());
            texture.init(img);
            pos = _pos + size / 2.0f;
        }
        void init(const Image& img, const vec2& _size, const vec2& _pos){
            size = _size; 
            texture.init(img);
            pos = _pos + size / 2.0f;
        }
        void setPosition(const vec2& _pos) {
            pos = _pos + size / 2.0f;
        }
        void render(const Window& window){
            float f = 1.0f;
            if (st == 1)f = 0.7f;
            else if (st == 2)f = 0.3f;
            vec2 p = (window.viewport.pos() + pos) / (window.viewport.size() / 2.0f);
            p = vec2(p.x - 1.0f, 1.0f - p.y);
            drawTexture(texture, 
                translate4(vec3(p,0.0f))
                * scale4(vec3(size/window.viewport.size(),0.0f)),scale4(vec3(f)));
        }
        void check(const Window& window) {
            vec2 v = window.getMouse().winPos()
                ,p1=pos-size/2.0f,p2=pos+size/2.0f;
            st= window.getMouse().inWindow()&&p1.x<=v.x&&v.x<=p2.x&&p1.y<=v.y&&v.y<=p2.y;
            if (st&&window.getMouse().left.pressed())st++;
            //if (st)cout << st << endl;
        }
        bool update(const Window& window) {
            check(window);
            render(window);
            state.update(st == 2);
            return st == 2;
        }
    };
}
