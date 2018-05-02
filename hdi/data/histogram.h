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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <vector>
#include <assert.h>

namespace hdi{
  namespace data{

    //! Histogram data structure
    /*!
      Histogram data structure
      \author Nicola Pezzotti
    */
    template <typename scalar_type>
    class Histogram{
    public:
      typedef std::vector<scalar_type> scalar_vector_type;

    public:
      Histogram();
      Histogram(scalar_type min, scalar_type max, unsigned int num_buckets);
      //! Clear the container
      void clear();
      //! Resize the container
      void resize(scalar_type min, scalar_type max, unsigned int num_buckets);
      //! add a value in the histogram
      void add(scalar_type v);

      //! return the data
      const scalar_vector_type& data()const{return _histogram;}
      //! return the data
      scalar_vector_type& data(){return _histogram;}
      //! return number of buckets
      unsigned int num_buckets()const{return _num_buckets;}
      //! return max val
      scalar_type min()const{return _min;}
      //! return min val
      scalar_type max()const{return _max;}
      //! return min val
      scalar_type sum()const;

      //! return the limits for a given bucket id
      std::pair<scalar_type,scalar_type> getBucketLimits(unsigned int id)const;

    private:
      scalar_type _min;
      scalar_type _max;
      unsigned int _num_buckets;
      scalar_vector_type _histogram;
    };


  }
}

#endif
