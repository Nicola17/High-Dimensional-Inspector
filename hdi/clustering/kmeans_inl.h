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

#ifndef KMEANS_INL
#define KMEANS_INL

#include "kmeans.h"
#include "hdi/utils/math_utils.h"
#include <limits>
#include <random>

namespace hdi{
  namespace clustering{

    template <typename scalar_type>
    void KMeans<scalar_type>::initialize(){
      std::default_random_engine generator;
      std::uniform_int_distribution<> distribution_int(0, _num_points-1);
      for(unsigned int c = 0; c < _num_clusters; ++c){
        const unsigned int i = distribution_int(generator);
        for(unsigned int d = 0; d < _dimensionality; ++d){
          _centroids[c*_dimensionality+d] += _data[i*_dimensionality+d];
        }
      }
    }

    template <typename scalar_type>
    void KMeans<scalar_type>::doAnIteration(){
      _clusters.resize(_num_points);
      _cluster_size.resize(_num_clusters);

      for(unsigned int c = 0; c < _num_clusters; ++c){
        _cluster_size[c] = 0;
      }

      //1) Assign data-points to centroids
      for(unsigned int i = 0; i < _num_points; ++i){
        double min_distance(std::numeric_limits<double>::max());
        for(unsigned int c = 0; c < _num_clusters; ++c){
          auto distance = utils::euclideanDistanceSquared(_data+i*_dimensionality,_data+(i+1)*_dimensionality,_centroids+c*_dimensionality,_centroids+(c+1)*_dimensionality);
          if(min_distance > distance){
            min_distance = distance;
            _clusters[i] = c;
          }
        }
        ++_cluster_size[_clusters[i]];
      }

      //2) Update centroid position
      for(unsigned int c = 0; c < _num_clusters; ++c){
        if(_cluster_size[c] == 0){
          continue;
        }
        for(unsigned int d = 0; d < _dimensionality; ++d){
          _centroids[c*_dimensionality+d] = 0;
        }
      }
      for(unsigned int i = 0; i < _num_points; ++i){
        const unsigned int c = _clusters[i];
        for(unsigned int d = 0; d < _dimensionality; ++d){
          _centroids[c*_dimensionality+d] += _data[i*_dimensionality+d];
        }
      }
      for(unsigned int c = 0; c < _num_clusters; ++c){
        for(unsigned int d = 0; d < _dimensionality; ++d){
          _centroids[c*_dimensionality+d] /= _cluster_size[c];
        }
      }

    }

  }
}

#endif
