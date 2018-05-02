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

#include <stdint.h>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"

int main(int argc, char *argv[]){
  try{
    hdi::utils::CoutLog log;

    if(argc != 5){
      hdi::utils::secureLog(&log,"Wrong number of input parameters...");
      return 1;
    }
    std::ofstream out_file(argv[1], std::ios::out);
    std::ofstream out_file_bin("mnist.bin", std::ios::out|std::ios::binary);
    std::ifstream file_data(argv[2], std::ios::in|std::ios::binary);
    std::ifstream file_labels(argv[3], std::ios::in|std::ios::binary);
    if (!file_labels.is_open()){
      throw std::runtime_error("label file cannot be open");
    }
    if (!file_data.is_open()){
      throw std::runtime_error("data file cannot be open");
    }
    if (!out_file.is_open()){
      throw std::runtime_error("output file cannot be open");
    }

    const unsigned int num_pics(std::atoi(argv[4]));
    const unsigned int num_dims(28*28);

    out_file << "DY" << std::endl;
    out_file << num_pics << std::endl;
    out_file << num_dims << std::endl;

    for(int i = 0; i < num_dims; ++i){
      out_file << "D" << i;
      if(i!=(num_dims-1)){
        out_file << ";";
      }else{
        out_file << std::endl;
      }
    }

    {//removing headers
      int32_t appo;
      file_labels.read((char*)&appo,4);
      file_labels.read((char*)&appo,4);
      file_data.read((char*)&appo,4);
      file_data.read((char*)&appo,4);
      file_data.read((char*)&appo,4);
      file_data.read((char*)&appo,4);
    }

    for(int i = 0; i < num_pics; ++i){
      unsigned char label;
      file_labels.read((char*)&label,1);

      std::vector<unsigned char> data(num_dims,0);
      file_data.read((char*)data.data(),num_dims);
      out_file << "P" << i << ";";
      for(int d = 0; d < num_dims; ++d){
        out_file <<  int(data[d]) << ".0;";
        float fp_v(data[d]);
        out_file_bin.write((char*)&(fp_v),4);
      }
      out_file <<  int(label) << ".0" << std::endl;
    }

  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
