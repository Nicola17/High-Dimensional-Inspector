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

#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"

int main(int argc, char *argv[]){

  hdi::utils::CoutLog log;

  const unsigned int num_files(argc-2);
  std::ofstream out_file(argv[1], std::ios::out|std::ios::binary);

  std::vector<std::vector<float> > data(num_files);



  {
    for(int i = 0; i < num_files; ++i){
      std::ifstream in_file(argv[2+i], std::ios::in);
      std::string line;
      std::getline(in_file,line);
      std::stringstream      line_stream(line);
      std::string        cell;

      while(std::getline(line_stream,cell,',')){
        float v = std::atof(cell.c_str());
        data[i].push_back(v);
      }
    }
  }

  for(int d = 0; d < data.size(); ++d){
    float avg = 0;
    float avg_sq = 0;
    for(int i = 0; i < data[d].size(); ++i){
      avg += data[d][i];
      avg_sq += data[d][i]*data[d][i];
    }
    avg/=data[d].size();
    avg_sq/=data[d].size();
    float std_dev = std::sqrt(avg_sq-avg*avg+0.000001);
    for(int i = 0; i < data[d].size(); ++i){
      data[d][i] = (data[d][i]-avg)/std_dev;
    }
  }


  hdi::utils::secureLogValue(&log, "\t#columns", data.size());
  hdi::utils::secureLogValue(&log, "\t#rows", data[0].size());

  for(int i = 0; i < data[0].size(); ++i){
    for(int d = 0; d < data.size(); ++d){
      if(i >= data[d].size()){
        hdi::utils::secureLogValue(&log, "WRONG SIZE OF FILE", d);
      }
      out_file.write((char*)(&(data[d][i])),sizeof(float));
    }
  }


}
