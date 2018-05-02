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

#ifndef VECTOR_UTILS_H
#define VECTOR_UTILS_H

#include <vector>
#include <assert.h>
#include <cmath>

namespace hdi{
  namespace utils{

    template <typename scalar_type>
    void sum(std::vector<scalar_type>& res, const std::vector<scalar_type>& a, const std::vector<scalar_type>& b){
      assert( res.size() == a.size() && a.size() == b.size() );
      for(int i = 0; i < res.size(); ++i){
        res[i] = a[i] + b[i];
      }
    }

    template <typename scalar_type>
    void multiply(std::vector<scalar_type>& v, scalar_type a){
      for(int i = 0; i < v.size(); ++i){
        v[i] *= a;
      }
    }

    template <typename scalar_type>
    void normalize(std::vector<scalar_type>& v){
      scalar_type sum = 0;
      for(int i = 0; i < v.size(); ++i){
        sum += v[i]*v[i];
      }
      if(sum != 0){
        sum = std::sqrt(sum);
        for(int i = 0; i < v.size(); ++i){
          v[i] /= sum;
        }
      }else{
        for(int i = 0; i < v.size(); ++i){
          v[i] = 1./v.size();
        }
      }
    }

    template <typename scalar_type>
    void normalizeL1(std::vector<scalar_type>& v){
      double sum = 0;
      for(int i = 0; i < v.size(); ++i){
        sum += v[i];
      }
      if(sum != 0){
        for(int i = 0; i < v.size(); ++i){
          v[i] /= sum;
        }
      }else{
        for(int i = 0; i < v.size(); ++i){
          v[i] = 1./v.size();
        }
      }
    }
    template <typename scalar_type>
    void softMax(std::vector<scalar_type>& v){
      double sum = 0;
      for(int i = 0; i < v.size(); ++i){
        v[i] = std::exp(v[i]);
        sum += v[i];
      }
      for(int i = 0; i < v.size(); ++i){
        v[i] /= sum;
      }
    }

    template <typename scalar_type>
    scalar_type dotProduct(const std::vector<scalar_type>& a, const std::vector<scalar_type>& b){
      assert(a.size() == b.size());
      double res(0);
      for(int i = 0; i < a.size(); ++i){
        res += double(a[i])*double(b[i]);
      }
      return static_cast<scalar_type>(res);
    }

  }
}
#endif
