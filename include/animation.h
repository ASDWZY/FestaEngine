#pragma once

#include "common/shapes.h"
#include "Window.h"

#define MAX_BONE_INFLUENCE 4
#define MAX_BONES 100

namespace Festa {
	typedef std::unordered_map<std::string, Meshes::BoneData> BoneDatas_t;

	struct MeshNode {
		uint id;
		mat4 trans = mat4(1.0f), offset = mat4(1.0f);
		std::vector<MeshNode*> children;
		~MeshNode() {
			for (MeshNode*& ptr : children)delete ptr;
		}
	};

	class Animation {
	public:
		std::vector<amr_ptr<Bone>> bones;
		amr_ptr<MeshNode> root;
		std::vector<mat4> boneMatrices;
		Animation() {}
		Animation(const aiScene* scene, aiAnimation* animation, BoneDatas_t& boneDatas) {
			//cout << "anim" << endl;
			bones.resize(boneDatas.size());
			boneMatrices.resize(boneDatas.size(), identity4());
			for (uint i = 0; i < animation->mNumChannels; i++) {
				aiNodeAnim* anim = animation->mChannels[i];
				std::string name = anim->mNodeName.C_Str();
				if (boneDatas.find(name) == boneDatas.end()) {
					boneDatas[name].id = uint(boneDatas.size() + 1);
					bones.emplace_back(amr_ptr<Bone>());
					boneMatrices.emplace_back(identity4());
				}
				bones[boneDatas[name].id - 1].ptr = new Bone(anim, name);
			}
			duration = animation->mDuration;
			ticksPerSecond = animation->mTicksPerSecond;
			reset();
			//cout << "anim" << endl;
			root.ptr = buildTree(scene->mRootNode, boneDatas);
		}
		~Animation() {

		}
		void finish() {
			currentTime = duration;
		}
		bool finished()const {
			return currentTime == duration;
		}
		void reset() {
			currentTime = 0.0;
			for (auto& bone : bones)if (bone)bone->reset();
		}
		// seconds
		void update(double dt) {
			if (finished())return;
			currentTime += ticksPerSecond * dt;
			currentTime = std::min(currentTime, duration);
			calcBoneTransform(root.ptr, identity4());
		}
		void setMatrices(Program* program, const std::string& name = "boneMatrices") {
			const std::string prefix = name + "[";
			for (uint i = 0; i < boneMatrices.size(); i++)
				program->setMat4(prefix + toString(i + 1) + "]", boneMatrices[i]);
		}
		double getDuration()const {
			return duration / ticksPerSecond;
		}
		double getCurrentTime()const {
			return currentTime / ticksPerSecond;
		}
		void setCurrentTime(double time) {
			for (auto& bone : bones)if (bone)bone->reset();
			currentTime = time * ticksPerSecond;
			currentTime = std::min(currentTime, duration);
			calcBoneTransform(root.ptr, identity4());
		}
	private:
		double duration = 0.0, currentTime = 0.0, ticksPerSecond = 0.0;
		MeshNode* buildTree(const aiNode* node, BoneDatas_t& boneDatas) {
			MeshNode* n = new MeshNode();
			n->trans = convertMatrix(node->mTransformation);
			std::string name = node->mName.C_Str();
			if (boneDatas.find(name) == boneDatas.end()) {
				n->id = 0;
			}
			else {
				Meshes::BoneData& data = boneDatas[name];
				n->id = data.id;
				n->offset = data.offset;
			}
			for (uint i = 0; i < node->mNumChildren; i++)
				n->children.push_back(buildTree(node->mChildren[i], boneDatas));
			return n;
		}
		void calcBoneTransform(MeshNode* node, const mat4& parentMatrix) {
			mat4 transform = node->trans, matrix(1.0f);
			if (node->id != 0) {
				Bone* bone = bones[node->id - 1].ptr;
				if (bone)transform = bone->update(float(currentTime)).toMatrix();
				matrix = parentMatrix * transform;
				boneMatrices[node->id - 1] = matrix * node->offset;
			}
			for (uint i = 0; i < node->children.size(); i++)
				calcBoneTransform(node->children[i], matrix);
		}
	};


	class ModelLoader :public Meshes {
	public:
		std::vector<Animation*> animations;
		std::vector<Material*> materials;
		std::unordered_map<std::string, Image*> textures;
		std::string file, directory;

		ModelLoader() {}
		ModelLoader(const std::string& path, bool filpuv = true) {
			load(path, filpuv);
		}
		ModelLoader(const char* path, bool filpuv = true) {
			load(path, filpuv);
		}
		void release() {
		}
		~ModelLoader() {
			//release();
		}
		void load(const std::string& path, bool filpuv = true) {
			file = path;
			Assimp::Importer importer;
			uint flag = aiProcess_Triangulate;
			if (filpuv)flag |= aiProcess_FlipUVs;
			//aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
			const aiScene* scene = importer.ReadFile(path, flag);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				LOGGER.error("AssimpError: " + std::string(importer.GetErrorString()));
				return;
			}
			directory = path.substr(0, path.find_last_of('/'));
			materials.resize(scene->mNumMaterials);
			processNode(scene->mRootNode, scene);
			if (scene->mNumAnimations) {
				animations.resize(scene->mNumAnimations);
				for (uint i = 0; i < scene->mNumAnimations; i++)
					animations[i]=new Animation(scene, scene->mAnimations[i], bones);
			}
		}
		void processNode(aiNode* node, const aiScene* scene) {
			std::string name = node->mName.C_Str();
			if (node->mNumMeshes == 1)processMesh(scene->mMeshes[node->mMeshes[0]], scene, name);
			else {
				for (uint i = 0; i < node->mNumMeshes; i++)
					processMesh(scene->mMeshes[node->mMeshes[i]], scene, name + "_" + toString(i));
			}
			for (uint i = 0; i < node->mNumChildren; i++)
				processNode(node->mChildren[i], scene);
		}
		void processMesh(aiMesh* mesh, const aiScene* scene, const std::string& name) {
			Mesh m;
			m.format = "3";
			uint numBones = mesh->mNumBones;
			if (mesh->HasNormals())m.format.append(3);
			if (mesh->mTextureCoords[0]) m.format.append(2);
			if (numBones)m.format += "44";
			uint stride = m.format.stride,idx=0;
			m.vertices.resize(stride * mesh->mNumVertices, 0.0f);
			for (uint i = 0; i < mesh->mNumVertices; i++) {
				m.vertices[idx++] = mesh->mVertices[i].x;
				m.vertices[idx++] = mesh->mVertices[i].y;
				m.vertices[idx++] = mesh->mVertices[i].z;
				if (mesh->HasNormals()) {
					m.vertices[idx++] = mesh->mNormals[i].x;
					m.vertices[idx++] = mesh->mNormals[i].y;
					m.vertices[idx++] = mesh->mNormals[i].z;
				}
				if (mesh->mTextureCoords[0]) {
					m.vertices[idx++] = mesh->mTextureCoords[0][i].x;
					m.vertices[idx++] = mesh->mTextureCoords[0][i].y;
				}
				if (numBones)
					for (uint j = 0; j < 4; j++)m.vertices[idx++] = -1.0f;
			}
			if (numBones)
				for (uint i = 0; i < numBones; i++)processBone(mesh->mBones[i], stride, &m.vertices[0]);

			uint numIndices = 0;
			for (uint i = 0; i < mesh->mNumFaces; i++)
				numIndices += mesh->mFaces[i].mNumIndices;
			m.indices.resize(numIndices); idx = 0;
			for (uint i = 0; i < mesh->mNumFaces; i++) {
				aiFace& face = mesh->mFaces[i];
				for (uint j = 0; j < face.mNumIndices; j++)
					m.indices[idx++]=face.mIndices[j];
			}
			uint boneid = bones[name].id;
			meshes.push_back({ m,getMaterial(mesh,scene),boneid,name });
		}
		void getTexture(aiMaterial* material, aiTextureType type,Texture& texture) {
			aiString str; material->GetTexture(type, 0, &str);
			std::string name = str.C_Str();
			Image*& img = textures[name];
			if (!img)img = new Image(directory + "/" + std::string(str.C_Str()));
			texture.init(*img);
		}
		Material* getMaterial(aiMesh* mesh, const aiScene* scene) {
			uint mati = mesh->mMaterialIndex;
			Material*& mat = materials[mati];
			if (mat)return mat;
			mat = new Material();
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			aiColor3D color;
			if (material->GetTextureCount(aiTextureType_DIFFUSE))
				getTexture(material, aiTextureType_DIFFUSE, mat->diffuseMap);
			else if (material->GetTextureCount(aiTextureType_HEIGHT))
				//cout << "height\n";
				getTexture(material, aiTextureType_HEIGHT, mat->diffuseMap);
			else {
				material->Get(AI_MATKEY_COLOR_AMBIENT, color);
				mat->ambient = vec3(color.r, color.g, color.b);
				material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
				mat->diffuse = vec3(color.r, color.g, color.b);
			}

			if (material->GetTextureCount(aiTextureType_SPECULAR))
				getTexture(material, aiTextureType_SPECULAR, mat->specularMap);
			else {
				material->Get(AI_MATKEY_COLOR_SPECULAR, color);
				mat->specular = vec3(color.r, color.g, color.b);
			}
			material->Get(AI_MATKEY_SHININESS, mat->shininess);
			if (mat->shininess < EPS_FLOAT)mat->shininess = 1.0f;
			return mat;
		}
		void processBone(aiBone* bone, uint stride, float* vertices) {
			uint boneID = 0;
			std::string name = bone->mName.C_Str();
			if (bones.find(name) == bones.end()) {
				boneID = uint(bones.size() + 1);//from 1 to 100
				bones[name] = BoneData({ boneID,convertMatrix(bone->mOffsetMatrix) });
			}
			else boneID = bones[name].id;
			for (uint i = 0; i < bone->mNumWeights; i++)
				setBoneData(vertices, stride, bone->mWeights[i].mVertexId, boneID, bone->mWeights[i].mWeight);
		}
	private:
		void setBoneData(float* vertices, uint stride, uint vid, uint boneid, float weight)const {
			for (int i = 0; i < 4; i++) {
				uint idx = vid * stride + stride - 8 + i;
				if (vertices[idx] < 0.0f) {
					vertices[idx] = float(boneid);
					vertices[idx + 4] = weight;
					return;
				}
			}
		}
	};





	class Model :public Transform {
	public:
		struct TriMesh {
			VAO vao;
			//Material material;
			const Material* material=nullptr;
			AABB aabb; uint boneid = 0;
			TriMesh() {}
			void init(const Mesh& mesh, const Material* m) {
				vao.ptr = new _VAO(mesh);
				material = m;
				//material.ptr = m;
			}
			~TriMesh() {
				//if (material)delete material;
				//cout << "tri del "<<vao.ptr<<","<<material<<endl;
			}
		};
		int currentAnimation = -1;
		std::vector<TriMesh> meshes;
		std::vector<amr_ptr<Animation>> animations;
		AABB aabb;
		Model() {}
		Model(const Meshes& m) {
			load(m);
		}
		Model(const ModelLoader& m) {
			load(m);
		}
		~Model() {
		}
		void setAnimation(uint idx) {
			if (idx >= animations.size())LOGGER.error("Invaild animation index");
			currentAnimation = idx;
		}
		void setMatrices()const {
			Program* program = Program::activeProgram;
			program->setMat4("model", toMatrix());
			if (currentAnimation != -1)
				animations[currentAnimation]->setMatrices(program);
		}
		void drawMesh(uint i)const {
			//cout << "mm " << bool(meshes[i].material) << bool(meshes[i].vao) << endl;
			meshes[i].material->bind("material");
			meshes[i].vao->draw();
		}
		void drawMeshInstanced(uint i, uint count)const {
			meshes[i].material->bind("material");
			meshes[i].vao->drawInstanced(count);
		}
		void updateAnimation(float speed = 1.0f) {
			if (currentAnimation == -1)return;
			animations[currentAnimation]->update(timer.interval() * speed);
		}
		void draw()const {
			setMatrices();
			//cout << "draw" << hasAnimations() << endl;
			for (uint i = 0; i < meshes.size(); i++)drawMesh(i);
		}
		void drawInstanced(uint count)const {
			setMatrices();
			for (uint i = 0; i < meshes.size(); i++)drawMeshInstanced(i, count);
		}
		void setTransform(const Transform& trans) {
			pos = trans.pos; rot = trans.rot; scaling = trans.scaling;
		}
		Animation* animation()const {
			if (currentAnimation == -1)return nullptr;
			else return animations[currentAnimation].ptr;
		}
		bool hasAnimations()const {
			return animations.size();
		}
		void instanceBuffer(GLBuffer buffer, const std::string& format) {
			for (int i = 0; i < meshes.size(); i++)meshes[i].vao->instanceBuffer(buffer, format);
		}
		AABB getAABB() {
			if (hasAnimations()) {
				aabb.clear();
				Animation& anim = *animation();
				for (TriMesh& mesh : meshes) {
					aabb.update(mesh.aabb * anim.boneMatrices[mesh.boneid]);
				}
			}
			return aabb * toMatrix();
		}
		void load(const Meshes& m) {
			uint size = uint(m.meshes.size());
			meshes.resize(size);
			for (uint i = 0; i < size; i++) {
				meshes[i].init(m.meshes[i].mesh, m.meshes[i].material);
				meshes[i].aabb.update(m.meshes[i].mesh);
				aabb.update(meshes[i].aabb);
				meshes[i].boneid = m.meshes[i].boneid;
			}
		}
		void load(const ModelLoader& m) {
			uint size = uint(m.meshes.size());
			meshes.resize(size);
			for (uint i = 0; i < size; i++) {
				meshes[i].init(m.meshes[i].mesh, m.meshes[i].material);
				meshes[i].aabb.update(m.meshes[i].mesh);
				aabb.update(meshes[i].aabb);
				meshes[i].boneid = m.meshes[i].boneid;
			}
			animations.resize(m.animations.size());
			for (uint i = 0; i < m.animations.size(); i++)
				animations[i].ptr = m.animations[i];
			if (animations.size()) setAnimation(0);
		}
	protected:
		FrameTimer timer;
		bool firstFrame = true;
	};



}

