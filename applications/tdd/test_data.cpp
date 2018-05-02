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
#include "hdi/data/panel_data.h"
#include "hdi/data/text_data.h"
#include "hdi/data/histogram.h"

TEST_CASE( "Initialization of a PanelData", "[PanelData]" ) {
  hdi::data::PanelData<float> panel_data;
  std::vector<float> high_dim(5);

  REQUIRE_THROWS(panel_data.initialize());
  REQUIRE(panel_data.isInitialized() == false);
  REQUIRE_THROWS(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim));
  REQUIRE_THROWS(panel_data.reserve(10));

////////////////////////////////////////////////
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())) == 4);
  REQUIRE(panel_data.numDimensions() == 5);

  REQUIRE_NOTHROW(panel_data.initialize());
  REQUIRE(panel_data.isInitialized() == true);
  REQUIRE_NOTHROW(panel_data.reserve(10));

  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim));
  REQUIRE(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),high_dim) == 4);
  REQUIRE(panel_data.numDataPoints() == 5);

  REQUIRE_THROWS(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(10)));

  REQUIRE(panel_data.getData().size() == 25);
  REQUIRE(panel_data.getDataPoints().size() == 5);
  REQUIRE(panel_data.getDimensions().size() == 5);
  REQUIRE(panel_data.getFlagsDataPoints().size() == 5);
  REQUIRE(panel_data.getFlagsDimensions().size() == 5);

////////////////////////////////////////////////
  REQUIRE_NOTHROW(panel_data.clear());
  REQUIRE(panel_data.numDimensions() == 0);
  REQUIRE(panel_data.numDataPoints() == 0);
  REQUIRE(panel_data.getData().size() == 0);
  REQUIRE(panel_data.getDataPoints().size() == 0);
  REQUIRE(panel_data.getDimensions().size() == 0);
  REQUIRE(panel_data.getFlagsDataPoints().size() == 0);
  REQUIRE(panel_data.getFlagsDimensions().size() == 0);
}

TEST_CASE( "Transposed PanelData", "[PanelData]" ) {
  hdi::data::PanelData<float> panel_data;
  hdi::data::PanelData<float> transposed_panel_data;

////////////////////////////////////////////////
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())) == 4);
  REQUIRE(panel_data.numDimensions() == 5);

  REQUIRE_NOTHROW(panel_data.initialize());
  REQUIRE(panel_data.isInitialized() == true);
  REQUIRE_NOTHROW(panel_data.reserve(10));

  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,0)));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,1)));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,2)));
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,3)));

  REQUIRE_NOTHROW(hdi::data::transposePanelData(panel_data,transposed_panel_data));

  for(int i = 0; i < panel_data.numDataPoints(); ++i){
    for(int f = 0; f < panel_data.numDimensions(); ++f){
      REQUIRE(panel_data.dataAt(i,f) == transposed_panel_data.dataAt(f,i));
    }
  }

}

TEST_CASE( "PanelData properties", "[PanelData]" ) {
  hdi::data::PanelData<float> panel_data;


////////////////////////////////////////////////
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())) == 4);
  REQUIRE(panel_data.numDimensions() == 5);

  REQUIRE_NOTHROW(panel_data.initialize());
  REQUIRE(panel_data.isInitialized() == true);
  REQUIRE_NOTHROW(panel_data.reserve(10));

  REQUIRE(panel_data.hasProperty("Test") == false);
  REQUIRE_NOTHROW(panel_data.requestProperty("Test"));
  REQUIRE(panel_data.hasProperty("Test") == true);
  REQUIRE(panel_data.getProperty("Test").size() == 0);
  REQUIRE_THROWS(panel_data.getProperty("Test2"));


  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,0)));
  REQUIRE(panel_data.getProperty("Test").size() == 1);
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,1)));
  REQUIRE(panel_data.getProperty("Test").size() == 2);
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,2)));
  REQUIRE(panel_data.getProperty("Test").size() == 3);
  REQUIRE_NOTHROW(panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData()),std::vector<float>(5,3)));
  REQUIRE(panel_data.getProperty("Test").size() == 4);


  REQUIRE(panel_data.hasProperty("Test") == true);
  REQUIRE_NOTHROW(panel_data.releaseProperty("Test"));
  REQUIRE(panel_data.hasProperty("Test") == false);

  REQUIRE(panel_data.hasProperty("Test") == false);
  REQUIRE_NOTHROW(panel_data.requestProperty("Test"));
  REQUIRE(panel_data.hasProperty("Test") == true);
  REQUIRE(panel_data.getProperty("Test").size() == 4);
}

TEST_CASE( "PanelData dimension properties", "[PanelData]" ) {
  hdi::data::PanelData<float> panel_data;

////////////////////////////////////////////////
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE_NOTHROW(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())));
  REQUIRE(panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData())) == 3);
  REQUIRE(panel_data.numDimensions() == 4);

  REQUIRE_THROWS(panel_data.requestDimProperty("Test"));
  REQUIRE_THROWS(panel_data.releaseDimProperty("Test"));

  REQUIRE_NOTHROW(panel_data.initialize());
  REQUIRE(panel_data.isInitialized() == true);

  REQUIRE(panel_data.hasDimProperty("Test") == false);
  REQUIRE_NOTHROW(panel_data.requestDimProperty("Test"));
  REQUIRE(panel_data.hasDimProperty("Test") == true);
  REQUIRE(panel_data.getDimProperty("Test").size() == 4);
  REQUIRE_THROWS(panel_data.getDimProperty("Test2"));

  REQUIRE(panel_data.hasDimProperty("Test") == true);
  REQUIRE_NOTHROW(panel_data.releaseDimProperty("Test"));
  REQUIRE(panel_data.hasDimProperty("Test") == false);

  REQUIRE(panel_data.hasDimProperty("Test") == false);
  REQUIRE_NOTHROW(panel_data.requestDimProperty("Test"));
  REQUIRE(panel_data.hasDimProperty("Test") == true);
  REQUIRE(panel_data.getDimProperty("Test").size() == 4);

}

template <typename scalar_type>
void testHistogram(){
  hdi::data::Histogram<scalar_type> h;
  REQUIRE(h.min() == 0);
  REQUIRE(h.max() == 0);
  REQUIRE(h.num_buckets() == 0);

  h = hdi::data::Histogram<scalar_type>(0,10,10);
  REQUIRE(h.min() == 0);
  REQUIRE(h.max() == 10);
  REQUIRE(h.num_buckets() == 10);

  REQUIRE_NOTHROW(h.add(5));
  REQUIRE_NOTHROW(h.add(1));
  REQUIRE_NOTHROW(h.add(2));
  REQUIRE_NOTHROW(h.add(3));
  REQUIRE_NOTHROW(h.add(5));
  REQUIRE_NOTHROW(h.add(5));

  REQUIRE(h.sum() == 6);

  REQUIRE_NOTHROW(h.add(0));
  REQUIRE_NOTHROW(h.add(10));

  REQUIRE(h.sum() == 8);

  REQUIRE_NOTHROW(h.add(200));
  REQUIRE_NOTHROW(h.add(-10));

  REQUIRE(h.sum() == 8);
  REQUIRE(h.data()[5] == 3);
  REQUIRE_NOTHROW(h.add(5.5));
  REQUIRE(h.sum() == 9);
  REQUIRE(h.data()[5] == 4);
  REQUIRE(h.data()[9] == 1);
  REQUIRE(h.data()[1] == 1);
  REQUIRE(h.data()[0] == 1);
}


TEST_CASE( "Histogram<float>", "[Histogram]" ) {
  testHistogram<float>();
  testHistogram<double>();
}
