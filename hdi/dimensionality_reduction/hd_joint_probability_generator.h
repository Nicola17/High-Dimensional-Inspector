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

#ifndef HD_JOINT_PROBABILITY_GENERATOR_H
#define HD_JOINT_PROBABILITY_GENERATOR_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"
#include <map>
#include <unordered_map>
#include <random>
#include <unordered_set>
#include "hdi/data/map_mem_eff.h"

namespace hdi{
  namespace dr{
    //! Generator for a joint probability distribution that describes similarities in the high dimensional data
    /*!
      Generator for a joint probability distribution that describes similarities in the high dimensional data.
      For further details refer to the Barnes Hut SNE paper.
      \author Nicola Pezzotti
      \warning Due to numeric limits, the output matrix is not normalized. In order to have a joint-probability distribution each cell must be divided by 2*num_dps
    */
    template <typename scalar = float, typename sparse_scalar_matrix = std::vector<hdi::data::MapMemEff<unsigned int, float> >>
    class HDJointProbabilityGenerator{
    public:
      typedef scalar scalar_type;
      typedef sparse_scalar_matrix sparse_scalar_matrix_type;
      typedef std::vector<scalar_type> scalar_vector_type; //! Vector of scalar_type

    public:
      //! Parameters used for the initialization of the algorithm
      class Parameters{
      public:
        Parameters();
      public:
        scalar_type _perplexity;      //! Perplexity value in evert distribution.
        int     _perplexity_multiplier; //! Multiplied by the perplexity gives the number of nearest neighbors used
        int     _num_trees;       //! Number of trees used int the AKNN
        int     _num_checks;      //! Number of checks used int the AKNN
      };

      //!
      //! \brief Collector of Statistics on the computation performed
      //! \note All time are in seconds with millisecond resolution
      //!
      class Statistics{
      public:
        Statistics();
        //! Reset the statistics
        void reset();
        //! Log the current statistics to logger
        void log(utils::AbstractLog* logger)const;

      public:
        scalar_type _total_time;
        scalar_type _trees_construction_time;
        scalar_type _aknn_time;
        scalar_type _distribution_time;
      };

    public:
      HDJointProbabilityGenerator();

      void computeJointProbabilityDistribution(/*const*/ scalar_type* high_dimensional_data, unsigned int num_dim, unsigned int num_dps, sparse_scalar_matrix& distribution, Parameters params = Parameters());
      void computeProbabilityDistributions(/*const*/ scalar_type* high_dimensional_data, unsigned int num_dim, unsigned int num_dps, sparse_scalar_matrix& distribution, Parameters params = Parameters());
      void computeProbabilityDistributions(/*const*/ scalar_type* high_dimensional_data, unsigned int num_dim, unsigned int num_dps, std::vector<scalar_type>& probabilities, std::vector<int>& indices, Parameters params = Parameters());
      void computeProbabilityDistributionsFromDistanceMatrix(const std::vector<scalar_type>& squared_distance_matrix, unsigned int num_dps, sparse_scalar_matrix& distribution, Parameters params = Parameters());


      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}

      //! Return statistics on the computation of the last scale
      const Statistics& statistics(){ return _statistics; }

    private:
      //! Compute the euclidean distances between points
      void computeHighDimensionalDistances(/*const*/ scalar_type* high_dimensional_data, unsigned int num_dim, unsigned int num_dps, std::vector<scalar_type>& dsitances, std::vector<int>& indices, Parameters& params);
      //! Compute a gaussian distribution for each data-point
      void computeGaussianDistributions(const std::vector<scalar_type>& dsitances, const std::vector<int>& indices, sparse_scalar_matrix& matrix, Parameters& params);
      //! Compute a gaussian distribution for each data-point
      void computeGaussianDistributions(const std::vector<scalar_type>& dsitances, const std::vector<int>& indices, std::vector<scalar_type>& probabilities, Parameters& params);
      //! Create joint distribution
      void symmetrize(sparse_scalar_matrix& matrix);

    private:
      utils::AbstractLog* _logger;
      Statistics      _statistics;

    };


  }
}
#endif
