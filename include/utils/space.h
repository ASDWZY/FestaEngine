#include "../common/shapes.h"
#include "../animation.h"
#include "../FestaEngine/assets.h"

namespace Festa {
	class QuadTree {
	public:
		struct Node {
			vector<Model*> models;
			amr_ptr<Node> children[4];
		};
		Node root;
		AABB space;
		QuadTree() {}
		QuadTree(const AABB& space) :space(space) {}
		static AABB NodeAABB(const AABB& aabb, int i) {
			AABB tmp;
			vec3 c = aabb.center();
			tmp.updateX(c.x); tmp.updateZ(c.z);
			tmp.updateY(aabb.min.y); tmp.updateY(aabb.max.y);
			if (i == 0) {
				tmp.updateX(aabb.max.x); tmp.updateZ(aabb.min.z);
			}
			else if (i == 1) {
				tmp.updateX(aabb.min.x); tmp.updateZ(aabb.min.z);
			}
			else if (i == 2) {
				tmp.updateX(aabb.min.x); tmp.updateZ(aabb.max.z);
			}
			else if (i == 3) {
				tmp.updateX(aabb.max.x); tmp.updateZ(aabb.max.z);
			}
			return tmp;
		}
		static int findChildren(const AABB& aabb, const AABB& model) {
			vec3 c = aabb.center();
			bool left = model.max.x <= c.x, right = model.min.x >= c.x,
				top = model.max.z <= c.z, bottom = model.min.z >= c.z;
			if (right && top)return 0;
			else if (left && top)return 1;
			else if (left && bottom)return 2;
			else if (right && bottom)return 3;
			else return -1;
		}
		void insert(Model& model) {
			Node* node = &root; AABB aabb = space;
			while (1) {
				int i = findChildren(aabb, model.getAABB());
				//cout << "find " << i << endl;
				if (i == -1)break;
				if (!node->children[i])node->children[i].ptr = new Node();
				node = node->children[i].ptr; aabb = NodeAABB(aabb, i);
			}
			node->models.push_back(&model);
		}
		void build(const vector<Model*>& models) {
			for (uint i = 0; i < models.size(); i++)
				space.update(models[i]->getAABB());
			for (uint i = 0; i < models.size(); i++)
				insert(*models[i]);
		}
		void build(const Assets& assets) {
			for (auto& i : assets.models)
				space.update(i.second->ptr->getAABB());
			for (auto& i : assets.models)
				insert(*i.second->ptr);
		}
	};


}

