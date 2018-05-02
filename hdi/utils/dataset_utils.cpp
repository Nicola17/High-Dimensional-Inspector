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

#include "hdi/utils/dataset_utils.h"
#include <QDir>

namespace hdi{
  namespace utils{
    namespace IO{

      template void loadMNIST(data::PanelData<double>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images, int label_to_be_selected);
      template void loadMNIST(data::PanelData<float>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images, int label_to_be_selected);

      void loadTwitterFollowers(const std::string& folder, std::vector<Roaring>& follower_to_target, std::vector<Roaring>& target_to_follower,
                    std::unordered_map<std::string,uint32_t>& follower_id, std::unordered_map<std::string,uint32_t>& target_id)
      {
        follower_to_target.clear();
        target_to_follower.clear();
        follower_id.clear();
        target_id.clear();

        QStringList nameFilter("*.txt");
        QDir directory(folder.c_str());
        QFileInfoList files = directory.entryInfoList(nameFilter);

        uint32_t file_id = 0;
        for(auto& file: files){
          target_to_follower.resize(file_id+1);
          target_id[file.baseName().toStdString()] = file_id;
          std::cout << file.absoluteFilePath().toStdString() << std::endl;
          std::string line;
          std::ifstream myfile (file.absoluteFilePath().toStdString().c_str());
          if (myfile.is_open()){
            while (std::getline(myfile,line) ){
              uint32_t id = 0;
              auto it = follower_id.find(line);
              if(it == follower_id.end()){
                id = follower_id.size();
                follower_id[line] = id;
                follower_to_target.resize(id+1);
              }else{
                id = it->second;
              }

              follower_to_target[id].add(file_id);
              target_to_follower[file_id].add(id);
            }
            myfile.close();
          }
          ++file_id;
        }
      }

    }
  }
}
