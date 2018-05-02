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

#ifndef EMBEDDING_H
#define EMBEDDING_H

#include <vector>
#include <assert.h>

namespace hdi{
  namespace data{

    //! Container for a n-dimensional embedding
    /*!
      Container for a n-dimensional embedding
      \author Nicola Pezzotti
    */
    template <typename scalar_type>
    class Embedding{
    public:
      typedef std::vector<scalar_type> scalar_vector_type;

    public:
      Embedding();
      Embedding(unsigned int num_dimensions, unsigned int num_data_points, scalar_type v = 0);

      //! Clear the container
      void clear();
      //! Resize the container
      void resize(unsigned int num_dimensions, unsigned int num_data_points, scalar_type v = 0);
      //! Return the dimensionality of the embedding
      unsigned int numDimensions()const{return _num_dimensions;}
      //! Return the number of data points in the container
      unsigned int numDataPoints()const{return _num_data_points;}
      //! Compute a boundinb box that contains the embedding. An offset can be provided (percentage)
      void computeEmbeddingBBox(scalar_vector_type& limits, scalar_type offset = 0, bool squared_limits = true);

      //! Move the embedding so that is 0-centered
      void zeroCentered();
      //! If the embedding is contained in a squared region smaller than diameter, rescale it so that it will be contained in squared region of size diameter centered in zero
      void scaleIfSmallerThan(scalar_type diameter);

      scalar_vector_type& getContainer(){return _embedding;}
      const scalar_vector_type& getContainer()const{return _embedding;}

      inline scalar_type& dataAt(unsigned int data_point, unsigned int dimension){
        assert(data_point < _num_data_points);
        assert(dimension < _num_dimensions);
        return _embedding[data_point*_num_dimensions+dimension];
      }
      inline const scalar_type& dataAt(unsigned int data_point, unsigned int dimension)const{
        assert(data_point < _num_data_points);
        assert(dimension < _num_dimensions);
        return _embedding[data_point*_num_dimensions+dimension];
      }

    private:
      unsigned int _num_dimensions;
      unsigned int _num_data_points;
      scalar_vector_type _embedding;
    };

    //!
    //! \brief Weighted average of the embedding position
    //! \author Nicola Pezzotti
    //!
    template <typename scalar_type, typename sparse_matrix_type>
    void interpolateEmbeddingPositions(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const sparse_matrix_type& weights);

    //!
    //! \brief Copies the 1D embedding in a 2D vertical embedding at the given coordinates
    //! \author Nicola Pezzotti
    //!
    template <typename scalar_type>
    void copyAndRemap1D2DVertical(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const std::vector<scalar_type>& limits);

    //!
    //! \brief Copies the 2D embedding in a 2D embedding at the given coordinates
    //! \author Nicola Pezzotti
    //!
    template <typename scalar_type>
    void copyAndRemap2D2D(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const std::vector<scalar_type>& limits, bool fix_aspect_ratio = true);


  }
}

#endif
