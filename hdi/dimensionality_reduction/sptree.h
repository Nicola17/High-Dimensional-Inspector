/*
 *
 * Copyright (c) 2014, Laurens van der Maaten (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY LAURENS VAN DER MAATEN ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL LAURENS VAN DER MAATEN BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLA PEZZOTTI ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLA PEZZOTTI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#ifndef SPTREE_H
#define SPTREE_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

namespace hdi{
  namespace dr{

    //! Sparse Partitioning Tree used for the Barnes Hut approximation
    /*!
      Sparse Partitioning Tree used for the Barnes Hut approximation.
      The original version was implemented by Laurens van der Maaten,
      \author Laurens van der Maaten
      \author Nicola Pezzotti
    */
    template <typename scalar_type>
    class SPTree{
    public:

      typedef double hp_scalar_type;

    private:
      class Cell {
        unsigned int _emb_dimension;
        hp_scalar_type* corner;
        hp_scalar_type* width;

      public:
        Cell(unsigned int inp__emb_dimension);
        Cell(unsigned int inp__emb_dimension, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
        ~Cell();

        hp_scalar_type getCorner(unsigned int d);
        hp_scalar_type getWidth(unsigned int d);
        void setCorner(unsigned int d, hp_scalar_type val);
        void setWidth(unsigned int d, hp_scalar_type val);
        bool containsPoint(scalar_type point[]);
      };

      // Fixed constants
      static const unsigned int QT_NODE_CAPACITY = 1;

      // A buffer we use when doing force computations
      //hp_scalar_type* buff;

      // Properties of this node in the tree
      SPTree* parent;
      unsigned int _emb_dimension;
      bool is_leaf;
      unsigned int size;
      unsigned int cum_size;

      // Axis-aligned bounding box stored as a center with half-_emb_dimensions to represent the boundaries of this quad tree
      Cell* boundary;

      // Indices in this space-partitioning tree node, corresponding center-of-mass, and list of all children
      scalar_type* _emb_positions;
      hp_scalar_type* _center_of_mass;
      unsigned int index[QT_NODE_CAPACITY];

      // Children
      SPTree** children;
      unsigned int no_children;

    public:
      SPTree(unsigned int D, scalar_type* inp_data, unsigned int N);
    private:
      SPTree(unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
      SPTree(unsigned int D, scalar_type* inp_data, unsigned int N, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
      SPTree(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, unsigned int N, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
      SPTree(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
    public:
      ~SPTree();
      void setData(scalar_type* inp_data);
      SPTree* getParent();
      void construct(Cell boundary);
      bool insert(unsigned int new_index);
      void subdivide();
      bool isCorrect();
      void rebuildTree();
      void getAllIndices(unsigned int* indices);
      unsigned int getDepth();
      void computeNonEdgeForcesOMP(unsigned int point_index, hp_scalar_type theta, hp_scalar_type neg_f[], hp_scalar_type& sum_Q)const;
      void computeNonEdgeForces(unsigned int point_index, hp_scalar_type theta, hp_scalar_type neg_f[], hp_scalar_type* sum_Q)const;
      void computeEdgeForces(unsigned int* row_P, unsigned int* col_P, hp_scalar_type* val_P, hp_scalar_type sum_P, int N, hp_scalar_type* pos_f)const;

      template <typename sparse_scalar_matrix>
      void computeEdgeForces(const sparse_scalar_matrix& matrix, hp_scalar_type multiplier, hp_scalar_type* pos_f)const;

      void print();

    private:
      void init(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width);
      void fill(unsigned int N);
      unsigned int getAllIndices(unsigned int* indices, unsigned int loc);
      bool isChild(unsigned int test_index, unsigned int start, unsigned int end);
    };


/////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    template <typename sparse_scalar_matrix>
    void SPTree<scalar_type>::computeEdgeForces(const sparse_scalar_matrix& sparse_matrix, hp_scalar_type multiplier, hp_scalar_type* pos_f)const{
      const int n = sparse_matrix.size();

      // Loop over all edges in the graph
#ifdef __APPLE__
      //std::cout << "GCD dispatch, sptree 180.\n";
      dispatch_apply(n, dispatch_get_global_queue(0, 0), ^(size_t j) {
#else
#pragma omp parallel for
      for(int j = 0; j < n; ++j) {
#endif //__APPLE__
        std::vector<hp_scalar_type> buff(_emb_dimension,0);
        unsigned int ind1, ind2;
        hp_scalar_type q_ij_1;
        ind1 = j * _emb_dimension;
        for(auto elem: sparse_matrix[j]) {
          // Compute pairwise distance and Q-value
          q_ij_1 = 1.0;
          ind2 = elem.first * _emb_dimension;
          for(unsigned int d = 0; d < _emb_dimension; d++)
            buff[d] = _emb_positions[ind1 + d] - _emb_positions[ind2 + d]; //buff contains (yi-yj) per each _emb_dimension
          for(unsigned int d = 0; d < _emb_dimension; d++)
            q_ij_1 += buff[d] * buff[d];

          hp_scalar_type p_ij = elem.second;
          hp_scalar_type res = hp_scalar_type(p_ij) * multiplier / q_ij_1 / n;

          // Sum positive force
          for(unsigned int d = 0; d < _emb_dimension; d++)
            pos_f[ind1 + d] += res * buff[d] * multiplier; //(p_ij*q_j*mult) * (yi-yj)
        }
      }
#ifdef __APPLE__
      );
#endif
    }

  }
}
#endif
