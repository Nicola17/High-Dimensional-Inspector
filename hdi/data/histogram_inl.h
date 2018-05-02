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

#ifndef HISTOGRAM_INL
#define HISTOGRAM_INL

#include "hdi/data/histogram.h"
#include <algorithm>
#include <limits>
#include <algorithm>

namespace hdi{
  namespace data{

    template <typename scalar_type>
    Histogram<scalar_type>::Histogram():
      _max(0),
      _min(0),
      _num_buckets(0)
    {}
    template <typename scalar_type>
    Histogram<scalar_type>::Histogram(scalar_type min, scalar_type max, unsigned int num_buckets):
      _max(max),
      _min(min),
      _num_buckets(num_buckets),
      _histogram(num_buckets,0)
    {}

    template <typename scalar_type>
    void Histogram<scalar_type>::resize(scalar_type min, scalar_type max, unsigned int num_buckets){
      _max = max;
      _min = min;
      _num_buckets = num_buckets;
      _histogram.resize(num_buckets);
      clear();
    }

    template <typename scalar_type>
    void Histogram<scalar_type>::clear(){
      for(auto& b: _histogram){
        b = 0;
      }
    }

    template <typename scalar_type>
    scalar_type Histogram<scalar_type>::sum()const{
      scalar_type n = 0;
      for(auto& b: _histogram){
        n += b;
      }
      return n;
    }

    template <typename scalar_type>
    void Histogram<scalar_type>::add(scalar_type v){
      if(v < _min || v > _max){
        return;
      }
      if(v == _max){
        ++_histogram[_num_buckets-1];
        return;
      }
      int id = (v-_min)/(_max-_min)*num_buckets();
      ++_histogram[id];
    }


    template <typename scalar_type>
    std::pair<scalar_type, scalar_type> Histogram<scalar_type>::getBucketLimits(unsigned int id)const{
      scalar_type l = (_max-_min)/num_buckets();
      return std::pair<scalar_type,scalar_type>(_min+id*l,_min+(id+1)*l);
    }

  }
}

#endif
