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

#ifndef VISUAL_UTILS_INL
#define VISUAL_UTILS_INL

#include "hdi/utils/visual_utils.h"
#include <QColor>
#include <assert.h>
#include <cmath>

namespace hdi{
  namespace utils{

    template <typename scalar_type>
    QImage imageFromMatrix(const std::vector<std::vector<scalar_type>>& matrix, scalar_type max){
      const unsigned int size = static_cast<unsigned int>(matrix.size());
      QImage image(size,size,QImage::Format_RGB32);
      for(int j = 0; j < size; ++j){
        for(int i = 0; i < size; ++i){
          scalar_type norm = std::min<scalar_type>(matrix[j][i]/max,1);
          auto color = qRgb(norm*255,norm*255,norm*255);
          image.setPixel(i,j,color);
        }
      }
      return image;
    }

    template <typename map_type>
    QImage imageFromSparseMatrix(const std::vector<map_type>& sparse_matrix){
      typedef typename map_type::mapped_type scalar_type;

      const unsigned int size = static_cast<unsigned int>(sparse_matrix.size());
      QImage image(size,size,QImage::Format_RGB32);
      scalar_type max = -std::numeric_limits<scalar_type>::max();

      for(int j = 0; j < size; ++j){
        for(int i = 0; i < size; ++i){
          image.setPixel(i,j,qRgb(50,0,0));
        }
      }

      //max
      for(int j = 0; j < size; ++j){
        for(auto& v: sparse_matrix[j]){
          max = std::max(max,v.second);
        }
      }

      for(int j = 0; j < size; ++j){
        for(auto& v: sparse_matrix[j]){
          assert(v.first < size);
          scalar_type norm = v.second/max;
          auto color = qRgb(norm*255,norm*255,norm*255);
          image.setPixel(v.first,j,color);
        }
      }

      return image;
    }


    template <typename map_type>
    QImage imageFromZeroCenteredSparseMatrix(const std::vector<map_type>& sparse_matrix){
      typedef typename map_type::mapped_type scalar_type;

      const unsigned int size = static_cast<unsigned int>(sparse_matrix.size());
      QImage image(size,size,QImage::Format_RGB32);
      scalar_type max = -std::numeric_limits<scalar_type>::max();

      for(int j = 0; j < size; ++j){
        for(int i = 0; i < size; ++i){
          image.setPixel(i,j,qRgb(50,0,0));
        }
      }

      //max
      for(int j = 0; j < size; ++j){
        for(auto& v: sparse_matrix[j]){
          max = std::max(max,std::abs(v.second));
        }
      }

      for(int j = 0; j < size; ++j){
        for(auto& v: sparse_matrix[j]){
          assert(v.first < size);
          scalar_type norm = std::abs(v.second)/max;
          if(v.second >= 0){
            auto color = qRgb(norm*255,0,0);
            image.setPixel(v.first,j,color);
          }else{
            auto color = qRgb(0,norm*255,0);
            image.setPixel(v.first,j,color);
          }
        }
      }

      return image;
    }

  }
}

#endif // TIMERS_H
