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

#include "catch.hpp"
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/map_mem_eff.h"
#include "hdi/data/map_helpers.h"
#include "hdi/utils/timers.h"
#include "hdi/utils/scoped_timers.h"
#include <map>

/***********************************************/
/***********************************************/
/***********************************************/

TEST_CASE( "Low level memory test", "[MapMemEff]" ) {
  hdi::utils::CoutLog log;
  std::pair<int,float> pair;

  hdi::data::MapMemEff<int,float> map;
  SECTION("Pair is the right choice"){
    hdi::utils::secureLogValue(&log,"pair size: ", sizeof(pair));
    REQUIRE( sizeof(pair) == 8 );
  }

  SECTION("Right initialization"){
    REQUIRE( map.size() == 0 );
    REQUIRE( map.capacity() == 0 );
  }

  SECTION("operator[] is working properly"){
    REQUIRE_NOTHROW(map[0] += 1);
    REQUIRE_NOTHROW(map[1] += 1);
    REQUIRE_NOTHROW(map[2] += 1);
    REQUIRE_NOTHROW(map[1000] += 1);
    REQUIRE_NOTHROW(map[2] += 1);
    REQUIRE_NOTHROW(map[1000] += 1);
    REQUIRE_NOTHROW(map[1] += 1);
    REQUIRE_NOTHROW(map[1000] += 1);
    REQUIRE_NOTHROW(map[2] += 1);

    REQUIRE(map[0] == 1);
    REQUIRE(map[1] == 2);
    REQUIRE(map[2] == 3);
    REQUIRE(map[1000] == 3);

    REQUIRE(map.size() == 4);
    REQUIRE(map.capacity() == 4);
  }

  SECTION("operator[] is working properly"){
    std::map<int,float> map_stl;
    int n_test = 30000;
    for(int i = 0; i < n_test; ++i){
      int f = rand()%1000;
      ++map[f];
      ++map_stl[f];
    }
    int sum = 0;
    for(auto &p: map){
      sum += p.second;
    }
    REQUIRE(sum == n_test);

    for(auto &p: map){
      REQUIRE(p.second == map_stl[p.first]);
    }
    map.shrink_to_fit();
    REQUIRE(map.capacity() == map.size());
  }

  SECTION("initialize is working properly"){
    std::map<int,float> map_stl;
    int n_test = 30000;
    for(int i = 0; i < n_test; ++i){
      int f = rand()%1000;
      ++map_stl[f];
    }
    REQUIRE_NOTHROW(map.initialize(map_stl.begin(),map_stl.end()));

    int sum = 0;
    for(auto &p: map){
      sum += p.second;
    }
    REQUIRE(sum == n_test);

    for(auto &p: map){
      REQUIRE(p.second == map_stl[p.first]);
    }
    REQUIRE(map.capacity() == map.size());
  }

}


template <typename Map>
void testMapInitialize(){
  typedef hdi::data::MapHelpers<typename Map::key_type,typename Map::mapped_type, Map> map_helpers_type;
  Map map;
  std::map<typename Map::key_type,typename Map::mapped_type> map_stl;
  int n_test = 30000;
  for(int i = 0; i < n_test; ++i){
    int f = rand()%1000;
    ++map_stl[f];
  }

  REQUIRE_NOTHROW(map_helpers_type::initialize(map,map_stl.begin(),map_stl.end()));

  int sum = 0;
  for(auto &p: map){
    sum += p.second;
  }
  REQUIRE(sum == n_test);

  for(auto &p: map){
    REQUIRE(p.second == map_stl[p.first]);
  }
}

TEST_CASE( "MapHelpers::initialize", "[MapHelpers]" ) {
  SECTION("MapMemEff"){
    testMapInitialize<hdi::data::MapMemEff<int,float>>();
    testMapInitialize<hdi::data::MapMemEff<int,double>>();
    testMapInitialize<hdi::data::MapMemEff<unsigned int,float>>();
    testMapInitialize<hdi::data::MapMemEff<unsigned int,double>>();
  }
  SECTION("std::map"){
    testMapInitialize<std::map<int,float>>();
    testMapInitialize<std::map<int,double>>();
    testMapInitialize<std::map<unsigned int,float>>();
    testMapInitialize<std::map<unsigned int,double>>();
  }
  SECTION("std::unordered_map"){
    testMapInitialize<std::unordered_map<int,float>>();
    testMapInitialize<std::unordered_map<int,double>>();
    testMapInitialize<std::unordered_map<unsigned int,float>>();
    testMapInitialize<std::unordered_map<unsigned int,double>>();
  }
}




template <typename Map>
void testMapInvert(){
  typedef hdi::data::MapHelpers<typename Map::key_type,typename Map::mapped_type, Map> map_helpers_type;
  typedef std::vector<Map> sparse_matrix_type;

  hdi::utils::CoutLog log;
  int n = 1500;
  sparse_matrix_type matrix(n);
  for(int j = 0; j < matrix.size(); ++j){
    for(int t = 0; t < matrix.size()/10; ++t){
      auto i = rand()%n;
      matrix[j][i] = rand()%1000;
    }
  }
  sparse_matrix_type inverse;
  double time(0);

  {
    hdi::utils::ScopedTimer<double> timer(time);
    map_helpers_type::invert(matrix,inverse);
  }
  hdi::utils::secureLogValue(&log,"Invert (s)", time);
  for(int j = 0; j < matrix.size(); ++j){
    for(auto& e: matrix[j]){
      REQUIRE(e.second == inverse[e.first][j]);
    }
  }

}

TEST_CASE( "MapHelpers::invert", "[MapHelpers]" ) {
  hdi::utils::CoutLog log;
  SECTION("MapMemEff"){
    log.display("MapMemEff");
    testMapInvert<hdi::data::MapMemEff<int,float>>();
    testMapInvert<hdi::data::MapMemEff<int,double>>();
    testMapInvert<hdi::data::MapMemEff<unsigned int,float>>();
    testMapInvert<hdi::data::MapMemEff<unsigned int,double>>();
  }
  SECTION("std::map"){
    log.display("std::map");
    testMapInvert<std::map<int,float>>();
    testMapInvert<std::map<int,double>>();
    testMapInvert<std::map<unsigned int,float>>();
    testMapInvert<std::map<unsigned int,double>>();
  }
  SECTION("std::unordered_map"){
    log.display("std::unordered_map");
    testMapInvert<std::unordered_map<int,float>>();
    testMapInvert<std::unordered_map<int,double>>();
    testMapInvert<std::unordered_map<unsigned int,float>>();
    testMapInvert<std::unordered_map<unsigned int,double>>();
  }
}
