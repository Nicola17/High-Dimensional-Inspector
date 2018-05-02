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

#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/graph_algorithms.h"
#include <numeric>
#include <stdint.h>
#include <iostream>
#include <fstream>

namespace hdi{
  namespace analytics{

    MultiscaleEmbedderSystem::MultiscaleEmbedderSystem():
      _logger(nullptr),
      _interface_initializer(nullptr),
      _selection_linked_to_data_points(false),
      _verbose(false),
      _name("HSNE_Analysis")
    {}

    void MultiscaleEmbedderSystem::initialize(unsigned int num_scales, hsne_type::Parameters rw_params){
      checkAndThrowLogic(_panel_data.numDataPoints()!=0,"Panel data must not be empty");
      checkAndThrowLogic(num_scales>0,"At least one scale must be requested");

      _hSNE.setLogger(_logger);
      _hSNE.setDimensionality(_panel_data.numDimensions());
      _hSNE.initialize(_panel_data.getData().data(),_panel_data.numDataPoints(),rw_params);
      _hSNE.statistics().log(_logger);
      for(int i = 0; i < num_scales-1; ++i){
        _hSNE.addScale();
      }

      _multiscale_analysis.resize(num_scales);
      _analysis_counter.resize(num_scales,0);
      _clusters.resize(num_scales);

      _sankey_diagram.setLogger(_logger);
      _sankey_diagram.setVerbose(true);
      //_sankey_diagram.show();
      _sankey_diagram.resize(QSize(750,350));
    }

    void MultiscaleEmbedderSystem::initializeWithMaxPoints(unsigned int num_points, hsne_type::Parameters rw_params){
      checkAndThrowLogic(_panel_data.numDataPoints()!=0,"Panel data must not be empty");
      checkAndThrowLogic(num_points>0,"At least one scale must be requested");

      _hSNE.setLogger(_logger);
      _hSNE.setDimensionality(_panel_data.numDimensions());
      _hSNE.initialize(_panel_data.getData().data(),_panel_data.numDataPoints(),rw_params);
      _hSNE.statistics().log(_logger);
      while(_hSNE.hierarchy()[_hSNE.hierarchy().size()-1].size() > num_points){
        _hSNE.addScale();
      }

      unsigned int num_scales = _hSNE.hierarchy().size();
      _multiscale_analysis.resize(num_scales);
      _analysis_counter.resize(num_scales,0);
      _clusters.resize(num_scales);

      _sankey_diagram.setLogger(_logger);
      _sankey_diagram.setVerbose(true);
      //_sankey_diagram.show();
      _sankey_diagram.resize(QSize(750,350));
    }

    void MultiscaleEmbedderSystem::initializeFromFile(std::string filename){
      checkAndThrowLogic(_panel_data.numDataPoints()!=0,"Panel data must not be empty");

      std::ifstream in_file(filename.c_str(), std::ios::in|std::ios::binary);
      checkAndThrowRuntime(in_file.is_open(),"Unable to open file in MultiscaleEmbedderSystem::initializeFromFile");

      dr::IO::loadHSNE(_hSNE,in_file,_logger);
      unsigned int num_scales = _hSNE.hierarchy().size();
      checkAndThrowRuntime(num_scales > 0,"Empty hierarchy in MultiscaleEmbedderSystem::initializeFromFile");
      checkAndThrowRuntime(_hSNE.scale(0).size() == _panel_data.numDataPoints(),"num of data points in H-SNE and in the panel data should agree (MultiscaleEmbedderSystem::initializeFromFile)");

      _multiscale_analysis.resize(num_scales);
      _analysis_counter.resize(num_scales,0);
      _clusters.resize(num_scales);

      _sankey_diagram.setLogger(_logger);
      _sankey_diagram.setVerbose(true);
      //_sankey_diagram.show();
      _sankey_diagram.resize(QSize(750,350));
    }

    void MultiscaleEmbedderSystem::connectEmbedder(embedder_type* embedder){
      connect(embedder,&MultiscaleEmbedderSingleView::sgnNewAnalysisTriggered,this,&MultiscaleEmbedderSystem::onNewAnalysisTriggered);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateUserDefinedMode,this,&MultiscaleEmbedderSystem::onActivateUserDefinedMode);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateSelectionMode,this,&MultiscaleEmbedderSystem::onActivateSelectionMode);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateInfluenceMode,this,&MultiscaleEmbedderSystem::onActivateInfluenceMode);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnPropagateSelection,this,&MultiscaleEmbedderSystem::onPropagateSelection);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnClusterizeSelection,this,&MultiscaleEmbedderSystem::onClusterizeSelection);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnExport,this,&MultiscaleEmbedderSystem::onExport);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnKeyPressedOnCanvas,this,&MultiscaleEmbedderSystem::sgnKeyPressedOnCanvas);
      connect(embedder,&MultiscaleEmbedderSingleView::sgnKeyPressedOnCanvas,this,&MultiscaleEmbedderSystem::sgnKeyPressedOnCanvas );

      connect(embedder,&MultiscaleEmbedderSingleView::sgnSelection,this,&MultiscaleEmbedderSystem::onSelection);
    }

    MultiscaleEmbedderSystem::embedder_type& MultiscaleEmbedderSystem::getEmbedder(embedder_id_type id){
      return *(_multiscale_analysis[std::get<0>(id)][std::get<1>(id)]._embedder);
    }
    const MultiscaleEmbedderSystem::embedder_type& MultiscaleEmbedderSystem::getEmbedder(embedder_id_type id)const{
      return *(_multiscale_analysis[std::get<0>(id)][std::get<1>(id)]._embedder);
    }

    void MultiscaleEmbedderSystem::createTopLevelEmbedder(){
      utils::secureLog(_logger,"Generating the top level embedder");
      checkAndThrowLogic(_interface_initializer!=nullptr,"interdace initializer not set");

      unsigned int top_level_id = _multiscale_analysis.size()-1;

      _multiscale_analysis[top_level_id].push_back(Analysis());
      checkAndThrowLogic(_multiscale_analysis[top_level_id].size()==1,"Top level is already initialized!");

      Analysis& analysis = _multiscale_analysis[top_level_id][0];
      analysis._my_id = embedder_id_type(top_level_id,0);
      analysis._parent_id = embedder_id_type(top_level_id,0);

      data::newPanelDataFromIndexes(_panel_data, analysis._embedder->getPanelData(), _hSNE.scale(top_level_id)._landmark_to_original_data_idx);
      analysis._embedder->setLogger(_logger);

      const unsigned int size = _hSNE.scale(top_level_id)._transition_matrix.size();
      analysis._scale_idxes.resize(size);
      std::iota(analysis._scale_idxes.begin(),analysis._scale_idxes.end(),0);
      analysis._embedder->initialize(_hSNE.scale(top_level_id)._transition_matrix,embedder_id_type(top_level_id,_analysis_counter[top_level_id]));
      ++_analysis_counter[top_level_id];

      analysis._landmark_weight = _hSNE.scale(top_level_id)._landmark_weight;
      analysis._selection.resize(size,0);

      _interface_initializer->initializeStandardVisualization(analysis._embedder.get(),_hSNE.scale(top_level_id)._landmark_to_original_data_idx);
      _interface_initializer->initializeInfluenceVisualization(analysis._embedder.get(),_hSNE.scale(top_level_id)._landmark_to_original_data_idx,analysis._landmark_weight.data());
      _interface_initializer->initializeSelectionVisualization(analysis._embedder.get(),_hSNE.scale(top_level_id)._landmark_to_original_data_idx,analysis._landmark_weight.data());
      connectEmbedder(analysis._embedder.get());

      //the first cluster is a special one and represent all the datapoints on screen
      if(_clusters[top_level_id].size() == 0){
        _clusters[top_level_id].push_back(data::Cluster());
        for(int i = 0; i < size; ++i){
          _clusters[top_level_id][0].add(i);
        }
      }

      visualizeTheFlow();
    }

    void MultiscaleEmbedderSystem::createFullScaleEmbedder(unsigned int scale_id){
      utils::secureLogValue(_logger,"Generating full scale embedder",scale_id);
      checkAndThrowLogic(_interface_initializer!=nullptr,"interdace initializer not set");

      _multiscale_analysis[scale_id].push_back(Analysis());

      Analysis& analysis = _multiscale_analysis[scale_id][0];
      data::newPanelDataFromIndexes(_panel_data, analysis._embedder->getPanelData(), _hSNE.scale(scale_id)._landmark_to_original_data_idx);
      analysis._embedder->setLogger(_logger);


      const unsigned int size = _hSNE.scale(scale_id)._transition_matrix.size();
      analysis._scale_idxes.resize(size);
      std::iota(analysis._scale_idxes.begin(),analysis._scale_idxes.end(),0);
      analysis._embedder->initialize(_hSNE.scale(scale_id)._transition_matrix,embedder_id_type(scale_id,_analysis_counter[scale_id]));
      ++_analysis_counter[scale_id];
      analysis._landmark_weight = _hSNE.scale(scale_id)._landmark_weight;
      analysis._selection.resize(size,0);

      _interface_initializer->initializeStandardVisualization(analysis._embedder.get(),_hSNE.scale(scale_id)._landmark_to_original_data_idx);
      _interface_initializer->initializeInfluenceVisualization(analysis._embedder.get(),_hSNE.scale(scale_id)._landmark_to_original_data_idx,analysis._landmark_weight.data());
      _interface_initializer->initializeSelectionVisualization(analysis._embedder.get(),_hSNE.scale(scale_id)._landmark_to_original_data_idx,analysis._landmark_weight.data());
      connectEmbedder(analysis._embedder.get());

      visualizeTheFlow();
    }

    void MultiscaleEmbedderSystem::clusterizeSelection(embedder_id_type id, QColor color) {
      utils::secureLog(_logger, "\n-----------------------------------------");
      utils::secureLog(_logger, "Cluster creation");

      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id, scale_id, analysis_id);

      utils::secureLogValue(_logger, "\tScale", scale_id);
      utils::secureLogValue(_logger, "\tAnalysis idx", analysis_id);

      Analysis& analysis = _multiscale_analysis[scale_id][analysis_id];
      std::vector<unsigned int> idxes_selected_landmarks_in_scale;
      std::vector<unsigned int> idxes_selected_landmarks_in_analysis;
      getSelectionInTheAnalysis(analysis, idxes_selected_landmarks_in_analysis);
      getSelectionInTheScale(analysis, idxes_selected_landmarks_in_scale);
      utils::secureLogValue(_logger, "\t#selected landmarks", idxes_selected_landmarks_in_scale.size());

      {//Clusters
        _clusters[scale_id].push_back(data::Cluster(color));
        //there is something on screen already... I add the new one
        for (auto id : idxes_selected_landmarks_in_scale){
          for (int s = 0; s < _clusters[scale_id].size() - 1; ++s){
            if (_clusters[scale_id][s].contains(id)){
              _clusters[scale_id][s].remove(id);
            }
          }
          _clusters[scale_id][_clusters[scale_id].size() - 1].add(id);
        }
      }
    }

    void MultiscaleEmbedderSystem::doAnIterateOnAllEmbedder(){
      for(auto& scale: _multiscale_analysis){
        for(auto& analysis: scale){
          analysis._embedder->doAnIteration();
        }
      }
    }

    void MultiscaleEmbedderSystem::getScaleAndAnalysisId(embedder_id_type id, unsigned int& scale_id, unsigned int& analysis_id)const{
      scale_id = std::get<0>(id);
      checkAndThrowLogic(scale_id < _multiscale_analysis.size(),"Invalid scale");
      analysis_id = -1;
      for(int i = 0; i < _multiscale_analysis[scale_id].size(); ++i){
        auto current_id = _multiscale_analysis[scale_id][i]._embedder->getId();
        if(std::get<1>(id) == std::get<1>(current_id)){
          analysis_id = i;
          break;
        }
      }
      checkAndThrowLogic(analysis_id != -1,"Analysis not found!");
    }

    void MultiscaleEmbedderSystem::onUpdateViews(){
      for(auto& s: _multiscale_analysis){
        for(auto& a: s){
          a._embedder->onUpdateViewer();
        }
      }
    }

    void MultiscaleEmbedderSystem::onActivateUserDefinedMode(embedder_id_type id){
      utils::secureLog(_logger,"Activate user defined mode");
      for(auto& s: _multiscale_analysis){
        for(auto& a: s){
          a._embedder->onActivateUserDefinedMode();
        }
      }
    }

    void MultiscaleEmbedderSystem::onActivateSelectionMode(embedder_id_type id){
      utils::secureLog(_logger,"Activate selection mode");
      for(auto& s: _multiscale_analysis){
        for(auto& a: s){
          a._embedder->onActivateSelectionMode();
        }
      }
    }

    void MultiscaleEmbedderSystem::onActivateInfluenceMode(embedder_id_type id){
      utils::secureLog(_logger,"Activate influence mode");
      for(auto& s: _multiscale_analysis){
        for(auto& a: s){
          a._embedder->onActivateInfluencedMode();
        }
      }
    }

    void MultiscaleEmbedderSystem::getSelectionInTheAnalysis(const analysis_type& analysis, std::vector<unsigned int>& selection)const{
      const auto& flags = analysis._embedder->getPanelData().getFlagsDataPoints();
      for(int i = 0; i < flags.size(); ++i){
        if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
          selection.push_back(i);
        }
      }
    }
    void MultiscaleEmbedderSystem::getSelectionInTheScale(const analysis_type& analysis, std::vector<unsigned int>& selection)const{
      const auto& flags = analysis._embedder->getPanelData().getFlagsDataPoints();
      for(int i = 0; i < flags.size(); ++i){
        if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
          selection.push_back(analysis._scale_idxes[i]);
        }
      }
    }
    void MultiscaleEmbedderSystem::getSelectionInTheData(const analysis_type& analysis, std::vector<unsigned int>& selection)const{
      const auto& flags = analysis._embedder->getPanelData().getFlagsDataPoints();
      for(int i = 0; i < flags.size(); ++i){
        if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
          auto in_scale_id = analysis._scale_idxes[i];
          auto scale_id = std::get<0>(analysis._embedder->getId());
          selection.push_back(_hSNE.scale(scale_id)._landmark_to_original_data_idx[in_scale_id]);
        }
      }
    }

    void MultiscaleEmbedderSystem::saveImagesToFile(std::string prefix){
      for(auto& scale: _multiscale_analysis){
        for(auto& analysis: scale){
          analysis._embedder->saveImageToFile(prefix);
        }
      }
    }

    void MultiscaleEmbedderSystem::visualizeTheFlow(){
  //      typedef typename flow_model_type::flow_type flow_type;
//      typedef typename flow_model_type::node_type node_type;

/*
      const unsigned int node_per_level = 10000;
      unsigned int link_id = 0;
      _flow = flow_model_type();
      _flow.addNode(node_type(0,"NonViz",qRgb(20,20,20)));

      unsigned int scale  = 0;
      for(scale = 0; scale < _multiscale_analysis.size(); ++scale){
        if(_multiscale_analysis[scale].size()!=0){
          break;
        }
        _flow.addNode(node_type(node_per_level*(scale+1),"NonViz",qRgb(20,20,20)));
        _flow.addFlow(flow_type(link_id,node_per_level*(scale),node_per_level*(scale+1),1,qRgb(20,20,20)));
        ++link_id;
      }
      for(; scale < _multiscale_analysis.size(); ++scale){

        _flow.addNode(node_type(node_per_level*(scale+1),"NonClust",qRgb(50,20,20)));
        _flow.addFlow(flow_type(link_id,node_per_level*(scale),node_per_level*(scale+1),1,qRgb(20,20,20)));
        ++link_id;
      }
      */

      //_hSNE.flowBetweenClusters(_clusters,_flow);
      //_sankey_diagram.visualizeFlow(_flow);
    }

/////////////////////////////////////////////////////////////////////////////////////////

    void MultiscaleEmbedderSystem::onNewAnalysisTriggered(embedder_id_type id){
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"New Analysis triggered");

      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id,scale_id,analysis_id);
      checkAndThrowLogic(scale_id > 0,"Invalid scale");


      utils::secureLogValue(_logger,"\tScale",scale_id);
      utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id);

      {
        unsigned int new_scale_id = scale_id-1;
        _multiscale_analysis[new_scale_id].push_back(Analysis());
        unsigned int new_analysis_id  = _multiscale_analysis[new_scale_id].size()-1;

            Analysis& new_analysis = _multiscale_analysis[new_scale_id][new_analysis_id];
        const Analysis& analysis   = _multiscale_analysis[scale_id][analysis_id];

        new_analysis._my_id = embedder_id_type(new_scale_id,new_analysis_id);
        new_analysis._parent_id = embedder_id_type(scale_id,analysis_id);

        //New data for the analysis
        hsne_type::sparse_scalar_matrix_type new_transition_matrix;
        std::vector<unsigned int> new_landmarks_orig_data;
        std::vector<unsigned int> idxes_selected_landmarks;
        {
          //Compute from the selected elements to the original scale
          getSelectionInTheAnalysis(analysis,new_analysis._parent_selection);
          getSelectionInTheScale(analysis,idxes_selected_landmarks);
          utils::secureLogValue(_logger,"\t#selected landmarks", idxes_selected_landmarks.size());

          //Given the selected landmarks at the previous scale, I compute a set of neighbors using random walks
          std::map<unsigned int, scalar_type> neighbors;
          _hSNE.getInfluencedLandmarksInPreviousScale(scale_id,idxes_selected_landmarks,neighbors);
          std::vector<unsigned int> landmarks_to_add_prev_scale;
          for(auto n: neighbors){
            //if(n.second > 0.3) //QUICKPAPER
            if(n.second > 0.5)
              landmarks_to_add_prev_scale.push_back(n.first);
          }
          utils::secureLogValue(_logger,"\t#landmarks on the manifold at previous scale", neighbors.size());

          //Given new landmarks, I extract a transition matrix
          utils::extractSubGraph(_hSNE.scale(new_scale_id)._transition_matrix,landmarks_to_add_prev_scale,new_transition_matrix,new_analysis._scale_idxes,1);
          utils::secureLogValue(_logger,"\t#landmarks in the subgraph", new_analysis._scale_idxes.size());

          for(auto& e: new_analysis._scale_idxes){
            new_landmarks_orig_data.push_back(_hSNE.scale(new_scale_id)._landmark_to_original_data_idx[e]);
          }
        }


        //Using the original panel data I extract the new data points
        data::newPanelDataFromIndexes(_panel_data, new_analysis._embedder->getPanelData(), new_landmarks_orig_data);

        //... and the area of influence
        {
          new_analysis._landmark_weight.reserve(new_analysis._scale_idxes.size());
          for(auto id:  new_analysis._scale_idxes){
            new_analysis._landmark_weight.push_back(_hSNE.scale(new_scale_id)._landmark_weight[id]);
          }
        }
        //... and the selection
        {
          new_analysis._selection.resize(new_analysis._scale_idxes.size(),0);
        }

        new_analysis._embedder->setLogger(_logger);
        new_analysis._embedder->initialize(new_transition_matrix,embedder_id_type(new_scale_id,_analysis_counter[new_scale_id]));
        ++_analysis_counter[new_scale_id];

        _interface_initializer->initializeStandardVisualization(new_analysis._embedder.get(),new_landmarks_orig_data);
        _interface_initializer->initializeInfluenceVisualization(new_analysis._embedder.get(),new_landmarks_orig_data,new_analysis._landmark_weight.data());
        _interface_initializer->initializeSelectionVisualization(new_analysis._embedder.get(),new_landmarks_orig_data,new_analysis._landmark_weight.data());
        connectEmbedder(new_analysis._embedder.get());

        {//Clusters
          if(_clusters[new_scale_id].size() == 0){
            //the first cluster is a special one and represent all the datapoints on screen
            //I initialize it with all the landmarks in the embedding
            _clusters[new_scale_id].push_back(data::Cluster());
          }
          //there is something on screen already... I add the new one
          for(auto id: new_analysis._scale_idxes){
            bool is_not_on_screen = true;
            for(int s = 1; s < _clusters[new_scale_id].size(); ++s){
              if(_clusters[new_scale_id][s].contains(id)){
                is_not_on_screen = false;
                break;
              }
            }
            if(is_not_on_screen){
              _clusters[new_scale_id][0].add(id);
            }
          }
        }

      }
      visualizeTheFlow();
    }


    void MultiscaleEmbedderSystem::onPropagateSelection(embedder_id_type id){
      onLinkSelectionToDataPoints(id);
    }

    void MultiscaleEmbedderSystem::onLinkSelectionToDataPoints(embedder_id_type id){
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"Link selection to DataPoints");

      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id,scale_id,analysis_id);

      utils::secureLogValue(_logger,"\tScale",scale_id);
      utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id);

      Analysis& analysis =_multiscale_analysis[scale_id][analysis_id];
      std::vector<unsigned int> idxes_selected_landmarks_in_scale;
      getSelectionInTheScale(analysis,idxes_selected_landmarks_in_scale);
      utils::secureLogValue(_logger,"\t#selected landmarks", idxes_selected_landmarks_in_scale.size());

      std::vector<scalar_type> aoi;
      _hSNE.getAreaOfInfluence(scale_id,idxes_selected_landmarks_in_scale,aoi);
      _interface_initializer->dataPointSelectionChanged(aoi);
    }

    void MultiscaleEmbedderSystem::getSelectedLandmarksInScale(embedder_id_type id, std::vector<unsigned int>& selection)const{
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"Link selection to DataPoints");

      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id,scale_id,analysis_id);

      utils::secureLogValue(_logger,"\tScale",scale_id);
      utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id);

      const Analysis& analysis =_multiscale_analysis[scale_id][analysis_id];
      getSelectionInTheScale(analysis,selection);
      utils::secureLogValue(_logger,"\t#selected landmarks", selection.size());
    }

    const std::vector<data::Cluster>& MultiscaleEmbedderSystem::getClustersInScale(unsigned int scale_id) const {
      return _clusters[scale_id];
    }

    void MultiscaleEmbedderSystem::onSelection(embedder_id_type id){
      utils::secureLog(_logger,"\n-----------------------------------------",_verbose);
      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id,scale_id,analysis_id);
      utils::secureLog(_logger,"Selection: performing dim analysis",_verbose);
      utils::secureLogValue(_logger,"\tScale",scale_id,_verbose);
      utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id,_verbose);
      Analysis& analysis =_multiscale_analysis[scale_id][analysis_id];
      std::vector<unsigned int> idxes_selected_data;
      getSelectionInTheData(analysis,idxes_selected_data);

      const int n_dp = _panel_data.numDataPoints();
      const int n_dim = _panel_data.numDimensions();
      auto& flags = _panel_data.getFlagsDataPoints();
      for(int i = 0; i < n_dp; ++i){
        flags[i] &= ~panel_data_type::Selected;
      }
      for(auto id: idxes_selected_data){
        flags[id] |= panel_data_type::Selected;
      }

      bool has_dim_avg = _panel_data.hasDimProperty("selected_avg");
      bool has_dim_min = _panel_data.hasDimProperty("selected_min");
      bool has_dim_max = _panel_data.hasDimProperty("selected_max");
      bool has_dim_std_dev = _panel_data.hasDimProperty("selected_std_dev");
      if(has_dim_avg || has_dim_min || has_dim_max){
        std::vector<scalar_type> min(n_dim,std::numeric_limits<scalar_type>::max());
        std::vector<scalar_type> max(n_dim,-std::numeric_limits<scalar_type>::max());
        std::vector<scalar_type> avg(n_dim,0);
        std::vector<scalar_type> std_dev(n_dim,0);
        int selected = 0;

        for(int i = 0; i < n_dp; ++i){
          if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
            for(int d = 0; d < n_dim; ++d){
              auto v = _panel_data.dataAt(i,d);
              min[d] = std::min<scalar_type>(v,min[d]);
              max[d] = std::max<scalar_type>(v,max[d]);
              avg[d] += v;
              std_dev[d] += v*v;
            }
            ++selected;
          }
        }

        if(selected){
          for(int d = 0; d < n_dim; ++d){
            avg[d] /= selected;
            std_dev[d] /= selected;
            std_dev[d] = std::sqrt(std_dev[d]-avg[d]*avg[d]);
          }

          if(has_dim_avg){
            auto& avg_property = _panel_data.getDimProperty("selected_avg");
            for(int d = 0; d < n_dim; ++d){
              avg_property[d] = avg[d];
            }
          }
          if(has_dim_min){
            auto& min_property = _panel_data.getDimProperty("selected_min");
            for(int d = 0; d < n_dim; ++d){
              min_property[d] = min[d];
            }
          }
          if(has_dim_max){
            auto& max_property = _panel_data.getDimProperty("selected_max");
            for(int d = 0; d < n_dim; ++d){
              max_property[d] = max[d];
            }
          }
          if(has_dim_std_dev){
            auto& std_dev_property = _panel_data.getDimProperty("selected_std_dev");
            for(int d = 0; d < n_dim; ++d){
              std_dev_property[d] = std_dev[d];
            }
          }
        }

      }
    }

    void MultiscaleEmbedderSystem::onClusterizeSelection(embedder_id_type id) {
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"Cluster creation");

      unsigned int scale_id = 0;
      unsigned int analysis_id = 0;
      getScaleAndAnalysisId(id,scale_id,analysis_id);

      utils::secureLogValue(_logger,"\tScale",scale_id);
      utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id);

      Analysis& analysis =_multiscale_analysis[scale_id][analysis_id];
      std::vector<unsigned int> idxes_selected_landmarks_in_scale;
      std::vector<unsigned int> idxes_selected_landmarks_in_analysis;
      getSelectionInTheAnalysis(analysis,idxes_selected_landmarks_in_analysis);
      getSelectionInTheScale(analysis,idxes_selected_landmarks_in_scale);
      utils::secureLogValue(_logger,"\t#selected landmarks", idxes_selected_landmarks_in_scale.size());

      {//Clusters
        _clusters[scale_id].push_back(data::Cluster());
        //there is something on screen already... I add the new one
        for(auto id: idxes_selected_landmarks_in_scale){
          for(int s = 0; s < _clusters[scale_id].size()-1; ++s){
            if(_clusters[scale_id][s].contains(id)){
              _clusters[scale_id][s].remove(id);
            }
          }
          _clusters[scale_id][_clusters[scale_id].size()-1].add(id);
        }
      }
      visualizeTheFlow();
    }

    void MultiscaleEmbedderSystem::onExport(embedder_id_type id){
      utils::secureLog(_logger,"\n-----------------------------------------");
      utils::secureLog(_logger,"Exporting analysis");
      IO::exportToCSV(_hSNE,_multiscale_analysis,_name);
    }






#include <QDir>
#include <iostream>
#include <fstream>

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////  IO  //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

    namespace IO{
      void exportToCSV(const MultiscaleEmbedderSystem::hsne_type& hsne, const MultiscaleEmbedderSystem::multiscale_analysis_type& analysis, std::string folder){
        if(QDir(QString::fromStdString(folder)).exists()){
          QDir(QString::fromStdString(folder)).removeRecursively();
        }
        QDir().mkdir(QString::fromStdString(folder));

        std::vector<QString> scale_names;
        //create a directory for each scale
        for(int s = 0; s < analysis.size(); ++s){
          scale_names.push_back(QString("./%1/S_%2").arg(QString::fromStdString(folder)).arg(s));
        }
        for(int s = 0; s < analysis.size(); ++s){
          QDir().mkdir(scale_names[s]);
          for(int a = 0; a < analysis[s].size(); ++a){
            exportToCSV(hsne, analysis[s][a],scale_names[s].toStdString());
          }
        }
      }

      void exportToCSV(const MultiscaleEmbedderSystem::hsne_type& hsne, const MultiscaleEmbedderSystem::analysis_type& analysis, std::string folder){
        auto scale_id = std::get<0>(analysis._my_id);
        auto analysis_id = std::get<1>(analysis._my_id);

        QString my_folder_name(QString("%1/A_%2_%3").arg(QString::fromStdString(folder)).arg(scale_id).arg(analysis_id));
        QDir().mkdir(my_folder_name);

        {
          //info.txt -> link to parent,size
          std::ofstream file;
          file.open (QString("%1/info.txt").arg(my_folder_name).toStdString());
          if(analysis._my_id == analysis._parent_id){
            file << "Parent:\tN/A" << std::endl;
          }else{
            file << "Parent:\tA_"<< std::get<0>(analysis._parent_id) << "_" << std::get<1>(analysis._parent_id) << std::endl;
          }
          file << "Size:\t" << analysis._scale_idxes.size();
          file.close();
        }
        {
          //embedding.csv -> x,y,w,id
          std::ofstream file;
          file.open(QString("%1/embedding.csv").arg(my_folder_name).toStdString());
          const auto& embedding = analysis._embedder->getEmbedding();
          file << "x,y,w,id" << std::endl;
          for(int i = 0; i < analysis._landmark_weight.size(); ++i){
            file << embedding.dataAt(i,0) << ", ";
            file << embedding.dataAt(i,1) << ", ";
            file << hsne.scale(scale_id)._landmark_weight[analysis._landmark_weight[i]] << ", ";
            file << hsne.scale(scale_id)._landmark_to_original_data_idx[analysis._scale_idxes[i]] << std::endl;
          }
          file.close();
        }
        {
          //parent_selection.csv -> id
          std::ofstream file;
          file << "analysis id" << std::endl;
          file.open (QString("%1/parent_selection.csv").arg(my_folder_name).toStdString());
          for(int i = 0; i < analysis._parent_selection.size(); ++i){
            file << analysis._parent_selection[i] << std::endl;
          }
          file.close();
        }
      }
    }

  }
}
