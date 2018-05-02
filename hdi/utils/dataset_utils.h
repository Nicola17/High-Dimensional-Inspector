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

#ifndef DATASET_UTILS_H
#define DATASET_UTILS_H

#include "hdi/data/panel_data.h"
#include <vector>
#include <iostream>
#include <fstream>
#include "hdi/data/pixel_data.h"
#include "hdi/data/image_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/data/text_data.h"
#include "hdi/data/voxel_data.h"
#include "vector_utils.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/utils/log_helper_functions.h"
#include <sstream>
#include <set>
#include <unordered_map>
#include "roaring/roaring.hh"


namespace hdi{
  namespace utils{

    template <typename scalar_type>
    void loadGaussianSpheres(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, int n_spheres = 5, int n_points_sphere = 1000, int n_dimensions = 10);

    template <typename scalar_type>
    void loadGaussianSpheres(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, int n_spheres, int n_points_sphere, int n_dimensions){
      panel_data.clear();
      labels.clear();
      labels.reserve(n_spheres*n_points_sphere);
      for(int j = 0; j < n_dimensions; ++j){
        panel_data.addDimension(std::make_shared<hdi::data::TextData>(QString("Dim%1").arg(j).toStdString()));
      }
      panel_data.initialize();

      scalar_type radius = 10;

      for(int s = 0; s < n_spheres; ++s){
        std::vector<scalar_type> center(n_dimensions);
        for(int i = 0; i < center.size(); ++i){
          center[i] = (std::rand()%10000)/10000.;
        }
        utils::multiply(center,radius*n_spheres);

        for(int p = 0; p < n_points_sphere; ++p){
          std::vector<scalar_type> versor(n_dimensions);
          for(int i = 0; i < center.size(); ++i){
            versor[i] = (std::rand()%10000)/10000.-0.5;
          }
          utils::normalize(versor);
          utils::multiply(versor,scalar_type(radius*(std::rand()%10000)/10000.));
          std::vector<scalar_type> input(n_dimensions);
          utils::sum(input,center,versor);

          panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(), input);
          labels.push_back(s);
        }
      }
    }

    namespace IO{

      template <typename scalar_type>
      void loadMNIST(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images = 60000, int label_to_be_selected = -1);

      template <typename scalar_type>
      void loadCifar10(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points = 50000);

      template <typename scalar_type>
      void loadTimit(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points = 50000);

      template <typename scalar_type>
      void loadMitoticFigures(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images, int n_dim = 200);

      template <typename scalar_type>
      void loadMitoticFiguresAndVolume(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images);

      void loadTwitterFollowers(const std::string& folder, std::vector<Roaring>& follower_to_target, std::vector<Roaring>& target_to_follower,
                    std::unordered_map<std::string,uint32_t>& follower_id, std::unordered_map<std::string,uint32_t>& target_id);

      //////////////////////////////////////////////////////////////

      template <typename scalar_type>
      void loadMNIST(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, unsigned int num_images, int label_to_be_selected){
        panel_data.clear();
        const int num_dimensions(784);

        std::ifstream file_data(filename_data, std::ios::in|std::ios::binary);
        std::ifstream file_labels(filename_labels, std::ios::in|std::ios::binary);
        if (!file_labels.is_open()){
          throw std::runtime_error("label file cannot be found");
        }
        if (!file_data.is_open()){
          throw std::runtime_error("data file cannot be found");
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


        {//initializing panel data
          for(int j = 0; j < 28; ++j){
            for(int i = 0; i < 28; ++i){
              panel_data.addDimension(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(j,i,28,28)));
            }
          }
          panel_data.initialize();
        }

        std::vector<QImage> images;
        std::vector<std::vector<scalar_type> > input_data;
        {//reading data
          images.reserve(num_images);
          input_data.reserve(num_images);
          labels.reserve(num_images);

          for(int i = 0; i < num_images; ++i){
            unsigned char label;
            file_labels.read((char*)&label,1);
            labels.push_back(label);

            //still some pics to read for this digit
            input_data.push_back(std::vector<scalar_type>(num_dimensions));
            images.push_back(QImage(28,28,QImage::Format::Format_ARGB32));
            const int idx = int(input_data.size()-1);
            for(int i = 0; i < num_dimensions; ++i){
              unsigned char pixel;
              file_data.read((char*)&pixel,1);
              const scalar_type intensity(255.f - pixel);
              input_data[idx][i] = intensity;
              images[idx].setPixel(i%28,i/28,qRgb(intensity,intensity,intensity));
            }
          }

          {
            //moving a digit at the beginning digits of the vectors
            const int digit_to_be_moved = 1;
            int idx_to_be_swapped = 0;
            for(int i = 0; i < images.size(); ++i){
              if(labels[i] == digit_to_be_moved){
                std::swap(images[i],    images[idx_to_be_swapped]);
                std::swap(input_data[i],  input_data[idx_to_be_swapped]);
                std::swap(labels[i],    labels[idx_to_be_swapped]);
                ++idx_to_be_swapped;
              }
            }
          }

          for(int i = 0; i < images.size(); ++i){
            panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(images[i])), input_data[i]);
            if(labels[i] == label_to_be_selected){
              panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
            }
          }
        }
      }


      template <typename scalar_type>
      void loadCifar10(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points){
        panel_data.clear();
        std::ifstream file_data(filename_data, std::ios::in|std::ios::binary);
        std::ifstream file_labels(filename_labels, std::ios::in|std::ios::binary);
        if (!file_labels.is_open()){
          throw std::runtime_error("label file cannot be found");
        }
        if (!file_data.is_open()){
          throw std::runtime_error("data file cannot be found");
        }

        const int n_dim = 1024;
        for(int j = 0; j < n_dim; ++j){
          panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
        }
        panel_data.initialize();

        {
          for(int j = 0; j < n_points; ++j){
            std::vector<scalar_type> input_data(n_dim);
            for(int i = 0; i < n_dim; ++i){
              float appo;
              file_data.read((char*)&appo,4);
              input_data[i] = scalar_type(appo);
            }
            panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(), input_data);
          }
        }

        {
          for(int j = 0; j < n_points; ++j){
            unsigned char appo;
            file_labels.read((char*)&appo,1);
            labels.push_back(appo);
          }
        }
      }



      template <typename scalar_type>
      void loadTimit(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points){
        panel_data.clear();
        std::ifstream file_data(filename_data, std::ios::in|std::ios::binary);
        std::ifstream file_labels(filename_labels, std::ios::in|std::ios::binary);
        if (!file_labels.is_open()){
          throw std::runtime_error("label file cannot be found");
        }
        if (!file_data.is_open()){
          throw std::runtime_error("data file cannot be found");
        }

        const int n_dim = 39;
        for(int j = 0; j < n_dim; ++j){
          panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
        }
        panel_data.initialize();

        {
          for(int j = 0; j < n_points; ++j){
            std::vector<scalar_type> input_data(n_dim);
            for(int i = 0; i < n_dim; ++i){
              float appo;
              file_data.read((char*)&appo,4);
              input_data[i] = scalar_type(appo);
            }
            panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(), input_data);
          }
        }

        {
          for(int j = 0; j < n_points; ++j){
            float appo;
            file_labels.read((char*)&appo,4);
            labels.push_back(int(appo+0.0001));
          }
        }

      }


      template <typename scalar_type>
      void loadHDVOL(data::PanelData<scalar_type>& panel_data, std::vector<int>& limits, std::string filename, std::string feature_subset_filename= std::string(""), AbstractLog* log = nullptr){
        secureLogValue(log,"Loading high-dimensional volume file (.hvol)",filename.c_str());
        panel_data.clear();
        int n_points(0);
        int n_dimensions_on_file(0);
        int n_dimensions(0);
        limits.resize(6);

        std::ifstream file(filename, std::ios::in|std::ios::binary);
        if (!file.is_open()){
          throw std::runtime_error("data file cannot be found");
        }

        std::set<unsigned int> feature_subset;
        if(feature_subset_filename.compare("") != 0){
          secureLog(log,"Loading a SUBSET of the available features!");
          std::ifstream file_feature_subset(feature_subset_filename, std::ios::in);
          if (!file_feature_subset.is_open()){
            throw std::runtime_error("feature subset file cannot be found");
          }

          unsigned int feature;
          while (file_feature_subset >> feature){
            feature_subset.insert(feature);
          }
          if(feature_subset.size() == 0){
            throw std::runtime_error("Empty feature subset provided");
          }
        }else{
          secureLog(log,"Loading all features");

        }


        //Header
        file.read((char*)&n_points,4);
        file.read((char*)&n_dimensions_on_file,4);
        file.read((char*)(limits.data()+0),4);
        file.read((char*)(limits.data()+1),4);
        file.read((char*)(limits.data()+2),4);
        file.read((char*)(limits.data()+3),4);
        file.read((char*)(limits.data()+4),4);
        file.read((char*)(limits.data()+5),4);
        secureLogValue(log,"\t# data points",n_points);
        secureLogValue(log,"\t# dimensions",n_dimensions_on_file);
        secureLogValue(log,"\tx min",limits[0]);
        secureLogValue(log,"\tx max",limits[1]);
        secureLogValue(log,"\ty min",limits[2]);
        secureLogValue(log,"\ty max",limits[3]);
        secureLogValue(log,"\tz min",limits[4]);
        secureLogValue(log,"\tz max",limits[5]);

        if(feature_subset.size()){
          n_dimensions = feature_subset.size();
        }else{
          n_dimensions = n_dimensions_on_file;
        }
        secureLogValue(log,"Final dimensionality",n_dimensions);

        for(int j = 0; j < n_dimensions; ++j){
          panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
        }
        panel_data.initialize();

        {
          for(int j = 0; j < n_points; ++j){
            std::vector<scalar_type> input_data(n_dimensions);
            int x,y,z;
            file.read((char*)&x,4);
            file.read((char*)&y,4);
            file.read((char*)&z,4);
            int idx = 0;
            for(int i = 0; i < n_dimensions_on_file; ++i){
              float appo;
              file.read((char*)&appo,4);
              if(!feature_subset.empty() && feature_subset.find(i) == feature_subset.end()){
                continue;
              }
              input_data[idx] = scalar_type(appo);
              ++idx;
            }
            panel_data.addDataPoint(std::make_shared<hdi::data::VoxelData>(hdi::data::VoxelData(x,y,z)), input_data);
          }
        }

        secureLog(log,"Data loaded!");
      }

      template <typename scalar_type>
      void loadMitoticFigures(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points, int n_dim = 200){
        panel_data.clear();
        std::ifstream file_data(filename_data, std::ios::in|std::ios::binary);
        std::ifstream file_labels(filename_labels, std::ios::in);
        if (!file_labels.is_open()){
          throw std::runtime_error("label file cannot be found");
        }
        if (!file_data.is_open()){
          throw std::runtime_error("data file cannot be found");
        }

        for(int j = 0; j < n_dim; ++j){
          panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
        }
        panel_data.initialize();
        std::string line;
        std::getline(file_labels,line);

        for(int j = 0; j < n_points; ++j){
          std::getline(file_labels,line);
          std::stringstream line_stream(line);
          std::string cell;
          std::string img_path;
          std::getline(line_stream,cell,',');
          img_path = cell;
          std::getline(line_stream,cell,',');
          int lbl = std::atoi(cell.c_str());


          std::vector<scalar_type> input_data(n_dim);
          for(int i = 0; i < n_dim; ++i){
            float appo;
            file_data.read((char*)&appo,4);
            input_data[i] = scalar_type(appo);
          }
          panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(img_path), input_data);
          labels.push_back(lbl);
        }
      }

      template <typename scalar_type>
      void loadMitoticFiguresAndVolume(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels){
        std::vector<int> limits;
        loadHDVOL(panel_data,limits,filename_data);

        std::ifstream file_labels(filename_labels, std::ios::in);
        if (!file_labels.is_open()){
          throw std::runtime_error("label file cannot be found");
        }

        std::string line;
        std::getline(file_labels,line);

        for(int j = 0; j < panel_data.numDimensions(); ++j){
          std::getline(file_labels,line);
          std::stringstream line_stream(line);
          std::string cell;
          std::string img_path;
          std::getline(line_stream,cell,',');
          img_path = cell;
          std::getline(line_stream,cell,',');
          int lbl = std::atoi(cell.c_str());

          panel_data.getDimensions()[j] = std::make_shared<hdi::data::TextData>(img_path);
          labels.push_back(lbl);
        }
      }

      template <typename scalar_type>
      void loadImages(data::PanelData<scalar_type>& panel_data, std::vector<unsigned int>& labels, std::string filename_data, std::string filename_labels, int n_points){
        panel_data.clear();
        labels.clear();
        labels.reserve(n_points);
        std::ifstream file_pictures(filename_labels, std::ios::in);
        std::ifstream file_hd_space(filename_data, std::ios::in|std::ios::binary|std::ios::ate);
        if (!file_pictures.is_open()){
          throw std::runtime_error("label file cannot be found");
        }
        if (!file_hd_space.is_open()){
          throw std::runtime_error("data file cannot be found");
        }

        std::string line;
        std::getline(file_pictures, line);

        int num_pics(std::atoi(line.c_str()));
        const int num_dimensions(file_hd_space.tellg()/sizeof(float)/num_pics);


        file_hd_space.seekg (0, std::ios::beg);

        {//initializing panel data
          for(int i = 0; i < num_dimensions; ++i){
            panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
          }
          panel_data.initialize();
        }

        {//reading data
          int i = 0;
          while(std::getline(file_pictures, line)){
            if(i >= n_points)
              break;
            std::vector<scalar_type> data(num_dimensions);
            file_hd_space.read(reinterpret_cast<char*>(data.data()), sizeof(float) * num_dimensions);
            panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData(line)), data);
            labels.push_back(0);
            ++i;
          }
        }

      }

    }
  }
}

#endif
