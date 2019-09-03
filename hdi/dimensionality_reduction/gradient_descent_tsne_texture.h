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

#ifndef GRADIENT_DESCENT_TSNE_TEXTURE_H
#define GRADIENT_DESCENT_TSNE_TEXTURE_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"
#include <map>
#include <unordered_map>
#include "hdi/data/embedding.h"
#include "hdi/data/map_mem_eff.h"
#include "gpgpu_sne/gpgpu_sne_compute.h"
#include "gpgpu_sne/gpgpu_sne_raster.h"
#include "tsne_parameters.h"
#include <array>

namespace hdi {
  namespace dr {
    //! tSNE with sparse and user-defined probabilities
    /*!
    Implementation of the tSNE algorithm with sparse and user-defined probabilities
    \author Nicola Pezzotti
    */
    class GradientDescentTSNETexture {
    public:
      typedef float scalar_type;
      typedef std::vector<hdi::data::MapMemEff<uint32_t, float>> sparse_scalar_matrix_type;
      typedef std::vector<scalar_type> scalar_vector_type;
      typedef uint32_t data_handle_type;

    public:
      GradientDescentTSNETexture();
      //! Initialize the class with a list of distributions. A joint-probability distribution will be computed as in the tSNE algorithm
      void initialize(const sparse_scalar_matrix_type& probabilities, data::Embedding<scalar_type>* embedding, TsneParameters params = TsneParameters());
      //! Initialize the class with a joint-probability distribution. Note that it must be provided non initialized and with the weight of each row equal to 2.
      void initializeWithJointProbabilityDistribution(const sparse_scalar_matrix_type& distribution, data::Embedding<scalar_type>* embedding, TsneParameters params = TsneParameters());
      //! Reset the internal state of the class but it keeps the inserted data-points
      void reset();
      //! Reset the class and remove all the data points
      void clear();

      //! Get the position in the embedding for a data point
      void getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle)const;

      //! Get the number of data points
      unsigned int getNumberOfDataPoints() { return _P.size(); }
      //! Get P
      const sparse_scalar_matrix_type& getDistributionP()const { return _P; }
      //! Get Q
      const scalar_vector_type& getDistributionQ()const { return _Q; }

      //! Return the current log
      utils::AbstractLog* logger()const { return _logger; }
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger) { _logger = logger; }

      //! Do an iteration of the gradient descent
      void doAnIteration(double mult = 1);
      //! Compute the Kullback Leibler divergence
      double computeKullbackLeiblerDivergence();

      //! Set the current iterations
      void setIteration(unsigned int iteration) { _iteration = iteration; }
      //! iterations performed by the algo
      unsigned int iteration()const { return _iteration; }

      //! Set the adaptive texture scaling
      void setResolutionFactor(float factor) {
#ifndef __APPLE__
        if (GLAD_GL_VERSION_4_3)
        {
          _gpgpu_compute_tsne.setScalingFactor(factor);
        }
		else if (GLAD_GL_VERSION_3_3)
#endif // __APPLE__
        {
          _gpgpu_raster_tsne.setScalingFactor(factor);
        }
      }

      //! Exageration baseline
      double& exaggeration_baseline() { return _exaggeration_baseline; }
      const double& exaggeration_baseline()const { return _exaggeration_baseline; }

    private:
      //! Compute High-dimensional distribution
      void computeHighDimensionalDistribution(const sparse_scalar_matrix_type& probabilities);
      //! Initialize the point in the embedding
      void initializeEmbeddingPosition(int seed, double multiplier = .1);
      //! Do an iteration of the gradient descent
      void doAnIterationExact(double mult = 1);
      //! Do an iteration of the gradient descent
      void doAnIterationBarnesHut(double mult = 1);
      //! Compute Low-dimensional distribution
      void computeLowDimensionalDistribution();
      //! Compute tSNE gradient with the texture based algorithm
      void doAnIterationImpl(double exaggeration);

      //! Compute the exaggeration factor based on the current iteration
      double exaggerationFactor();

    public: //TODO remove
      sparse_scalar_matrix_type _P; //! Conditional probalility distribution in the High-dimensional space

    private:
      data::Embedding<scalar_type>* _embedding; //! embedding
	  data::Embedding<scalar_type>::scalar_vector_type* _embedding_container;
	  // TH: the below does not work in VS2013
	  // typename data::Embedding<scalar_type>::scalar_vector_type* _embedding_container;
      bool _initialized; //! Initialization flag

      double _exaggeration_baseline;


      scalar_vector_type _Q; //! Conditional probalility distribution in the Low-dimensional space
      scalar_type _normalization_Q; //! Normalization factor of Q - Z in the original paper

#ifndef __APPLE__
      GpgpuSneCompute _gpgpu_compute_tsne;
#endif // __APPLE__
      GpgpuSneRaster _gpgpu_raster_tsne;

      std::array<scalar_type, 4> _temp;

      TsneParameters _params;
      unsigned int _iteration;

      utils::AbstractLog* _logger;
    };
  }
}
#endif
