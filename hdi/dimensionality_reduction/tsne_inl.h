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


#ifndef TSNE_INL
#define TSNE_INL

#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/utils/math_utils.h"
#include "hdi/utils/log_helper_functions.h"

#include <time.h>
#include <cmath>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif


namespace hdi{
  namespace dr{
  /////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    TSNE<scalar_type>::InitParams::InitParams():
      _perplexity(30),
      _seed(0),
      _embedding_dimensionality(2),
      _minimum_gain(0.1),
      _eta(200),
      _momentum(0.5),
      _final_momentum(0.8),
      _mom_switching_iter(250),
      _exaggeration_factor(4),
      _remove_exaggeration_iter(250)
    {}

  /////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    TSNE<scalar_type>::TSNE():
      _initialized(false),
      _dimensionality(0),
      _logger(nullptr)
    {
  
    }

    template <typename scalar_type>
    typename TSNE<scalar_type>::data_handle_type TSNE<scalar_type>::addDataPoint(const scalar_type* ptr){
      checkAndThrowLogic(!_initialized,"Class should be uninitialized to add a data-point");
      checkAndThrowLogic(_dimensionality > 0,"Invalid dimensionality");
      _high_dimensional_data.push_back(ptr);
      return static_cast<data_handle_type>(_high_dimensional_data.size() - 1);
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::reset(){
      _initialized = false;
    }
  
    template <typename scalar_type>
    void TSNE<scalar_type>::clear(){
      _high_dimensional_data.clear();
      _embedding->clear();
      _initialized = false;
    }
  
    template <typename scalar_type>
    void TSNE<scalar_type>::getHighDimensionalDescriptor(scalar_vector_type& data_point, data_handle_type handle)const{
      data_point.resize(_dimensionality);
      for(int i = 0; i < _dimensionality; ++i){
        data_point[i] = *(_high_dimensional_data[handle]+i);
      }
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle)const{
      if(!_initialized){
        throw std::logic_error("Algorithm must be initialized before ");
      }
      embedding_position.resize(_init_params._embedding_dimensionality);
      for(int i = 0; i < _init_params._embedding_dimensionality; ++i){
        embedding_position[i] = _embedding->getContainer()[handle*_init_params._embedding_dimensionality + i];
      }
    }
  

  /////////////////////////////////////////////////////////////////////////


    template <typename scalar_type>
    void TSNE<scalar_type>::initialize(data::Embedding<scalar_type>* embedding, InitParams params){
      utils::secureLog(_logger,"Initializing tSNE...");
      if(size() == 0){
        throw std::logic_error("Cannot initialize an empty dataset");
      }
      {
        _embedding = embedding;
        int size_sq = size();
        size_sq *= size_sq;
        _P.resize(size_sq);
        _Q.resize(size_sq);
        _distances_squared.resize(size_sq);
        _embedding->resize(params._embedding_dimensionality,size(),0);
        _embedding_container = &_embedding->getContainer();
        _gradient.resize(size()*params._embedding_dimensionality,0);
        _previous_gradient.resize(size()*params._embedding_dimensionality,0);
        _gain.resize(size()*params._embedding_dimensionality,1);
        _sigmas.resize(size());
        _init_params = _init_params;
      }

      //compute distances between data-points
      computeHighDimensionalDistances();
      //Compute gaussian distributions
      computeGaussianDistributions(params._perplexity);
      //Compute High-dimensional distribution
      computeHighDimensionalDistribution();


      //Initialize Embedding position
      initializeEmbeddingPosition(params._seed);

      _iteration = 0;

      _initialized = true;
      utils::secureLog(_logger,"Initialization complete!");
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::computeHighDimensionalDistances(){
      utils::secureLog(_logger,"Computing High-dimensional distances...");
      const int n = size();
#ifdef __APPLE__
      std::cout << "GCD dispatch, tsne_inl 165.\n";
      dispatch_apply(n, dispatch_get_global_queue(0, 0), ^(size_t j) {
#else
      #pragma omp parallel for
      for(int j = 0; j < n; ++j){
#endif //__APPLE__
        _distances_squared[j*n + j] = 0;
        for(int i = j+1; i < n; ++i){
          scalar_type res(utils::euclideanDistance<scalar_type>(_high_dimensional_data[i],_high_dimensional_data[i]+_dimensionality, _high_dimensional_data[j],_high_dimensional_data[j]+_dimensionality));
          //scalar_type res(utils::euclideanDistanceSquared<scalar_type>(_high_dimensional_data[i],_high_dimensional_data[i]+_dimensionality, _high_dimensional_data[j],_high_dimensional_data[j]+_dimensionality));
          _distances_squared[j*n + i] = res;
          _distances_squared[i*n + j] = res;
        }
      }
#ifdef __APPLE__
      );
#endif
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::computeGaussianDistributions(double perplexity){
      utils::secureLog(_logger,"Computing gaussian distributions...");
      const int n = size();
#ifdef __APPLE__
      std::cout << "GCD dispatch, tsne_inl 189.\n";
      dispatch_apply(n, dispatch_get_global_queue(0, 0), ^(size_t j) {
#else
      #pragma omp parallel for
      for(int j = 0; j < n; ++j){
#endif //__APPLE__
        const auto sigma =  utils::computeGaussianDistributionWithFixedPerplexity<scalar_vector_type>(
                  _distances_squared.begin() + j*n,
                  _distances_squared.begin() + (j + 1)*n,
                  _P.begin() + j*n,
                  _P.begin() + (j + 1)*n,
                  perplexity,
                  200,
                  1e-5,
                  j
                );
        _P[j*n + j] = 0.;
        _sigmas[j] = static_cast<scalar_type>(sigma);
      }
#ifdef __APPLE__
      );
#endif
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::computeHighDimensionalDistribution(){
      utils::secureLog(_logger,"Computing high-dimensional joint probability distribution...");
      const int n = size();
  //#pragma omp parallel for
      for(int j = 0; j < n; ++j){
        for(int i = j+1; i < n; ++i){
          const double v = (_P[j*n + i]+_P[i*n + j])*0.5/n;
          _P[j*n + i] = static_cast<scalar_type>(v);
          _P[i*n + j] = static_cast<scalar_type>(v);
        }
      }
    }


    template <typename scalar_type>
    void TSNE<scalar_type>::initializeEmbeddingPosition(int seed, double multiplier){
      utils::secureLog(_logger,"Initializing the embedding...");
      if(seed < 0){
        std::srand(static_cast<unsigned int>(time(NULL)));
      }
      else{
        std::srand(seed);
      }

      for(auto& v : _embedding->getContainer()){
        double x(0.);
        double y(0.);
        double radius(0.);
        do {
          x = 2 * (rand() / ((double)RAND_MAX + 1)) - 1;
          y = 2 * (rand() / ((double)RAND_MAX + 1)) - 1;
          radius = (x * x) + (y * y);
        } while((radius >= 1.0) || (radius == 0.0));

        radius = sqrt(-2 * log(radius) / radius);
        x *= radius;
        y *= radius;
        v = static_cast<scalar_type>(x * multiplier);
      }
    }
    

    template <typename scalar_type>
    void TSNE<scalar_type>::doAnIteration(double mult){
      if(!_initialized){
        throw std::logic_error("Cannot compute a gradient descent iteration on unitialized data");
      }

      if(_iteration == _init_params._mom_switching_iter){
        utils::secureLog(_logger,"Switch to final momentum...");
      }
      if(_iteration == _init_params._remove_exaggeration_iter){
        utils::secureLog(_logger,"Remove exaggeration...");
      }

      //Compute Low-dimensional distribution
      computeLowDimensionalDistribution();

      //Compute gradient of the KL function
      computeGradient((_iteration<_init_params._remove_exaggeration_iter)?_init_params._exaggeration_factor:1.);

      //Compute gradient of the KL function
      updateTheEmbedding(mult);
    }
    
    template <typename scalar_type>
    void TSNE<scalar_type>::computeLowDimensionalDistribution(){
      const int n = size();
#ifdef __APPLE__
      std::cout << "GCD dispatch, tsne_inl 283.\n";
      dispatch_apply(n, dispatch_get_global_queue(0, 0), ^(size_t j) {
#else
      #pragma omp parallel for
      for(int j = 0; j < n; ++j){
#endif //__APPLE__
        _Q[j*n + j] = 0;
        for(int i = j+1; i < n; ++i){
          const double euclidean_dist_sq(
              utils::euclideanDistanceSquared<scalar_type>(
                _embedding_container->begin()+j*_init_params._embedding_dimensionality,
                _embedding_container->begin()+(j+1)*_init_params._embedding_dimensionality,
                _embedding_container->begin()+i*_init_params._embedding_dimensionality,
                _embedding_container->begin()+(i+1)*_init_params._embedding_dimensionality
              )
            );
          const double v = 1./(1.+euclidean_dist_sq);
          _Q[j*n + i] = static_cast<scalar_type>(v);
          _Q[i*n + j] = static_cast<scalar_type>(v);
        }
      }
#ifdef __APPLE__
      );
#endif
      double sum_Q = 0;
      for(auto& v : _Q){
        sum_Q += v;
      }
      _normalization_Q = static_cast<scalar_type>(sum_Q);
    }

    template <typename scalar_type>
    void TSNE<scalar_type>::computeGradient(double exaggeration){
      const int n = size();
      const int dim = _init_params._embedding_dimensionality;

  //#pragma omp parallel for
      for(int i = 0; i < n; ++i){
        for(int d = 0; d < dim; ++d){
          _gradient[i * dim + d] = 0;
          double sum_positive(0.);
          double sum_negative(0.);
          for(int j = 0; j < n; ++j){
            const int idx = i*n + j;
            const double distance((*_embedding_container)[i * dim + d] - (*_embedding_container)[j * dim + d]);
            const double positive(_P[idx] * _Q[idx] * distance);
            const double negative(_Q[idx] * _Q[idx] / _normalization_Q * distance);
            sum_positive += positive;
            sum_negative += negative;
          }
          _gradient[i * dim + d] = static_cast<scalar_type>(4 * (exaggeration*sum_positive - sum_negative));
        }
      }

    }
  
    //temp
    template <typename T>
    T sign(T x) { return (x == .0 ? .0 : (x < .0 ? -1.0 : 1.0)); }

    template <typename scalar_type>
    void TSNE<scalar_type>::updateTheEmbedding(double mult){
      for(int i = 0; i < _gradient.size(); ++i){
        _gain[i] = static_cast<scalar_type>((sign(_gradient[i]) != sign(_previous_gradient[i])) ? (_gain[i] + .2) : (_gain[i] * .8));
        if(_gain[i] < _init_params._minimum_gain){
          _gain[i] = static_cast<scalar_type>(_init_params._minimum_gain);
        }
        _gradient[i] = static_cast<scalar_type>((_gradient[i]>0?1:-1)*std::abs(_gradient[i]*_init_params._eta* _gain[i])/(_init_params._eta*_gain[i]));

        _previous_gradient[i] = static_cast<scalar_type>(((_iteration<_init_params._mom_switching_iter)?_init_params._momentum:_init_params._final_momentum) * _previous_gradient[i] - _init_params._eta * _gain[i] * _gradient[i]);
        (*_embedding_container)[i] += _previous_gradient[i] * mult;
      }
      ++_iteration;
    }

    template <typename scalar_type>
    double TSNE<scalar_type>::computeKullbackLeiblerDivergence(){
      double kl = 0;
      const int n = size();
      for(int j = 0; j < n; ++j){
        for(int i = 0; i < n; ++i){
          if(i == j)
            continue;
          kl += _P[j*n + i] * std::log(_P[j*n + i] / (_Q[j*n + i]/_normalization_Q));
        }
      }
      return kl;
    }
  }
}
#endif 

