/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2015, Caper Holdings, LLC
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef DTVOXEL_VOXELGEOMETRY_H_
#define DTVOXEL_VOXELGEOMETRY_H_

#include <dtVoxel/export.h>
#include <dtPhysics/geometry.h>
#include <dtVoxel/aabbintersector.h>
#include <pal/palFactory.h>
#include <pal/palGeometry.h>
#include <openvdb/openvdb.h>
// remove
#include <iostream>

namespace dtVoxel
{
   template<typename GridType>
   //typedef openvdb::BoolGrid GridType;
   class ColliderCallback : public palCustomGeometryCallback
   {
   public:
      typedef AABBIntersector<GridType> IntersectorType;
      typedef typename GridType::Ptr GridPtr;

      ColliderCallback(const palBoundingBox& shapeBoundingBox, GridPtr grid)
      : palCustomGeometryCallback(shapeBoundingBox)
      , mBoxCollider(new AABBIntersector<GridType>(grid))
      {
      }

      /*override*/ ~ColliderCallback() {}

      /**
       * Override this to return the triangles within the given axis aligned bounding box.
       */
      virtual void operator()(const palBoundingBox& bbBox, TriangleVector& trianglesOut)
      {
         static const int faces[] =
               {
                     0, 2, 1,
                     0, 3, 2, // -x
                     0, 1, 4,
                     6, 0, 4, // +z
                     4, 5, 6,
                     5, 7, 6, // +x
                     2, 3, 5,
                     5, 3, 7, // -z
                     0, 6, 3,
                     3, 6, 7, // +y
                     1, 2, 4,
                     4, 2, 5  // -y
               };
         typedef typename GridType::ValueOnIter GridItr;
         openvdb::BBoxd bb(openvdb::Vec3d(bbBox.min.x,bbBox.min.y,bbBox.min.z), openvdb::Vec3d(bbBox.max.x,bbBox.max.y,bbBox.max.z));
         mBoxCollider->SetWorldBB(bb);
         mBoxCollider->Intersect();
         GridPtr grid = mBoxCollider->GetHits();
         GridItr i = grid->beginValueOn();
         //i.setMinDepth(GridItr::LEAF_DEPTH);
         //i.setMaxDepth(GridItr::LEAF_DEPTH);
         for (; i.test(); ++i)
         {
            if (i.isVoxelValue())
            {
               openvdb::Coord coord = i.getCoord();
               openvdb::BBoxd iBox(openvdb::Vec3d(double(coord.x()), double(coord.y()), double(coord.z())), 0);
               iBox.expand(0.5f);
               openvdb::BBoxd voxelBBox = grid->transform().indexToWorld(iBox);
               typename GridType::ConstAccessor ca = mBoxCollider->GetGrid()->getConstAccessor();
               bool activeNeighbors[6];
               activeNeighbors[0] = ca.isValueOn(openvdb::Coord(coord.x()-1,coord.y(),coord.z()));
               activeNeighbors[1] = ca.isValueOn(openvdb::Coord(coord.x(),coord.y(),coord.z()+1));
               activeNeighbors[2] = ca.isValueOn(openvdb::Coord(coord.x()+1,coord.y(),coord.z()));
               activeNeighbors[3] = ca.isValueOn(openvdb::Coord(coord.x(),coord.y(),coord.z()-1));
               activeNeighbors[4] = ca.isValueOn(openvdb::Coord(coord.x(),coord.y()+1,coord.z()));
               activeNeighbors[5] = ca.isValueOn(openvdb::Coord(coord.x(),coord.y()-1,coord.z()));
               //std::cout << " Neighbors ";
               //for (unsigned n = 0 ; n < 6 ; ++n) std::cout << activeNeighbors[n];
               //std::cout << std::endl;
               openvdb::math::Vec3<Float> min = voxelBBox.min();
               openvdb::math::Vec3<Float> max = voxelBBox.max();
               //std::cout << "voxel min max: " << min << max << std::endl;
               Float cube_vertices[] =
                     {
                           min.x(),  max.y(),  max.z(),
                           min.x(),  min.y(),  max.z(),
                           min.x(),  min.y(),  min.z(),
                           min.x(),  max.y(),  min.z(),
                           max.x(),  min.y(),  max.z(),
                           max.x(),  min.y(),  min.z(),
                           max.x(),  max.y(),  max.z(),
                           max.x(),  max.y(),  min.z()
                     };
               int triCount = 12;
               for (int i=0; i < triCount; ++i)
               {
                  if (!activeNeighbors[i/2])
                  {
                     palTriangle tri;
                     //std::cout << "triangle ";
                     for (unsigned j = 0; j < 3; ++j)
                     {
                        tri.vertices[j].x = cube_vertices[3 * faces[3*i + j] + 0];
                        tri.vertices[j].y = cube_vertices[3 * faces[3*i + j] + 1];
                        tri.vertices[j].z = cube_vertices[3 * faces[3*i + j] + 2];
                        //std::cout << "[" << tri.vertices[j].x << " " << tri.vertices[j].y << " " << tri.vertices[j].z << "]";
                     }
                     //std::cout << std::endl;
                     trianglesOut.push_back(tri);
                  }
               }
            }
            else
            {
               //std::cout << "not a voxel\n";

            }
         }
      }

      boost::shared_ptr<IntersectorType> mBoxCollider;
   };

   class DT_VOXEL_EXPORT VoxelGeometry : public dtPhysics::Geometry
   {
   public:

      template<typename GridTypePtr>
      static dtCore::RefPtr<VoxelGeometry> CreateVoxelGeometry(const dtCore::Transform& worldxform, float mass, GridTypePtr grid)
      {
         typedef typename GridTypePtr::element_type GridType;

         openvdb::CoordBBox bbox;
         grid->tree().getIndexRange(bbox);
         openvdb::Vec3d start = grid->indexToWorld(bbox.getStart());
         openvdb::Vec3d end = grid->indexToWorld(bbox.getEnd());
         palBoundingBox palBB;
         palBB.min.Set(Float(start.x()), Float(start.y()), Float(start.z()));
         palBB.max.Set(Float(end.x()), Float(end.y()), Float(end.z()));
         ColliderCallback<GridType>* cc = new ColliderCallback<GridType>(palBB, grid);
         return CreateVoxelGeometryWithCallback(worldxform, mass, cc);
      }
      static dtCore::RefPtr<VoxelGeometry> CreateVoxelGeometryWithCallback(const dtCore::Transform& worldxform, float mass, palCustomGeometryCallback* callBack);

   protected:
      VoxelGeometry();
      virtual ~VoxelGeometry();
   };

   typedef dtCore::RefPtr<VoxelGeometry> VoxelGeometryPtr;
} /* namespace dtVoxel */

#endif /* DTVOXEL_VOXELGEOMETRY_H_ */
