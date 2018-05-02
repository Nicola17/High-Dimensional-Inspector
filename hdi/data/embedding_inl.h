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

#ifndef EMBEDDING_INL
#define EMBEDDING_INL

#include "hdi/data/embedding.h"
#include <algorithm>
#include <limits>
#include <algorithm>
#include "hdi/utils/assert_by_exception.h"

namespace hdi{
  namespace data{

    template <typename scalar_type>
    Embedding<scalar_type>::Embedding():
      _num_dimensions(0),
      _num_data_points(0)
    {}

    template <typename scalar_type>
    Embedding<scalar_type>::Embedding(unsigned int num_dimensions, unsigned int num_data_points, scalar_type v){
      resize(num_dimensions,num_data_points, v);
    }

    template <typename scalar_type>
    void Embedding<scalar_type>::resize(unsigned int num_dimensions, unsigned int num_data_points, scalar_type v){
      _num_data_points = num_data_points;
      _num_dimensions = num_dimensions;
      _embedding.resize(_num_data_points*_num_dimensions,v);
    }
    template <typename scalar_type>
    void Embedding<scalar_type>::clear(){
      _num_data_points = 0;
      _num_dimensions = 0;
      _embedding.clear();
    }
    template <typename scalar_type>
    void Embedding<scalar_type>::computeEmbeddingBBox(scalar_vector_type& limits, scalar_type offset, bool squared_limits){
      limits.resize(_num_dimensions*2);
      for(int d = 0; d < _num_dimensions; ++d){
        limits[d*2] = std::numeric_limits<scalar_type>::max();
        limits[d*2+1] = -std::numeric_limits<scalar_type>::max();
      }

      for(int i = 0; i < _num_data_points; ++i){
        for(int d = 0; d < _num_dimensions; ++d){
          int idx = i*_num_dimensions+d;
          auto v = _embedding[idx];
          if(v < limits[d*2]){
            limits[d*2] = v;
          }
          if(v > limits[d*2+1]){
            limits[d*2+1] = v;
          }
        }
      }

      if(offset <= 0){
        return;
      }

      scalar_type max_dist = 0;
      for(int d = 0; d < _num_dimensions; ++d){
        auto diff = limits[d*2+1] - limits[d*2];
        limits[d*2] -= diff*offset/2;
        limits[d*2+1] += diff*offset/2;
        max_dist = std::max(diff,max_dist);
      }

      if(squared_limits){
        for(int d = 0; d < _num_dimensions; ++d){
          auto central_pnt = (limits[d*2+1] + limits[d*2]) * 0.5;
          limits[d*2]   = central_pnt - max_dist/2/(1-offset);
          limits[d*2+1]   = central_pnt + max_dist/2/(1-offset);
        }
      }
    }

    //! Move the embedding so that is 0-centered
    template <typename scalar_type>
    void Embedding<scalar_type>::zeroCentered(){
      scalar_vector_type limits;
      computeEmbeddingBBox(limits);

      scalar_vector_type shifts(_num_dimensions,0);
      for(int d = 0; d < _num_dimensions; ++d){
        shifts[d] = -0.5*(limits[d*2+1]+limits[d*2]);
      }

      for(int i = 0; i < _num_data_points; ++i){
        for(int d = 0; d < _num_dimensions; ++d){
          int idx = i*_num_dimensions+d;
          _embedding[idx] += shifts[d];
        }
      }
    }

    template <typename scalar_type>
    void Embedding<scalar_type>::scaleIfSmallerThan(scalar_type diameter){
      scalar_vector_type limits;
      computeEmbeddingBBox(limits);

      if((limits[1]-limits[0]) >= diameter){ // beacuse the limits are squared
        return;
      }

      double scale_factor = diameter/(limits[1]-limits[0]);
      scalar_vector_type shifts(_num_dimensions,0);
      for(int d = 0; d < _num_dimensions; ++d){
        shifts[d] = -0.5*(limits[d*2+1]+limits[d*2]);
      }

      for(int i = 0; i < _num_data_points; ++i){
        for(int d = 0; d < _num_dimensions; ++d){
          int idx = i*_num_dimensions+d;
          _embedding[idx] += shifts[d];
          _embedding[idx] *= scale_factor;
        }
      }
    }


/////////////////////////////////////////////////////////////

    template <typename scalar_type, typename sparse_matrix_type>
    void interpolateEmbeddingPositions(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const sparse_matrix_type& weights){
      unsigned int num_dim = input.numDimensions();
      output.clear();
      output.resize(input.numDimensions(),weights.size(),0);

      for(int i = 0; i < output.numDataPoints(); ++i){
        double total_weight = 0;
        for(auto& w_elem: weights[i]){
          double w = w_elem.second;
          unsigned int idx = w_elem.first;
          assert(idx < input.numDataPoints());
          for(int d = 0; d < num_dim; ++d){
            output.dataAt(i,d) += input.dataAt(idx,d) * w;
          }
          total_weight += w;
        }
        for(int d = 0; d < num_dim; ++d){
          output.dataAt(i,d) = output.dataAt(i,d) / total_weight;
        }
      }
    }

/////////////////////////////////////////////////////////////


    template <typename scalar_type>
    void copyAndRemap1D2DVertical(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const std::vector<scalar_type>& limits){
      checkAndThrowLogic(input.numDimensions() == 1, "input embedding must be one-dimensional");
      checkAndThrowLogic(limits.size() == 4, "invalid limits");

      const unsigned int N = input.numDataPoints();
      output.resize(2,N);

      scalar_type min = std::numeric_limits<scalar_type>::max();
      scalar_type max = -min;

      const auto& input_container = input.getContainer();
      auto& output_container = output.getContainer();

      for(int i = 0; i < N; ++i){
        auto v = input_container[i];
        if(v > max){
          max = v;
        }
        if(v < min){
          min = v;
        }
      }

      const scalar_type vertical_position = (limits[0]+limits[1])*0.5;
      for(int i = 0; i < N; ++i){
        output_container[i*2]   = vertical_position;
        output_container[i*2+1] = (input_container[i]-min)/(max-min) * (limits[3]-limits[2]) + limits[2];
      }

    }


    template <typename scalar_type>
    void copyAndRemap2D2D(const Embedding<scalar_type>& input, Embedding<scalar_type>& output, const std::vector<scalar_type>& limits, bool fix_aspect_ratio){
      checkAndThrowLogic(input.numDimensions() == 2, "input embedding must be two-dimensional");
      output.resize(2,input.numDataPoints());

      const unsigned int N = input.numDataPoints();
      output.resize(2,N);

      scalar_type min_x = std::numeric_limits<scalar_type>::max();
      scalar_type max_x = -min_x;
      scalar_type min_y = std::numeric_limits<scalar_type>::max();
      scalar_type max_y = -min_y;

      const auto& input_container = input.getContainer();
      auto& output_container = output.getContainer();

      for(int i = 0; i < N; ++i){
        auto v_x = input_container[i*2];
        auto v_y = input_container[i*2+1];
        if(v_x > max_x){
          max_x = v_x;
        }
        if(v_x < min_x){
          min_x = v_x;
        }
        if(v_y > max_y){
          max_y = v_y;
        }
        if(v_y < min_y){
          min_y = v_y;
        }
      }

      if(!fix_aspect_ratio){

        for(int i = 0; i < N; ++i){
          output_container[i*2]   = (input_container[2*i  ]-min_x)/(max_x-min_x) * (limits[1]-limits[0]) + limits[0];
          output_container[i*2+1] = (input_container[2*i+1]-min_y)/(max_y-min_y) * (limits[3]-limits[2]) + limits[2];
        }
      }else{ //fix_aspect_ratio
        if(max_x-min_x > max_y-min_y){ //x is bigger then y
          double ratio = (max_y-min_y)/(max_x-min_x);
          for(int i = 0; i < N; ++i){
            output_container[i*2]   = (input_container[2*i  ]-min_x)/(max_x-min_x) * (limits[1]-limits[0]) + limits[0];// it is spanning on all x
            output_container[i*2+1] = ((input_container[2*i+1]-min_y)/(max_x-min_x) + (1.-ratio)/2)* (limits[3]-limits[2]) + limits[2];
          }
        }else{//y is bigger then x
          double ratio = (max_x-min_x)/(max_y-min_y);
          for(int i = 0; i < N; ++i){
            output_container[i*2]   = ((input_container[2*i  ]-min_x)/(max_y-min_y) + (1.-ratio)/2) * (limits[1]-limits[0]) + limits[0];
            output_container[i*2+1] = (input_container[2*i+1]-min_y)/(max_y-min_y) * (limits[3]-limits[2]) + limits[2];// it is spanning on all y
          }

        }
      }
    }

  }
}

#endif
