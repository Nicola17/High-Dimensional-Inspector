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

#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>
#include <map>
#include <unordered_map>
#include "hdi/data/map_mem_eff.h"
#include "hdi/utils/assert_by_exception.h"

namespace hdi{
  namespace data{

    template <typename Key, typename T, typename Map>
    class MapHelpers{
    public:
      //! Reduces the memory occupation to the minimum
      static void shrinkToFit(Map& map){throw std::logic_error("MapHelpers::shrinkToFit: function not implemented");}
      //! Returns the exact memory in Bytes occupied by the map. -1 if it cannot be computed
      static double memoryOccupation(Map& map){throw std::logic_error("MapHelpers::shrinkToFit: function not implemented"); return -1;}
      //! Initializes the map with a set of elements ordered by key. No check is performed internally!
      template <typename It>
      static void initialize(Map& map, It begin, It end, T thresh = 0){throw std::logic_error("MapHelpers::shrinkToFit: function not implemented");}
      //! Invert a sparse matrix implemented with a vector of maps
      static void invert(const std::vector<Map>& matrix, std::vector<Map>& inverse){throw std::logic_error("MapHelpers::invert: function not implemented");}
    };



    template <typename Key, typename T>
    class MapHelpers<Key,T,std::map<Key,T>>{
    public:
      static void shrinkToFit(std::map<Key,T>& map){}
      static double memoryOccupation(std::map<Key,T>& map){return -1;}
      template <typename It>
      static void initialize(std::map<Key,T>& map, It begin, It end, T thresh = 0){
        auto it = begin;
        while(it != end){
          if(it->second > thresh){
            map[it->first] = it->second;
          }
          ++it;
        }
      }
      static void invert(const std::vector<std::map<Key,T>>& matrix, std::vector<std::map<Key,T>>& inverse){
        inverse.resize(matrix.size());
        for(int j = 0; j < matrix.size(); ++j){
          for(auto& e: matrix[j]){
            inverse[e.first][j] = e.second;
          }
        }
      }
    };



    template <typename Key, typename T>
    class MapHelpers<Key,T,std::unordered_map<Key,T>>{
    public:
      static void shrinkToFit(std::unordered_map<Key,T>& map){}
      static double memoryOccupation(std::unordered_map<Key,T>& map){return -1;}
      template <typename It>
      static void initialize(std::unordered_map<Key,T>& map, It begin, It end, T thresh = 0){
        auto it = begin;
        while(it != end){
          if(it->second > thresh){
            map[it->first] = it->second;
          }
          ++it;
        }
      }
      static void invert(const std::vector<std::unordered_map<Key,T>>& matrix, std::vector<std::unordered_map<Key,T>>& inverse){
        inverse.resize(matrix.size());
        for(int j = 0; j < matrix.size(); ++j){
          for(auto& e: matrix[j]){
            inverse[e.first][j] = e.second;
          }
        }
      }
    };



    template <typename Key, typename T>
    class MapHelpers<Key,T,MapMemEff<Key,T>>{
    public:
      static void shrinkToFit(MapMemEff<Key,T>& map){
        map.shrink_to_fit();
      }
      static double memoryOccupation(MapMemEff<Key,T>& map){
        return map.capacity()*sizeof(typename MapMemEff<Key,T>::value_type);
      }
      template <typename It>
      static void initialize(MapMemEff<Key,T>& map, It begin, It end, T thresh = 0){
        map.initialize(begin,end, thresh);
      }
      static void invert(const std::vector<hdi::data::MapMemEff<Key,T>>& matrix, std::vector<hdi::data::MapMemEff<Key,T>>& inverse){
        inverse.resize(matrix.size());
        std::vector<unsigned int> inverse_row_size(inverse.size());
        for(int j = 0; j < matrix.size(); ++j){
          for(auto& e: matrix[j]){
            ++inverse_row_size[e.first];
          }
        }
        for(int j = 0; j < inverse.size(); ++j){
          inverse[j].memory().reserve(inverse_row_size[j]);
        }
        for(int j = 0; j < matrix.size(); ++j){
          for(auto& e: matrix[j]){
            inverse[e.first].memory().push_back(std::make_pair(j,e.second));
          }
        }
      }
    };

  }
}

#endif
