#ifndef stk_percept_BuildBoundingBoxes_hpp
#define stk_percept_BuildBoundingBoxes_hpp

#include <stk_search/CoarseSearch.hpp>
#include <stk_search/BoundingBox.hpp>
#include <stk_search/diag/BoundingBox.hpp>
#include <stk_search/IdentProc.hpp>
#include <stk_search/diag/IdentProc.hpp>

#include <Shards_CellTopology.hpp>
#include <stk_util/parallel/ParallelReduce.hpp>

#include <stk_percept/function/ElementOp.hpp>

#define EXTRA_PRINT 0

namespace stk
{
  namespace percept
  {

    typedef mesh::Field<double>                     ScalarFieldType ;
    typedef mesh::Field<double, mesh::Cartesian>    VectorFieldType ;

    template<unsigned SpatialDim>
    class BuildBoundingBoxes : public ElementOp
    {

    public:
      typedef stk::search::ident::IdentProc<uint64_t,unsigned> IdentProc;
      typedef stk::search::box::PointBoundingBox<IdentProc,double,SpatialDim> BoundingPoint;
      typedef stk::search::box::AxisAlignedBoundingBox<IdentProc,double,SpatialDim> AABoundingBox;

      std::vector<AABoundingBox>& m_boxes;
      VectorFieldType *m_coords_field;
      bool m_notInitialized;
    public:
      BuildBoundingBoxes(std::vector<AABoundingBox>& boxes, VectorFieldType *coords_field) :  m_boxes(boxes), m_coords_field(coords_field),
                                                                                              m_notInitialized(false)
      {
      }

      virtual ~BuildBoundingBoxes() {
    	  m_coords_field = NULL;
    	  m_notInitialized = true;
      }

      void init(std::vector<AABoundingBox>& boxes, VectorFieldType *coords_field)
      {
        m_boxes(boxes);
        //m_boxes = boxes;
        m_coords_field(coords_field);
      }

      void init_elementOp()
      {
      }
      void fini_elementOp()
      {
        m_notInitialized=true;  // force this object to be used only once
      }
      bool operator()(const stk::mesh::Entity element, stk::mesh::FieldBase *field,  const mesh::BulkData& bulkData)
      {
        if (m_notInitialized)
          throw std::runtime_error("BuildBoundingBoxes::operator(): you must re-construct this object before reusing it");

        AABoundingBox bb = getBoundingBox(element, bulkData);
        if (0 || EXTRA_PRINT) std::cout << "bb = " << bb << std::endl;
        m_boxes.push_back(bb);
        return false;  // never break out of the enclosing loop
      }

      AABoundingBox getBoundingBox(const stk::mesh::Entity element, const mesh::BulkData& bulkData)
      {
        double bbox[2*SpatialDim];
        const MyPairIterRelation elem_nodes(bulkData, element, stk::mesh::MetaData::NODE_RANK );
        unsigned numNodes = elem_nodes.size();
        for (unsigned iNode = 0; iNode < numNodes; iNode++)
          {
            mesh::Entity node = elem_nodes[iNode].entity();
            double * coord_data = bulkData.field_data( *m_coords_field, node);
            if (iNode == 0)
              {
                for (unsigned iDim = 0; iDim < SpatialDim; iDim++)
                  {
                    bbox[iDim]              = coord_data[iDim];
                    bbox[iDim + SpatialDim] = coord_data[iDim];
                  }
              }
            else
              {
                for (unsigned iDim = 0; iDim < SpatialDim; iDim++)
                  {
                    bbox[iDim]              = std::min(bbox[iDim],              coord_data[iDim]);
                    bbox[iDim + SpatialDim] = std::max(bbox[iDim + SpatialDim], coord_data[iDim]);
                  }
              }
          }

        AABoundingBox bb;
        bb.key.ident = bulkData.identifier(element);
        bb.set_box(bbox);

        return bb;
      }
    };

  } //namespace percept
} //namespace stk

#endif
