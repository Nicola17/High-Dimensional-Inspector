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

#ifndef HIERARCHICAL_SNE_H
#define HIERARCHICAL_SNE_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"
#include <map>
#include <unordered_map>
#include <random>
#include <unordered_set>
#include "hdi/data/flow_model.h"
#include "hdi/data/map_mem_eff.h"

namespace hdi{
  namespace dr{
    //! Hierarchical Stochastic Neighbor Embedding algorithm
    /*!
      Algorithm for the generation of a hierarchical representation of the data as presented in the Hierarchical Stochastic Neighbor Embedding paper
      \author Nicola Pezzotti
    */
    template <typename scalar = float, typename sparse_scalar_matrix = std::vector<hdi::data::MapMemEff<uint32_t, float> > >
    class HierarchicalSNE{
    public:
      typedef sparse_scalar_matrix sparse_scalar_matrix_type;
      typedef scalar scalar_type;
      typedef uint32_t unsigned_int_type;
      typedef int32_t int_type;
      typedef std::vector<scalar_type> scalar_vector_type; //! Vector of scalar_type
      typedef uint32_t data_handle_type;

    public:
      class Scale{
      public:
        unsigned_int_type size()const{return _landmark_to_original_data_idx.size();}

      public:
        scalar_type mimMemoryOccupation()const;

      public:
        std::vector<unsigned_int_type> _landmark_to_original_data_idx;
        std::vector<unsigned_int_type> _landmark_to_previous_scale_idx;
        sparse_scalar_matrix_type _transition_matrix;
        std::vector<scalar_type> _landmark_weight;

        ////////
        std::vector<int_type> _previous_scale_to_landmark_idx;//lower level
        sparse_scalar_matrix_type _area_of_influence; //lower level
      };
      typedef Scale scale_type;
      typedef std::vector<scale_type> hierarchy_type;

    public:
      //! Parameters used for the initialization of the algorithm
      class Parameters{
      public:
        Parameters();
        int _seed; //! Seed for random algorithms. If a negative value is provided, a time-based seed is used.
        unsigned_int_type _num_neighbors; //! Number of neighbors used in the KNN graph
        unsigned_int_type _aknn_num_trees; //! Number of trees in the Approximated KNN algorithm (See Approximated and User Steerable tSNE paper)
        unsigned_int_type _aknn_num_checks; //! Number of checks in the Approximated KNN algorithm (See Approximated and User Steerable tSNE paper)

        /////////////////// Landmark Selection ////////////////////////
        bool _monte_carlo_sampling; //! Select landmarks with a Markov Chain Monte Carlo sampling (MCMCS)
        unsigned_int_type _mcmcs_num_walks; //! Num walks per landmark in the MCMCS (beta in the paper)
        scalar_type _mcmcs_landmark_thresh; //! Threshold for landmark selection (beta threshold in the paper)
        unsigned_int_type _mcmcs_walk_length; //! MCMC walk length (theta in the paper)

        //if the MCMCS is not selected a random sampling will be used
        scalar_type _rs_reduction_factor_per_layer; //! Reduction factor per layer used in the random sampling (RS)
        unsigned_int_type _rs_outliers_removal_jumps; //! Random walks used in the RS to avoid outliers

        /////////////////// AoI computation ////////////////////////
        bool _out_of_core_computation; //! Memory preserving implementation
        unsigned_int_type _num_walks_per_landmark; //! Random walks used to compute the area of influence
        scalar_type _transition_matrix_prune_thresh; //! Min walks to be considered in the computation of the transition matrix
      };

      //!
      //! \brief Collector of Statistics about the last scale created
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
        scalar_type _init_knn_time; //! Time requested for the initialization of the KNN graph at the first scale
        scalar_type _init_probabilities_time; //! Time requested for the computation of transision probabilities
        scalar_type _init_fmc_time; //! Time requested for the computation of the FMC from the KNN graph

        scalar_type _mcmc_sampling_time; //! Time requested for the computation of the importance sampling
        scalar_type _landmarks_selection_time; //! Time requested for the selection of landmarks
        scalar_type _landmarks_selection_num_walks; //! Number of walks used for landmark selection

        scalar_type _aoi_time; //! Time requested for the computation of the area of influence
        scalar_type _fmc_time; //! Time requested for the computation of the FMC
        scalar_type _aoi_num_walks; //! Number of walks used for the computation of the area of influence

        scalar_type _aoi_sparsity; //! Sparsity of the Area of Influence
        scalar_type _fmc_sparsity; //! Sparsity of the Finite Markov Chain
        scalar_type _fmc_effective_sparsity; //! Sparsity of the Finite Markov Chain considering only effective neighbors (> 1%)
      };


      //!
      //! \brief Clustering of the HSNE hierarchy
      //! \note Allows for a clusterization of the landmarks on different scales. Algorithms for connecting data points to clusters are provided
      //!
      class ClusterTree{
      public:
        class Cluster{
        public:
          typedef std::unordered_set<unsigned_int_type> landmark_set_type;
          static const int_type NULL_LINK = -1;

        public:
          const int_type& id()const{return _id;}
          int_type& id(){return _id;}
          const int_type& parent_id()const{return _parent_id;}
          int_type& parent_id(){return _parent_id;}
          const landmark_set_type& landmarks()const{return _landmarks;}
          landmark_set_type& landmarks(){return _landmarks;}
          const std::string& notes()const{return _notes;}
          std::string& notes(){return _notes;}

        private:
          int_type _id;
          int_type _parent_id;
          landmark_set_type _landmarks;
          std::string _notes;
        };

      public:
        typedef Cluster cluster_type;
        typedef std::vector<cluster_type> cluster_vector_type;
        typedef std::vector<cluster_vector_type> cluster_tree_type;

      public:
        ClusterTree(unsigned_int_type num_scales=0):_cluster_tree(num_scales){}
        ClusterTree(const HierarchicalSNE& hsne):_cluster_tree(hsne.hierarchy().size()){}

        //! Get a free id at a given scale
        int_type getFreeClusterId(unsigned_int_type scale);
        //! Add a new cluster in the given scale
        void addCluster(unsigned_int_type scale, const cluster_type& cluster);
        //! Remove a cluster
        void removeCluster(unsigned_int_type scale, int_type cluster_id);
        //! Verify if the cluster is in the tree
        bool hasClusterId(unsigned_int_type scale, int_type cluster_id)const;
        //! Return the tree (only const)
        const cluster_tree_type& clusterTree()const{return _cluster_tree;}
        //! Return a cluster (only const)
        const cluster_type& cluster(unsigned_int_type scale_id, int_type cluster_id)const;

        //! Check if a cluster in the tree is consistent with the provided HSNE output
        bool checkCluterConsistency(const HierarchicalSNE& hsne, unsigned_int_type scale, int_type cluster_id);
        //! Check if tree is consistent with the provided HSNE output
        bool checkTreeConsistency(const HierarchicalSNE& hsne);

        //! Associate a point (pnt_id in the original data) with a cluster in the tree
        void computePointToClusterAssociation(const HierarchicalSNE& hsne, unsigned_int_type pnt_id, std::tuple<unsigned_int_type,int_type,scalar_type>& res);
        //! Associate all the data points with a cluster in the tree
        void computePointsToClusterAssociation(const HierarchicalSNE& hsne, std::vector<std::tuple<unsigned_int_type,int_type,scalar_type>>& res);

        //! Return the current log
        utils::AbstractLog* logger()const{return _logger;}
        //! Set a pointer to an existing log
        void setLogger(utils::AbstractLog* logger){_logger = logger;}

      private:
        cluster_tree_type _cluster_tree;
        utils::AbstractLog* _logger;
      };


    public:
      HierarchicalSNE();
      //! Set the dimensionality of the data
      void setDimensionality(int dimensionality){
        checkAndThrowLogic(!_initialized,"Class should be uninitialized to change the dimensionality"); 
        checkAndThrowLogic(dimensionality > 0,"Invalid dimensionality");
        _dimensionality = dimensionality;
      }
      //! Initialize the class with the current data-points
      void initialize(scalar_type* high_dimensional_data, unsigned_int_type num_dps, Parameters params = Parameters());
      //! Initialize the class with the current data-points from a given similarity matrix
      void initialize(const sparse_scalar_matrix_type& similarities, Parameters params = Parameters());
      //! Reset the internal state of the class but it keeps the inserted data-points
      void reset();
      //! Reset the class and remove all the data points
      void clear();
      //!Add a new scale
      bool addScale();
      
      //! Get the dimensionality of the data
      unsigned_int_type dimensionality(){return _dimensionality;}
      //! Get the high dimensional descriptor for a data point
      void getHighDimensionalDescriptor(scalar_vector_type& data_point, data_handle_type handle)const;

      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}

      //! Return statistics on the computation of the last scale
      const Statistics& statistics(){ return _statistics; }

      //! Return the whole hierarchy
      hierarchy_type& hierarchy(){return _hierarchy;}
      //! Return the whole hierarchy
      const hierarchy_type& hierarchy()const{return _hierarchy;}

      //! Return a scale
      scale_type& scale(unsigned_int_type scale_id){return _hierarchy[scale_id];}
      //! Return a scale
      const scale_type& scale(unsigned_int_type scale_id)const{return _hierarchy[scale_id];}

      //! Return the top scale
      scale_type& top_scale(){return _hierarchy[_hierarchy.size()-1];}
      //! Return the top scale
      const scale_type& top_scale()const{return _hierarchy[_hierarchy.size()-1];}



      //! Return the indexes of landmarks at "scale_id-1" that are influenced by the landmarks in idxes of scale "scale_id"
      void getInfluencedLandmarksInPreviousScale(unsigned_int_type scale_id, std::vector<unsigned_int_type>& idxes, std::map<unsigned_int_type,scalar_type>& neighbors)const;
      //! Return a sparse matrix that assigns to each data point the probability of being influenced by landmarks in the top scale. It can be used for interpolation like in hybri schemes
      //! \note a negative value of the scale parameter will force the algo to use the top one
      void getInterpolationWeights(sparse_scalar_matrix_type& influence, int scale = -1)const;
      //! Return a sparse matrix that assigns for a subset of the data points the probability of being influenced by landmarks in the top scale. It can be used for interpolation like in hybri schemes
      //! \note a negative value of the scale parameter will force the algo to use the top one
      void getInterpolationWeights(const std::vector<unsigned int>& data_points, sparse_scalar_matrix_type& influence, int scale = -1)const;
      //! Return the influence exercised on the data point by the landmarks in each scale
      void getInfluenceOnDataPoint(unsigned_int_type dp, std::vector<std::unordered_map<unsigned_int_type,scalar_type>>& influence, scalar_type thresh = 0, bool normalized = true)const;
      //! Return the influence exercised on the data point by a subset of landmarks in a given scale
      void getAreaOfInfluence(unsigned_int_type scale_id, const std::vector<unsigned_int_type>& set_selected_idxes, std::vector<scalar_type>& aoi)const;
      //! Return the influence exercised on the data point by a subset of landmarks in a given scale using a top-down approach
      void getAreaOfInfluenceTopDown(unsigned_int_type scale_id, const std::vector<unsigned_int_type>& set_selected_idxes, std::vector<scalar_type>& aoi)const;

      //! TODO
      void getStochasticLocationAtHigherScale(unsigned_int_type orig_scale, unsigned_int_type dest_scale, const std::vector<unsigned_int_type>& subset_orig_scale, sparse_scalar_matrix_type& location)const;
      //! TODO
      template <class Traits>
      void flowBetweenClusters(const std::vector<std::vector<std::unordered_set<unsigned_int_type>>>& clusters, data::FlowModel<Traits>& flow)const;

      template <class Functor>
      static void computeAoI(const HierarchicalSNE<scalar_type,sparse_scalar_matrix_type>& hsne, unsigned_int_type scale, const std::vector<unsigned_int_type>& idxes, Functor& functor);

    private:
      //! Compute the neighborhood graph
      void computeNeighborhoodGraph(scalar_vector_type& distance_based_probabilities, std::vector<int>& neighborhood_graph);
      //! Initialize the first scale
      void initializeFirstScale();
      //! Initialize the first scale
      void initializeFirstScale(const sparse_scalar_matrix_type& similarities);
      //! Compute a new scale
      bool addScaleImpl();
      //! Compute a new scale with a out-of-core
      bool addScaleOutOfCoreImpl();

      void selectLandmarks(const Scale& previous_scale,Scale& scale, unsigned_int_type& selected_landmarks);
      void selectLandmarksWithStationaryDistribution(const Scale& previous_scale,Scale& scale, unsigned_int_type& selected_landmarks);


      //! Return the seed for the random number generation
      unsigned_int_type seed()const;

    private:
      //!Compute a random walk using a transition matrix and return the end point after a max_length steps -> used for landmark selection
      inline unsigned_int_type randomWalk(unsigned_int_type starting_point, unsigned_int_type max_length, const sparse_scalar_matrix_type& transition_matrix, std::uniform_real_distribution<double>& distribution, std::default_random_engine& generator);
      //!Compute a random walk using a transition matrix that stops at a provided stopping point -> used for landmark similarity computation
      inline int randomWalk(unsigned_int_type starting_point, const std::vector<int>& stopping_points, unsigned_int_type max_length, const sparse_scalar_matrix_type& transition_matrix, std::uniform_real_distribution<double>& distribution, std::default_random_engine& generator);

    private:
      hierarchy_type _hierarchy;

    private:
      unsigned_int_type _dimensionality;
      unsigned_int_type _num_dps;
       scalar_type* _high_dimensional_data; //! High-dimensional data

      bool _initialized; //! Initialization flag
      bool _verbose;

      Parameters _params;

      utils::AbstractLog* _logger;
      Statistics _statistics;
    };

///////////////   AOI STATS   ////////////////////////////


    template <typename scalar_type, typename sparse_scalar_matrix_type>
    template <class Functor>
    void HierarchicalSNE<scalar_type,sparse_scalar_matrix_type>::computeAoI(const HierarchicalSNE<scalar_type,sparse_scalar_matrix_type>& hsne, unsigned_int_type scale, const std::vector<unsigned_int_type>& idxes, Functor& functor){
      std::vector<scalar_type> aoi;
      hsne.getAreaOfInfluence(scale,idxes,aoi);
      for(int i = 0; i < aoi.size(); ++i){
        functor(i,aoi[i]);
      }
    }


///////////////   IO   ///////////////////////////////////
    namespace IO{
      template <typename hsne_type, class output_stream_type>
      void saveHSNE(const hsne_type& hsne, output_stream_type& stream, utils::AbstractLog* log = nullptr);

      template <typename hsne_type, class input_stream_type>
      void loadHSNE(hsne_type& hsne, input_stream_type& stream, utils::AbstractLog* log = nullptr);
    }

//////////////////////////////////////////////////////////
#if 0
    template <class scalar_type>
    template <class Traits>
    void HierarchicalSNE<scalar_type>::flowBetweenClusters(const std::vector<std::vector<std::unordered_set<unsigned_int_type>>>& clusters, data::FlowModel<Traits>& flow)const{
      typedef data::FlowModel<Traits> flow_model_type;
      typedef typename flow_model_type::flow_type flow_type;
      typedef typename flow_model_type::node_type node_type;

      const unsigned_int_type node_per_level = 10000;
      unsigned_int_type link_id = 0;
      flow = flow_model_type();

      unsigned_int_type scale = 0;
      for(scale = 0; scale < clusters.size(); ++scale){
        if(clusters[scale].size()!=0){
          break;
        }
        flow.addNode(node_type(node_per_level*scale));

        if(scale > 0){
          flow.addFlow(flow_type(link_id,node_per_level*(scale-1),node_per_level*scale,_num_dps));
          ++link_id;
        }
      }

      for(; scale < clusters.size(); ++scale){
        flow.addNode(node_type(node_per_level*scale));//I add the node even if I don't have the flow
        for(int c = 0; c < clusters[scale].size(); ++c){
          flow.addNode(node_type(node_per_level*scale+c+1));
        }
        if(scale == 0){
          continue;
        }

        //weight of the clusters -> first one reserved for unclusterized items
        std::vector<std::vector<scalar_type> > clusters_weight(clusters[scale-1].size()+1,std::vector<scalar_type>(clusters[scale].size()+1,0));

        //landmarks at previous scale
        for(int pl = 0; pl < _hierarchy[scale]._area_of_influence.size(); ++pl){
          //weight of the landmark at the previous scale (or dp)
          scalar_type pl_weight(_hierarchy[scale-1]._landmark_weight[pl]);
          unsigned_int_type previuous_scale_cluster_id = 0;
          //see how the dp is distributed among the clusters
          for(int c = 0; c < clusters[scale-1].size(); ++c){
            if(clusters[scale-1][c].find(pl)!=clusters[scale-1][c].end()){
              previuous_scale_cluster_id = c+1;
              break;
            }
          }

          for(auto cp: _hierarchy[scale]._area_of_influence[pl]){
            unsigned_int_type this_scale_cluster_id = 0;
            //see how the dp is distributed among the clusters
            for(int c = 0; c < clusters[scale].size(); ++c){
              if(clusters[scale][c].find(cp.first)!=clusters[scale][c].end()){
                this_scale_cluster_id = c+1;
                break;
              }
            }
            clusters_weight[previuous_scale_cluster_id][this_scale_cluster_id] += cp.second * pl_weight;
          }
        }

        for(int c = 0; c < clusters_weight.size(); ++c){
          for(int d = 0; d < clusters_weight[c].size(); ++d){
            flow.addFlow(flow_type(link_id,node_per_level*(scale-1)+c,node_per_level*(scale)+d,clusters_weight[c][d]));
            ++link_id;
          }
        }

      }
/*
      for(int i = 0; i < _num_dps; ++i){
        assert(subset_orig_scale[i] < _hierarchy[orig_scale+1]._area_of_influence.size());
        closeness[i] = _hierarchy[orig_scale+1]._area_of_influence[subset_orig_scale[i]];

        for(int s = orig_scale+2; s <= dest_scale; ++s){
          typename sparse_scalar_matrix_type::value_type temp_link;
          for(auto l: closeness[i]){
            for(auto new_l: _hierarchy[s]._area_of_influence[l.first]){
              temp_link[new_l.first] += l.second * new_l.second;
            }
          }
          closeness[i] = temp_link;
        }

*/
      }
#endif

/*
      unsigned_int_type scale  = 0;
      for(scale = 0; scale < _multiscale_analysis.size(); ++scale){
        if(_multiscale_analysis[scale].size()!=0){
          break;
        }
        flow.addNode(node_type(node_per_level*(scale+1),"NonViz",qRgb(20,20,20)));
        flow.addFlow(flow_type(link_id,node_per_level*(scale),node_per_level*(scale+1),1,qRgb(20,20,20)));
        ++link_id;
      }
      for(; scale < _multiscale_analysis.size(); ++scale){

        flow.addNode(node_type(node_per_level*(scale+1),"NonClust",qRgb(50,20,20)));
        flow.addFlow(flow_type(link_id,node_per_level*(scale),node_per_level*(scale+1),1,qRgb(20,20,20)));
        ++link_id;
      }
    }
*/

  }
}
#endif
