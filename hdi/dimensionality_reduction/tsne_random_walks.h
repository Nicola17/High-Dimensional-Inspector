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

#ifndef TSNE_RANDOM_WALKS_H
#define TSNE_RANDOM_WALKS_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"

namespace hdi{
  namespace dr{
    //! tSNE algorithm with Random Walks
    /*!
      Implementation of the tSNE algorithm with Random Walks
      \author Nicola Pezzotti
      \note Extension of the original tSNE algorithm with computation of the high-dimensional probability 
      distribution P using a Random-Walks on the neighborhood graph
    */
    template <typename scalar_type = float>
    class TSNERandomWalks{
    public:
      typedef std::vector<scalar_type> scalar_vector_type;
      typedef uint32_t data_handle_type;

    public:
      //! Parameters used for the initialization of the algorithm
      class Parameters{
      public:
        Parameters();
        unsigned int _num_neighbors;
        int _seed;
        int _embedding_dimensionality;

        double _minimum_gain;
        double _eta;
        double _momentum;
        double _final_momentum;
        double _mom_switching_iter;
        double _exaggeration_factor;
        unsigned int _remove_exaggeration_iter;
        unsigned int _number_of_landmarks;
        unsigned int _num_walks_per_landmark;
        bool _distance_weighted_random_walk;
      };

      //! Collection of statistics on the algorithm
      class Statistics{
      public:
        Statistics();

        void log(utils::AbstractLog* logger)const;

      public:
        scalar_type _neighborhood_graph_time;
        scalar_type _landmarks_selection_time;
        scalar_type _random_walks_time;
        unsigned int _num_random_walks;
        scalar_type _avg_num_neighbors;
        scalar_type _avg_walk_length;
        scalar_type _landmarks_datapoints_ratio;
      };

    public:
      TSNERandomWalks();
      //! Get the dimensionality of the data
      void setDimensionality(int dimensionality){
        checkAndThrowLogic(!_initialized,"Class should be uninitialized to change the dimensionality"); 
        checkAndThrowLogic(dimensionality > 0,"Invalid dimensionality");
        _dimensionality = dimensionality;
      }
      //! Initialize the class with the current data-points
      void initialize(scalar_type* high_dimensional_data, unsigned int num_dps, Parameters params = Parameters());
      //! Reset the internal state of the class but it keeps the inserted data-points
      void reset();
      //! Reset the class and remove all the data points
      void clear();
      

      //! Get the dimensionality of the data
      unsigned int dimensionality(){return _dimensionality;}
      //! Get the high dimensional descriptor for a data point
      void getHighDimensionalDescriptor(scalar_vector_type& data_point, data_handle_type handle)const;
      //! Get the position in the embedding for a data point
      void getEmbeddingPosition(scalar_vector_type& embedding_position, data_handle_type handle)const;

      //! Get the number of data points
      unsigned int getNumberOfDataPoints(){  return _num_dps;  }
      //! Get the number of landmarks
      unsigned int getNumberOfLandmarks()const{return _params._number_of_landmarks;}
      //! Get all the data points
      const scalar_type* getDataPoints()const{ return _high_dimensional_data; }
      //! Get the embedding
      const scalar_vector_type& getEmbedding()const{ return _embedding; }
      //! Get the embedding
      scalar_vector_type& getEmbedding(){ return _embedding; }
      //! Get P
      const scalar_vector_type& getDistributionP()const{ return _P; }
      //! Get Q
      const scalar_vector_type& getDistributionQ()const{ return _Q; }
      //! Get landmarks ids
      const std::vector<int>& getLandmarksIds()const{ return _idx_landmarks_to_dps; }

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}

      //! Do an iteration of the gradient descent
      void doAnIteration(double mult = 1);
      //! Compute the Kullback Leibler divergence
      double computeKullbackLeiblerDivergence();
    
      const Statistics& statistics(){ return _statistics; }

    private:
      //! Compute the neighborhood graph
      void computeNeighborhoodGraph();
      //! Compute landmarks
      void computeLandmarks();
      //! Compute the random walks between data points
      void computeRandomWalks();
      //! Compute the random walks between data points
      void computeDistanceWeigthedRandomWalks();
      //! Compute High-dimensional distribution
      void computeHighDimensionalDistribution();
      //! Initialize the point in the embedding
      void initializeEmbeddingPosition(int seed, double multiplier = .0001);
      //! Compute Low-dimensional distribution
      void computeLowDimensionalDistribution();
      //! Compute tSNE gradient
      void computeGradient(double exaggeration);
      //! Update the embedding
      void updateTheEmbedding(double mult = 1.);

    

    private:
      unsigned int _dimensionality;
      unsigned int _num_dps;
       scalar_type* _high_dimensional_data; //! High-dimensional data

      scalar_vector_type _embedding; //! embedding
      bool _initialized; //! Initialization flag

      //LANDMARKS
      std::vector<int> _idx_landmarks_to_dps;
      std::vector<int> _idx_dps_to_landmarks;


      scalar_vector_type _rw_probabilities;
      std::vector<int> _knns;

      scalar_vector_type _P; //! Conditional probalility distribution in the High-dimensional space
      scalar_vector_type _Q; //! Conditional probalility distribution in the Low-dimensional space
      scalar_type _normalization_Q; //! Normalization factor of Q - Z in the original paper

      // Gradient descent
      scalar_vector_type _gradient; //! Current gradient
      scalar_vector_type _previous_gradient; //! Previous gradient
      scalar_vector_type _gain; //! Gain


      Parameters _params;
      unsigned int _iteration;

      utils::AbstractLog* _logger;
      Statistics _statistics;

  
    };
  }
}
#endif 
