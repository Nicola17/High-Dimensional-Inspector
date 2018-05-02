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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>

#include <stdint.h>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/dataset_utils.h"

int main(int argc, char *argv[]){
  try{
    typedef double scalar_type;
    hdi::utils::CoutLog log;

    if(argc != 5){
      hdi::utils::secureLog(&log,"Wrong number of input parameters...");
      return 1;
    }
    std::ofstream out_file(argv[1], std::ios::out);

    if (!out_file.is_open()){
      throw std::runtime_error("output file cannot be open");
    }

    hdi::data::PanelData<scalar_type> panel_data;
    std::vector<unsigned int> labels;

    hdi::utils::IO::loadTimit(panel_data,labels,argv[2],argv[3],std::atoi(argv[4]));
    hdi::utils::secureLogValue(&log,"test",panel_data.dataAt(3,3));
    //hdi::data::unitNormalization(panel_data);
    hdi::utils::secureLog(&log,"Data loaded...");

    const unsigned int num_dp(panel_data.numDataPoints());
    const unsigned int num_dims(panel_data.numDimensions());

    out_file << "DY" << std::endl;
    out_file << num_dp << std::endl;
    out_file << num_dims << std::endl;

    for(int i = 0; i < num_dims; ++i){
      out_file << "D" << i;
      if(i!=(num_dims-1)){
        out_file << ";";
      }else{
        out_file << std::endl;
      }
    }

    for(int i = 0; i < num_dp; ++i){
      out_file << "P" << i << ";";
      for(int d = 0; d < num_dims; ++d){
        out_file << std::setprecision(5) << panel_data.dataAt(i,d) << ";";
      }
      out_file <<  int(labels[i]) << ".0" << std::endl;
    }

  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
