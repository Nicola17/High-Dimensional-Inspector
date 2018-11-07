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

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"

int main(int argc, char *argv[]) {
  hdi::utils::CoutLog log;

  std::ofstream out_file(argv[1], std::ios::out | std::ios::binary);
  std::ifstream in_file(argv[2], std::ios::in);
  std::string line;

  std::map<unsigned int, unsigned int> stats;
  unsigned int num_rows = 0;

  while (std::getline(in_file, line)) {
    std::stringstream line_stream(line);
    std::string cell;

    unsigned int num_cells = 0;
    while (std::getline(line_stream, cell, ',')) {
      float v = std::atof(cell.c_str());
      out_file.write((char *)(&v), sizeof(float));
      ++num_cells;
    }
    ++stats[num_cells];
    ++num_rows;
  }

  hdi::utils::secureLogValue(&log, "#rows", num_rows);
  for (auto e : stats) {
    hdi::utils::secureLogValue(&log, "\tcolumns", e.first);
    hdi::utils::secureLogValue(&log, "\t#rows", e.second);
  }
}
