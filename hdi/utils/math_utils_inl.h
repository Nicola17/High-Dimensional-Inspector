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


#ifndef MATH_INL
#define MATH_INL
#include "hdi/utils/math_utils.h"
#include <assert.h>
#include <cmath>
#include <stdexcept>
#include <limits>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

namespace hdi{
  namespace utils{
    template <typename T>
    T euclideanDistance(const std::vector<T>& a, const std::vector<T>& b){
      assert(a.size() == b.size());
      assert(a.size() != 0);
      double res(0);
      for(int i = 0; i < a.size(); ++i){
        double diff(a[i] - b[i]);
        res += diff*diff;
      }
      return static_cast<T>(std::sqrt(res));
    }

    template <typename T>
    T euclideanDistanceSquared(const std::vector<T>& a, const std::vector<T>& b){
      assert(a.size() == b.size());
      assert(a.size() != 0);
      double res(0);
      for(int i = 0; i < a.size(); ++i){
        double diff(a[i] - b[i]);
        res += diff*diff;
      }
      return static_cast<T>(res);
    }

    template <typename T>
    T euclideanDistance(typename std::vector<T>::const_iterator a_begin, typename std::vector<T>::const_iterator a_end, typename std::vector<T>::const_iterator b_begin, typename std::vector<T>::const_iterator b_end){
      assert(std::distance(a_begin, a_end) == std::distance(b_begin, b_end));
      assert(std::distance(a_begin, a_end) != 0);

      double res(0);
      for(; a_begin != a_end; ++a_begin, ++b_begin){
        double diff(*a_begin - *b_begin);
        res += diff*diff;
      }
      return static_cast<T>(std::sqrt(res));
    }

    template <typename T>
    T euclideanDistanceSquared(typename std::vector<T>::const_iterator a_begin, typename std::vector<T>::const_iterator a_end, typename std::vector<T>::const_iterator b_begin, typename std::vector<T>::const_iterator b_end){
      assert(std::distance(a_begin, a_end) == std::distance(b_begin, b_end));
      assert(std::distance(a_begin, a_end) != 0);

      double res(0);
      for(; a_begin != a_end; ++a_begin, ++b_begin){
        double diff(*a_begin - *b_begin);
        res += diff*diff;
      }
      return static_cast<T>(res);
    }

    template <typename T>
    T euclideanDistance(const T* a_begin, const T* a_end, const T* b_begin, const T* b_end){
      assert(std::distance(a_begin, a_end) == std::distance(b_begin, b_end));
      assert(std::distance(a_begin, a_end) != 0);

      double res(0);
      for(; a_begin != a_end; ++a_begin, ++b_begin){
        double diff(*a_begin - *b_begin);
        res += diff*diff;
      }
      return static_cast<T>(std::sqrt(res));
    }

    template <typename T>
    T euclideanDistanceSquared(const T* a_begin, const T* a_end, const T* b_begin, const T* b_end){
      assert(std::distance(a_begin, a_end) == std::distance(b_begin, b_end));
      assert(std::distance(a_begin, a_end) != 0);

      double res(0);
      for(; a_begin != a_end; ++a_begin, ++b_begin){
        double diff(*a_begin - *b_begin);
        res += diff*diff;
      }
      return static_cast<T>(res);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename vector_type, typename sparse_matrix_type>
    void computeHeterogeneity(const sparse_matrix_type& matrix, vector_type& res){
      auto n = matrix.size();
      res.resize(n);
#ifdef __APPLE__
      std::cout << "GCD dispatch, mat_utils_onl 131.\n";
      dispatch_apply(matrix.size(), dispatch_get_global_queue(0, 0), ^(size_t i) {
#else
      #pragma omp parallel for
      for(int i = 0; i < matrix.size(); ++i){
#endif //__APPLE__
        vector_type distr(n,0);
        distr[i] = n;
        computeStationaryDistribution(matrix,&distr,5,1);
        res[i] = distr[i]/n;
      }
#ifdef __APPLE__
      );
#endif
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename Vector>
    double computeGaussianDistribution(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distribution_begin, typename Vector::iterator distribution_end, double sigma){
      assert(sigma > 0);
      const int size(static_cast<int>(std::distance(distances_begin, distances_end)));
      if(size != std::distance(distribution_begin, distribution_end) || size == 0){
        throw std::logic_error("Invalid containers");
      }
      auto distance_iter = distances_begin;
      auto distribution_iter = distribution_begin;
      double mult(-1. / (2 * sigma * sigma));
      double sum(0);
      for(int idx = 0; distance_iter != distances_end; ++distance_iter, ++distribution_iter, ++idx){
        *distribution_iter = std::exp((*distance_iter) * (*distance_iter) * mult);
        sum += *distribution_iter;
      }
      for(auto distribution_iter = distribution_begin; distribution_iter != distribution_end; ++distribution_iter){
        (*distribution_iter) /= sum;
      }
      return sum;

    }

    template <typename Vector>
    double computeGaussianDistributionWithFixedPerplexity(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distribution_begin, typename Vector::iterator distribution_end, double perplexity, int max_iterations, double tol, int ignore){
      const int size(static_cast<int>(std::distance(distances_begin, distances_end)));
      if(size != std::distance(distribution_begin, distribution_end) || size == 0){
        throw std::logic_error("Invalid containers");
      }

      bool found = false;
      double beta = 1.;
      double sigma = std::sqrt(1/(2*beta));
      double min_beta = -std::numeric_limits<double>::max();
      double max_beta =  std::numeric_limits<double>::max();
    
      const double double_max = std::numeric_limits<double>::max();

      // Iterate until we found a good perplexity
      int iter = 0; 
      double sum_distribution = std::numeric_limits<double>::min();
      while(!found && iter < max_iterations) {
        // Compute Gaussian kernel row
        sum_distribution = std::numeric_limits<double>::min();
        {
          auto distance_iter = distances_begin;
          auto distribution_iter = distribution_begin;
          for(int idx = 0; distance_iter != distances_end; ++distance_iter, ++distribution_iter, ++idx){
            if(idx == ignore){
              (*distribution_iter) = 0;
              continue;
            }
            double v = exp(-beta * (*distance_iter));
            sigma = std::sqrt(1/(2*beta));
            //double v = exp(- (*distance_iter) / (2*sigma*sigma));
            (*distribution_iter) = static_cast<typename Vector::value_type>(v);
            sum_distribution += v;
          }

        }

        double H = .0; //entropy
        {
          auto distance_iter = distances_begin;
          auto distribution_iter = distribution_begin;
          for(int idx = 0; distance_iter != distances_end; ++distance_iter, ++distribution_iter, ++idx){
            if(idx == ignore)
              continue;
            H += beta * ((*distance_iter) * (*distribution_iter));
          }
          H = (H / sum_distribution) + log(sum_distribution);
        }
        
      
        // Evaluate whether the entropy is within the tolerance level
        double Hdiff = H - log(perplexity);
        if(Hdiff < tol && -Hdiff < tol){
          found = true;
        } 
        else{
          if(Hdiff > 0){
            min_beta = beta;
            if(max_beta == double_max || max_beta == -double_max)
              beta *= 2.0;
            else
              beta = (beta + max_beta) / 2.0;
          }
          else{
            max_beta = beta;
            if(min_beta == -double_max || min_beta == double_max)
              beta /= 2.0;
            else
              beta = (beta + min_beta) / 2.0;
          }
        }
        iter++;
      
      }
      if(!found){
        double v = 1./(size+((ignore<0||ignore>=size)?0:-1));
        for(auto distribution_iter = distribution_begin; distribution_iter != distribution_end; ++distribution_iter){
          (*distribution_iter) = v;
        }
        return 0;
      }
      for(auto distribution_iter = distribution_begin; distribution_iter != distribution_end; ++distribution_iter){
        (*distribution_iter) /= sum_distribution;
      }
      return sigma;
    }

    template <typename Vector>
    double computeGaussianDistributionWithFixedWeight(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distribution_begin, typename Vector::iterator distribution_end, double weight, int max_iterations, double tol, int ignore){
      const int size(static_cast<int>(std::distance(distances_begin, distances_end)));
      if(size != std::distance(distribution_begin, distribution_end) || size == 0){
        throw std::logic_error("Invalid containers");
      }

      bool found = false;
      double beta = 1.;
      double sigma = std::sqrt(1/(2*beta));
      double min_beta = -std::numeric_limits<double>::max();
      double max_beta =  std::numeric_limits<double>::max();
    
      const double double_max = std::numeric_limits<double>::max();

      // Iterate until we found a good perplexity
      int iter = 0; 
      double sum_distribution = std::numeric_limits<double>::min();
      while(!found && iter < max_iterations) {
        // Compute Gaussian kernel row
        sum_distribution = std::numeric_limits<double>::min();
        {
          auto distance_iter = distances_begin;
          auto distribution_iter = distribution_begin;
          for(int idx = 0; distance_iter != distances_end; ++distance_iter, ++distribution_iter, ++idx){
            if(idx == ignore)
              continue;
            //double v = exp(-beta * (*distance_iter));
            double sigma =std::sqrt(1/(2*beta));
            double v = exp(- (*distance_iter) / (2*sigma*sigma));
            (*distribution_iter) = static_cast<typename Vector::value_type>(v);
            sum_distribution += v;
          }
        }

        // Evaluate whether the weight is within the tolerance level
        if(sum_distribution-weight < tol && weight-sum_distribution < tol){
          found = true;
        } 
        else{
          if(sum_distribution > weight){
            min_beta = beta;
            if(max_beta == double_max || max_beta == -double_max)
              beta *= 2.0;
            else
              beta = (beta + max_beta) / 2.0;
          }
          else{
            max_beta = beta;
            if(min_beta == -double_max || min_beta == double_max)
              beta /= 2.0;
            else
              beta = (beta + min_beta) / 2.0;
          }
        }
        iter++;
      
      }
      if(!found){
        double v = 1./(size+((ignore<0||ignore>=size)?0:-1));
        for(auto distribution_iter = distribution_begin; distribution_iter != distribution_end; ++distribution_iter){
          (*distribution_iter) = v;
        }
        return 1;
      }
      for(auto distribution_iter = distribution_begin; distribution_iter != distribution_end; ++distribution_iter){
        (*distribution_iter) /= sum_distribution;
      }
      return sum_distribution;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    template <typename Vector>
    double computeGaussianFunction(typename Vector::const_iterator distances_begin, typename Vector::const_iterator distances_end, typename Vector::iterator distribution_begin, typename Vector::iterator distribution_end, double sigma, double alpha){
      assert(sigma > 0);
      assert(alpha > 0);
      const int size(static_cast<int>(std::distance(distances_begin, distances_end)));
      if(size != std::distance(distribution_begin, distribution_end) || size == 0){
        throw std::logic_error("Invalid containers");
      }
      auto distance_iter = distances_begin;
      auto distribution_iter = distribution_begin;
      double beta(-1. / (2 * sigma * sigma));
      double sum(0);
      for(int idx = 0; distance_iter != distances_end; ++distance_iter, ++distribution_iter, ++idx){
        *distribution_iter = alpha*std::exp((*distance_iter) * (*distance_iter) * beta);
        sum += *distribution_iter;
      }
      return sum;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////



  }
}
#endif 

