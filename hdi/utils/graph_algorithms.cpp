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

#include "graph_algorithms_inl.h"
#include <map>
#include <unordered_map>
#include "hdi/data/map_mem_eff.h"


namespace hdi{
  namespace utils{
    
    template void computeConnectedComponents(const std::vector<std::map<unsigned int,double>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,double>::mapped_type thresh);
    template void computeConnectedComponents(const std::vector<std::map<unsigned int,float>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,float>::mapped_type thresh);
    template void computeConnectedComponents(const std::vector<std::unordered_map<unsigned int,double>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,double>::mapped_type thresh);
    template void computeConnectedComponents(const std::vector<std::unordered_map<unsigned int,float>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,float>::mapped_type thresh);
    template void computeConnectedComponents(const std::vector<hdi::data::MapMemEff<unsigned int,double>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,double>::mapped_type thresh);
    template void computeConnectedComponents(const std::vector<hdi::data::MapMemEff<unsigned int,float>>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, std::map<unsigned int,float>::mapped_type thresh);

    template void extractSubGraph(const std::vector<std::map<unsigned int,double>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<std::map<unsigned int,double>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, double thresh);
    template void extractSubGraph(const std::vector<std::map<unsigned int,float>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<std::map<unsigned int,float>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, float thresh);
    template void extractSubGraph(const std::vector<std::unordered_map<unsigned int,double>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<std::unordered_map<unsigned int,double>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, double thresh);
    template void extractSubGraph(const std::vector<std::unordered_map<unsigned int,float>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<std::unordered_map<unsigned int,float>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, float thresh);
    template void extractSubGraph(const std::vector<hdi::data::MapMemEff<unsigned int,double>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<hdi::data::MapMemEff<unsigned int,double>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, double thresh);
    template void extractSubGraph(const std::vector<hdi::data::MapMemEff<unsigned int,float>>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<hdi::data::MapMemEff<unsigned int,float>>& new_transition_matrix, std::vector<unsigned int>& new_idxes, float thresh);

    template void removeEdgesToUnselectedVertices(std::vector<std::map<unsigned int, float>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);
    template void removeEdgesToUnselectedVertices(std::vector<std::map<unsigned int, double>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);
    template void removeEdgesToUnselectedVertices(std::vector<std::unordered_map<unsigned int, float>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);
    template void removeEdgesToUnselectedVertices(std::vector<std::unordered_map<unsigned int, double>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);
    template void removeEdgesToUnselectedVertices(std::vector<hdi::data::MapMemEff<unsigned int, float>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);
    template void removeEdgesToUnselectedVertices(std::vector<hdi::data::MapMemEff<unsigned int, double>>& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);

    template void expand(const std::vector<std::map<unsigned int, float>>& src, std::vector<std::map<unsigned int, float>>& dst);
    template void expand(const std::vector<std::map<unsigned int, double>>& src, std::vector<std::map<unsigned int, double>>& dst);
    template void expand(const std::vector<std::unordered_map<unsigned int, float>>& src, std::vector<std::unordered_map<unsigned int, float>>& dst);
    template void expand(const std::vector<std::unordered_map<unsigned int, double>>& src, std::vector<std::unordered_map<unsigned int, double>>& dst);
    template void expand(const std::vector<hdi::data::MapMemEff<unsigned int, float>>& src, std::vector<hdi::data::MapMemEff<unsigned int, float>>& dst);
    template void expand(const std::vector<hdi::data::MapMemEff<unsigned int, double>>& src, std::vector<hdi::data::MapMemEff<unsigned int, double>>& dst);

  }
}
