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

#include <dtVoxel/voxelactor.h>
#include <dtVoxel/voxelgriddebugdrawable.h>
#include <dtVoxel/aabbintersector.h>
#include <dtVoxel/voxelgeometry.h>
#include <dtVoxel/voxelmessagetype.h>
#include <dtVoxel/volumeupdatemessage.h>

#include <dtABC/application.h>

#include <dtCore/transformable.h>
#include <dtCore/propertymacros.h>
#include <dtCore/camera.h>
#include <dtCore/project.h>
#include <dtCore/transform.h>

#include <dtGame/basemessages.h>
#include <dtGame/gamemanager.h>
#include <dtGame/shaderactorcomponent.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>

#include <dtPhysics/physicsactcomp.h>

#include <dtUtil/functor.h>

#include <osg/Texture2D>
#include <osg/Image>
#include <osgDB/ReadFile>

namespace dtVoxel
{

   VoxelActor::VoxelActor()
   : mViewDistance(1000.0f)
   , mCreateRemotePhysics(false)
   {
   }

   VoxelActor::~VoxelActor()
   {
   }

   /////////////////////////////////////////////////////
   DT_IMPLEMENT_ACCESSOR_WITH_STATEMENT(VoxelActor, dtCore::ResourceDescriptor, Database, LoadGrid(value););

   /////////////////////////////////////////////////////
   void VoxelActor::LoadGrid(const dtCore::ResourceDescriptor& rd)
   {
      std::cout << "Loading Grid" << std::endl;

      if (rd != GetDatabase() && !rd.IsEmpty())
      {
         try
         {
            openvdb::io::File file(dtCore::Project::GetInstance().GetResourcePath(rd));
            file.open();
            mGrids = file.getGrids();
            file.close();

            std::cout << "Done Loading Grid" << std::endl;

         }
         catch (const openvdb::IoError& ioe)
         {
            std::cout << "Error Loading Grid" << std::endl;

            throw dtUtil::FileUtilIOException(ioe.what(), __FILE__, __LINE__);
         }
      }
      else if (rd.IsEmpty())
      {
         mGrids = NULL;
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::BuildPropertyMap()
   {
      BaseClass::BuildPropertyMap();

      typedef dtCore::PropertyRegHelper<VoxelActor> RegHelper;
      static dtUtil::RefString GROUP("VoxelActor");
      RegHelper regHelper(*this, this, GROUP);
      
      DT_REGISTER_PROPERTY_WITH_LABEL(ViewDistance, "View Distance", "The distance to at which voxels will be generated into groups of volumes and rendered.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(GridDimensions, "Grid Dimensions", "The size of the grid to allocate into blocks.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(BlockDimensions,"Block Dimensions", "The size of the blocks within the grid.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(CellDimensions, "Cell Dimensions", "The size of the cells within the blocks", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(TextureResolution, "Texture Resolution", "The dimensions of the 3d texture which holds individual voxels within a single cell.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(Offset, "Offset", "The offset of the database in world space.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(CreateRemotePhysics, "Create Remote Physics", "Create the voxel geometry for the physics if this actor is remote.", RegHelper, regHelper);

      DT_REGISTER_RESOURCE_PROPERTY(dtCore::DataType::VOLUME, Database, "Database", "Voxel database file", RegHelper, regHelper);
   }

   /////////////////////////////////////////////////////
   openvdb::GridPtrVecPtr VoxelActor::GetGrids()
   {
      return mGrids;
   }

   /////////////////////////////////////////////////////
   openvdb::GridBase::Ptr VoxelActor::GetGrid(int i)
   {
      if (mGrids) return (*mGrids)[i];
      return NULL;
   }

   /////////////////////////////////////////////////////
   size_t VoxelActor::GetNumGrids() const
   {
      size_t result = 0;
      if (mGrids) result = mGrids->size();
      return result;
   }

   /////////////////////////////////////////////////////
   openvdb::GridBase::Ptr VoxelActor::CollideWithAABB(const osg::BoundingBox& bb, int gridIdx)
   {
      openvdb::GridBase::Ptr result(NULL);
      openvdb::BBoxd bbox(openvdb::Vec3d(bb.xMin(), bb.yMin(), bb.zMin()), openvdb::Vec3d(bb.xMax(), bb.yMax(), bb.zMax()));
      if (openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::BoolGrid> aabb(gridB);
         aabb.SetWorldBB(bbox);
         aabb.Intersect();
         result = aabb.GetHits();
       }
      else if (openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::FloatGrid> aabb(gridF);
         aabb.SetWorldBB(bbox);
         aabb.Intersect();
         result = aabb.GetHits();
      }
      return result;
   }

   /////////////////////////////////////////////////////
   void VoxelActor::MarkVisualDirty(const osg::BoundingBox& bb, int gridIdx)
   {
      mGrid->MarkDirtyAABB(bb);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnTickLocal(const dtGame::TickMessage& tickMessage)
   {
      if (mGrid.valid())
      {
         dtGame::GameManager* gm = GetGameManager();

         if (gm != nullptr)
         {
            dtCore::Camera* cam = gm->GetApplication().GetCamera();

            osg::Vec3 pos;
            dtCore::Transform xform;
            cam->GetTransform(xform);
            xform.GetTranslation(pos);

            mGrid->UpdateGrid(pos);

            //std::cout << "Updating voxel grid with position (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
         }
      }
   }

   dtCore::RefPtr<osg::Texture2D> VoxelActor::LoadTexture(std::string texFile)
   {
      osg::Image* newImage = osgDB::readImageFile(texFile);
      if (newImage == nullptr)
      {
         LOG_ERROR("Failed to load texture from file [" + texFile + "].");
         return nullptr;
      }


      dtCore::RefPtr<osg::Texture2D> tex = new osg::Texture2D();
      tex->setImage(newImage);
      tex->dirtyTextureObject();
      tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
      tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
      tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
      tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
      tex->setUnRefImageDataAfterApply(true);

      return tex;
   }

   /////////////////////////////////////////////////////
   void VoxelActor::CreateDrawable()
   {
      //commented out, this is a simple debug rendering
      //dtCore::RefPtr<VoxelGridDebugDrawable> dd = new VoxelGridDebugDrawable();
      //dd->CreateDebugDrawable(*mGrid);
      //SetDrawable(*dd);

      mGrid = new VoxelGrid();
      SetDrawable(*mGrid);

      //bind multimap texture
      osg::Node* n = mGrid->GetOSGNode();

      osg::StateSet* ss = n->getOrCreateStateSet();

      const std::string filePath = dtCore::Project::GetInstance().GetContext() + std::string("/Textures/ShadersBase/");
           
      dtCore::RefPtr<osg::Texture2D> diffuse = LoadTexture(filePath + std::string("Snow_DIFF.png"));
      
      if (diffuse != nullptr)
      {
         osg::Uniform* diffuseTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "diffuseTexture");
         diffuseTexture->set(0);
         ss->addUniform(diffuseTexture);
         ss->setTextureAttributeAndModes(0, diffuse, osg::StateAttribute::ON);
      }

      dtCore::RefPtr<osg::Texture2D> normal = LoadTexture(filePath + std::string("Snow_NORM.png"));

      if (normal != nullptr)
      {
         osg::Uniform* normalTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "normalTexture");
         normalTexture->set(1);
         ss->addUniform(normalTexture);
         ss->setTextureAttributeAndModes(1, normal, osg::StateAttribute::ON);
      }


      dtCore::RefPtr<osg::Texture2D> spec = LoadTexture(filePath + std::string("Snow_SPEC.png"));

      if (spec != nullptr)
      {
         osg::Uniform* specularTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "specularTexture");
         specularTexture->set(2);
         ss->addUniform(specularTexture);
         ss->setTextureAttributeAndModes(2, spec, osg::StateAttribute::ON);
      }

      /*dtCore::RefPtr<osg::Texture2D> alph = LoadTexture(filePath + std::string("Snow_SPEC.png"));

      if (spec != nullptr)
      {
         osg::Uniform* alphaTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "alphaTexture");
         alphaTexture->set(3);
         ss->addUniform(alphaTexture);
         //ss->setTextureAttributeAndModes(3, alphaTexture, osg::StateAttribute::ON);
      }

      osg::Uniform* illumTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "illumTexture");
      illumTexture->set(4);
      ss->addUniform(illumTexture);
      //ss->setTextureAttributeAndModes(4, alphaTexture, osg::StateAttribute::ON);

      osg::Uniform* shadowTexture = new osg::Uniform(osg::Uniform::SAMPLER_2D, "shadowTexture");
      shadowTexture->set(5);
      ss->addUniform(shadowTexture);
      //ss->setTextureAttributeAndModes(5, shadowTexture, osg::StateAttribute::ON);

      osg::Uniform* d3d_ReflectionCubeMap = new osg::Uniform(osg::Uniform::SAMPLER_2D, "d3d_ReflectionCubeMap");
      d3d_ReflectionCubeMap->set(10);
      ss->addUniform(d3d_ReflectionCubeMap);
      //ss->setTextureAttributeAndModes(5, shadowTexture, osg::StateAttribute::ON);*/
   }

   /////////////////////////////////////////////////////
   template<typename GridTypePtr>
   void VoxelActor::UpdateGrid(GridTypePtr grid, const dtCore::NamedArrayParameter* indices, const dtCore::NamedArrayParameter* values,
         const dtCore::NamedArrayParameter* indicesDeactivated)
   {
      typedef typename GridTypePtr::element_type GridType;
      typedef typename GridType::ValueType ValueType;
      typedef typename dtCore::TypeToActorProperty<ValueType>::named_parameter_type ParameterType;
      typedef typename GridType::Accessor AccessorType;

      AccessorType accessor = grid->getAccessor();

      osg::BoundingBox bb;
      for (unsigned i = 0; i < indices->GetSize(); ++i)
      {
         const dtCore::NamedParameter* indexP = indices->GetParameter(i);
         if (indexP != nullptr && indexP->GetDataType() == dtCore::DataType::VEC3)
         {
            auto indexVp = static_cast<const dtCore::NamedVec3Parameter*>(indexP);
            osg::Vec3 idxVec = indexVp->GetValue();
            auto valueParam = dynamic_cast<const ParameterType*>(values->GetParameter(i));
            if (valueParam != NULL)
            {
               ValueType val = valueParam->GetValue();
               openvdb::Vec3d idxOVDBVec(idxVec.x(), idxVec.y(), idxVec.z());
               openvdb::Vec3d worldVec = grid->transform().indexToWorld(idxOVDBVec);
               bb.expandBy(osg::Vec3(worldVec.x(), worldVec.y(), worldVec.z()));
               openvdb::Coord c(std::round(idxVec.x()), std::round(idxVec.y()), std::round(idxVec.z()));
               accessor.setValue(c, val);
            }
         }
         else
         {
            LOGN_ERROR("voxelactor.cpp", "Received a VolumeUpdateMessage, but the indices are not Vec3 parameters.");
         }
      }
      for (unsigned i = 0; i < indicesDeactivated->GetSize(); ++i)
      {
         const dtCore::NamedParameter* indexP = indicesDeactivated->GetParameter(i);
         if (indexP != nullptr && indexP->GetDataType() == dtCore::DataType::VEC3)
         {
            auto indexVp = static_cast<const dtCore::NamedVec3Parameter*>(indexP);
            osg::Vec3 idxVec = indexVp->GetValue();
            openvdb::Vec3d idxOVDBVec(idxVec.x(), idxVec.y(), idxVec.z());
            openvdb::Vec3d worldVec = grid->transform().indexToWorld(idxOVDBVec);
            bb.expandBy(osg::Vec3(worldVec.x(), worldVec.y(), worldVec.z()));
            openvdb::Coord c(openvdb::Coord::round(idxOVDBVec));
            accessor.setValueOff(c, grid->background());
         }
         else
         {
            LOGN_ERROR("voxelactor.cpp", "Received a VolumeUpdateMessage, but the indices deactivated are not Vec3 parameters.");
         }
      }
      //re-add when not busted
      MarkVisualDirty(bb, 0);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnVolumeUpdate(const VolumeUpdateMessage& msg)
   {
      if (GetNumGrids() > 0 && (IsRemote() || GetLocalActorUpdatePolicy() != dtGame::GameActorProxy::LocalActorUpdatePolicy::IGNORE_ALL)
            && msg.GetSource() != GetGameManager()->GetMachineInfo())
      {
         const dtCore::NamedArrayParameter* indices = msg.GetIndicesChanged();
         const dtCore::NamedArrayParameter* values = msg.GetValuesChanged();
         const dtCore::NamedArrayParameter* indicesDeactivated = msg.GetIndicesDeactivated();

         openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(0));
         if (gridF)
         {
            UpdateGrid(gridF, indices, values, indicesDeactivated);
         }
         else
         {
            openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(0));
            if (gridB)
               UpdateGrid(gridB, indices, values, indicesDeactivated);
         }

      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnEnteredWorld()
   {
      RegisterForMessages(dtGame::MessageType::TICK_LOCAL, dtGame::GameActorProxy::TICK_LOCAL_INVOKABLE);


      osg::Vec3i res(int(mTextureResolution.x()), int(mTextureResolution.y()), int(mTextureResolution.z()));


      mGrid->Init(mOffset, mGridDimensions, mBlockDimensions, mCellDimensions, res);
      
      dtGame::GameManager* gm = GetGameManager();

      if (gm != nullptr)
      {
         dtCore::Camera* cam = gm->GetApplication().GetCamera();

         osg::Vec3 pos;
         dtCore::Transform xform;
         cam->GetTransform(xform);
         xform.GetTranslation(pos);

         mGrid->SetViewDistance(mViewDistance);
         mGrid->CreatePagedLODGrid(pos, *this);
      }
      else
      {
         LOG_ERROR("NULL GameManager!");
      }


      if (!IsRemote() || GetCreateRemotePhysics())
      {
         dtPhysics::PhysicsActCompPtr pac = GetComponent<dtPhysics::PhysicsActComp>();
         if (mGrids && pac.valid())
         {
            dtPhysics::PhysicsObject* po = pac->GetMainPhysicsObject();
            if (po != nullptr && po->GetPrimitiveType() == dtPhysics::PrimitiveType::CUSTOM_CONCAVE_MESH)
            {
               dtPhysics::TransformType xform;
               VoxelGeometryPtr geometry;
               openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(0));
               if (gridF)
               {
                  geometry = VoxelGeometry::CreateVoxelGeometry(xform, po->GetMass(), gridF);
               }
               else
               {
                  openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(0));
                  if (gridB)
                     geometry = VoxelGeometry::CreateVoxelGeometry(xform, po->GetMass(), gridB);
               }
               if (geometry.valid())
                  po->CreateFromGeometry(*geometry);
            }
         }
      }

      RegisterForMessagesAboutSelf(VoxelMessageType::INFO_VOLUME_CHANGED, dtUtil::MakeFunctor(&VoxelActor::OnVolumeUpdate, this));

      dtGame::ShaderActorComponent* shaderComp = nullptr;
      GetComponent(shaderComp);
      if (shaderComp != nullptr)
      {
         mGrid->SetShaderGroup(shaderComp->GetCurrentShader().GetResourceIdentifier());
      }
   }


} /* namespace dtVoxel */
