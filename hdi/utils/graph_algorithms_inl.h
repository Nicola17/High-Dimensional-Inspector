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

#ifndef GRAPH_ALGORITHMS_INL
#define GRAPH_ALGORITHMS_INL

#include "hdi/utils/graph_algorithms.h"
#include <limits>
#include <queue>
#include <map>
#include <unordered_map>
#include <cmath>
#include "hdi/utils/math_utils.h"

namespace hdi{
  namespace utils{

    template <class map_type>
    void computeConnectedComponents(const std::vector<map_type>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, typename map_type::mapped_type thresh){
      vertex_to_cluster.clear();
      cluster_size.clear();
      cluster_to_vertex.clear();

      const unsigned int invalid = std::numeric_limits<unsigned int>::max();
      vertex_to_cluster.resize(weighted_graph.size(),invalid);
      const unsigned int n = weighted_graph.size();
      unsigned int cluster_idx = 0;

      for(unsigned int i = 0; i < n; ++i){
        if(vertex_to_cluster[i] == invalid){
          std::queue<unsigned int> queue;
          queue.push(i);
          vertex_to_cluster[i] = cluster_idx;
          cluster_to_vertex.push_back(i);
          unsigned int c_size = 0;
          while(!queue.empty()){
            ++c_size;
            auto idx = queue.front();
            queue.pop();
            for(auto& v: weighted_graph[idx]){
              if(vertex_to_cluster[v.first] != invalid){
                continue;
              }
              if(v.second > thresh){
                vertex_to_cluster[v.first] = cluster_idx;
                queue.push(v.first);
              }
            }
          }
          cluster_size.push_back(c_size);
          ++cluster_idx;
        }
      }
    }


    template <class map_type>
    void extractSubGraph(const std::vector<map_type>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<map_type>& new_transition_matrix, std::vector<unsigned int>& new_idxes, typename map_type::mapped_type thresh){
      new_transition_matrix.clear();
      new_idxes.clear();
      std::map<unsigned int,unsigned int> map_selected_idxes;
      std::map<unsigned int,unsigned int> map_non_selected_idxes;
      //The selected rows must be taken completely
      for(auto id: selected_idxes){
        map_selected_idxes[id] = new_idxes.size();
        new_idxes.push_back(id);
      }

      //Vertices that are connected to a selected vertex
      for(auto& e: map_selected_idxes){
        for(auto& row_elem: orig_transition_matrix[e.first]){
          if(row_elem.second > thresh){
            if(map_selected_idxes.find(row_elem.first) == map_selected_idxes.end() &&
               map_non_selected_idxes.find(row_elem.first) == map_non_selected_idxes.end()){
              map_non_selected_idxes[row_elem.first] = new_idxes.size();
              new_idxes.push_back(row_elem.first);
            }
          }
        }
      }

      //Now that I have the maps, I generate the new transition matrix
      new_transition_matrix.resize(map_non_selected_idxes.size() + map_selected_idxes.size());
      for(auto e: map_selected_idxes){
        for(auto row_elem: orig_transition_matrix[e.first]){
          if(map_selected_idxes.find(row_elem.first) != map_selected_idxes.end()){
            new_transition_matrix[e.second][map_selected_idxes[row_elem.first]] = row_elem.second;
          }else if(map_non_selected_idxes.find(row_elem.first) != map_non_selected_idxes.end()){
            new_transition_matrix[e.second][map_non_selected_idxes[row_elem.first]] = row_elem.second;
          }
        }
      }
      for(auto e: map_non_selected_idxes){
        for(auto row_elem: orig_transition_matrix[e.first]){
          if(map_selected_idxes.find(row_elem.first) != map_selected_idxes.end()){
            new_transition_matrix[e.second][map_selected_idxes[row_elem.first]] = row_elem.second;
          }else if(map_non_selected_idxes.find(row_elem.first) != map_non_selected_idxes.end()){
            new_transition_matrix[e.second][map_non_selected_idxes[row_elem.first]] = row_elem.second;
          }
        }
      }

      //Finally, the new transition matrix must be normalized
      double sum = 0;
      for(auto& row: new_transition_matrix){
        for(auto& elem: row){
          sum += elem.second;
        }
      }
      for(auto& row: new_transition_matrix){
        for(auto& elem: row){
          elem.second = new_transition_matrix.size() * elem.second/sum;
        }
      }
    }

    template <class sparse_scalar_matrix_type>
    void removeEdgesToUnselectedVertices(sparse_scalar_matrix_type& adjacency_matrix, const std::vector<unsigned int>& valid_vertices){

      std::unordered_map<unsigned int,unsigned int> valid_set;
      for(int i = 0; i < valid_vertices.size(); ++i){
        valid_set[valid_vertices[i]] = i;
      }

      sparse_scalar_matrix_type new_map(adjacency_matrix.size());
      for(int i = 0; i < adjacency_matrix.size(); ++i){
        for (auto& elem: adjacency_matrix[i]){
          auto search_iter = valid_set.find(elem.first);
          if(search_iter != valid_set.end()){
            new_map[i][search_iter->second] = elem.second;
          }
        }
      }
      adjacency_matrix = new_map;

    }

    //! expand the matrix (see Markov Clustering)
    template <class sparse_scalar_matrix_type>
    void expand(const sparse_scalar_matrix_type& src, sparse_scalar_matrix_type& dst){
      typedef typename sparse_scalar_matrix_type::value_type::mapped_type scalar_type;
      dst.clear();
      dst.resize(src.size());

      int smoothing_iters = 2;

      for(int j = 0; j < src.size(); ++j){
        std::vector<scalar_type> a(src.size(),0);
        a[j] = 1;
        std::vector<scalar_type> c(src.size(),0);
        for(int i = 0; i < smoothing_iters; ++i){
          multiply(a,src,c);
          a = c;
        }
        for(int i = 0; i < c.size(); ++i){
          if(c[i]!=0 && i!=j){
            dst[j][i] = c[i];
          }
        }
      }

      //normalize
      for(int j = 0; j < dst.size(); ++j){
        double sum = 0;
        for(auto& it: dst[j]){
          sum += it.second;
        }
        for(auto& it: dst[j]){
          it.second /= sum;
        }
      }


      /*
      dst.clear();
      dst.resize(src.size());

      for(int j = 0; j < src.size(); ++j){
        for(auto& it_outer: src[j]){
          if(it_outer.first == j){
            continue;
          }
          double v = 0;
          for(auto& it_inner: src[j]){
            auto r = src[it_inner.first].find(it_outer.first);
            if(r!=src[it_inner.first].end()){
              v += it_inner.second * r->second;
            }
          }
          dst[j][it_outer.first] = std::pow(v,1./10.);
        }
      }

      //normalize
      for(int j = 0; j < dst.size(); ++j){
        double sum = 0;
        for(auto& it: dst[j]){
          sum += it.second;
        }
        for(auto& it: dst[j]){
          it.second /= sum;
        }
      }
      */
    }





  }
}

#endif
