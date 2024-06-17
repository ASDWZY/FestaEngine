#include "../Festa.hpp"
#include "../include/utils/audio.h"
#include "../include/utils/gui.h"
#include "../include/utils/game.h"

Mesh Festa::RECT2MESH(vector<float>{
	-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,
}, "2");

Mesh Festa::RECT332MESH(vector<float>{
	-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}, "332");

Mesh Festa::CUBE332MESH(vector<float>{
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
Mesh Festa::CUBELINEMESH(vector<float>{
	0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,

		0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
		0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f,

		0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f,
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

ProgramSource Pickup::program({ STANDARD_VS,PICKUP_FS });
ProgramSource Festa::TEXTURE_PROGRAM({ TEXTURE_VS,TEXTURE_FS });


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

ProgramSource Festa::SKYBOX_PROGRAM({ SKYBOX_VS,SKYBOX_FS });