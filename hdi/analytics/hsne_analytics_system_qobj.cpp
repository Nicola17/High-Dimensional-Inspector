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

#include "hsne_analytics_system_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/graph_algorithms.h"
#include <numeric>
#include <stdint.h>
#include <iostream>
#include <fstream>

namespace hdi{
    namespace analytics{

    HSNEAnalyticsSystem::HSNEAnalyticsSystem():
        _logger(nullptr),
        _factory(nullptr),
        _panel_data(nullptr),
        _initialized(false)
    {}

    void HSNEAnalyticsSystem::initialize(unsigned int num_scales, scalar_type* data, hsne_type::Parameters rw_params){
        checkAndThrowLogic(_factory != nullptr,"Invalid factory pointer");
        checkAndThrowLogic(_panel_data != nullptr,"Invalid panel data pointer");
        checkAndThrowLogic(_panel_data->numDataPoints()!=0,"Panel data must not be empty");
        checkAndThrowLogic(num_scales>0,"At least one scale must be requested");

        _hSNE.setLogger(_logger);
        _hSNE.setDimensionality(_panel_data->numDimensions());
        _hSNE.initialize(data,_panel_data->numDataPoints(),rw_params);
        _hSNE.statistics().log(_logger);
        for(int i = 0; i < num_scales-1; ++i){
            _hSNE.addScale();
            _hSNE.statistics().log(_logger);
        }

        _multiscale_analysis.resize(num_scales);
        _analysis_counter.resize(num_scales,0);

        _initialized = true;
    }

    void HSNEAnalyticsSystem::initializeFromFile(std::string filename){
        checkAndThrowLogic(_factory != nullptr,"Invalid factory pointer");
        checkAndThrowLogic(_panel_data != nullptr,"Invalid panel data pointer");
        checkAndThrowLogic(_panel_data->numDataPoints()!=0,"Panel data must not be empty");

        std::ifstream in_file(filename.c_str(), std::ios::in|std::ios::binary);
        checkAndThrowRuntime(in_file.is_open(),"Unable to open file in HSNEAnalyticsSystem::initializeFromFile");

        dr::IO::loadHSNE(_hSNE,in_file,_logger);
        unsigned int num_scales = _hSNE.hierarchy().size();
        checkAndThrowRuntime(num_scales > 0,"Empty hierarchy in HSNEAnalyticsSystem::initializeFromFile");
        checkAndThrowRuntime(_hSNE.scale(0).size() == _panel_data->numDataPoints(),"num of data points in H-SNE and in the panel data should agree (HSNEAnalyticsSystem::initializeFromFile)");

        _multiscale_analysis.resize(num_scales);
        _analysis_counter.resize(num_scales,0);

        _initialized = true;
    }

    void HSNEAnalyticsSystem::createTopLevelEmbedder(){
        utils::secureLog(_logger,"Generating the top level embedder");
        checkAndThrowLogic(_initialized,"HSNE analytics system must be initialized");
        checkAndThrowLogic(_factory != nullptr,"Invalid factory pointer");
        checkAndThrowLogic(_panel_data != nullptr,"Invalid panel data pointer");

        unsigned int top_level_id = _multiscale_analysis.size()-1;

        _multiscale_analysis[top_level_id].push_back(_factory->createEmbedder());
        checkAndThrowLogic(_multiscale_analysis[top_level_id].size()==1,"Top level is already initialized!");

        std::shared_ptr<embedder_type> embedder = _multiscale_analysis[top_level_id][0];
        embedder->setId(embedder_id_type(top_level_id,0));

        embedder->indicesInData() = _hSNE.scale(top_level_id)._landmark_to_original_data_idx;
        auto& idxes = embedder->indicesInScale();
        idxes.resize(_hSNE.scale(top_level_id)._transition_matrix.size());
        std::iota(idxes.begin(),idxes.end(),0);

        embedder->weights() = _hSNE.scale(top_level_id)._landmark_weight;
        embedder->initialize(_hSNE.scale(top_level_id)._transition_matrix);

        ++_analysis_counter[top_level_id];
        connectEmbedder(embedder.get());
    }

    void HSNEAnalyticsSystem::connectEmbedder(embedder_type* embedder){
    }

    void HSNEAnalyticsSystem::doAnIterateOnAllEmbedder(){
        for(auto& scale: _multiscale_analysis){
            for(auto& embedder: scale){
                embedder->onIterate();
            }
        }
    }

#if 0

        void HSNEAnalyticsSystem::connectEmbedder(embedder_type* embedder){
            connect(embedder,&MultiscaleEmbedderSingleView::sgnNewAnalysisTriggered,this,&HSNEAnalyticsSystem::onNewAnalysisTriggered);
            connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateUserDefinedMode,this,&HSNEAnalyticsSystem::onActivateUserDefinedMode);
            connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateSelectionMode,this,&HSNEAnalyticsSystem::onActivateSelectionMode);
            connect(embedder,&MultiscaleEmbedderSingleView::sgnActivateInfluenceMode,this,&HSNEAnalyticsSystem::onActivateInfluenceMode);
            connect(embedder,&MultiscaleEmbedderSingleView::sgnPropagateSelection,this,&HSNEAnalyticsSystem::onPropagateSelection);
            connect(embedder,&MultiscaleEmbedderSingleView::sgnClusterizeSelection,this,&HSNEAnalyticsSystem::onClusterizeSelection);
        }

        void HSNEAnalyticsSystem::createTopLevelEmbedder(){
            utils::secureLog(_logger,"Generating the top level embedder");
            checkAndThrowLogic(_interface_initializer!=nullptr,"interdace initializer not set");

            unsigned int top_level_id = _multiscale_analysis.size()-1;

            _multiscale_analysis[top_level_id].push_back(Analysis());
            checkAndThrowLogic(_multiscale_analysis[top_level_id].size()==1,"Top level is already initialized!");

            Analysis& analysis = _multiscale_analysis[top_level_id][0];
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
                _clusters[top_level_id].push_back(std::unordered_set<unsigned int>());
                for(int i = 0; i < size; ++i){
                    _clusters[top_level_id][0].insert(i);
                }
            }

            visualizeTheFlow();
        }

        void HSNEAnalyticsSystem::createFullScaleEmbedder(unsigned int scale_id){
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



        void HSNEAnalyticsSystem::getScaleAndAnalysisId(embedder_id_type id, unsigned int& scale_id, unsigned int& analysis_id){
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


        void HSNEAnalyticsSystem::onActivateUserDefinedMode(embedder_id_type id){
            utils::secureLog(_logger,"Activate user defined mode");
            for(auto& s: _multiscale_analysis){
                for(auto& a: s){
                    a._embedder->onActivateUserDefinedMode();
                }
            }
        }

        void HSNEAnalyticsSystem::onActivateSelectionMode(embedder_id_type id){
            utils::secureLog(_logger,"Activate selection mode");
            for(auto& s: _multiscale_analysis){
                for(auto& a: s){
                    a._embedder->onActivateSelectionMode();
                }
            }
        }

        void HSNEAnalyticsSystem::onActivateInfluenceMode(embedder_id_type id){
            utils::secureLog(_logger,"Activate influence mode");
            for(auto& s: _multiscale_analysis){
                for(auto& a: s){
                    a._embedder->onActivateInfluencedMode();
                }
            }
        }

        void HSNEAnalyticsSystem::getSelectionInTheAnalysis(const analysis_type& analysis, std::vector<unsigned int>& selection)const{
            const auto& flags = analysis._embedder->getPanelData().getFlagsDataPoints();
            for(int i = 0; i < flags.size(); ++i){
                if(flags[i] == panel_data_type::Selected){
                    selection.push_back(i);
                }
            }
        }
        void HSNEAnalyticsSystem::getSelectionInTheScale(const analysis_type& analysis, std::vector<unsigned int>& selection)const{
            const auto& flags = analysis._embedder->getPanelData().getFlagsDataPoints();
            for(int i = 0; i < flags.size(); ++i){
                if(flags[i] == panel_data_type::Selected){
                    selection.push_back(analysis._scale_idxes[i]);
                }
            }
        }

        void HSNEAnalyticsSystem::visualizeTheFlow(){
  //          typedef typename flow_model_type::flow_type flow_type;
//            typedef typename flow_model_type::node_type node_type;

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

        void HSNEAnalyticsSystem::onNewAnalysisTriggered(embedder_id_type id){
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
                const Analysis& analysis     = _multiscale_analysis[scale_id][analysis_id];

                //New data for the analysis
                typename hsne_type::sparse_scalar_matrix_type new_transition_matrix;
                std::vector<unsigned int> new_landmarks_orig_data;
                std::vector<unsigned int> idxes_selected_landmarks;
                {
                    //Compute from the selected elements to the original scale
                    getSelectionInTheScale(analysis,idxes_selected_landmarks);
                    utils::secureLogValue(_logger,"\t#selected landmarks", idxes_selected_landmarks.size());

                    //Given the selected landmarks at the previous scale, I compute a set of neighbors using random walks
                    std::map<unsigned int, scalar_type> neighbors;
                    _hSNE.getInfluencedLandmarksInPreviousScale(scale_id,idxes_selected_landmarks,neighbors);
                    std::vector<unsigned int> landmarks_to_add_prev_scale;
                    for(auto n: neighbors){
                        //if(n.second > 0.3) //QUICKPAPER
                        if(n.second > 0.5) //QUICKPAPER
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
                        _clusters[new_scale_id].push_back(std::unordered_set<unsigned int>());
                    }
                    //there is something on screen already... I add the new one
                    for(auto id: new_analysis._scale_idxes){
                        bool is_not_on_screen = true;
                        for(int s = 1; s < _clusters[new_scale_id].size(); ++s){
                            if(_clusters[new_scale_id][s].find(id)!=_clusters[new_scale_id][s].end()){
                                is_not_on_screen = false;
                                break;
                            }
                        }
                        if(is_not_on_screen){
                            _clusters[new_scale_id][0].insert(id);
                        }
                    }
                }

            }
            visualizeTheFlow();
        }


        void HSNEAnalyticsSystem::onPropagateSelection(embedder_id_type id){
            onLinkSelectionToDataPoints(id);
  #if 0

            utils::secureLog(_logger,"\n-----------------------------------------");
            utils::secureLog(_logger,"Propagate stochastic selection");

            unsigned int scale_id = 0;
            unsigned int analysis_id = 0;
            getScaleAndAnalysisId(id,scale_id,analysis_id);

            utils::secureLogValue(_logger,"\tScale",scale_id);
            utils::secureLogValue(_logger,"\tAnalysis idx", analysis_id);

            for(int s = 0; s < _multiscale_analysis.size(); ++s){
                for(int a = 0; a < _multiscale_analysis[s].size(); ++a){
                    _multiscale_analysis[s][a].resetStochasticSelection();
                }
            }

            Analysis& orig_analys =_multiscale_analysis[scale_id][analysis_id];
            std::vector<unsigned int> idxes_selected_landmarks_in_scale;
            std::vector<unsigned int> idxes_selected_landmarks_in_analysis;
            getSelectionInTheAnalysis(orig_analys,idxes_selected_landmarks_in_analysis);
            getSelectionInTheScale(orig_analys,idxes_selected_landmarks_in_scale);
            utils::secureLogValue(_logger,"\t#selected landmarks", idxes_selected_landmarks_in_scale.size());

            for(auto& id: idxes_selected_landmarks_in_analysis){
                orig_analys._selection[id] = 1./idxes_selected_landmarks_in_analysis.size();
            }
            _interface_initializer->updateSelection(orig_analys._embedder.get(),orig_analys._selection.data());

            for(int s = scale_id+1; s < _multiscale_analysis.size(); ++s){
                for(int a = 0; a < _multiscale_analysis[s].size(); ++a){
                    utils::secureLog(_logger,"Propagating to...");
                    utils::secureLogValue(_logger,"\tScale",s);
                    utils::secureLogValue(_logger,"\tAnalysis idx", a);

                    Analysis& dst_analys =_multiscale_analysis[s][a];

                    hsne_type::sparse_scalar_matrix location;
                    _hSNE.getStochasticLocationAtHigherScale(scale_id,s,idxes_selected_landmarks_in_scale,location);

                    hsne_type::sparse_scalar_matrix::value_type selection;
                    /*
                    {
                        scalar_type sum = 0;
                        for(int i = 0; i < location.size(); ++i){
                            for(auto& elem: location[i]){
                                selection[elem.first] += elem.second;
                                sum +=elem.second;
                            }
                        }
                        utils::secureLogValue(_logger,"sum",sum); //should be equal to location.size()
                        utils::secureLogValue(_logger,"selection size",selection.size());
                        for(auto& elem: selection){
                            elem.second /= sum;
                        }
                    }
                    */

                    {
                        scalar_type sum = 0;
                        for(int i = 0; i < location.size(); ++i){
                            for(auto& elem: location[i]){
                                selection[elem.first] += elem.second * orig_analys._landmark_weight[i];
                                sum +=elem.second * orig_analys._landmark_weight[i];
                            }
                        }
                        utils::secureLogValue(_logger,"sum",sum); //should be equal to location.size()
                        utils::secureLogValue(_logger,"selection size",selection.size());
                        for(auto& elem: selection){
                            elem.second /= sum;
                        }
                    }

                    for(int i = 0; i < dst_analys._scale_idxes.size(); ++i){
                        auto it = selection.find(dst_analys._scale_idxes[i]);
                        if(it!=selection.end()){
                            dst_analys._selection[i] = it->second;
                        }
                    }

                    utils::secureLog(_logger,"... done!");
                }
            }
#endif
        }

        void HSNEAnalyticsSystem::onLinkSelectionToDataPoints(embedder_id_type id){
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

            std::vector<scalar_type> selection(_hSNE.scale(0).size(),0);
            std::unordered_set<unsigned int> set_selected_idxes;
            set_selected_idxes.insert(idxes_selected_landmarks_in_scale.begin(),idxes_selected_landmarks_in_scale.end());

            if(scale_id == 0){
                for(int i = 0; i < idxes_selected_landmarks_in_scale.size(); ++i){
                    selection[idxes_selected_landmarks_in_scale[i]] = 1;
                }
            }else{
                int i = 0;
           #pragma omp parallel for
                for(i = 0; i < _hSNE.scale(0).size(); ++i){
                    sparse_scalar_matrix_type::value_type closeness = _hSNE.scale(1)._area_of_influence[i];
                    for(int s = 2; s <= scale_id; ++s){
                        sparse_scalar_matrix_type::value_type temp_link;
                        for(auto l: closeness){
                            for(auto new_l: _hSNE.scale(s)._area_of_influence[l.first]){
                                temp_link[new_l.first] += l.second * new_l.second;
                            }
                        }
                        closeness = temp_link;
                    }
                    for(auto e: closeness){
                        if(set_selected_idxes.find(e.first) != set_selected_idxes.end()){
                            selection[i] += e.second;
                        }
                    }
                }
            }
            _interface_initializer->dataPointSelectionChanged(selection);
        }

        void HSNEAnalyticsSystem::onClusterizeSelection(embedder_id_type id){
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
                _clusters[scale_id].push_back(std::unordered_set<unsigned int>());
                //there is something on screen already... I add the new one
                for(auto id: idxes_selected_landmarks_in_scale){
                    for(int s = 0; s < _clusters[scale_id].size()-1; ++s){
                        if(_clusters[scale_id][s].find(id)!=_clusters[scale_id][s].end()){
                            _clusters[scale_id][s].erase(id);
                        }
                    }
                    _clusters[scale_id][_clusters[scale_id].size()-1].insert(id);
                }
            }
            visualizeTheFlow();
        }
#endif
    }
}

