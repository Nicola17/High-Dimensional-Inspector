/*
*
* Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*    This product includes software developed by the Delft University of Technology.
* 4. Neither the name of the Delft University of Technology nor the names of
*    its contributors may be used to endorse or promote products derived from
*    this software without specific prior written permission.
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


#ifndef GRADIENT_DESCENT_TSNE_TEXTURE_INL
#define GRADIENT_DESCENT_TSNE_TEXTURE_INL

#include "hdi/dimensionality_reduction/gradient_descent_tsne_texture.h"
#include "hdi/utils/math_utils.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/scoped_timers.h"
#include "sptree.h"
#include <random>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#define __block
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

namespace hdi {
  namespace dr {

    GradientDescentTSNETexture::GradientDescentTSNETexture() :
      _initialized(false),
      _logger(nullptr),
      _exaggeration_baseline(1)
    {

    }

    void GradientDescentTSNETexture::reset() {
      _initialized = false;
    }

    void GradientDescentTSNETexture::clear() {
      _embedding->clear();
      _initialized = false;
    }

    void GradientDescentTSNETexture::getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle) const {
      if (!_initialized) {
        throw std::logic_error("Algorithm must be initialized before ");
      }
      embedding_position.resize(_params._embedding_dimensionality);
      for (int i = 0; i < _params._embedding_dimensionality; ++i) {
        (*_embedding_container)[i] = (*_embedding_container)[handle*_params._embedding_dimensionality + i];
      }
    }


    /////////////////////////////////////////////////////////////////////////


    void GradientDescentTSNETexture::initialize(const sparse_scalar_matrix_type& probabilities, data::Embedding<scalar_type>* embedding, TsneParameters params) {
      utils::secureLog(_logger, "Initializing tSNE...");
      {//Aux data
        _params = params;
        unsigned int size = probabilities.size();
        unsigned int size_sq = probabilities.size()*probabilities.size();

        _embedding = embedding;
        _embedding_container = &(embedding->getContainer());
        _embedding->resize(_params._embedding_dimensionality, size);
        _P.clear();
        _P.resize(size);
      }

      utils::secureLogValue(_logger, "Number of data points", _P.size());

      computeHighDimensionalDistribution(probabilities);
      initializeEmbeddingPosition(params._seed, params._rngRange);

#ifndef __APPLE__
      if (GLAD_GL_VERSION_4_3)
      {
        _gpgpu_compute_tsne.initialize(_embedding, _params, _P);
      }
      else if (GLAD_GL_VERSION_3_3)
#endif // __APPLE__
      {
        std::cout << "Compute shaders not available, using rasterization fallback" << std::endl;
        _gpgpu_raster_tsne.initialize(_embedding, _params, _P);
      }

      _iteration = 0;

      _initialized = true;
      utils::secureLog(_logger, "Initialization complete!");
    }

    void GradientDescentTSNETexture::initializeWithJointProbabilityDistribution(const sparse_scalar_matrix_type& distribution, data::Embedding<scalar_type>* embedding, TsneParameters params) {
      utils::secureLog(_logger, "Initializing tSNE with a user-defined joint-probability distribution...");
      {//Aux data
        _params = params;
        unsigned int size = distribution.size();
        unsigned int size_sq = distribution.size()*distribution.size();

        _embedding = embedding;
        _embedding_container = &(embedding->getContainer());
        _embedding->resize(_params._embedding_dimensionality, size);
        _P.resize(size);
      }

      utils::secureLogValue(_logger, "Number of data points", _P.size());

      _P = distribution;
      initializeEmbeddingPosition(params._seed, params._rngRange);

#ifndef __APPLE__
      if (GLAD_GL_VERSION_4_3)
      {
        utils::secureLog(_logger, "Init GPGPU gradient descent using compute shaders.");
        _gpgpu_compute_tsne.initialize(_embedding, _params, _P);
      }
      else if (GLAD_GL_VERSION_3_3)
#endif // __APPLE__
      {
        utils::secureLog(_logger, "Init GPU gradient descent. Compute shaders not available, using rasterization fallback.");
        _gpgpu_raster_tsne.initialize(_embedding, _params, _P);
      }


      _iteration = 0;

      _initialized = true;
      utils::secureLog(_logger, "Initialization complete!");
    }

    void GradientDescentTSNETexture::computeHighDimensionalDistribution(const sparse_scalar_matrix_type& probabilities) {
      utils::secureLog(_logger, "Computing high-dimensional joint probability distribution...");

      const int n = getNumberOfDataPoints();
      for (int j = 0; j < n; ++j) {
        for (auto& elem : probabilities[j]) {
          scalar_type v0 = elem.second;
          auto iter = probabilities[elem.first].find(j);
          scalar_type v1 = 0.;
          if (iter != probabilities[elem.first].end())
            v1 = iter->second;

          _P[j][elem.first] = static_cast<scalar_type>((v0 + v1)*0.5);
          _P[elem.first][j] = static_cast<scalar_type>((v0 + v1)*0.5);
        }
      }
    }


    void GradientDescentTSNETexture::initializeEmbeddingPosition(int seed, double multiplier) {
      utils::secureLog(_logger, "Initializing the embedding...");

      if (seed < 0) {
        std::srand(static_cast<unsigned int>(time(NULL)));
      }
      else {
        std::srand(seed);
      }

      for (int i = 0; i < _embedding->numDataPoints(); ++i) {
        double x(0.);
        double y(0.);
        double radius(0.);
        do {
          x = 2 * (rand() / ((double)RAND_MAX + 1)) - 1;
          y = 2 * (rand() / ((double)RAND_MAX + 1)) - 1;
          radius = (x * x) + (y * y);
        } while ((radius >= 1.0) || (radius == 0.0));

        radius = sqrt(-2 * log(radius) / radius);
        x *= radius * multiplier;
        y *= radius * multiplier;
        _embedding->dataAt(i, 0) = x;
        _embedding->dataAt(i, 1) = y;
      }
    }

    void GradientDescentTSNETexture::doAnIteration(double mult) {
      if (!_initialized) {
        throw std::logic_error("Cannot compute a gradient descent iteration on unitialized data");
      }

      if (_iteration == _params._mom_switching_iter) {
        utils::secureLog(_logger, "Switch to final momentum...");
      }
      if (_iteration == _params._remove_exaggeration_iter) {
        utils::secureLog(_logger, "Remove exaggeration...");
      }

      doAnIterationImpl(mult);
    }

    double GradientDescentTSNETexture::exaggerationFactor() {
      scalar_type exaggeration = _exaggeration_baseline;

      if (_iteration <= _params._remove_exaggeration_iter) {
        exaggeration = _params._exaggeration_factor;
      }
      else if (_iteration <= (_params._remove_exaggeration_iter + _params._exponential_decay_iter)) {
        //double decay = std::exp(-scalar_type(_iteration-_params._remove_exaggeration_iter)/30.);
        double decay = 1. - double(_iteration - _params._remove_exaggeration_iter) / _params._exponential_decay_iter;
        exaggeration = _exaggeration_baseline + (_params._exaggeration_factor - _exaggeration_baseline)*decay;
        //utils::secureLogValue(_logger,"Exaggeration decay...",exaggeration);
      }

      return exaggeration;
    }

    void GradientDescentTSNETexture::doAnIterationImpl(double mult) {
      // Compute gradient of the KL function using a compute shader approach
#ifndef __APPLE__
      if (GLAD_GL_VERSION_4_3)
      {
        _gpgpu_compute_tsne.compute(_embedding, exaggerationFactor(), _iteration, mult);
      }
      else if (GLAD_GL_VERSION_3_3)
#endif // __APPLE__
      {
        _gpgpu_raster_tsne.compute(_embedding, exaggerationFactor(), _iteration, mult);
      }
      ++_iteration;
    }

    double GradientDescentTSNETexture::computeKullbackLeiblerDivergence() {
      const int n = _embedding->numDataPoints();

      //std::vector<float> _Q(n * n);
      double sum_Q = 0;
      for (int j = 0; j < n; ++j) {
        //_Q[j*n + j] = 0;

        for (int i = j + 1; i < n; ++i) {
          const double euclidean_dist_sq(
            utils::euclideanDistanceSquared<float>(
              _embedding->getContainer().begin() + j*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + (j + 1)*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + i*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + (i + 1)*_params._embedding_dimensionality
              )
          );
          const double v = 1. / (1. + euclidean_dist_sq);

          //_Q[j*n + i] = static_cast<float>(v);
          //_Q[i*n + j] = static_cast<float>(v);
          sum_Q += v * 2;
        }
      }

      //double sum_Q = 0;
      //for (auto& v : _Q) {
      //  sum_Q += v;
      //}

      double kl = 0;
      
      for (int i = 0; i < n; ++i) {
        for (const auto& pij : _P[i]) {
          uint32_t j = pij.first;

          // Calculate Qij
          const double euclidean_dist_sq(
            utils::euclideanDistanceSquared<float>(
              _embedding->getContainer().begin() + j*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + (j + 1)*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + i*_params._embedding_dimensionality,
              _embedding->getContainer().begin() + (i + 1)*_params._embedding_dimensionality
              )
          );
          const double v = 1. / (1. + euclidean_dist_sq);

          double p = pij.second / (2 * n);
          float klc = p * std::log(p / (v / sum_Q));
          //if (klc > 0.00001)
          //{
          //  std::cout << "KLC: " << klc << " i: " << i << "neighbour: " << neighbour_id << std::endl;
          //}
          
          kl += klc;
        }
      }
      return kl;
    }
  }
}
#endif
