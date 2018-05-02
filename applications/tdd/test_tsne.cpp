/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *    its contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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



#include <catch.hpp>
#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/utils/cout_log.h"
#include "hdi/data/embedding.h"


template <typename scalar_type>
void test_tsne(){
  hdi::utils::CoutLog coutLog;

  hdi::dr::TSNE<scalar_type> tSNE;
  std::vector<scalar_type>a(10, 1);
  std::vector<scalar_type>b(10, 2);
  std::vector<scalar_type>c(10, 3);

    hdi::data::Embedding<scalar_type> embedding_container;

  REQUIRE((tSNE.logger() == nullptr));
  tSNE.setLogger(&coutLog);
  REQUIRE((tSNE.logger() == &coutLog));

  REQUIRE(tSNE.dimensionality() == 0);
  REQUIRE_THROWS(tSNE.addDataPoint(a.data()));
  REQUIRE_THROWS(tSNE.setDimensionality(-2));
  REQUIRE_NOTHROW(tSNE.setDimensionality(a.size()));

    REQUIRE_THROWS(tSNE.initialize(&embedding_container));
  REQUIRE_THROWS(tSNE.doAnIteration());

  REQUIRE_THROWS(tSNE.setDimensionality(0));

  REQUIRE_NOTHROW(tSNE.addDataPoint(a.data()));
  REQUIRE_NOTHROW(tSNE.addDataPoint(b.data()));
  REQUIRE_NOTHROW(tSNE.addDataPoint(c.data()));

  std::vector<scalar_type> test_dp;
  REQUIRE_NOTHROW(tSNE.getHighDimensionalDescriptor(test_dp,0));
  for(int i = 0; i < a.size(); ++i){
    REQUIRE(a[i] == test_dp[i]);
  }
  REQUIRE_NOTHROW(tSNE.getHighDimensionalDescriptor(test_dp,1));
  for(int i = 0; i < a.size(); ++i){
    REQUIRE(b[i] == test_dp[i]);
  }
  REQUIRE_NOTHROW(tSNE.getHighDimensionalDescriptor(test_dp,2));
  for(int i = 0; i < a.size(); ++i){
    REQUIRE(c[i] == test_dp[i]);
  }

    REQUIRE_NOTHROW(tSNE.initialize(&embedding_container));
  REQUIRE_NOTHROW(tSNE.doAnIteration());

  REQUIRE_NOTHROW(tSNE.reset());
    REQUIRE_NOTHROW(tSNE.initialize(&embedding_container));
  REQUIRE_NOTHROW(tSNE.doAnIteration());

  REQUIRE_NOTHROW(tSNE.clear());
    REQUIRE_THROWS(tSNE.initialize(&embedding_container));
  REQUIRE_THROWS(tSNE.doAnIteration());
}


TEST_CASE( "tSNE - float", "[algorithms_embedding]" ) {
  typedef float scalar_type;
  test_tsne<scalar_type>();
}

TEST_CASE( "tSNE - double", "[algorithms_embedding]" ) {
  typedef double scalar_type;
  test_tsne<scalar_type>();
}
