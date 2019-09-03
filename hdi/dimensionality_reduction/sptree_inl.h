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

#ifndef SPTREE_INL
#define SPTREE_INL

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include "sptree.h"
#include <math.h>
#include <algorithm>

namespace hdi{
  namespace dr{

    //! Constructs cell
    template <typename scalar_type>
    SPTree<scalar_type>::Cell::Cell(unsigned int inp__emb_dimension) {
      _emb_dimension = inp__emb_dimension;
      corner = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
      width  = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
    }

    template <typename scalar_type>
    SPTree<scalar_type>::Cell::Cell(unsigned int inp__emb_dimension, hp_scalar_type* inp_corner, hp_scalar_type* inp_width) {
      _emb_dimension = inp__emb_dimension;
      corner = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
      width  = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
      for(int d = 0; d < _emb_dimension; d++) setCorner(d, inp_corner[d]);
      for(int d = 0; d < _emb_dimension; d++) setWidth( d,  inp_width[d]);
    }

    //! Destructs cell
    template <typename scalar_type>
    SPTree<scalar_type>::Cell::~Cell() {
      free(corner);
      free(width);
    }

    template <typename scalar_type>
    typename SPTree<scalar_type>::hp_scalar_type SPTree<scalar_type>::Cell::getCorner(unsigned int d) {
      return corner[d];
    }

    template <typename scalar_type>
    typename SPTree<scalar_type>::hp_scalar_type SPTree<scalar_type>::Cell::getWidth(unsigned int d) {
      return width[d];
    }

    template <typename scalar_type>
    void SPTree<scalar_type>::Cell::setCorner(unsigned int d, hp_scalar_type val) {
      corner[d] = val;
    }

    template <typename scalar_type>
    void SPTree<scalar_type>::Cell::setWidth(unsigned int d, hp_scalar_type val) {
      width[d] = val;
    }

    // Checks whether a point lies in a cell
    template <typename scalar_type>
    bool SPTree<scalar_type>::Cell::containsPoint(scalar_type point[])
    {
      for(int d = 0; d < _emb_dimension; d++) {
        if(corner[d] - width[d] > point[d]) return false;
        if(corner[d] + width[d] < point[d]) return false;
      }
      return true;
    }

/////////////////////////////////////////////////////////////////////////////////////////////

    //! Default constructor for SPTree -- build tree, too!
    template <typename scalar_type>
    SPTree<scalar_type>::SPTree(unsigned int D, scalar_type* inp_data, unsigned int N){
      // Compute mean, width, and height of current map (boundaries of SPTree)
      hp_scalar_type* mean_Y = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type)); for(unsigned int d = 0; d < D; d++) mean_Y[d] = .0;
      hp_scalar_type*  min_Y = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type)); for(unsigned int d = 0; d < D; d++)  min_Y[d] =  DBL_MAX;
      hp_scalar_type*  max_Y = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type)); for(unsigned int d = 0; d < D; d++)  max_Y[d] = -DBL_MAX;
      for(unsigned int n = 0; n < N; n++) {
        for(unsigned int d = 0; d < D; d++) {
          mean_Y[d] += inp_data[n * D + d];
          if(inp_data[n * D + d] < min_Y[d]) min_Y[d] = inp_data[n * D + d];
          if(inp_data[n * D + d] > max_Y[d]) max_Y[d] = inp_data[n * D + d];
        }
      }
      for(int d = 0; d < D; d++) mean_Y[d] /= (hp_scalar_type) N;

      // Construct SPTree
      hp_scalar_type* width = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type));
      for(int d = 0; d < D; d++)
        //width[d] = fmax(max_Y[d] - mean_Y[d], mean_Y[d] - min_Y[d]) + 1e-5; //C++11
          width[d] = std::max(max_Y[d] - mean_Y[d], mean_Y[d] - min_Y[d]) + 1e-5;
      init(NULL, D, inp_data, mean_Y, width);
      fill(N);

      // Clean up memory
      free(mean_Y);
      free(max_Y);
      free(min_Y);
      free(width);
    }

    //! Constructor for SPTree with particular size and parent -- build the tree, too!
    template <typename scalar_type>
    SPTree<scalar_type>::SPTree(unsigned int D, scalar_type* inp_data, unsigned int N, hp_scalar_type* inp_corner, hp_scalar_type* inp_width){
      init(NULL, D, inp_data, inp_corner, inp_width);
      fill(N);
    }

    //! Constructor for SPTree with particular size (do not fill the tree)
    template <typename scalar_type>
    SPTree<scalar_type>::SPTree(unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width){
      init(NULL, D, inp_data, inp_corner, inp_width);
    }

    //! Constructor for SPTree with particular size and parent (do not fill tree)
    template <typename scalar_type>
    SPTree<scalar_type>::SPTree(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width){
      init(inp_parent, D, inp_data, inp_corner, inp_width);
    }

    //! Constructor for SPTree with particular size and parent -- build the tree, too!
    template <typename scalar_type>
    SPTree<scalar_type>::SPTree(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, unsigned int N, hp_scalar_type* inp_corner, hp_scalar_type* inp_width){
      init(inp_parent, D, inp_data, inp_corner, inp_width);
      fill(N);
    }

    //! Main initialization function
    template <typename scalar_type>
    void SPTree<scalar_type>::init(SPTree* inp_parent, unsigned int D, scalar_type* inp_data, hp_scalar_type* inp_corner, hp_scalar_type* inp_width){
      parent = inp_parent;
      _emb_dimension = D;
      no_children = 2;
      for(unsigned int d = 1; d < D; d++) no_children *= 2;
      _emb_positions = inp_data;
      is_leaf = true;
      size = 0;
      cum_size = 0;

      boundary = new Cell(_emb_dimension);
      for(unsigned int d = 0; d < D; d++) boundary->setCorner(d, inp_corner[d]);
      for(unsigned int d = 0; d < D; d++) boundary->setWidth( d, inp_width[d]);

      children = (SPTree**) malloc(no_children * sizeof(SPTree*));
      for(unsigned int i = 0; i < no_children; i++) children[i] = NULL;

      _center_of_mass = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type));
      for(unsigned int d = 0; d < D; d++) _center_of_mass[d] = .0;
      //buff = (hp_scalar_type*) malloc(D * sizeof(hp_scalar_type));
    }

    // Destructor for SPTree
    template <typename scalar_type>
    SPTree<scalar_type>::~SPTree()
    {
      for(unsigned int i = 0; i < no_children; i++) {
        if(children[i] != NULL) delete children[i];
      }
      free(children);
      free(_center_of_mass);
      //free(buff);
      delete boundary;
    }

    // Update the _emb_positions underlying this tree
    template <typename scalar_type>
    void SPTree<scalar_type>::setData(scalar_type* inp_data)
    {
      _emb_positions = inp_data;
    }

    // Get the parent of the current tree
    template <typename scalar_type>
    SPTree<scalar_type>* SPTree<scalar_type>::getParent()
    {
      return parent;
    }

    // Insert a point into the SPTree
    template <typename scalar_type>
    bool SPTree<scalar_type>::insert(unsigned int new_index)
    {
    //#pragma critical
      {
        // Ignore objects which do not belong in this quad tree
        scalar_type* point = _emb_positions + new_index * _emb_dimension;
        if(!boundary->containsPoint(point))
          return false;

        // Online update of cumulative size and center-of-mass
        cum_size++;
        hp_scalar_type mult1 = (hp_scalar_type) (cum_size - 1) / (hp_scalar_type) cum_size;
        hp_scalar_type mult2 = 1.0 / (hp_scalar_type) cum_size;
        for(unsigned int d = 0; d < _emb_dimension; d++) _center_of_mass[d] *= mult1;
        for(unsigned int d = 0; d < _emb_dimension; d++) _center_of_mass[d] += mult2 * point[d];

        // If there is space in this quad tree and it is a leaf, add the object here
        if(is_leaf && size < QT_NODE_CAPACITY) {
          index[size] = new_index;
          size++;
          return true;
        }

        // Don't add duplicates for now (this is not very nice)
        bool any_duplicate = false;
        for(unsigned int n = 0; n < size; n++) {
          bool duplicate = true;
          for(unsigned int d = 0; d < _emb_dimension; d++) {
            if(point[d] != _emb_positions[index[n] * _emb_dimension + d]) { duplicate = false; break; }
          }
          any_duplicate = any_duplicate | duplicate;
        }
        if(any_duplicate) return true;

        // Otherwise, we need to subdivide the current cell
        if(is_leaf) subdivide();

        // Find out where the point can be inserted
        for(unsigned int i = 0; i < no_children; i++) {
          if(children[i]->insert(new_index)) return true;
        }

        // Otherwise, the point cannot be inserted (this should never happen)
        return false;
      }
    }

    // Create four children which fully divide this cell into four quads of equal area
    template <typename scalar_type>
    void SPTree<scalar_type>::subdivide() {

      // Create new children
      hp_scalar_type* new_corner = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
      hp_scalar_type* new_width  = (hp_scalar_type*) malloc(_emb_dimension * sizeof(hp_scalar_type));
      for(unsigned int i = 0; i < no_children; i++) {
        unsigned int div = 1;
        for(unsigned int d = 0; d < _emb_dimension; d++) {
          new_width[d] = .5 * boundary->getWidth(d);
          if((i / div) % 2 == 1) new_corner[d] = boundary->getCorner(d) - .5 * boundary->getWidth(d);
          else           new_corner[d] = boundary->getCorner(d) + .5 * boundary->getWidth(d);
          div *= 2;
        }
        children[i] = new SPTree(this, _emb_dimension, _emb_positions, new_corner, new_width);
      }
      free(new_corner);
      free(new_width);

      // Move existing points to correct children
      for(unsigned int i = 0; i < size; i++) {
        bool success = false;
        for(unsigned int j = 0; j < no_children; j++) {
          if(!success) success = children[j]->insert(index[i]);
        }
        index[i] = -1;
      }

      // Empty parent node
      size = 0;
      is_leaf = false;
    }

    // Build SPTree on dataset
    template <typename scalar_type>
    void SPTree<scalar_type>::fill(unsigned int N)
    {
      int i = 0;
    //#pragma omp parallel for
      for(i = 0; i < N; i++)
        insert(i);
    }

    // Checks whether the specified tree is correct
    template <typename scalar_type>
    bool SPTree<scalar_type>::isCorrect()
    {
      for(unsigned int n = 0; n < size; n++) {
        scalar_type* point = _emb_positions + index[n] * _emb_dimension;
        if(!boundary->containsPoint(point)) return false;
      }
      if(!is_leaf) {
        bool correct = true;
        for(int i = 0; i < no_children; i++) correct = correct && children[i]->isCorrect();
        return correct;
      }
      else return true;
    }

    // Build a list of all indices in SPTree
    template <typename scalar_type>
    void SPTree<scalar_type>::getAllIndices(unsigned int* indices)
    {
      getAllIndices(indices, 0);
    }

    // Build a list of all indices in SPTree
    template <typename scalar_type>
    unsigned int SPTree<scalar_type>::getAllIndices(unsigned int* indices, unsigned int loc)
    {

      // Gather indices in current quadrant
      for(unsigned int i = 0; i < size; i++) indices[loc + i] = index[i];
      loc += size;

      // Gather indices in children
      if(!is_leaf) {
        for(int i = 0; i < no_children; i++) loc = children[i]->getAllIndices(indices, loc);
      }
      return loc;
    }

    template <typename scalar_type>
    unsigned int SPTree<scalar_type>::getDepth() {
      if(is_leaf) return 1;
      unsigned int depth = 0;
      for(unsigned int i = 0; i < no_children; i++)
        //depth = fmax(depth, children[i]->getDepth()); //C++11
        depth = std::max(depth, children[i]->getDepth());
      return 1 + depth;
    }

    // Compute non-edge forces using Barnes-Hut algorithm
    template <typename scalar_type>
    void SPTree<scalar_type>::computeNonEdgeForcesOMP(unsigned int point_index, hp_scalar_type theta, hp_scalar_type neg_f[], hp_scalar_type& sum_Q)const
    {
      std::vector<hp_scalar_type> buff(_emb_dimension,0);
      // Make sure that we spend no time on empty nodes or self-interactions
      if(cum_size == 0 || (is_leaf && size == 1 && index[0] == point_index)) return;

      // Compute distance between point and center-of-mass
      hp_scalar_type D = .0;
      unsigned int ind = point_index * _emb_dimension;
      for(unsigned int d = 0; d < _emb_dimension; d++) buff[d] = _emb_positions[ind + d] - _center_of_mass[d];
      for(unsigned int d = 0; d < _emb_dimension; d++) D += buff[d] * buff[d];

      // Check whether we can use this node as a "summary"
      hp_scalar_type max_width = 0.0;
      hp_scalar_type cur_width;
      for(unsigned int d = 0; d < _emb_dimension; d++) {
        cur_width = boundary->getWidth(d);
        max_width = (max_width > cur_width) ? max_width : cur_width;
      }
      if(is_leaf || max_width / sqrt(D) < theta) {

        // Compute and add t-SNE force between point and current node
        D = 1.0 / (1.0 + D);
        hp_scalar_type mult = cum_size * D;
        sum_Q += mult;

        mult *= D;
        for(unsigned int d = 0; d < _emb_dimension; d++) neg_f[d] += mult * buff[d];
      }
      else {

        // Recursively apply Barnes-Hut to children
        for(unsigned int i = 0; i < no_children; i++) children[i]->computeNonEdgeForcesOMP(point_index, theta, neg_f, sum_Q);
      }
    }

    // Compute non-edge forces using Barnes-Hut algorithm
    template <typename scalar_type>
    void SPTree<scalar_type>::computeNonEdgeForces(unsigned int point_index, hp_scalar_type theta, hp_scalar_type neg_f[], hp_scalar_type* sum_Q)const
    {

      std::vector<hp_scalar_type> buff(_emb_dimension,0);
      // Make sure that we spend no time on empty nodes or self-interactions
      if(cum_size == 0 || (is_leaf && size == 1 && index[0] == point_index)) return;

      // Compute distance between point and center-of-mass
      hp_scalar_type D = .0;
      unsigned int ind = point_index * _emb_dimension;
      for(unsigned int d = 0; d < _emb_dimension; d++) buff[d] = _emb_positions[ind + d] - _center_of_mass[d];
      for(unsigned int d = 0; d < _emb_dimension; d++) D += buff[d] * buff[d];

      // Check whether we can use this node as a "summary"
      hp_scalar_type max_width = 0.0;
      hp_scalar_type cur_width;
      for(unsigned int d = 0; d < _emb_dimension; d++) {
        cur_width = boundary->getWidth(d);
        max_width = (max_width > cur_width) ? max_width : cur_width;
      }
      if(is_leaf || max_width / sqrt(D) < theta) {

        // Compute and add t-SNE force between point and current node
        D = 1.0 / (1.0 + D);
        hp_scalar_type mult = cum_size * D;
        *sum_Q += mult;

        mult *= D;
        for(unsigned int d = 0; d < _emb_dimension; d++) neg_f[d] += mult * buff[d];
      }
      else {

        // Recursively apply Barnes-Hut to children
        for(unsigned int i = 0; i < no_children; i++) children[i]->computeNonEdgeForces(point_index, theta, neg_f, sum_Q);
      }
    }

    // Computes edge forces
    template <typename scalar_type>
    void SPTree<scalar_type>::computeEdgeForces(unsigned int* row_P, unsigned int* col_P, hp_scalar_type* val_P, hp_scalar_type multiplier, int N, hp_scalar_type* pos_f)const
    {
#ifdef __USE_GCD__
      std::cout << "GCD dispatch, sptree_inl 468.\n";
      dispatch_apply(N, dispatch_get_global_queue(0, 0), ^(size_t n) {
#else
      #pragma omp parallel for
      for(int n = 0; n < N; n++) {
#endif //__USE_GCD__
        std::vector<hp_scalar_type> buff(_emb_dimension,0);
        // Loop over all edges in the graph
        unsigned int ind1, ind2;
        hp_scalar_type D;
        ind1 = n * _emb_dimension;
        for(unsigned int i = row_P[n]; i < row_P[n + 1]; i++) {
          // Compute pairwise distance and Q-value
          D = 1.0;
          ind2 = col_P[i] * _emb_dimension;
          for(unsigned int d = 0; d < _emb_dimension; d++)
            buff[d] = _emb_positions[ind1 + d] - _emb_positions[ind2 + d];
          for(unsigned int d = 0; d < _emb_dimension; d++)
            D += buff[d] * buff[d];
          D = val_P[i] * multiplier / D;

          // Sum positive force
          for(unsigned int d = 0; d < _emb_dimension; d++)
            pos_f[ind1 + d] += D * buff[d];
        }
      }
#ifdef __USE_GCD__
      );
#endif
    }


/*
    void SPTree::computeEdgeForces(const KNNSparseMatrix<hp_scalar_type>& sparse_matrix, hp_scalar_type multiplier, hp_scalar_type* pos_f)const{
      const int N = sparse_matrix._symmetric_matrix.size();
      //std::lock_guard<std::mutex> lock(sparse_matrix._write_mutex);
      // Loop over all edges in the graph
      int n = 0;
    #pragma omp parallel for
      for(n = 0; n < N; n++) {
        std::vector<hp_scalar_type> buff(_emb_dimension,0);
        unsigned int ind1, ind2;
        hp_scalar_type q_ij_1;
        ind1 = n * _emb_dimension;
        for(auto idx: sparse_matrix._symmetric_matrix[n]) {
          // Compute pairwise distance and Q-value
          q_ij_1 = 1.0;
          ind2 = idx.first * _emb_dimension;
          for(unsigned int d = 0; d < _emb_dimension; d++)
            buff[d] = _emb_positions[ind1 + d] - _emb_positions[ind2 + d]; //buff contains (yi-yj) per each _emb_dimension
          for(unsigned int d = 0; d < _emb_dimension; d++)
            q_ij_1 += buff[d] * buff[d];

          auto a = std::get<0>(idx.second);
          auto b = std::get<1>(idx.second);
          hp_scalar_type p_ij = std::get<1>(a)/sparse_matrix._row_normalization[std::get<0>(a)] + std::get<1>(b)/sparse_matrix._row_normalization[std::get<0>(b)];
          hp_scalar_type res = p_ij * multiplier / q_ij_1 / sparse_matrix._normalization_factor;

          // Sum positive force
          for(unsigned int d = 0; d < _emb_dimension; d++)
            pos_f[ind1 + d] += res * buff[d]; //(p_ij*q_j*mult) * (yi-yj)
        }
      }
    }

    void SPTree::computeEdgeForces(const KNNSparseMatrix<hp_scalar_type>& sparse_matrix, hp_scalar_type* pos_f)const{
      const int N = sparse_matrix._symmetric_matrix.size();
      //std::lock_guard<std::mutex> lock(sparse_matrix._write_mutex);
      // Loop over all edges in the graph
      int n = 0;
    #pragma omp parallel for
      for(n = 0; n < N; n++) {
        std::vector<hp_scalar_type> buff(_emb_dimension,0);
        unsigned int ind1, ind2;
        hp_scalar_type q_ij_1;
        ind1 = n * _emb_dimension;
        for(auto idx: sparse_matrix._symmetric_matrix[n]) {
          // Compute pairwise distance and Q-value
          q_ij_1 = 1.0;
          ind2 = idx.first * _emb_dimension;
          for(unsigned int d = 0; d < _emb_dimension; d++)
            buff[d] = _emb_positions[ind1 + d] - _emb_positions[ind2 + d]; //buff contains (yi-yj) per each _emb_dimension
          for(unsigned int d = 0; d < _emb_dimension; d++)
            q_ij_1 += buff[d] * buff[d];

          auto a = std::get<0>(idx.second);
          auto b = std::get<1>(idx.second);
          hp_scalar_type p_ij = std::get<1>(a)/sparse_matrix._row_normalization[std::get<0>(a)] + std::get<1>(b)/sparse_matrix._row_normalization[std::get<0>(b)];
          hp_scalar_type res = p_ij * sparse_matrix._exaggeration_factors[n] / q_ij_1 / sparse_matrix._normalization_factor;

          // Sum positive force
          for(unsigned int d = 0; d < _emb_dimension; d++)
            pos_f[ind1 + d] += res * buff[d]; //(p_ij*q_j*mult) * (yi-yj)
        }
      }
    }
    */

    //! Print out tree
    template <typename scalar_type>
    void SPTree<scalar_type>::print()
    {
      if(cum_size == 0) {
        printf("Empty node\n");
        return;
      }

      if(is_leaf) {
        printf("Leaf node; _emb_positions = [");
        for(int i = 0; i < size; i++) {
          scalar_type* point = _emb_positions + index[i] * _emb_dimension;
          for(int d = 0; d < _emb_dimension; d++) printf("%f, ", point[d]);
          printf(" (index = %d)", index[i]);
          if(i < size - 1) printf("\n");
          else printf("]\n");
        }
      }
      else {
        printf("Intersection node with center-of-mass = [");
        for(int d = 0; d < _emb_dimension; d++) printf("%f, ", _center_of_mass[d]);
        printf("]; children are:\n");
        for(int i = 0; i < no_children; i++) children[i]->print();
      }
    }
  }
}

#endif
