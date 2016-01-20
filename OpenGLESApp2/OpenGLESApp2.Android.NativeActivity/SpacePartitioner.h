#ifndef SPACEPARTITIONER_H_INCLUDED
#define SPACEPARTITIONER_H_INCLUDED
#include "../../Common/Vector.h"
#include "Collision.h"
#include <list>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace Mai {

  struct Region
  {
	Position3F min;
	Position3F max;
  };

  struct Obj {
	ObjectPtr object;
	Collision::RigidBodyPtr collision;
	Vector3F offset;
  };

  struct Cell
  {
	Region region;
	std::list<Obj> objects;
  };

  class SpacePartitioner
  {
  public:
	typedef std::vector<Cell> CellListType;
	typedef CellListType::iterator iterator;
	typedef CellListType::const_iterator const_iterator;

	SpacePartitioner(const Position3F& min, const Position3F& max, float uy, int /*maxObjects*/)
	{
	  cells.resize(std::ceil((max.y - min.y) / uy));
	  rootRegion.min = min;
	  rootRegion.max = max;
	  unitY = uy;
	  for (int i = 0; i < cells.size(); ++i) {
		cells[i].region.min = Position3F(min.x, min.y + i * unitY, min.z);
		cells[i].region.max = Position3F(max.x, min.y + (i + 1) * unitY, max.z);
	  }
	}
	void Insert(const ObjectPtr& obj, const Collision::RigidBodyPtr& c = Collision::RigidBodyPtr(), const Vector3F& offset = Vector3F::Unit()) {
	  const float y = obj->Position().y;
	  Cell& cell = GetCell(y);
	  cell.objects.push_back({ obj, c, offset });
	  if (c) {
		world.Insert(c);
	  }
	}
	int GetCellId(float y) const {
	  return std::max<int>(
		0,
		std::min<int>(
		  cells.size() - 1,
		  static_cast<int>((y - rootRegion.min.y) / unitY)
		  )
		);
	}
	Cell& GetCell(float y) { return cells[GetCellId(y)]; }
	const Cell& GetCell(float y) const { return cells[GetCellId(y)]; }
	void Update(float f) {
	  world.Step(f);
	  for (auto& c : cells) {
		for (auto& e : c.objects) {
		  if (e.collision) {
			const Position3F pos = e.collision->Position() + e.collision->ApplyRotation(e.offset);
			e.object->SetTranslation(Vector3F(pos));
		  }
		  Object* p = e.object.get();
		  p->Update(f);
		}
	  }
	}
	iterator Begin() { return cells.begin(); }
	iterator End() { return cells.end(); }
	const_iterator Begin() const { return cells.begin(); }
	const_iterator End() const { return cells.end(); }
	void Clear() { cells.clear(); }

  private:
	Collision::World world;
	Region rootRegion;
	float unitY;
	std::vector<Cell> cells;
  };

} // namespace Mai

#endif // SPACEPARTITIONER_H_INCLUDED