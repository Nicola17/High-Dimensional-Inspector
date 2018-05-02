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

#ifndef GRAPH_ALGORITHMS_H
#define GRAPH_ALGORITHMS_H

#include <vector>

namespace hdi{
  namespace utils{

    template <class map_type>
    void computeConnectedComponents(const std::vector<map_type>& weighted_graph, std::vector<unsigned int>& vertex_to_cluster, std::vector<unsigned int>& cluster_to_vertex, std::vector<unsigned int>& cluster_size, typename map_type::mapped_type thresh = 0);

    //! Extract a subgraph with the selected indexes and the vertices connected to them by an edge with a weight higher then thresh
    template <class map_type>
    void extractSubGraph(const std::vector<map_type>& orig_transition_matrix, const std::vector<unsigned int>& selected_idxes, std::vector<map_type>& new_transition_matrix, std::vector<unsigned int>& new_idxes, typename map_type::mapped_type thresh = 0);

    //! Remove the edges to the vertices that  are not included in valid vertices
    template <class sparse_scalar_matrix_type>
    void removeEdgesToUnselectedVertices(sparse_scalar_matrix_type& adjacency_matrix, const std::vector<unsigned int>& valid_vertices);

    //! expand the matrix (see Markov Clustering)
    template <class sparse_scalar_matrix_type>
    void expand(const sparse_scalar_matrix_type& src, sparse_scalar_matrix_type& dest);
  }
}

#endif
