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

#ifndef SET_INTERSECTION_TREE_H
#define SET_INTERSECTION_TREE_H

#include "roaring/roaring.hh"
#include <queue>

namespace hdi{
  namespace data{

    class SetIntersectionTree{
      class Node{
      public:
        Node():_ptr_int(nullptr),_ptr_no_int(nullptr){}
        ~Node(){
          if(_ptr_int!=nullptr)     delete(_ptr_int);
          if(_ptr_no_int!=nullptr)  delete(_ptr_no_int);
        }

        uint32_t _l;
        uint32_t _r;

        uint32_t _num_duplicates;
        uint32_t _num_int;
        uint32_t _num_no_int;

        Node* _ptr_int;
        Node* _ptr_no_int;

        Roaring _union_int;
        Roaring _union_no_int;
        Roaring _shared;
      };

    public:

      SetIntersectionTree():_max_bucket_size(50),_n_queries(0),_n_node_visited(0),_n_node_traversed(0){}
      void build(const std::vector<Roaring>& sets);

      void getNonDuplicatedPoints(std::vector<uint32_t>& idxes, std::vector<uint32_t>& repetitions)const;

      void searchJaccardKNN(const Roaring& pnt, uint32_t K, std::vector<double>& distances, std::vector<uint32_t>& idxes)const;
      void searchJaccardNN(const Roaring& pnt, double& distance, uint32_t& idxes)const;

      uint32_t max_depth()const{return _max_depth;}
      uint32_t num_nodes()const{return _num_nodes;}
      uint32_t tot_num_duplicates()const{return _tot_num_duplicates;}
      uint32_t max_in_bucket()const{return _max_in_bucket;}
      double   nodes_per_query()const{return double(_n_node_visited)/_n_queries;}
      double   nodes_traversed_per_query()const{return double(_n_node_traversed)/_n_queries;}

    private:
      double jaccardDistance(const Roaring& a, const Roaring& b);
      uint32_t highestCardinality(uint32_t l, uint32_t r);
      uint32_t lowestCardinality(uint32_t l, uint32_t r);


      void build(Node& node, uint32_t l, uint32_t r, const Roaring& shared_parent, uint32_t depth);
      void getNonDuplicatedPoints(const Node* node, std::vector<uint32_t>& idxes, std::vector<uint32_t>& repetitions)const;
      void searchJaccardNN(Node& node, const Roaring& pnt, double& distance, uint32_t& idxes)const;
      void searchJaccardKNN(Node& node, const Roaring& pnt, uint32_t K, std::priority_queue<std::pair<double,uint32_t>>& res, uint32_t depth, uint32_t& n_visit)const;

    private:
      uint32_t        _N;
      std::vector<uint32_t> _idxes;
      std::vector<double>   _distances;
      const Roaring*    _sets;
      Node*         _root;
      uint32_t        _num_nodes;
      uint32_t        _max_depth;
      uint32_t        _tot_num_duplicates;
      uint32_t        _max_in_bucket;

      uint32_t        _max_bucket_size;


      mutable uint32_t        _n_queries;
      mutable uint32_t        _n_node_visited;
      mutable uint32_t        _n_node_traversed;
    };

  }
}

#endif
