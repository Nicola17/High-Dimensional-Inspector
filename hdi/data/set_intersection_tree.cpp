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


#include "hdi/data/set_intersection_tree.h"
#include <numeric>


namespace hdi{
  namespace data{

    void SetIntersectionTree::searchJaccardNN(const Roaring& pnt, double& distance, uint32_t& idx)const{
      distance = 2;
      searchJaccardNN(*_root, pnt, distance, idx);
    }
    void SetIntersectionTree::searchJaccardNN(Node& node, const Roaring& pnt, double& distance, uint32_t& idx)const{
      const Roaring& pivot = _sets[_idxes[node._l]];

      double jaccard = 1-pnt.jaccard_index(pivot);
      if(jaccard < distance){
        distance = jaccard;
        idx = _idxes[node._l];
      }
      if(pnt.and_cardinality(node._union_int) && node._ptr_int){
        searchJaccardNN(*node._ptr_int,pnt,distance,idx);
      }
      if(pnt.and_cardinality(node._union_no_int) && node._ptr_no_int){
        searchJaccardNN(*node._ptr_no_int,pnt,distance,idx);
      }
    }

    void SetIntersectionTree::searchJaccardKNN(const Roaring& pnt, uint32_t K, std::vector<double>& distances, std::vector<uint32_t>& idxes)const{
      ++_n_queries;
      std::priority_queue<std::pair<double,uint32_t>> res;
      uint32_t n_visit = 0;
      searchJaccardKNN(*_root, pnt, K, res, 1, n_visit);

      distances.resize(K);
      idxes.resize(K);

      assert(res.size() == K);
      for(int i = K-1; i >= 0; --i){
        auto top = res.top();
        res.pop();
        distances[i] = top.first;
        idxes[i] = top.second;
      }
    }
    void SetIntersectionTree::searchJaccardKNN(Node& node, const Roaring& pnt, uint32_t K, std::priority_queue<std::pair<double,uint32_t>>& res, uint32_t depth, uint32_t& n_visit )const{
      ++_n_node_visited;
      const Roaring& pivot = _sets[_idxes[node._l]];
      uint32_t pnt_cardinality   = pnt.cardinality();
      uint32_t pivot_cardinality = pivot.cardinality();

      if(n_visit > 10000){
        return;
      }

      uint32_t intrsc_pivot = pnt.and_cardinality(pivot);
      uint32_t union_pivot  = pnt.or_cardinality(pivot);
      double jaccard_distance = 1-double(intrsc_pivot)/union_pivot;
      auto res_size = res.size();

      if(res_size < K){
        res.push(std::make_pair(jaccard_distance,_idxes[node._l]));
        ++res_size;
      }else{
        //visit for AKNN
        ++n_visit;
        if(res.top().first > jaccard_distance){
          res.pop();
          res.push(std::make_pair(jaccard_distance,_idxes[node._l]));
        }
      }

      uint32_t intrsc_union_int = pnt.and_cardinality(node._union_int);
      if(node._ptr_int && intrsc_union_int){
        if(res_size >= K){
          double candidate = res.top().first;
        //////
          {
            const double min_distance_in_sub_tree = 1-double(intrsc_union_int)/pnt_cardinality;
            if(candidate < min_distance_in_sub_tree){
              return;
            }
          }
        /////
          if(intrsc_union_int < pivot_cardinality+1){
            const double lower_bound = 1-double(intrsc_union_int)/(pnt_cardinality+pivot_cardinality+1-2*intrsc_union_int);
            if(candidate < lower_bound){
              return;
            }
          }

        }
        searchJaccardKNN(*node._ptr_int,pnt,K,res,depth+1,n_visit);
      }
      if(node._ptr_no_int){
        searchJaccardKNN(*node._ptr_no_int,pnt,K,res,depth,n_visit);
      }
    }

    void SetIntersectionTree::getNonDuplicatedPoints(std::vector<uint32_t>& idxes, std::vector<uint32_t>& repetitions)const{
      idxes.clear();
      repetitions.clear();
      idxes.reserve(_N-_tot_num_duplicates);
      repetitions.reserve(_N-_tot_num_duplicates);
      getNonDuplicatedPoints(_root,idxes,repetitions);
    }
    void SetIntersectionTree::getNonDuplicatedPoints(const Node* node, std::vector<uint32_t>& idxes, std::vector<uint32_t>& repetitions)const{
      idxes.push_back(_idxes[node->_l]);
      repetitions.push_back(node->_num_duplicates+1);

      if(node->_ptr_no_int){
        getNonDuplicatedPoints(node->_ptr_no_int,idxes,repetitions);
      }
      if(node->_ptr_int){
        getNonDuplicatedPoints(node->_ptr_int,idxes,repetitions);
      }
      if(!node->_ptr_no_int && !node->_ptr_int){
        for(int i = node->_l+1; i < node->_r; ++i){
          idxes.push_back(_idxes[i]);
          repetitions.push_back(1);
        }
      }
    }

    void SetIntersectionTree::build(const std::vector<Roaring>& sets){
      _N = sets.size();
      _sets = sets.data();

      _idxes.resize(sets.size());
      _distances.resize(sets.size());

      std::iota(_idxes.begin(),_idxes.end(),0);
      std::random_shuffle (_idxes.begin(), _idxes.end());

      _num_nodes = 0;
      _max_depth = 0;
      _tot_num_duplicates = 0;
      _max_in_bucket = 0;

      _root = new Node();
      Roaring unused;
      build(*_root,0,sets.size(),unused,1);
    }

    void SetIntersectionTree::build(Node& node, uint32_t l, uint32_t r, const Roaring& shared_parent, uint32_t depth){
      //uint32_t pivot = highestCardinality(l,r);
      uint32_t pivot = lowestCardinality(l,r);
      //uint32_t pivot = l;
      std::swap(_idxes[l],_idxes[pivot]);

      const Roaring& set_pivot = _sets[_idxes[l]];
      uint32_t card_pivot    = set_pivot.cardinality();


      ++_num_nodes;
      _max_depth = std::max(_max_depth,depth);

      node._l = l;
      node._r = r;
      node._union_int = set_pivot;

      node._num_duplicates = 0;
      node._num_int = 0;
      node._num_no_int = 0;

      node._shared = set_pivot;

      if(r-l < _max_bucket_size){
        return;
      }

      uint32_t swap_no_int  = r-1;
      uint32_t swap_identity  = l+1;

      for(uint32_t i = l+1; i <= swap_no_int; ++i){
        const Roaring& other_set = _sets[_idxes[i]];
        const Roaring set_intersection(set_pivot&other_set);

        if((set_intersection-shared_parent).cardinality()!=0){
          //intersection (excluding the parent set)
          node._union_int   |= other_set;
          node._shared    &= other_set; //TODO: can skip this if cardinality == 1
          ++node._num_int;
        }else{
          //no intersection
          node._union_no_int  |= other_set;
          ++node._num_no_int;
          std::swap(_idxes[i],_idxes[swap_no_int]);
          --i;
          --swap_no_int;
        }
        if(set_intersection.cardinality() == card_pivot && card_pivot == other_set.cardinality()){
          ++node._num_duplicates;
          std::swap(_idxes[i],_idxes[swap_identity]);
          ++swap_identity;
        }
      }
      ++swap_no_int;
      _tot_num_duplicates += node._num_duplicates;

      uint32_t shared_parent_cardinality = shared_parent.cardinality();
      uint32_t shared_cardinality = node._shared.cardinality();
      uint32_t union_int_cardinality = node._union_int.cardinality();
      uint32_t union_no_int_cardinality = node._union_no_int.cardinality();

      /////////////////////////////////
      //CHECKS
      /*
      for(int i = l; i < swap_identity; ++i){
        assert((_sets[_idxes[i]]&_sets[_idxes[l]]).cardinality()==_sets[_idxes[i]].cardinality() &&  _sets[_idxes[i]].cardinality() == _sets[_idxes[l]].cardinality());
      }
      for(int i = swap_identity; i < swap_no_int; ++i){
        assert((_sets[_idxes[i]]&_sets[_idxes[l]]).cardinality()!=0);
      }
      for(int i = swap_no_int; i < r; ++i){
        assert((_sets[_idxes[i]]&_sets[_idxes[l]]).cardinality()==0);
      }
      */
      /////////////////////////////////

      //
      if(node._num_int){
        node._ptr_int = new Node();
        build(*(node._ptr_int),swap_identity,swap_no_int,node._shared,depth+1);
      }
      if(node._num_no_int){
        node._ptr_no_int = new Node();
        build(*(node._ptr_no_int),swap_no_int,r,shared_parent,depth+1);
      }
    }

    uint32_t SetIntersectionTree::highestCardinality(uint32_t l, uint32_t r){
      uint32_t max_card = 0;
      uint32_t res = 0;

      for(int i = l; i < r; ++i){
        uint32_t c = _sets[_idxes[i]].cardinality();
        if(max_card < c){
          max_card = c;
          res = i;
        }
      }
      return res;
    }
    uint32_t SetIntersectionTree::lowestCardinality(uint32_t l, uint32_t r){
      uint32_t min_card = std::numeric_limits<uint32_t>::max();
      uint32_t max_centrality = 0;
      uint32_t res = 0;

      for(int i = l; i < r; ++i){
        uint32_t c = _sets[_idxes[i]].cardinality();

        if(c == 1){
          return i;
        }
        if(min_card > c){
          min_card = c;
          res = i;
        }
      }
      return res;
    }

    double SetIntersectionTree::jaccardDistance(const Roaring& a, const Roaring& b){
      uint32_t n1 = a.cardinality();
      uint32_t n2 = b.cardinality();
      uint32_t intersection = (a&b).cardinality();
      return 1 - double(intersection)/(n1+n2-intersection);
    }

  }
}
