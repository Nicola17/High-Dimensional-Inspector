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

#include "hdi/analytics/waow_vis_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/graph_algorithms.h"
#include <numeric>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <QDir>
#include <iostream>
#include <fstream>
#include <map>
#include "hdi/data/histogram.h"
#include "hdi/data/set_intersection_tree.h"
#include "hdi/data/map_helpers.h"
#include "hdi/utils/scoped_timers.h"


#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/visualization/scatterplot_text_drawer.h"

#include "hdi/visualization/text_view_qobj.h"

#include "hdi/visualization/embedding_lines_drawer.h"
#include "hdi/data/io.h"
#include "hdi/utils/math_utils.h"

namespace hdi{
  namespace analytics{

    WAOWVis::WAOWVis():
      _logger(nullptr),
      _interface_initializer(nullptr),
      _selection_linked_to_data_points(false),
      _verbose(false),
      _visualization_mode("Default")
    {}

    void WAOWVis::initialize(std::shared_ptr<sets_type> sets_A,
                 std::shared_ptr<sets_type> sets_B,
                 const std::vector<std::shared_ptr<data::AbstractData>>& sets_A_data,
                 const std::vector<std::shared_ptr<data::AbstractData>>& sets_B_data,
                 hsne_type::Parameters hsne_params_A,
                 hsne_type::Parameters hsne_params_B)
    {
      checkAndThrowLogic(sets_A->size()!=0,"Set A be must not be empty");
      checkAndThrowLogic(sets_B->size()!=0,"Set B be must not be empty");
      checkAndThrowLogic(sets_A_data.size()==sets_A->size(),"sets_A_data.size()==sets_A.size()");
      checkAndThrowLogic(sets_B_data.size()==sets_B->size(),"sets_B_data.size()==sets_B.size()");

      utils::secureLog(_logger,"Initializing WAOW-Vis");
      _sets_A = sets_A;
      _sets_B = sets_B;

      _sets_A_data = sets_A_data;
      _sets_B_data = sets_B_data;

      utils::secureLog(_logger,"Computing histograms...");
      //Computing histograms
      _histogram_set_A    = hdi::data::Histogram<scalar_type>(0,sets_B->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_set_B    = hdi::data::Histogram<scalar_type>(0,sets_A->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_unique_set_A = hdi::data::Histogram<scalar_type>(0,sets_B->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_unique_set_B = hdi::data::Histogram<scalar_type>(0,sets_A->size(),std::min<uint32_t>(sets_B->size(),100));

      int edges = 0;
      for(const auto& set_A: *sets_A){
        edges += set_A.cardinality();
        _histogram_set_A.add(set_A.cardinality());
      }
      for(const auto& set_B: *sets_B){
        _histogram_set_B.add(set_B.cardinality());
      }

      utils::secureLogValue(_logger,"#Sets A",sets_A->size());
      utils::secureLogValue(_logger,"#Sets B",sets_B->size());
      utils::secureLogValue(_logger,"#Edges",edges);

      utils::secureLog(_logger,"Processing Sets A...");
      {
        sparse_scalar_matrix_type fmc_A;
        preprocessSets(*_sets_A,_unique_pnts_A,_num_duplicates_A,fmc_A);
        {//just to be sure
          std::ofstream file("fmca.sparse",std::ios::out|std::ios::binary);
          if(file.is_open()){
            hdi::data::IO::saveSparseMatrix(fmc_A,file);
          }
        }
        computeHSNE(fmc_A,_num_duplicates_A,_hSNE_set_A);
        computeInfluence(_hSNE_set_A,_pnts_in_landmarks_A, _pnts_to_landmarks_A);
      }

      std::vector<uint32_t> followers;
      for(auto set: *sets_B){
        followers.push_back(set.cardinality());
      }
      utils::secureLog(_logger,"Processing Sets B...");
      {
        sparse_scalar_matrix_type fmc_B;
        preprocessSets(*_sets_B,_unique_pnts_B,_num_duplicates_B,fmc_B);
        computeHSNE(fmc_B,followers,_hSNE_set_B);
        computeInfluence(_hSNE_set_B,_pnts_in_landmarks_B, _pnts_to_landmarks_B);
      }

      for(const auto id_set_A: _unique_pnts_A){
        _histogram_unique_set_A.add((*sets_A)[id_set_A].cardinality());
      }
      for(const auto id_set_B: _unique_pnts_B){
        _histogram_unique_set_B.add((*sets_B)[id_set_B].cardinality());
      }
    }

    void WAOWVis::initialize(std::shared_ptr<sets_type> sets_A,
                 std::shared_ptr<sets_type> sets_B,
                 const std::vector<std::shared_ptr<data::AbstractData>>& sets_A_data,
                 const std::vector<std::shared_ptr<data::AbstractData>>& sets_B_data,
                 const std::string& directory)
    {
      checkAndThrowLogic(sets_A->size()!=0,"Set A be must not be empty");
      checkAndThrowLogic(sets_B->size()!=0,"Set B be must not be empty");
      checkAndThrowLogic(sets_A_data.size()==sets_A->size(),"sets_A_data.size()==sets_A.size()");
      checkAndThrowLogic(sets_B_data.size()==sets_B->size(),"sets_B_data.size()==sets_B.size()");

      utils::secureLog(_logger,"Initializing WAOW-Vis");
      int edges = 0;
      _sets_A = sets_A;
      _sets_B = sets_B;

      _sets_A_data = sets_A_data;
      _sets_B_data = sets_B_data;

      utils::secureLog(_logger,"Computing histograms...");
      //Computing histograms
      _histogram_set_A    = hdi::data::Histogram<scalar_type>(0,sets_B->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_set_B    = hdi::data::Histogram<scalar_type>(0,sets_A->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_unique_set_A = hdi::data::Histogram<scalar_type>(0,sets_B->size(),std::min<uint32_t>(sets_B->size(),100));
      _histogram_unique_set_B = hdi::data::Histogram<scalar_type>(0,sets_A->size(),std::min<uint32_t>(sets_B->size(),100));

      for(const auto& set_A: *sets_A){
        edges += set_A.cardinality();
        _histogram_set_A.add(set_A.cardinality());
      }
      for(const auto& set_B: *sets_B){
        _histogram_set_B.add(set_B.cardinality());
      }
      std::vector<uint32_t> followers;
      for(auto set: *sets_B){
        followers.push_back(set.cardinality());
      }

      utils::secureLogValue(_logger,"#Sets A",sets_A->size());
      utils::secureLogValue(_logger,"#Sets B",sets_B->size());
      utils::secureLogValue(_logger,"#Edges",edges);

      {
        utils::secureLog(_logger,"Loading WAOW from Disk...");
        load(directory);
      }
      //HACK PAPER -> to fix
      for(int i = 0; i < _unique_pnts_B.size(); ++i){
        _hSNE_set_B.scale(0)._landmark_weight[i] = followers[_unique_pnts_B[i]];
      }

      //TEMP PAPER -> option and main computation
      {

        auto& tmat = _hSNE_set_B.scale(0)._transition_matrix;
        double perplexity = tmat.size()/20.;//TEMP
        double p_mult = 3;
        for(int j = 0; j < tmat.size(); ++j){
          //KNN
          std::priority_queue<std::pair<scalar_type,uint32_t>> knn;
          for(auto& e: tmat[j]){
            knn.push(std::pair<scalar_type,uint32_t>(e.second,e.first));
          }

          std::vector<uint32_t>  id(perplexity*p_mult);
          std::vector<scalar_type> distances(perplexity*p_mult);
          std::vector<scalar_type> distribution;

          for(int i = 0; i < perplexity*p_mult; ++i){
            distances[i] = 1-knn.top().first;
            id[i] = knn.top().second;
            knn.pop();
          }
          distribution.resize(distances.size());
          utils::computeGaussianDistributionWithFixedPerplexity<std::vector<scalar_type>>(distances.begin(),
                                      distances.end(),
                                      distribution.begin(),
                                      distribution.end(),
                                      perplexity);
          tmat[j].clear();
          for(int i = 0; i < perplexity*3; ++i){
            tmat[j][id[i]] = distribution[i];
          }
        }

      }
      for(const auto id_set_A: _unique_pnts_A){
        _histogram_unique_set_A.add((*sets_A)[id_set_A].cardinality());
      }
      for(const auto id_set_B: _unique_pnts_B){
        _histogram_unique_set_B.add((*sets_B)[id_set_B].cardinality());
      }
    }

    void WAOWVis::save(const std::string& directory)const{
      {
        QDir q_dir(directory.c_str());
        if(q_dir.exists()){
          q_dir.removeRecursively();
        }
        QDir().mkdir(directory.c_str());
      }

      {//HSNE A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("HSNEA.hsne").toStdString();
        std::ofstream file_hsne_A(f_name,std::ios::out|std::ios::binary);
        if(!file_hsne_A.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::dr::IO::saveHSNE(_hSNE_set_A,file_hsne_A);
      }
      {//HSNE B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("HSNEB.hsne").toStdString();
        std::ofstream file_hsne_B(f_name,std::ios::out|std::ios::binary);
        if(!file_hsne_B.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::dr::IO::saveHSNE(_hSNE_set_B,file_hsne_B);
      }
      {//Unique A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("UniqueA.uintv").toStdString();
        std::ofstream file_unique_A(f_name,std::ios::out|std::ios::binary);
        if(!file_unique_A.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVector(_unique_pnts_A,file_unique_A);
      }
      {//Unique B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("UniqueB.uintv").toStdString();
        std::ofstream file_unique_B(f_name,std::ios::out|std::ios::binary);
        if(!file_unique_B.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVector(_unique_pnts_B,file_unique_B);
      }
      {//Duplicates A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("DuplicatesA.uintv").toStdString();
        std::ofstream file_duplicates_A(f_name,std::ios::out|std::ios::binary);
        if(!file_duplicates_A.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVector(_num_duplicates_A,file_duplicates_A);
      }
      {//Duplicates B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("DuplicatesB.uintv").toStdString();
        std::ofstream file_duplicates_B(f_name,std::ios::out|std::ios::binary);
        if(!file_duplicates_B.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVector(_num_duplicates_B,file_duplicates_B);
      }
      {//Pnts2Landmarks A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("Pnts2LandmarksA.uintvv").toStdString();
        std::ofstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVectorVector(_pnts_to_landmarks_A,file);
      }
      {//Pnts2Landmarks B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("Pnts2LandmarksB.uintvv").toStdString();
        std::ofstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveUIntVectorVector(_pnts_to_landmarks_B,file);
      }
      {//PntsInLandmarks A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("PntsInLandmarksA.uintvv").toStdString();
        std::ofstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveRoaringVectorVector(_pnts_in_landmarks_A,file);
      }
      {//PntsInLandmarks B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("PntsInLandmarksB.uintvv").toStdString();
        std::ofstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("can't save %1").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::saveRoaringVectorVector(_pnts_in_landmarks_B,file);
      }
    }


    void WAOWVis::load(const std::string& directory){
      {
        QDir q_dir(directory.c_str());
        if(!q_dir.exists()){
          throw std::runtime_error(QString("Directory %1 does not exists").arg(directory.c_str()).toStdString());
        }
      }
      {//HSNE A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("HSNEA.hsne").toStdString();
        std::ifstream file_hsne_A(f_name,std::ios::in|std::ios::binary);
        if(!file_hsne_A.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::dr::IO::loadHSNE(_hSNE_set_A,file_hsne_A);
      }
      {//HSNE B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("HSNEB.hsne").toStdString();
        std::ifstream file_hsne_B(f_name,std::ios::in|std::ios::binary);
        if(!file_hsne_B.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::dr::IO::loadHSNE(_hSNE_set_B,file_hsne_B);
      }
      {//Unique A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("UniqueA.uintv").toStdString();
        std::ifstream file_unique_A(f_name,std::ios::in|std::ios::binary);
        if(!file_unique_A.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVector(_unique_pnts_A,file_unique_A);
      }
      {//Unique B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("UniqueB.uintv").toStdString();
        std::ifstream file_unique_B(f_name,std::ios::in|std::ios::binary);
        if(!file_unique_B.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVector(_unique_pnts_B,file_unique_B);
      }
      {//Duplicates A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("DuplicatesA.uintv").toStdString();
        std::ifstream file_duplicates_A(f_name,std::ios::in|std::ios::binary);
        if(!file_duplicates_A.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVector(_num_duplicates_A,file_duplicates_A);
      }
      {//Duplicates B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("DuplicatesB.uintv").toStdString();
        std::ifstream file_duplicates_B(f_name,std::ios::in|std::ios::binary);
        if(!file_duplicates_B.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVector(_num_duplicates_B,file_duplicates_B);
      }
      {//Pnts2Landmarks A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("Pnts2LandmarksA.uintvv").toStdString();
        std::ifstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVectorVector(_pnts_to_landmarks_A,file);
      }
      {//Pnts2Landmarks B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("Pnts2LandmarksB.uintvv").toStdString();
        std::ifstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadUIntVectorVector(_pnts_to_landmarks_B,file);
      }
      {//PntsInLandmarks A
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("PntsInLandmarksA.uintvv").toStdString();
        std::ifstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadRoaringVectorVector(_pnts_in_landmarks_A,file);
      }
      {//PntsInLandmarks B
        auto f_name = QString("%1/%2").arg(QString::fromStdString(directory)).arg("PntsInLandmarksB.uintvv").toStdString();
        std::ifstream file(f_name,std::ios::out|std::ios::binary);
        if(!file.is_open()){
          throw std::runtime_error(QString("%1 cannot be loaded").arg(f_name.c_str()).toStdString());
        }
        hdi::data::IO::loadRoaringVectorVector(_pnts_in_landmarks_B,file);
      }
    }



    void WAOWVis::preprocessSets(const sets_type& sets,
                   std::vector<uint32_t>& unique_pnts,
                   std::vector<uint32_t>& num_duplicates,
                   sparse_scalar_matrix_type& fmc)const
    {
      double time;
      utils::secureLog(_logger,"\tBuilding tree...");
      data::SetIntersectionTree tree;
      {
        utils::ScopedTimer<double> timer(time);
        tree.build(sets);
      }
      utils::secureLogValue(_logger,"\tdone in (msec)",time);
      utils::secureLogValue(_logger,"\t\tmax_depth",tree.max_depth());
      utils::secureLogValue(_logger,"\t\tnum_nodes",tree.num_nodes());
      utils::secureLogValue(_logger,"\t\ttot_num_duplicates",tree.tot_num_duplicates());
      utils::secureLogValue(_logger,"\t\tunique points",sets.size() - tree.tot_num_duplicates());

      utils::secureLog(_logger,"\tIdentifying unique pnts and repetitions...");
      {
        utils::ScopedTimer<double> timer(time);
        tree.getNonDuplicatedPoints(unique_pnts,num_duplicates);
      }
      utils::secureLogValue(_logger,"\tdone in (msec)",time);
/*
      //To improve search times
      int n_reduced = 11135000; //TODO
      if(unique_pnts.size() > n_reduced){//TODO
        std::random_shuffle(unique_pnts.begin(),unique_pnts.end());
        unique_pnts.resize(n_reduced);
      }
*/

      std::map<uint64_t,uint64_t> unique_inv_map;
      utils::secureLog(_logger,"\tComputing inverse mapping...");
      {
        utils::ScopedTimer<double> timer(time);
        int BUUUUUG = 0;
        for(int i = 0; i < unique_pnts.size(); ++i){
          auto v = unique_pnts[i];
          if(unique_inv_map.find(v) == unique_inv_map.end()){
            unique_inv_map[v] = i;
          }else{
            ++BUUUUUG;
          }
        }
        std::cout << "BUUUUUG " << BUUUUUG << "!" << std::endl;
      }
      utils::secureLogValue(_logger,"\tdone in (msec)",time);

      fmc.resize(unique_pnts.size());

      uint32_t n_nearest_neighbors = 30; //TODO
      utils::secureLog(_logger,"\tComputing similarities...");
      {
        if(fmc.size() > n_nearest_neighbors * 10){ //TODO
          hdi::utils::ScopedTimer<double> timer(time);
          //load balancing
          std::vector<uint32_t> knn_idxes(fmc.size());
          std::iota(knn_idxes.begin(),knn_idxes.end(),0);
          std::random_shuffle(knn_idxes.begin(),knn_idxes.end());
  #pragma omp parallel for
          for(int i = 0; i < fmc.size(); ++i){
            std::vector<double> distances;//external allocation?
            std::vector<uint32_t> res_idxes;
            tree.searchJaccardKNN(sets[unique_pnts[knn_idxes[i]]],n_nearest_neighbors,distances,res_idxes);
            std::map<uint32_t,scalar_type> temp_fmc;
            double sum = 0;
            for(int j = 0; j < distances.size(); ++j){
              auto it = unique_inv_map.find(res_idxes[j]);
              if(it != unique_inv_map.end()){
                if(distances[j] != 1){
                  temp_fmc[it->second] = 1-distances[j];
                  sum += 1-distances[j];
                }
              }
            }
            for(auto& v: temp_fmc){
              v.second /= sum;
            }
            typedef data::MapHelpers<uint32_t,scalar_type,sparse_scalar_matrix_type::value_type> map_helper;
            map_helper::initialize(fmc[knn_idxes[i]],temp_fmc.begin(),temp_fmc.end());
          }
        }else{
          for(int i = 0; i < fmc.size(); ++i){
            for(int j = i+1; j < fmc.size(); ++j){
              scalar_type jaccard = sets[unique_pnts[i]].jaccard_index(sets[unique_pnts[j]]);
              fmc[i][j] = jaccard;
              fmc[j][i] = jaccard;
            }
          }
          //normalization
          for(int i = 0; i < fmc.size(); ++i){
            scalar_type sum = 0;
            for(int j = 0; j < fmc.size(); ++j){
              sum += fmc[j][i];
            }
            if(sum > 0){
              for(int j = 0; j < fmc.size(); ++j){
                fmc[j][i] /= sum;
              }
            }
          }
        }
      }
      utils::secureLogValue(_logger,"\tdone in (msec)",time);
    }


    void WAOWVis::computeHSNE(const sparse_scalar_matrix_type& fmc,
                  const std::vector<uint32_t>& num_duplicates,
                  hsne_type& hsne)const
    {
      utils::secureLog(_logger,"\tComputing HSNE...");
      double time;
      {
        utils::ScopedTimer<double> timer(time);
        hsne.setLogger(_logger);
        hsne.initialize(fmc);
        //changing the weights according to the duplicates
        assert(hsne.scale(0)._landmark_weight.size() == num_duplicates.size());
        for(int i = 0; i < num_duplicates.size(); ++i){
          hsne.scale(0)._landmark_weight[i] = num_duplicates[i];
        }
        while(hsne.top_scale()._transition_matrix.size() > 3000){//TODO
          hsne.addScale();
        }
      }
      utils::secureLogValue(_logger,"\tHSNE computationt time", time);
    }

    void WAOWVis::computeInfluence(const hsne_type& hsne, std::vector<std::vector<Roaring>>& pnts_in_landmarks, std::vector<std::vector<uint32_t>>& pnts_to_landmarks)const{
      utils::secureLog(_logger,"\tComputing AoI...");
      const int n_p = hsne.scale(0).size();
      const int n_s = hsne.hierarchy().size();
      assert(n_s>0);
      pnts_in_landmarks.resize(n_s);
      pnts_to_landmarks.resize(n_s);
      for(int s = 0; s < n_s; ++s){
        pnts_in_landmarks[s].resize(hsne.scale(s).size());
        pnts_to_landmarks[s].resize(n_p);
      }
      if(n_s > 1){
        for(int i = 0; i < n_p; ++i){
          std::vector<std::unordered_map<uint32_t,scalar_type>> influence;
          hsne.getInfluenceOnDataPoint(i,influence,0.0005);
          for(int s = 0; s < n_s; ++s){
            int max_id = -1;
            scalar_type max_value = 0;
            for(auto& v: influence[s]){
              if(v.second > max_value){
                max_value = v.second;
                max_id = v.first;
              }
            }
            pnts_in_landmarks[s][max_id].add(i);
            pnts_to_landmarks[s][i] = max_id;
          }
        }
      }else{
        for(int i = 0; i < n_p; ++i){
          pnts_in_landmarks[0][i].add(i);
          pnts_to_landmarks[0][i] = i;
        }
      }
    }

    void WAOWVis::connectView(embedder_type* embedder){
      connect(embedder,&WAOWVisSingleViewEmbedder::sgnCreateNewView, this,&WAOWVis::onCreateNewView);
      connect(embedder,&WAOWVisSingleViewEmbedder::sgnSelection, this,&WAOWVis::onSelection);
    }

    void WAOWVis::createOverview(){

      utils::secureLog(_logger,"Generating the overview");
      checkAndThrowLogic(_multiscale_views.size()==0,"An overview is already initialized!");
      //checkAndThrowLogic(_interface_initializer!=nullptr,"interface initializer is not set");

      embedder_id_type id = _multiscale_views.size();
      _multiscale_views.push_back(std::make_shared<embedder_type>());

      std::shared_ptr<embedder_type>& view = _multiscale_views[id];
      view->_scale_A         = _hSNE_set_A.hierarchy().size()-1;
      view->_scale_B         = _hSNE_set_B.hierarchy().size()-1;
      view->_my_id         = id;
      view->_parent_id       = id;
      view->_landmark_weights_A  = _hSNE_set_A.top_scale()._landmark_weight;
      view->_landmark_weights_B  = _hSNE_set_B.top_scale()._landmark_weight;

      view->_scale_idxes_A.resize(_hSNE_set_A.top_scale().size());
      view->_scale_idxes_B.resize(_hSNE_set_B.top_scale().size());
      std::iota(view->_scale_idxes_A.begin(),view->_scale_idxes_A.end(),0);
      std::iota(view->_scale_idxes_B.begin(),view->_scale_idxes_B.end(),0);

      view->_selection_A.resize(_hSNE_set_A.top_scale().size());
      view->_selection_B.resize(_hSNE_set_B.top_scale().size());
      std::fill(view->_selection_A.begin(),view->_selection_A.end(),0);
      std::fill(view->_selection_B.begin(),view->_selection_B.end(),0);

      view->setLogger(_logger);
      {//Panel data A
        auto& panel_data_A    = view->getPanelDataA();
        const auto& orig_idx  = _hSNE_set_A.top_scale()._landmark_to_original_data_idx;

        //Initializing panel data
        panel_data_A.initializeWithEmptyDimensions(1);
        std::vector<scalar_type> dummy_data(1,0);
        for(auto id: orig_idx){
          assert(id < _unique_pnts_A.size());
          assert(_unique_pnts_A[id] < _sets_A_data.size());
          panel_data_A.addDataPoint(_sets_A_data[_unique_pnts_A[id]],dummy_data);
        }

      }
      {//Panel data B
        auto& panel_data_B    = view->getPanelDataB();
        const auto& orig_idx  = _hSNE_set_B.top_scale()._landmark_to_original_data_idx;

        panel_data_B.initializeWithEmptyDimensions(1);
        std::vector<scalar_type> dummy_data(1,0);
        for(auto id: orig_idx){
          assert(id < _unique_pnts_B.size());
          assert(_unique_pnts_B[id] < _sets_B_data.size());

          panel_data_B.addDataPoint(_sets_B_data[_unique_pnts_B[id]],dummy_data);
        }
      }
      {//connections
        computeConnections(*view);
      }

      view->initialize(_hSNE_set_A.top_scale()._transition_matrix,
                     _hSNE_set_B.top_scale()._transition_matrix,
                     id);
      _interface_initializer->initialize(view);

      connectView(view.get());
    }

    void WAOWVis::computeConnectionsGreedy(embedder_type& embedder)const{
      //Greedy
      utils::secureLog(_logger,"Computing connections...");

      //used for mapping the points to the landmakrs
      std::unordered_map<uint32_t,uint32_t> bitmap_to_scale_idx_A;
      std::unordered_map<uint32_t,uint32_t> bitmap_to_scale_idx_B;

      for(int i = 0; i < embedder._scale_idxes_A.size(); ++i){
        int id_scale  = embedder._scale_idxes_A[i];
        int id_hsne   = _hSNE_set_A.scale(embedder._scale_A)._landmark_to_original_data_idx[id_scale];
        int id_bitmap = _unique_pnts_A[id_hsne];
        bitmap_to_scale_idx_A[id_bitmap] = i;
      }

      for(int i = 0; i < embedder._scale_idxes_B.size(); ++i){
        int id_scale  = embedder._scale_idxes_B[i];
        int id_hsne   = _hSNE_set_B.scale(embedder._scale_B)._landmark_to_original_data_idx[id_scale];
        int id_bitmap = _unique_pnts_B[id_hsne];
        bitmap_to_scale_idx_B[id_bitmap] = i;
      }


      {//Computing the connectivity
        std::vector<std::unordered_map<uint32_t,uint32_t>>& connection_AB = embedder._connections_AB;
        std::vector<std::unordered_map<uint32_t,uint32_t>>& connection_BA = embedder._connections_BA;
        std::vector<scalar_type>& incoming_connections_A = embedder._incoming_connections_A;
        std::vector<scalar_type>& incoming_connections_B = embedder._incoming_connections_B;

        connection_AB.resize(embedder.getPanelDataA().numDataPoints());
        connection_BA.resize(embedder.getPanelDataB().numDataPoints());
        incoming_connections_A.resize(embedder.getPanelDataA().numDataPoints());
        incoming_connections_B.resize(embedder.getPanelDataB().numDataPoints());

        for(auto a: bitmap_to_scale_idx_A){
          for(auto l: (*_sets_A)[a.first]){
            auto it = bitmap_to_scale_idx_B.find(l);
            if(it!=bitmap_to_scale_idx_B.end()){
              ++connection_AB[a.second][it->second];
              ++connection_BA[it->second][a.second];
              ++incoming_connections_A[a.second];
              ++incoming_connections_B[it->second];
            }
          }
        }
      }
    }


    void WAOWVis::computeConnections(embedder_type& embedder)const{
      //Greedy
      utils::secureLog(_logger,"Computing connections...");

      //used for mapping the points to the landmakrs
      std::unordered_map<uint32_t,uint32_t> scale_id_to_view_id_A;
      std::unordered_map<uint32_t,uint32_t> scale_id_to_view_id_B;

      const uint32_t scale_A = embedder._scale_A;
      const uint32_t scale_B = embedder._scale_B;

      for(int i = 0; i < embedder._scale_idxes_A.size(); ++i){
        int id_scale  = embedder._scale_idxes_A[i];
        scale_id_to_view_id_A[id_scale] = i;
      }

      for(int i = 0; i < embedder._scale_idxes_B.size(); ++i){
        int id_scale  = embedder._scale_idxes_B[i];
        scale_id_to_view_id_B[id_scale] = i;
      }

      {//Computing the connectivity
        std::vector<std::unordered_map<uint32_t,uint32_t>>& connection_AB = embedder._connections_AB;
        std::vector<std::unordered_map<uint32_t,uint32_t>>& connection_BA = embedder._connections_BA;
        std::vector<scalar_type>& incoming_connections_A = embedder._incoming_connections_A;
        std::vector<scalar_type>& incoming_connections_B = embedder._incoming_connections_B;

        connection_AB.resize(embedder.getPanelDataA().numDataPoints());
        connection_BA.resize(embedder.getPanelDataB().numDataPoints());
        incoming_connections_A.resize(embedder.getPanelDataA().numDataPoints());
        incoming_connections_B.resize(embedder.getPanelDataB().numDataPoints());

        //for each landmark in the scale
        for(auto a: scale_id_to_view_id_A){
          //for each bitmap in the landmark
          const auto& bitmap_idx = _pnts_in_landmarks_A[scale_A][a.first];
          for(const auto bitmap_id: bitmap_idx){
            uint32_t unique_id = _unique_pnts_A[bitmap_id];
            //I look at the connections
            for(auto l: (*_sets_A)[unique_id]){
              //from the link I get the data point in B
              int32_t dp_id = -1;
              for(int i = 0; i < _unique_pnts_B.size(); ++i){
                if(_unique_pnts_B[i] == l){
                  dp_id = i;
                }
              }

              if(dp_id != -1){
                //and get the landmark at scale B
                const uint32_t land_B = _pnts_to_landmarks_B[scale_B][dp_id];
                //I check if it is in the view
                auto it = scale_id_to_view_id_B.find(land_B);
                if(it!=scale_id_to_view_id_B.end()){
//                  connection_AB[a.second][it->second] += embedder._landmark_weights_A[a.second];
//                  connection_BA[it->second][a.second] += embedder._landmark_weights_A[a.second];
//                  incoming_connections_A[a.second]  += embedder._landmark_weights_A[a.second];
//                  incoming_connections_B[it->second]  += embedder._landmark_weights_A[a.second];

//                  connection_AB[a.second][it->second] += std::sqrt(embedder._landmark_weights_A[a.second]);
//                  connection_BA[it->second][a.second] += std::sqrt(embedder._landmark_weights_A[a.second]);
//                  incoming_connections_A[a.second]  += std::sqrt(embedder._landmark_weights_A[a.second]);
//                  incoming_connections_B[it->second]  += std::sqrt(embedder._landmark_weights_A[a.second]);

                  ++connection_AB[a.second][it->second];
                  ++connection_BA[it->second][a.second];
                  ++incoming_connections_A[a.second];
                  ++incoming_connections_B[it->second];
                }
              }
            }
          }
        }
      }
    }

    void WAOWVis::doAnIterateOnAllViews(){
      for(auto& view: _multiscale_views){
        view->doAnIteration();
      }
    }


    void WAOWVis::setVisualizationMode(const std::string& visualization_mode){
      utils::secureLogValue(_logger,"Setting visualization mode",visualization_mode);
      _visualization_mode = visualization_mode;
      onUpdateViews();
    }

    void WAOWVis::onUpdateViews(){
      for(auto& views: _multiscale_views){
        views->setVisualizationMode(_visualization_mode);
        views->onUpdateCanvas();
      }
    }

    void WAOWVis::saveImagesToFile(std::string prefix){
      for(auto& view: _multiscale_views){
        view->saveImageToFile(prefix);
      }
    }

/////////////////////////////////////////////////////////////////////////////////////////

    void WAOWVis::onCreateNewView(embedder_id_type id){
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"Creating a new View");

      const std::shared_ptr<embedder_type>& trigger_view = _multiscale_views[id];

      //collecting selections in the current scale for both sets
      std::vector<uint32_t> selection_A;
      std::vector<uint32_t> selection_B;
      trigger_view->getSelectionA(selection_A);
      trigger_view->getSelectionB(selection_B);

      //collecting the selection in the HSNE scale
      std::vector<uint32_t> selection_scale_A(selection_A);
      std::vector<uint32_t> selection_scale_B(selection_B);
      for(auto& v: selection_scale_A){v = trigger_view->_scale_idxes_A[v];}
      for(auto& v: selection_scale_B){v = trigger_view->_scale_idxes_B[v];}


      utils::secureLogValue(_logger,"# of selected points on A",selection_A.size());
      utils::secureLogValue(_logger,"# of selected points on B",selection_B.size());


      //if no data point is selected there is no need for drilling
      if(selection_A.size()){
        utils::secureLog(_logger,"Drilling-in for sets A");
        _multiscale_views.push_back(std::make_shared<embedder_type>());
        std::shared_ptr<embedder_type>& view = _multiscale_views[_multiscale_views.size()-1];
        const std::shared_ptr<embedder_type>& trigger_view = _multiscale_views[id];//TODO

        view->_scale_A         = trigger_view->_scale_A-1;
        view->_scale_B         = trigger_view->_scale_B;
        view->_my_id         = _multiscale_views.size()-1;
        view->_parent_id       = id;

        std::map<unsigned int, scalar_type> influenced_pnts;
        _hSNE_set_A.getInfluencedLandmarksInPreviousScale(trigger_view->_scale_A,selection_scale_A,influenced_pnts);
        std::vector<unsigned int> to_be_selected;
        for(auto pnt: influenced_pnts){
          if(pnt.second > 0.5){//TODO
            to_be_selected.push_back(pnt.first);
          }
        }
        hsne_type::sparse_scalar_matrix_type fmc_A;
        utils::extractSubGraph(_hSNE_set_A.scale(view->_scale_A)._transition_matrix,to_be_selected,fmc_A,view->_scale_idxes_A,1);
        for(auto pnt: view->_scale_idxes_A){
          view->_landmark_weights_A.push_back(_hSNE_set_A.scale(view->_scale_A)._landmark_weight[pnt]);
        }


        utils::secureLogValue(_logger,"# of points in the new view (A)",view->_scale_idxes_A.size());
        utils::secureLogValue(_logger,"# of points in the new view (B)",view->_scale_idxes_B.size());

        view->setLogger(_logger);
        {//Panel data A
          auto& panel_data_A    = view->getPanelDataA();
          const auto& orig_idx  = _hSNE_set_A.scale(view->_scale_A)._landmark_to_original_data_idx;

          panel_data_A.initializeWithEmptyDimensions(1);
          std::vector<scalar_type> dummy_data(1,0);
          for(auto id: view->_scale_idxes_A){
            panel_data_A.addDataPoint(_sets_A_data[_unique_pnts_A[id]],dummy_data);
          }
          view->_selection_A.resize(panel_data_A.numDataPoints());
          view->_selection_B.resize(panel_data_A.numDataPoints());
          std::fill(view->_selection_A.begin(),view->_selection_A.end(),0);
          std::fill(view->_selection_B.begin(),view->_selection_B.end(),0);
        }

        bool drill_in_B = false;
        //TODO
        {//Panel data B
          const double sel_thresh = 0.3;
          for(int i = 0; i < trigger_view->_selection_B.size() && !drill_in_B; ++i){
            if(trigger_view->_selection_B[i] > sel_thresh){
              drill_in_B = true;
            }
          }
          if(drill_in_B){
            auto& panel_data_B    = view->getPanelDataB();
            const auto& orig_idx  = _hSNE_set_B.scale(trigger_view->_scale_B)._landmark_to_original_data_idx;

            panel_data_B.initializeWithEmptyDimensions(1);
            std::vector<scalar_type> dummy_data(1,0);
            for(int i = 0; i < trigger_view->_selection_B.size(); ++i){
              if(trigger_view->_selection_B[i] > sel_thresh){
                auto id = orig_idx[trigger_view->_scale_idxes_B[i]];
                panel_data_B.addDataPoint(_sets_B_data[_unique_pnts_B[id]],dummy_data); // scale is missing
                view->_landmark_weights_B.push_back(trigger_view->_landmark_weights_B[i]);
                view->_scale_idxes_B   .push_back(trigger_view->_scale_idxes_B[i]);
              }
            }
          }else{
            view->_landmark_weights_B  = trigger_view->_landmark_weights_B;
            view->_scale_idxes_B     = trigger_view->_scale_idxes_B;

            auto& panel_data_B    = view->getPanelDataB();
            const auto& orig_idx  = _hSNE_set_B.top_scale()._landmark_to_original_data_idx;

            panel_data_B.initializeWithEmptyDimensions(1);
            std::vector<scalar_type> dummy_data(1,0);
            for(auto id: orig_idx){
              panel_data_B.addDataPoint(_sets_B_data[_unique_pnts_B[id]],dummy_data);
            }
          }
        }
        {//connections
          computeConnections(*view);
        }

        if(!drill_in_B){
          view->initialize(fmc_A,
                     _hSNE_set_B.top_scale()._transition_matrix, //TODO
                     view->_my_id);
        }else{
          sparse_scalar_matrix_type fmc_B(view->_scale_idxes_B.size());
          for(int i = 0; i < fmc_B.size(); ++i){
            for(int j = i+1; j < fmc_B.size(); ++j){
              scalar_type jaccard = (*_sets_B)[_hSNE_set_B.scale(view->_scale_B)._landmark_to_original_data_idx[view->_scale_idxes_B[i]]].
                  jaccard_index((*_sets_B)[_hSNE_set_B.scale(view->_scale_B)._landmark_to_original_data_idx[view->_scale_idxes_B[j]]]);
              fmc_B[i][j] = jaccard;
              fmc_B[j][i] = jaccard;
            }
          }
          for(int i = 0; i < fmc_B.size(); ++i){
            scalar_type sum = 0;
            for(int j = 0; j < fmc_B.size(); ++j){
              sum += fmc_B[j][i];
            }
            if(sum > 0){
              for(int j = 0; j < fmc_B.size(); ++j){
                fmc_B[j][i] /= sum;
              }
            }
          }
          view->initialize(fmc_A,
                     fmc_B, //TODO
                     view->_my_id);

        }
        _interface_initializer->initialize(view);

        {//Panel data B
          const auto& orig_idx  = _hSNE_set_B.top_scale()._landmark_to_original_data_idx;
          const auto D1_trigger = trigger_view->getEmbeddingB1D().getContainer().data();
          const auto D2_trigger = trigger_view->getEmbeddingB2D().getContainer().data();
          auto D1_new       = view    ->getEmbeddingB1D().getContainer().data();
          auto D2_new       = view    ->getEmbeddingB2D().getContainer().data();
          for(auto id: orig_idx){
            D1_new[id]    = D1_trigger[id];
            D2_new[id*2+0]  = D2_trigger[id*2+0];
            D2_new[id*2+1]  = D2_trigger[id*2+1];
          }
          view->gettSNEB1D().setIteration(1500);
          view->gettSNEB2D().setIteration(1500);
        }

        connectView(view.get());

      }else{
        utils::secureLog(_logger,"Nothing to do");
      }
      //TODO B
    }


    void WAOWVis::onSelection(embedder_id_type id){

    }



//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////  Interface initializer  ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////




    void WAOWVisDefaultInterface::initialize(std::shared_ptr<embedder_type> embedder){
      initializeDefault(embedder);
      initializeSimple(embedder);
      initializeWeights(embedder);
    }
    void WAOWVisDefaultInterface::initializeDefault(std::shared_ptr<embedder_type> embedder){
      const std::vector<scalar_type>&   weights_A = embedder->_landmark_weights_A;
      const std::vector<scalar_type>&   weights_B = embedder->_landmark_weights_B;

      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_A_2D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_A_2D->setData(embedder->getEmbeddingA2DView().getContainer().data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
      drawer_A_2D->setAlpha(0.15);
      drawer_A_2D->setPointSize(10);
      embedder->addDrawer("Default","A_2D",drawer_A_2D);


      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_A_1D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_A_1D->setData(embedder->getEmbeddingA1DView().getContainer().data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
      drawer_A_1D->setAlpha(0.05);
      drawer_A_1D->setPointSize(10);
      embedder->addDrawer("Default","A_1D",drawer_A_1D);

      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_B_2D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_B_2D->setData(embedder->getEmbeddingB2DView().getContainer().data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
      drawer_B_2D->setAlpha(0.15);
      drawer_B_2D->setPointSize(10);
      embedder->addDrawer("Default","B_2D",drawer_B_2D);

      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_B_1D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_B_1D->setData(embedder->getEmbeddingB1DView().getContainer().data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
      drawer_B_1D->setAlpha(0.05);
      drawer_B_1D->setPointSize(10);
      embedder->addDrawer("Default","B_1D",drawer_B_1D);

      std::shared_ptr<hdi::viz::TextView> text_view_B = std::make_shared<hdi::viz::TextView>();
      text_view_B->setPanelData(&(embedder->getPanelDataB()));
      text_view_B->updateView();
      text_view_B->show();
      embedder->addDataView("Default","B_text",text_view_B);
    }


    void WAOWVisDefaultInterface::initializeSimple(std::shared_ptr<embedder_type> embedder){
      const std::vector<scalar_type>&   weights_A = embedder->_landmark_weights_A;
      const std::vector<scalar_type>&   weights_B = embedder->_landmark_weights_B;

      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_A_2D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_A_2D->setData(embedder->getEmbeddingA2DView().getContainer().data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
      drawer_A_2D->setColor(qRgb(150,0,0));
      drawer_A_2D->setAlpha(0.15);
      drawer_A_2D->setPointSize(10);
      embedder->addDrawer("Simple","A_2D",drawer_A_2D);

      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer_B_2D = std::make_shared<hdi::viz::ScatterplotDrawerFixedColor>();
      drawer_B_2D->setData(embedder->getEmbeddingB2DView().getContainer().data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
      drawer_B_2D->setColor(qRgb(150,0,0));
      drawer_B_2D->setAlpha(0.15);
      drawer_B_2D->setPointSize(10);
      embedder->addDrawer("Simple","B_2D",drawer_B_2D);
    }

    void WAOWVisDefaultInterface::initializeWeights(std::shared_ptr<embedder_type> embedder){
      const std::vector<scalar_type>&   weights_A = embedder->_landmark_weights_A;
      const std::vector<scalar_type>&   weights_B = embedder->_landmark_weights_B;

      QColor color_A      (qRgb(0,100,160));
      QColor color_B      (qRgb(0,128,0));
      QColor selection_color  (qRgb(200,130,0));

      std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer_A_2D = std::make_shared<hdi::viz::ScatterplotDrawerScalarAttribute>();
      drawer_A_2D->setData(embedder->getEmbeddingA2DView().getContainer().data(), weights_A.data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
      drawer_A_2D->setAlpha(0.15);
      drawer_A_2D->setPointSize(50);
      drawer_A_2D->setMinPointSize(5);
      drawer_A_2D->updateLimitsFromData();
      drawer_A_2D->setColor(color_A);
      drawer_A_2D->setSelectionColor(selection_color);
      embedder->addDrawer("Weights","A_2D",drawer_A_2D);

      std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer_A_1D = std::make_shared<hdi::viz::ScatterplotDrawerScalarAttribute>();
      drawer_A_1D->setData(embedder->getEmbeddingA1DView().getContainer().data(), weights_A.data(), embedder->getPanelDataA().getFlagsDataPoints().data(), embedder->getPanelDataA().numDataPoints());
      drawer_A_1D->setAlpha(0.05);
      drawer_A_1D->setPointSize(50);
      drawer_A_1D->setMinPointSize(5);
      drawer_A_1D->updateLimitsFromData();
      drawer_A_1D->setColor(color_A);
      drawer_A_1D->setSelectionColor(selection_color);
      embedder->addDrawer("Weights","A_1D",drawer_A_1D);


      std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer_B_2D = std::make_shared<hdi::viz::ScatterplotDrawerScalarAttribute>();
      drawer_B_2D->setData(embedder->getEmbeddingB2DView().getContainer().data(), weights_B.data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
      drawer_B_2D->setAlpha(0.3);
      drawer_B_2D->setPointSize(35);
      drawer_B_2D->setMinPointSize(7);
      drawer_B_2D->updateLimitsFromData();
      drawer_B_2D->setColor(color_B);
      drawer_B_2D->setSelectionColor(selection_color);
      embedder->addDrawer("Weights","B_2D",drawer_B_2D);

      std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer_B_1D = std::make_shared<hdi::viz::ScatterplotDrawerScalarAttribute>();
      drawer_B_1D->setData(embedder->getEmbeddingB1DView().getContainer().data(), weights_B.data(), embedder->getPanelDataB().getFlagsDataPoints().data(), embedder->getPanelDataB().numDataPoints());
      drawer_B_1D->setAlpha(0.2);
      drawer_B_1D->setPointSize(35);
      drawer_B_1D->setMinPointSize(7);
      drawer_B_1D->updateLimitsFromData();
      drawer_B_1D->setColor(color_B);
      drawer_B_1D->setSelectionColor(selection_color);
      embedder->addDrawer("Weights","B_1D",drawer_B_1D);

      std::shared_ptr<hdi::viz::EmbeddingLinesDrawer> lines_A_B = std::make_shared<hdi::viz::EmbeddingLinesDrawer>();
      lines_A_B->setData(embedder->getEmbeddingA1DView().getContainer().data(), embedder->getEmbeddingB1DView().getContainer().data());
      lines_A_B->setAlpha(0.2);
      lines_A_B->setColor(selection_color);

      std::vector<std::pair<uint32_t,uint32_t>> lines;
      lines.push_back(std::pair<uint32_t,uint32_t>(0,0));
      lines.push_back(std::pair<uint32_t,uint32_t>(0,1));
      lines.push_back(std::pair<uint32_t,uint32_t>(0,2));
      lines.push_back(std::pair<uint32_t,uint32_t>(1,3));
      lines.push_back(std::pair<uint32_t,uint32_t>(1,4));
      lines.push_back(std::pair<uint32_t,uint32_t>(1,5));
      lines_A_B->setLines(lines);
      embedder->addDrawer("Weights","Lines_A_B",lines_A_B);

    }



  }
}
