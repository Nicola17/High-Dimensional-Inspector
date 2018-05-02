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

#ifndef TSNE_H
#define TSNE_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/data/embedding.h"

namespace hdi{
  namespace dr{
    //! tSNE algorithm
    /*!
      Implementation of the tSNE algorithm
      \author Nicola Pezzotti
      \note 
      This class is aimed at the analysis of very small data, therefore it lacks a lot of optimizations, e.g. reduntant data in P and Q
      It is written more for educational purposes than for a real usage

    */
    template <typename scalar_type = float>
    class TSNE{
    public:
      typedef std::vector<scalar_type> scalar_vector_type;
      typedef uint32_t data_handle_type;

    public:
      //! Parameters used for the initialization of the algorithm
      class InitParams{
      public:
        InitParams();
        double _perplexity;
        int _seed;
        int _embedding_dimensionality;

        double _minimum_gain;
        double _eta;
        double _momentum;
        double _final_momentum;
        double _mom_switching_iter;
        double _exaggeration_factor;
        int _remove_exaggeration_iter;
      };


    public:
      TSNE();
      //! Get the dimensionality of the data
      void setDimensionality(int dimensionality){
        checkAndThrowLogic(!_initialized,"Class should be uninitialized to change the dimensionality"); 
        checkAndThrowLogic(dimensionality > 0,"Invalid dimensionality");
        _dimensionality = dimensionality;
      }
      //! Add a data-point to be processd
      //! \warning the class should not be initialized
      data_handle_type addDataPoint(const scalar_type* ptr);
      //! Initialize the class with the current data-points
      void initialize(data::Embedding<scalar_type>* embedding, InitParams params = InitParams());
      //! Reset the internal state of the class but it keeps the inserted data-points
      void reset();
      //! Reset the class and remove all the data points
      void clear();
      //! Return the number of data points
      int size(){  return static_cast<int>(_high_dimensional_data.size());  }

      //! Get the dimensionality of the data
      int dimensionality(){return _dimensionality;}
      //! Get the high dimensional descriptor for a data point
      void getHighDimensionalDescriptor(scalar_vector_type& data_point, data_handle_type handle)const;
      //! Get the position in the embedding for a data point
      void getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle)const;


      //! Get all the data points
      const std::vector<const scalar_type*>& getDataPoints()const{ return _high_dimensional_data; }

      //! Get the embedding
      //const scalar_vector_type& getEmbedding()const{ return _embedding; }
      //! Get the embedding
      //scalar_vector_type& getEmbedding(){ return _embedding; }
      //! Get distances between data-points
      const scalar_vector_type& getDistancesSquared()const{ return _distances_squared; }
      //! Get P
      const scalar_vector_type& getDistributionP()const{ return _P; }
      //! Get Q
      const scalar_vector_type& getDistributionQ()const{ return _Q; }
      //! Get Sigmas
      const scalar_vector_type& getSigmas()const{ return _sigmas; }

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}

      //! Do an iteration of the gradient descent
      void doAnIteration(double mult = 1);
      //! Compute the Kullback Leibler divergence
      double computeKullbackLeiblerDivergence();
    

    private:
      //! Compute the euclidean distances between points
      void computeHighDimensionalDistances();
      //! Compute a gaussian distribution for each data-point
      void computeGaussianDistributions(double perplexity);
      //! Compute High-dimensional distribution
      void computeHighDimensionalDistribution();
      //! Initialize the point in the embedding
      void initializeEmbeddingPosition(int seed, double multipleir = .0001);
      //! Compute Low-dimensional distribution
      void computeLowDimensionalDistribution();
      //! Compute tSNE gradient
      void computeGradient(double exaggeration);
      //! Update the embedding
      void updateTheEmbedding(double mult = 1.);

    

    private:
      int _dimensionality;
      std::vector<const scalar_type*> _high_dimensional_data; //! High-dimensional data
      data::Embedding<scalar_type>* _embedding; //! embedding
      typename data::Embedding<scalar_type>::scalar_vector_type* _embedding_container;

      bool _initialized; //! Initialization flag

      scalar_vector_type _P; //! Conditional probalility distribution in the High-dimensional space
      scalar_vector_type _Q; //! Conditional probalility distribution in the Low-dimensional space
      scalar_type _normalization_Q; //! Normalization factor of Q - Z in the original paper

      scalar_vector_type _distances_squared; //! High-dimensional distances
      scalar_vector_type _sigmas; //! Sigmas of the gaussian probability distributions

      // Gradient descent
      scalar_vector_type _gradient; //! Current gradient
      scalar_vector_type _previous_gradient; //! Previous gradient
      scalar_vector_type _gain; //! Gain


      InitParams _init_params;
      int _iteration;

      utils::AbstractLog* _logger;
  
    };
  }
}
#endif 
