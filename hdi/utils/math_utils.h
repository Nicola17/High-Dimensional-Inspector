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

#ifndef MATH_H
#define MATH_H

#include <vector>
#include <stdint.h>
#include <cmath>
#include <cassert>
#include <cmath>
#include <math.h> 
#include <iostream>

namespace hdi{
  namespace utils{

    template <typename T>
    T euclideanDistance(const std::vector<T>& a, const std::vector<T>& b);
    template <typename T>
    T euclideanDistanceSquared(const std::vector<T>& a, const std::vector<T>& b);

    template <typename T>
    T euclideanDistance(typename std::vector<T>::const_iterator a_begin, typename std::vector<T>::const_iterator a_end, typename std::vector<T>::const_iterator b_begin, typename std::vector<T>::const_iterator b_end);
    template <typename T>
    T euclideanDistanceSquared(typename std::vector<T>::const_iterator a_begin, typename std::vector<T>::const_iterator a_end, typename std::vector<T>::const_iterator b_begin, typename std::vector<T>::const_iterator b_end);

    template <typename T>
    T euclideanDistance(const T* a_begin, const T* a_end, const T* b_begin, const T* b_end);
    template <typename T>
    T euclideanDistanceSquared(const T* a_begin, const T* a_end, const T* b_begin, const T* b_end);

///////////////////////////////////////////////////////////////////////////////////////////////////

    //! Compute a gaussian distribution given the distances of the points. The normalization factor is returned
    template <typename Vector>
    double computeGaussianDistribution(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distr_begin, typename Vector::iterator distr_end, double sigma);

    template <typename Vector>
    double computeGaussianDistributionWithFixedPerplexity(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distr_begin, typename Vector::iterator distr_end, double perplexity, int max_iterations = 500, double tol = 1e-5, int ignore = -1);

    template <typename Vector>
    double computeGaussianDistributionWithFixedWeight(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distr_begin, typename Vector::iterator distr_end, double weight, int max_iterations = 500, double tol = 1e-5, int ignore = -1);

///////////////////////////////////////////////////////////////////////////////////////////////////

    //! Compute a gaussian function given the distances of the points, the sigma and the multiplication factor alpha. The weigth of the function is returned.
    template <typename Vector>
    double computeGaussianFunction(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distr_begin, typename Vector::iterator distr_end, double sigma, double alpha = 1.);

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename vector_type, typename sparse_matrix_type>
    void computeHeterogeneity(const sparse_matrix_type& matrix, vector_type& res);

///////////////////////////////////////////////////////////////////////////////////////////////////

    //! Compute perplexity of a probability distribution
    template <typename iterator_type>
    double computePerplexity(iterator_type begin, iterator_type end){
      double entropy(0);
      double sum_distribution(0);
      auto iter = begin;
      while(iter != end){
        //assert(*iter >= 0); //TEMP
        if(*iter > 0){
          entropy += -(*iter)*std::log2(*iter);
        }
        ++iter;
      }
      assert(std::abs(sum_distribution-1) < 10-5);
      return std::pow(2.,entropy);
    }

    //! Compute perplexity of a probability distribution
    template <typename map_type>
    double computePerplexity(map_type& map){
      double entropy(0);
      double sum_distribution(0);
      for(auto& v: map){
        assert(v.second >= 0);
        if(v.second > 0){
          entropy += -v.second*std::log2(v.second);
        }
      }
      assert(std::abs(sum_distribution-1) < 10-5);
      return std::pow(2.,entropy);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename sparse_matrix_type, typename vector_type>
    void multiply(const vector_type& a, const sparse_matrix_type& b, vector_type& c){
      assert(a.size() == b.size());
      c.resize(a.size());
      const unsigned int n = a.size();
      for(unsigned int i = 0; i < n; ++i){
        c[i] = 0;
      }
      for(unsigned int i = 0; i < n; ++i){
        if(a[i] == 0){
          continue;
        }
        for(auto& v: b[i]){
          c[v.first]  += static_cast<typename vector_type::value_type>(v.second * a[i]);
        }
      }
    }

    template <typename sparse_matrix_type, typename vector_type>
    void computeStationaryDistribution(const sparse_matrix_type& fmc, vector_type* distribution, uint32_t iterations, typename vector_type::value_type eps){
      const unsigned int n = fmc.size();
      assert(fmc.size() == distribution->size());


      for(int i = 0; i < n; ++i){
        double sum = 0;
        for(auto p: fmc[i]){
          sum += p.second;
        }
        if(std::abs(sum-1) > 0.001)
          std::cout << "fmc test(" << i << "): " << sum <<std::endl;
      }


      vector_type temp_distr(n,0);
      vector_type* a(distribution);
      vector_type* c(&temp_distr);
      for(unsigned int it = 0; it < iterations; ++it){
        multiply(*a,fmc,*c);
        std::swap(a,c);
        double sum = 0;
        for(unsigned int i = 0; i < n; ++i){
          if((*a)[i] < eps){
            (*a)[i] = 0;
          }
          sum+=(*a)[i];
        }
        //("%f",sum);
        //std::cout << "sum: " <<sum<<std::endl;
      }

      //copy to distribution if needed
      if(c == &temp_distr){
        std::swap((*distribution),temp_distr);
      }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename sparse_matrix_type, typename vector_type>
    void stationaryDistributionFMC(const vector_type& initial_distribution, const sparse_matrix_type& transition_matrix, vector_type& stationary_distribution, unsigned int iterations = 1000){
      vector_type distribution(initial_distribution);
      for(unsigned int i = 0; i < iterations; ++i){
        multiply(distribution,transition_matrix,stationary_distribution);
        distribution = stationary_distribution;
      }
    }

  }
}
#endif 
