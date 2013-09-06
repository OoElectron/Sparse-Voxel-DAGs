/**
 * SparseVoxelOctree.hpp
 * 
 * by Brent Williams
 */

#ifndef SPARSE_VOXEL_OCTREE_HPP
#define SPARSE_VOXEL_OCTREE_HPP

#include "Triangle.hpp"
#include "Intersect.hpp"
#include "Vec3.hpp"
#include "Voxels.hpp"
#include "BoundingBox.hpp"

#include <vector>

class SparseVoxelOctree
{
   private:
      void build(std::vector<Triangle>& triangles, BoundingBox& bb);
      void voxelizeTriangle(Triangle triangle, Voxels& voxels);
   
   public:
      SparseVoxelOctree(std::vector<Triangle>& triangles, BoundingBox &boundingBox);
};


#endif
