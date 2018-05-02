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


#ifndef TSNE_RANDOM_WALKS_INL
#define TSNE_RANDOM_WALKS_INL

#include "hdi/dimensionality_reduction/tsne_random_walks.h"
#include "hdi/utils/math_utils.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/scoped_timers.h"
#include <random>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

#pragma warning( push )
#pragma warning( disable : 4267)
#pragma warning( push )
#pragma warning( disable : 4291)
#pragma warning( push )
#pragma warning( disable : 4996)
#pragma warning( push )
#pragma warning( disable : 4018)
#pragma warning( push )
#pragma warning( disable : 4244)
//#define FLANN_USE_CUDA
#include "flann/flann.h"
#pragma warning( pop )
#pragma warning( pop )
#pragma warning( pop )
#pragma warning( pop )
#pragma warning( pop )

namespace hdi{
  namespace dr{
  /////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    TSNERandomWalks<scalar_type>::Parameters::Parameters():
      _num_neighbors(30),
      _seed(0),
      _embedding_dimensionality(2),
      _minimum_gain(0.1),
      _eta(200),
      _momentum(0.5),
      _final_momentum(0.8),
      _mom_switching_iter(250),
      _exaggeration_factor(4),
      _remove_exaggeration_iter(250),
      _number_of_landmarks(100),
      _num_walks_per_landmark(2000),
      _distance_weighted_random_walk(true)
    {}

  /////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    TSNERandomWalks<scalar_type>::Statistics::Statistics():
      _neighborhood_graph_time(0),
      _landmarks_selection_time(0),
      _random_walks_time(0),
      _num_random_walks(0),
      _avg_walk_length(0),
      _landmarks_datapoints_ratio(0)
    {}

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::Statistics::log(utils::AbstractLog* logger)const{
      utils::secureLog(logger,"tSNE Random Walks");
      utils::secureLogValue(logger,"\tLandmarks/datapoints ratio", _landmarks_datapoints_ratio);
      utils::secureLogValue(logger,"\tNeighborhood graph computation time", _neighborhood_graph_time);
      utils::secureLogValue(logger,"\tLandmarks selection time", _landmarks_selection_time);
      utils::secureLogValue(logger,"\tRandom walks computation time", _random_walks_time);
      utils::secureLogValue(logger,"\t# of random walks", _num_random_walks);
      utils::secureLogValue(logger,"\tmsec per random walk", _random_walks_time / _num_random_walks);
      utils::secureLogValue(logger,"\tavg # of neigh", _avg_num_neighbors);
      utils::secureLogValue(logger,"\tavg walk length", _avg_walk_length);
    }

  /////////////////////////////////////////////////////////////////////////

    template <typename scalar_type>
    TSNERandomWalks<scalar_type>::TSNERandomWalks():
      _initialized(false),
      _dimensionality(0),
      _logger(nullptr),
      _high_dimensional_data(nullptr)
    {
  
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::reset(){
      _initialized = false;
    }
  
    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::clear(){
      _high_dimensional_data = nullptr;
      _embedding.clear();
      _initialized = false;
    }
  
    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::getHighDimensionalDescriptor(scalar_vector_type& data_point, data_handle_type handle)const{
      data_point.resize(_dimensionality);
      for(unsigned int i = 0; i < _dimensionality; ++i){
        data_point[i] = *(_high_dimensional_data + handle*_dimensionality +i);
      }
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle)const{
      if(!_initialized){
        throw std::logic_error("Algorithm must be initialized before ");
      }
      embedding_position.resize(_params._embedding_dimensionality);
      for(int i = 0; i < _params._embedding_dimensionality; ++i){
        embedding_position[i] = _embedding[handle*_params._embedding_dimensionality + i];
      }
    }
  

  /////////////////////////////////////////////////////////////////////////


    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::initialize(scalar_type* high_dimensional_data, unsigned int num_dps, Parameters params){
      utils::secureLog(_logger,"Initializing tSNE...");
      {//Aux data
        _params = params;
        _high_dimensional_data = high_dimensional_data;
        _num_dps = num_dps;

        _statistics._landmarks_datapoints_ratio = scalar_type(_params._number_of_landmarks)/_num_dps;
        int size_sq = params._number_of_landmarks;
        size_sq *= size_sq;
        _P.resize(size_sq);
        _Q.resize(size_sq);
        _embedding.resize(getNumberOfLandmarks()*params._embedding_dimensionality,0);
        _gradient.resize(getNumberOfLandmarks()*params._embedding_dimensionality,0);
        _previous_gradient.resize(getNumberOfLandmarks()*params._embedding_dimensionality,0);
        _gain.resize(getNumberOfLandmarks()*params._embedding_dimensionality,1);
      }
      
      utils::secureLogValue(_logger,"Number of landmarks",params._number_of_landmarks);
      utils::secureLogValue(_logger,"Number of data points",_num_dps);

      computeNeighborhoodGraph();
      computeLandmarks();
      if(_params._distance_weighted_random_walk){
        computeDistanceWeigthedRandomWalks();
      }else{
        computeRandomWalks();
      }
      computeHighDimensionalDistribution();
      initializeEmbeddingPosition(params._seed);

      _iteration = 0;

      _initialized = true;
      utils::secureLog(_logger,"Initialization complete!");
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeNeighborhoodGraph(){
      utils::ScopedTimer<scalar_type, utils::Milliseconds> timer(_statistics._neighborhood_graph_time);
      utils::secureLog(_logger,"Computing the neighborhood graph...");
      flann::Matrix<scalar_type> dataset  (_high_dimensional_data,_num_dps,_dimensionality);
      flann::Matrix<scalar_type> query  (_high_dimensional_data,_num_dps,_dimensionality);
      
      flann::Index<flann::L2<scalar_type> > index(dataset, flann::KDTreeIndexParams(4)); //TEMP
      //flann::Index<flann::L2<scalar_type> > index(dataset, flann::KDTreeCuda3dIndexParams()); //TEMP
      index.buildIndex();

      unsigned int nn = _params._num_neighbors + 1;

      _knns.resize(_num_dps*nn);
      _rw_probabilities.resize(_num_dps*nn);

      flann::Matrix<int> indices_mat(_knns.data(), query.rows, nn);
      flann::Matrix<scalar_type> dists_mat(_rw_probabilities.data(), query.rows, nn);

      flann::SearchParams params(1024); //TEMP
      params.cores = 8;
      index.knnSearch(query, indices_mat, dists_mat, nn, params);

      for(int d = 0; d < _num_dps; ++d){
        {
          double avg = 0;
          double std_dev = 0;
          for(int n = 1; n < nn; ++n){
            avg += _rw_probabilities[d*nn+n];
            std_dev += _rw_probabilities[d*nn+n]*_rw_probabilities[d*nn+n];
          }
          avg /= nn-1;
          std_dev /= nn-1;
          std_dev = std::sqrt(std_dev-avg*avg);
          for(int n = 1; n < nn; ++n){
            _rw_probabilities[d*nn+n] = _rw_probabilities[d*nn+n]/std_dev/2;
          }
        }

        double norm = 0;
        for(int n = 1; n < nn; ++n){
          double p = _rw_probabilities[d*nn+n];
          double p_sq = p*p;
          p = std::exp(-p_sq);
          _rw_probabilities[d*nn+n] = p;
          norm += p;
        }
        for(int n = 1; n < nn; ++n){
          _rw_probabilities[d*nn+n] /= norm;
        }
        for(int n = 1; n < nn; ++n){
          _rw_probabilities[d*nn+n] += _rw_probabilities[d*nn+n-1];
        }
      }
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeLandmarks(){
      utils::secureLog(_logger,"Computing landmarks...");
      utils::ScopedTimer<scalar_type, utils::Milliseconds> timer(_statistics._landmarks_selection_time);
      _idx_landmarks_to_dps.clear();
      _idx_dps_to_landmarks.clear();

      _idx_landmarks_to_dps.resize(_params._number_of_landmarks,-1);
      _idx_dps_to_landmarks.resize(_num_dps,-1);


      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, _num_dps-1);

      int selected_landmarks = 0;
      while(selected_landmarks < _params._number_of_landmarks){
        int idx = dis(gen);
        assert(idx >= 0);
        assert(idx < _num_dps);

        if(_idx_dps_to_landmarks[idx] != -1){
          continue;
        }

        _idx_dps_to_landmarks[idx] = selected_landmarks;
        _idx_landmarks_to_dps[selected_landmarks] = idx;

        ++selected_landmarks;
      }
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeDistanceWeigthedRandomWalks(){
      utils::ScopedTimer<scalar_type, utils::Milliseconds> timer(_statistics._random_walks_time);
      utils::secureLog(_logger,"Computing some random walks...");
      _statistics._num_random_walks = 0;
      _statistics._avg_num_neighbors = 0;

      //Temp -> probability
      unsigned int nn = _params._num_neighbors + 1;
      std::default_random_engine generator;
      std::uniform_real_distribution<double> distribution(0.0, 1.0);

      const int n = getNumberOfLandmarks();
      const int num_walks_per_landmark = _params._num_walks_per_landmark;
    //#pragma omp parallel for
      for(int l = 0; l < n; ++l){
        std::vector<scalar_type> rw_stats(n,0);
        for(int rw = 0; rw < num_walks_per_landmark; ++rw){
          //Random walk
          int dp_idx = _idx_landmarks_to_dps[l];
          int walk_length = 0;
          do{
            //TEMP - no probability
            const double rnd_num = distribution(generator);
            int idx_knn = 0;
            for(idx_knn = 1; idx_knn < nn; ++idx_knn){
              if(rnd_num < _rw_probabilities[nn*dp_idx + idx_knn]){
                break;
              }
            }
            const int idx_knn_global = nn*dp_idx + idx_knn;
            dp_idx = _knns[idx_knn_global];
            ++walk_length;
          } while(!(_idx_dps_to_landmarks[dp_idx] != -1 && _idx_dps_to_landmarks[dp_idx] != l));

          ++rw_stats[_idx_dps_to_landmarks[dp_idx]];
          ++_statistics._num_random_walks;
          _statistics._avg_walk_length += walk_length;
        }
        //computing the probabilities
        for(auto& v : rw_stats){
          if(v != 0){
            ++_statistics._avg_num_neighbors;
          }
          v /= num_walks_per_landmark;
        }
        for(int i = 0; i < n; ++i){
          _P[l*n + i] = rw_stats[i];
        }
      }
      _statistics._avg_num_neighbors /= n;
      _statistics._avg_walk_length /= (n*num_walks_per_landmark);
      
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeRandomWalks(){
      utils::ScopedTimer<scalar_type, utils::Milliseconds> timer(_statistics._random_walks_time);
      utils::secureLog(_logger,"Computing some random walks...");
      _statistics._num_random_walks = 0;
      _statistics._avg_num_neighbors = 0;

      //Temp -> probability
      unsigned int nn = _params._num_neighbors + 1;
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(1, nn-1);

      const int n = getNumberOfLandmarks();
      const int num_walks_per_landmark = 2000;
    //#pragma omp parallel for
      for(int l = 0; l < n; ++l){
        std::vector<scalar_type> rw_stats(n,0);
        for(int rw = 0; rw < num_walks_per_landmark; ++rw){
          //Random walk
          int dp_idx = _idx_landmarks_to_dps[l];
          do{
            //TEMP - no probability
            const int idx_knn = dis(gen);
            const int idx_knn_global = nn*dp_idx + idx_knn;
            dp_idx = _knns[idx_knn_global];
          } while(!(_idx_dps_to_landmarks[dp_idx] != -1 && _idx_dps_to_landmarks[dp_idx] != l));

          ++rw_stats[_idx_dps_to_landmarks[dp_idx]];
          ++_statistics._num_random_walks;
        }
        //computing the probabilities
        for(auto& v : rw_stats){
          if(v != 0){
            ++_statistics._avg_num_neighbors;
          }
          v /= num_walks_per_landmark;
        }
        for(int i = 0; i < n; ++i){
          _P[l*n + i] = rw_stats[i];
        }
      }
      _statistics._avg_num_neighbors /= n;
      
    }

    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeHighDimensionalDistribution(){
      utils::secureLog(_logger,"Computing high-dimensional joint probability distribution...");
      const int n = getNumberOfLandmarks();
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
    void TSNERandomWalks<scalar_type>::initializeEmbeddingPosition(int seed, double multiplier){
      utils::secureLog(_logger,"Initializing the embedding...");
      if(seed < 0){
        std::srand(static_cast<unsigned int>(time(NULL)));
      }
      else{
        std::srand(seed);
      }

      for(auto& v : _embedding){
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
    void TSNERandomWalks<scalar_type>::doAnIteration(double mult){
      if(!_initialized){
        throw std::logic_error("Cannot compute a gradient descent iteration on unitialized data");
      }

      if(_iteration == _params._mom_switching_iter){
        utils::secureLog(_logger,"Switch to final momentum...");
      }
      if(_iteration == _params._remove_exaggeration_iter){
        utils::secureLog(_logger,"Remove exaggeration...");
      }

      //Compute Low-dimensional distribution
      computeLowDimensionalDistribution();

      //Compute gradient of the KL function
      computeGradient((_iteration<_params._remove_exaggeration_iter)?_params._exaggeration_factor:1.);

      //Compute gradient of the KL function
      updateTheEmbedding(mult);
    }
    
    template <typename scalar_type>
    void TSNERandomWalks<scalar_type>::computeLowDimensionalDistribution(){
      const int n = getNumberOfLandmarks();
#ifdef __APPLE__
      std::cout << "GCD dispatch, tsne_random_walks_inl 455.\n";
      dispatch_apply(n, dispatch_get_global_queue(0, 0), ^(size_t j) {
#else
      #pragma omp parallel for
      for(int j = 0; j < n; ++j){
#endif //__APPLE__
        _Q[j*n + j] = 0;
        for(int i = j+1; i < n; ++i){
          const double euclidean_dist_sq(
              utils::euclideanDistanceSquared<scalar_type>(
                _embedding.begin()+j*_params._embedding_dimensionality,
                _embedding.begin()+(j+1)*_params._embedding_dimensionality,
                _embedding.begin()+i*_params._embedding_dimensionality,
                _embedding.begin()+(i+1)*_params._embedding_dimensionality
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
    void TSNERandomWalks<scalar_type>::computeGradient(double exaggeration){
      const int n = getNumberOfLandmarks();
      const int dim = _params._embedding_dimensionality;

  //#pragma omp parallel for
      for(int i = 0; i < n; ++i){
        for(int d = 0; d < dim; ++d){
          _gradient[i * dim + d] = 0;
          double sum_positive(0.);
          double sum_negative(0.);
          for(int j = 0; j < n; ++j){
            const int idx = i*n + j;
            const double distance(_embedding[i * dim + d] - _embedding[j * dim + d]);
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
    void TSNERandomWalks<scalar_type>::updateTheEmbedding(double mult){
      for(int i = 0; i < _gradient.size(); ++i){
        _gain[i] = static_cast<scalar_type>((sign(_gradient[i]) != sign(_previous_gradient[i])) ? (_gain[i] + .2) : (_gain[i] * .8));
        if(_gain[i] < _params._minimum_gain){
          _gain[i] = static_cast<scalar_type>(_params._minimum_gain);
        }
        _gradient[i] = static_cast<scalar_type>((_gradient[i]>0?1:-1)*std::abs(_gradient[i]*_params._eta* _gain[i])/(_params._eta*_gain[i]));

        _previous_gradient[i] = static_cast<scalar_type>(((_iteration<_params._mom_switching_iter)?_params._momentum:_params._final_momentum) * _previous_gradient[i] - _params._eta * _gain[i] * _gradient[i]);
        _embedding[i] += static_cast<scalar_type>(_previous_gradient[i] * mult);
      }
      ++_iteration;
    }

    template <typename scalar_type>
    double TSNERandomWalks<scalar_type>::computeKullbackLeiblerDivergence(){
      double kl = 0;
      const int n = getNumberOfLandmarks();
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

