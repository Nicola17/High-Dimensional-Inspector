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

#include "hdi/deep_learning/controller_layer_qobj.h"
#include <QSpacerItem>
#include "hdi/data/empty_data.h"
#include "hdi/data/embedding.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/dimensionality_reduction/wtsne.h"

#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_multiple_scalar_attributes.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/utils/vector_utils.h"
#include "hdi/utils/math_utils.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include <QInputDialog>
#include <thread>


namespace hdi{
    namespace dl{

        void ControllerLayer::initialize(std::shared_ptr<ModelCaffeSolver> solver, std::shared_ptr<ModelLayer> layer, std::shared_ptr<ViewEmbedding> view_input_landscape, std::shared_ptr<ViewLayerDetailed> view_layer_detailed, std::shared_ptr<ViewLayerOverview> view_layer_overview, QWidget* widget){
            _samples_per_input = 10;
            _solver = solver;
            _layer = layer;
            _view_input_landscape = view_input_landscape;
            _view_layer_detailed = view_layer_detailed;
            _view_layer_overview = view_layer_overview;


            _iter = 0;
            _rf_scale = 150;
            //_rf_scale = 1;
            _rf_zero_value = 125;

            _feature_view = true;


            _main_gridlayout = std::shared_ptr<QGridLayout>(new QGridLayout());

            {//General setup
                //Controls
                _options_grpbx = std::shared_ptr<QGroupBox>(new QGroupBox());
                //_options_grpbx->setTitle("Controls");

                //QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding); //MEMORY

                _landscape_analysis_tabwdg = std::shared_ptr<QTabWidget>(new QTabWidget());
                _main_gridlayout->addWidget(_view_layer_detailed.get(),0,0,1,3);
                _view_layer_detailed->setMinimumSize(400,150);
                //_view_layer_detailed->show();
                QLabel* fml = new QLabel("Filter Map");
                QLabel* iml = new QLabel("Input Map");
                QFont font;
                font.setPointSize(14);
                fml->setAlignment(Qt::AlignCenter);
                iml->setAlignment(Qt::AlignCenter);
                fml->setFont(font);
                iml->setFont(font);
                _main_gridlayout->addWidget(fml,1,2,1,1);
                _main_gridlayout->addWidget(iml,1,0,1,2);
                _main_gridlayout->addWidget(_landscape_analysis_tabwdg.get(),2,0,1,2);
                _landscape_analysis_tabwdg->setMinimumSize(400,400);
                _main_gridlayout->addWidget(_options_grpbx.get(),3,0,1,3);
                //_main_gridlayout->addItem(spacer);
                widget->setLayout(_main_gridlayout.get());
            }

            //Connections
            {
                connect(_view_layer_detailed.get(),&ViewLayerDetailed::sgnClickOnFilter,this,&ControllerLayer::onSelectFilter);
                connect(_view_layer_overview.get(),&ViewLayerOverview::sgnClickOnPerplexityBucket,this,&ControllerLayer::onSelectPerplexityBucket);

            }

            {//Setup of the controls
                _controls_layout = std::shared_ptr<QGridLayout>(new QGridLayout());
                _inputs_available_lbl = std::shared_ptr<QLabel>(new QLabel());

                _collect_inputs_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _collect_inputs_btn->setText("Collect input data");
                _collect_inputs_btn->setCheckable(true);
                _collect_inputs_btn->setChecked(false);
                _reset_data_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _reset_data_btn->setText("Clear data");
                connect(_reset_data_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onClearInputData);
                _compute_embedding_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _compute_embedding_btn->setText("Compute Filter Map");
                connect(_compute_embedding_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onComputeEmbedding);
                _compute_hsne_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _compute_hsne_btn->setText("Compute Input Map");
                connect(_compute_hsne_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onComputeHSNEEmbedding);
                _optimize_for_selection_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _optimize_for_selection_btn->setText("Optimize for selection");
                connect(_optimize_for_selection_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onOptimizeForSelection);
                _switch_embedding_view_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _switch_embedding_view_btn->setText("Switch embedding view");
                connect(_switch_embedding_view_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onSwitchEmbeddingView);
                _hsne_drill_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _hsne_drill_btn->setText("Drill in the selection");
                connect(_hsne_drill_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onHSNEDrill);
                _sort_filters_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _sort_filters_btn->setText("Sort filters");
                _sort_filters_btn->setCheckable(true);
                connect(_sort_filters_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onSortFilters);
                _reset_filter_timings_btn = std::shared_ptr<QPushButton>(new QPushButton());
                _reset_filter_timings_btn->setText("Reset filter timings");
                connect(_reset_filter_timings_btn.get(),&QPushButton::clicked,this,&ControllerLayer::onResetFilterTimings);
                _filter_similiarities_chbx = std::shared_ptr<QCheckBox>(new QCheckBox());
                _filter_similiarities_chbx->setText("Compute filter similarities");

                _visible_feature_cmbbx = std::shared_ptr<QComboBox>(new QComboBox());
                connect(_visible_feature_cmbbx.get(),static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this,&ControllerLayer::onChangeSelectedProperty);

                _heat_modes_cmbbx = std::shared_ptr<QComboBox>(new QComboBox());
                _heat_modes_cmbbx->addItem("Max activation");
                _heat_modes_cmbbx->addItem("Frequency");

                _lr_multiplier_dspbx = std::shared_ptr<QDoubleSpinBox>(new QDoubleSpinBox());
                _lr_multiplier_dspbx->setValue(1);
                _lr_multiplier_dspbx->setSingleStep(0.1);
                connect(_lr_multiplier_dspbx.get(),static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),this,&ControllerLayer::onLRMultChanged);

                _controls_layout->addWidget(_inputs_available_lbl.get(),0,0);
                _controls_layout->addWidget(_collect_inputs_btn.get(),0,1);
                _controls_layout->addWidget(_reset_data_btn.get(),0,2);
                _controls_layout->addWidget(_sort_filters_btn.get(),0,3);
                _controls_layout->addWidget(_filter_similiarities_chbx.get(),0,4);
                _controls_layout->addWidget(_compute_embedding_btn.get(),1,0);
                _controls_layout->addWidget(_compute_hsne_btn.get(),1,1);
                _controls_layout->addWidget(_hsne_drill_btn.get(),1,2);
                _controls_layout->addWidget(_reset_filter_timings_btn.get(),1,3);

                //_controls_layout->addWidget(_optimize_for_selection_btn.get(),2,0);
                _controls_layout->addWidget(_lr_multiplier_dspbx.get(),4,1);
                _controls_layout->addWidget(_visible_feature_cmbbx.get(),0,5);
                _controls_layout->addWidget(_switch_embedding_view_btn.get(),1,4);
                _controls_layout->addWidget(_heat_modes_cmbbx.get(),1,5);
                _options_grpbx->setLayout(_controls_layout.get());
            }

            {//panel data
                for(int i = 0; i <_layer->size(); ++i){
                    _layer->_input_data.requestProperty(_layer->_filter_names[i]);
                }
                _layer->_input_data.requestProperty("Label");
                _layer->_input_data.requestProperty("No Features");
                _layer->_input_data.requestProperty("Perplexity");
                _layer->_input_data.requestProperty("Perplexity Inverse");
                _layer->_input_data.requestProperty("Max Activation");
                _layer->_input_data.requestProperty("Max Activation Inverse");
                _layer->_selected_property = 0;

                std::vector<std::string> names;
                _layer->_input_data.getAvailableProperties(names);
                for(const auto& name: names){
                    _visible_feature_cmbbx->addItem(name.c_str());
                }

            }
        }

        void ControllerLayer::onNewData(){
            _inputs_available_lbl->setText(QString("%1 inputs to embed").arg(_layer->_input_data.numDataPoints()));
            updatePerplexityHistograms();
            updateActivationValues();
            updatePerplexityActivationValues();
            if(_filter_similiarities_chbx->isChecked()){
                updateWeightedJaccard();
            }
            if(_collect_inputs_btn->isChecked()){
                collectData();
            }
            if(_sort_filters_btn->isChecked()){
                onSortFilters();
            }
            _layer->_last_refresh = _iter;

            if(_heat_modes_cmbbx->currentIndex() == 0){
                _view_layer_detailed->_filter_activations_viz->setMaxViz();
            }else{
                _view_layer_detailed->_filter_activations_viz->setFreqViz();
            }
            ++_iter;
            std::cout << _layer->_layer_max_activation.second << std::endl;
        }

        void ControllerLayer::onSelectionOnEmbedding(analytics::MultiscaleEmbedderSingleView* ptr){
            utils::secureLog(_log,"Selection on an embedding...");

            for(int i = 0; i < _layer->_max_activations_perplexity.size(); ++i){
                _layer->_max_activations_perplexity[i] = ModelLayer::timed_activation_type(_iter,0);
            }

            auto& panel_data = ptr->getPanelData();
            //TEMP
            for(int p = 0; p < panel_data.numDataPoints(); ++p){
                if((panel_data.getFlagsDataPoints()[p]&analytics::MultiscaleEmbedderSingleView::panel_data_type::Selected) == analytics::MultiscaleEmbedderSingleView::panel_data_type::Selected){
                    for(int i = 0; i < _layer->_max_activations_perplexity.size(); ++i){
                        auto v = panel_data.getProperty(QString("Filter_%1").arg(i).toStdString())[p];
                        if(_layer->_max_activations[i].second!=0){
                            v = v/_layer->_max_activations[i].second;
                        }else{
                            v = 0;
                        }
                        _layer->_max_activations_perplexity[i].second = std::max<scalar_type>(_layer->_max_activations_perplexity[i].second,v);
                    }
                }
            }
            _layer->_last_refresh = _iter;
            _view_layer_detailed->_filter_activations_perplexity_viz->setMaxValue(1);
            _view_layer_detailed->update();
        }

        void ControllerLayer::updatePerplexityHistograms(){
            auto size = _layer->size();
            _layer->patchPerplexity().clear();

            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto shape = top_vecs[_layer->id()][0]->shape();
            auto batch_size = shape[0];
            auto height     = shape.size()==4?shape[2]:0;
            auto width      = shape.size()==4?shape[3]:0;

            int n_samples = 10; //MAGICNUMBER
            int tot = 0;

            for(int b = 0; b < batch_size; ++b){
                if(height != 0){//convolution
                    for(int s = 0; s < n_samples; ++s){
                        //random selection of the depth column
                        int i = rand()%width;
                        int j = rand()%height;

                        std::vector<double> activations(size,0);
                        bool valid = false;
                        for(int l = 0; l < size; ++l){
                            activations[l] = top_vecs[_layer->id()][0]->data_at(b,l,j,i);
                            if(activations[l]<0){
                                activations[l] = 0;
                            }
                            if(activations[l] > 0){
                                valid = true;
                            }
                        }
                        if(valid == false){//empty input
                            //--s;
                            //continue;
                        }

                        utils::normalizeL1(activations);
                        double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                        _layer->patchPerplexity().add(perplexity);
                        ++tot;
                    }
                }else{
                    std::vector<double> activations(size);
                    for(int l = 0; l < size; ++l){
                        activations[l] = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                        if(_layer->_type != ModelLayer::SoftMax){
                            if(activations[l]<0){
                                activations[l] = 0;
                            }
                        }
                    }
                    if(_layer->_type == ModelLayer::SoftMax){
                        utils::softMax(activations);
                    }else{
                        utils::normalizeL1(activations);
                    }
                    double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                    for(int i = 0; i < n_samples; ++i){
                        _layer->patchPerplexity().add(perplexity);
                    }
                    ++tot;
                }
            }
            _layer->updateSmoothedHistogram();
            _layer->computeLearningHistogram();
        }

        void ControllerLayer::updateActivationValues(){
            auto size = _layer->size();


            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto shape = top_vecs[_layer->id()][0]->shape();
            auto batch_size = shape[0];
            auto height     = shape.size()==4?shape[2]:0;
            auto width      = shape.size()==4?shape[3]:0;
            int n_samples = 10;

            for(int b = 0; b < batch_size; ++b){
                int label = net->top_vecs()[0][1]->data_at(b,0,0,0);
                for(int s = 0; s < n_samples; ++s){
                    if(height != 0){//convolution
                        int i = rand()%width;
                        int j = rand()%height;
                        for(int l = 0; l < size; ++l){
                            double v = top_vecs[_layer->id()][0]->data_at(b,l,j,i);
                            if(v < 0) v = 0;
                            if(_layer->_max_activations[l].second*(1-_layer->_refresh_thresh) < v){
                                if(_layer->_max_activations[l].second < v){
                                    _layer->_max_activations[l].second = v;
                                }
                                _layer->_max_activations[l].first = _iter;
                            }
                            if(_layer->_layer_max_activation.second*(1-_layer->_refresh_thresh) < v){
                                if(_layer->_layer_max_activation.second < v){
                                    _layer->_layer_max_activation.second = v;
                                }
                                _layer->_layer_max_activation.first = _iter;
                            }
                            _layer->_activation_sum_per_label[l][label] += v;

/*
                            if(v > _layer->_max_activations[l].second * 0.3){
                                _layer->_activation_freq_num[l] += 1;
                            }
                            _layer->_activation_freq_den[l] += 1;
                            */
                            _layer->_activation_freq_num[l] += v;
                            _layer->_activation_freq_den[l] += _layer->_max_activations[l].second;
                            if(_layer->_activation_freq_den[l] != 0){
                                _layer->_activation_freq[l] = _layer->_activation_freq_num[l] / _layer->_activation_freq_den[l];
                            }else{
                                _layer->_activation_freq[l] = 1;
                            }

                        }
                    }else{
                        for(int l = 0; l < size; ++l){
                            double v = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                            if(v < 0) v = 0;
                            if(_layer->_max_activations[l].second*(1-_layer->_refresh_thresh) < v){
                                if(_layer->_max_activations[l].second < v){
                                    _layer->_max_activations[l].second = v;
                                }
                                _layer->_max_activations[l].first = _iter;
                            }
                            if(_layer->_layer_max_activation.second*(1-_layer->_refresh_thresh) < v){
                                if(_layer->_layer_max_activation.second < v){
                                    _layer->_layer_max_activation.second = v;
                                }
                                _layer->_layer_max_activation.first = _iter;
                            }
                            _layer->_activation_sum_per_label[l][label] += v;

                            /*
                            if(v > _layer->_max_activations[l].second * 0.5){
                                _layer->_activation_freq_num[l] += 1;
                            }
                            _layer->_activation_freq_den[l] += 1;
                            */
                            _layer->_activation_freq_num[l] += v;
                            _layer->_activation_freq_den[l] += _layer->_max_activations[l].second;
                            if(_layer->_activation_freq_den[l] != 0){
                                _layer->_activation_freq[l] = _layer->_activation_freq_num[l] / _layer->_activation_freq_den[l];
                            }else{
                                _layer->_activation_freq[l] = 1;
                            }
                        }
                    }
                }
            }
            //std::cout << _layer->_activation_freq_num[0] << " - " << _layer->_activation_freq_num[0] << " - " << _layer->_activation_freq[0] << std::endl;
        }

        void ControllerLayer::updatePerplexityActivationValues(){
            if(_layer->_selected_histogram_bucket < 0){
                return;
            }

            //interval of accepted perplexity
            auto perplexity_interval = _layer->_patch_perplexity.getBucketLimits(_layer->_selected_histogram_bucket);
            auto size = _layer->size();

            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto shape = top_vecs[_layer->id()][0]->shape();
            auto batch_size = shape[0];
            auto height     = shape.size()==4?shape[2]:0;
            auto width      = shape.size()==4?shape[3]:0;
            int n_samples = 10;

            for(int b = 0; b < batch_size; ++b){
                if(height != 0){//convolution
                    for(int s = 0; s < n_samples; ++s){
                        int i = rand()%width;
                        int j = rand()%height;

                        std::vector<double> activations(size,0);
                        bool valid = false;
                        for(int l = 0; l < size; ++l){
                            activations[l] = top_vecs[_layer->id()][0]->data_at(b,l,j,i);
                            if(activations[l]< 0){
                                activations[l] = 0;
                            }
                            if(activations[l] > 0){
                                valid = true;
                            }
                        }
                        /*
                        if(valid == false){//empty input
                            --s;
                            continue;
                        }
                        */
                        utils::normalizeL1(activations);
                        double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                        if(perplexity < perplexity_interval.first || perplexity > perplexity_interval.second){
                            continue;
                        }

                        for(int l = 0; l < size; ++l){
                            double v = top_vecs[_layer->id()][0]->data_at(b,l,j,i);
                            if(_layer->_max_activations_perplexity[l].second*(1-_layer->_refresh_thresh) < v){
                                if(_layer->_max_activations_perplexity[l].second < v){
                                    _layer->_max_activations_perplexity[l].second = v;
                                }
                                _layer->_max_activations_perplexity[l].first = _iter;
                            }
                        }
                    }
                }else{
                    std::vector<double> activations(size,0);
                    for(int l = 0; l < size; ++l){
                        activations[l] = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                        if(activations[l]<0){
                            activations[l] = 0;
                        }
                    }
                    utils::normalizeL1(activations);
                    double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                    if(perplexity < perplexity_interval.first || perplexity > perplexity_interval.second){
                        continue;
                    }

                    for(int l = 0; l < size; ++l){
                        double v = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                        if(_layer->_max_activations_perplexity[l].second*(1-_layer->_refresh_thresh) < v){
                            if(_layer->_max_activations_perplexity[l].second < v){
                                _layer->_max_activations_perplexity[l].second = v;
                            }
                            _layer->_max_activations_perplexity[l].first = _iter;
                        }
                    }
                }
            }
        }




        void ControllerLayer::updateWeightedJaccard(){
            auto size = _layer->size();

            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto shape = top_vecs[_layer->id()][0]->shape();
            auto batch_size = shape[0];
            auto height     = shape.size()==4?shape[2]:0;
            auto width      = shape.size()==4?shape[3]:0;
            int n_samples = 10;

            for(int b = 0; b < batch_size; ++b){
                for(int s = 0; s < n_samples; ++s){
                    if(height != 0){//convolution
                        //if(size > 50 || _iter < 300)return;
                        int i = rand()%width;
                        int j = rand()%height;

                        for(int l0 = 0; l0 < size; ++l0){
                            for(int l1 = l0 + 1; l1 < size; ++l1){
                                double v0 = top_vecs[_layer->id()][0]->data_at(b,l0,j,i);
                                double v1 = top_vecs[_layer->id()][0]->data_at(b,l1,j,i);
                                if(v0 < 0) v0 = 0;
                                if(v1 < 0) v1 = 0;
                                double numerator = std::min(v0,v1);
                                double denumerator = std::max(v0,v1);
                                _layer->_weighted_jaccard_numerator[l0][l1] += numerator;
                                _layer->_weighted_jaccard_numerator[l1][l0] += numerator;
                                _layer->_weighted_jaccard_denumerator[l0][l1] += denumerator;
                                _layer->_weighted_jaccard_denumerator[l1][l0] += denumerator;

                                _layer->_filter_similarity[l0][l1] = _layer->_weighted_jaccard_numerator[l0][l1]/_layer->_weighted_jaccard_denumerator[l0][l1];
                                _layer->_filter_similarity[l1][l0] = _layer->_weighted_jaccard_numerator[l1][l0]/_layer->_weighted_jaccard_denumerator[l1][l0];

                                _layer->_filter_similarity[l0][l0] = 1;
                            }
                        }
                    }else{
                        //if(_iter < 100 )return;
                        std::vector<std::pair<scalar_type,unsigned int>> activations(size);
                        for(int l = 0; l < size; ++l){
                            activations[l].first = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                            activations[l].second = l;
                            _layer->_total_activation[l] += activations[l].first;
                        }
                        //smart computation
                        std::sort(activations.begin(),activations.end(),std::greater<std::pair<scalar_type,unsigned int>>());

                        for(int m = 0; m < size; ++m){
                            unsigned int l0 = activations[m].second;
                            double v0 = activations[m].first;
                            if(v0 < 0) v0 = 0;
                            if(v0 < _layer->_max_activations[l0].second/10){
                                continue;
                            }

                            for(int n = m+1; n < size; ++n){
                                unsigned int l1 = activations[n].second;
                                double v1 = activations[n].first;
                                if(v1 < 0) {
                                    v1 = 0;
                                }

                                if(v0 / 2. > v1){
                                   // break;
                                }

                                double min_v = std::min(v0,v1);
                                _layer->_weighted_jaccard_numerator[l0][l1] += min_v;
                                _layer->_weighted_jaccard_numerator[l1][l0] += min_v;

                                double max_v = std::max(v0,v1);
                                _layer->_weighted_jaccard_denumerator[l0][l1] += max_v;
                                _layer->_weighted_jaccard_denumerator[l1][l0] += max_v;

                                double sim = _layer->_weighted_jaccard_numerator[l0][l1]/(_layer->_total_activation[l0] + _layer->_total_activation[l1] - _layer->_weighted_jaccard_numerator[l0][l1]);
                                double sim_2 = _layer->_weighted_jaccard_numerator[l0][l1]/_layer->_weighted_jaccard_denumerator[l0][l1];
                                if(std::abs(sim - sim_2) > 0.05){
                                 //   std::cout << sim << " " << sim_2 << std::endl;
                                }

                                _layer->_filter_similarity[l0][l1] = sim_2;
                                _layer->_filter_similarity[l1][l0] = sim_2;

                                _layer->_filter_similarity[l0][l0] = 1;
                            }
                        }
                    }
                }
            }
        }

        /*
        void ControllerLayer::updateWeightedJaccard(){
            auto size = _layer->size();

            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto shape = top_vecs[_layer->id()][0]->shape();
            auto batch_size = shape[0];
            auto height     = shape.size()==4?shape[2]:0;
            auto width      = shape.size()==4?shape[3]:0;
            int n_samples = 10;

            for(int b = 0; b < batch_size; ++b){
                for(int s = 0; s < n_samples; ++s){
                    if(height != 0){//convolution
                        if(size > 50 || _iter < 300)return;
                        int i = rand()%width;
                        int j = rand()%height;

                        for(int l0 = 0; l0 < size; ++l0){
                            for(int l1 = l0 + 1; l1 < size; ++l1){
                                double v0 = top_vecs[_layer->id()][0]->data_at(b,l0,j,i);
                                double v1 = top_vecs[_layer->id()][0]->data_at(b,l1,j,i);
                                if(v0 < 0) v0 = 0;
                                if(v1 < 0) v1 = 0;
                                double numerator = std::min(v0,v1);
                                double denumerator = std::max(v0,v1);
                                _layer->_weighted_jaccard_numerator[l0][l1] += numerator;
                                _layer->_weighted_jaccard_numerator[l1][l0] += numerator;
                                _layer->_weighted_jaccard_denumerator[l0][l1] += denumerator;
                                _layer->_weighted_jaccard_denumerator[l1][l0] += denumerator;

                                _layer->_filter_similarity[l0][l1] = _layer->_weighted_jaccard_numerator[l0][l1]/_layer->_weighted_jaccard_denumerator[l0][l1];
                                _layer->_filter_similarity[l1][l0] = _layer->_weighted_jaccard_numerator[l1][l0]/_layer->_weighted_jaccard_denumerator[l1][l0];

                                _layer->_filter_similarity[l0][l0] = 1;
                            }
                        }
                    }else{
                        if(_iter < 200)return;
                        std::vector<std::pair<scalar_type,unsigned int>> activations(size);
                        for(int l = 0; l < size; ++l){
                            activations[l].first = top_vecs[_layer->id()][0]->data_at(b,l,0,0);
                            activations[l].second = l;
                        }

                        for(int l0 = 0; l0 < size; ++l0){
                            double v0 = activations[l0].first;
                            if(v0 < 0) v0 = 0;
                            if(v0 < _layer->_max_activations[l0].second/10){
                                continue;
                            }
                            for(int l1 = l0+1; l1 < size; ++l1){
                                double v1 = activations[l1].first;

                                if(v1 < 0) v1 = 0;
                                double numerator = std::min(v0,v1);
                                double denumerator = std::max(v0,v1);
                                _layer->_weighted_jaccard_numerator[l0][l1] += numerator;
                                _layer->_weighted_jaccard_numerator[l1][l0] += numerator;
                                _layer->_weighted_jaccard_denumerator[l0][l1] += denumerator;
                                _layer->_weighted_jaccard_denumerator[l1][l0] += denumerator;

                                _layer->_filter_similarity[l0][l1] = _layer->_weighted_jaccard_numerator[l0][l1]/_layer->_weighted_jaccard_denumerator[l0][l1];
                                _layer->_filter_similarity[l1][l0] = _layer->_weighted_jaccard_numerator[l1][l0]/_layer->_weighted_jaccard_denumerator[l1][l0];

                                _layer->_filter_similarity[l0][l0] = 1;
                            }
                        }
                    }
                }
            }
        }
*/
        void ControllerLayer::onSelectFilter(int id){
            utils::secureLogValue(_log,"Select filter",id);
            int cmb_bx_id = _visible_feature_cmbbx->findText(QString::fromStdString(_layer->_filter_names[id]));
            _visible_feature_cmbbx->setCurrentIndex(cmb_bx_id);
            _view_layer_detailed->update();
        }

        void ControllerLayer::onSelectPerplexityBucket(int id){
            utils::secureLogValue(_log,"Select perplexity bucket",id);
            _layer->_selected_histogram_bucket = id;
            _layer->_max_activations_perplexity = ModelLayer::timed_activation_vector_type(_layer->size(),ModelLayer::timed_activation_type(_iter,0));
        }

        void ControllerLayer::onResetFilterTimings(){
            utils::secureLog(_log,"Reset filters timing...");

            int refresh_tresh = 50; //MAGICNUMBER
            _layer->_layer_max_activation.second = 0;
            auto size = _layer->size();
            for(int i = 0; i < size; ++i){
                if(_layer->_last_refresh - _layer->_max_activations[i].first > refresh_tresh){
                    _layer->_max_activations[i].first = _layer->_last_refresh;
                    _layer->_max_activations[i].second = 0;
                }else{
                    _layer->_layer_max_activation.second = std::max(_layer->_layer_max_activation.second,_layer->_max_activations[i].second);
                }
                if(_layer->_last_refresh - _layer->_max_activations_perplexity[i].first > refresh_tresh){
                    _layer->_max_activations_perplexity[i].first = _layer->_last_refresh;
                    _layer->_max_activations_perplexity[i].second = 0;
                }
            }
            _view_layer_detailed->update();
        }

        void ControllerLayer::onSortFilters(){
            //utils::secureLog(_log,"Sort filters...");

            if(_heat_modes_cmbbx->currentIndex() == 0){
                auto size = _layer->size();
                std::vector<std::pair<scalar_type,uint32_t>> to_sort;
                to_sort.reserve(size);

                for(int i = 0; i < size; ++i){
                    to_sort.push_back(std::pair<scalar_type,uint32_t>(_layer->_max_activations[i].second,i));
                }

                std::sort(to_sort.begin(),to_sort.end());
                for(int i = 0; i < size; ++i){
                    _layer->_order_in_visualization[to_sort[i].second] = i;
                }
            //_view_layer_detailed->update();
            }else{
                auto size = _layer->size();
                std::vector<std::pair<scalar_type,uint32_t>> to_sort;
                to_sort.reserve(size);

                for(int i = 0; i < size; ++i){
                    to_sort.push_back(std::pair<scalar_type,uint32_t>(1-_layer->_activation_freq[i],i));
                }

                std::sort(to_sort.begin(),to_sort.end());
                for(int i = 0; i < size; ++i){
                    _layer->_order_in_visualization[to_sort[i].second] = i;
                }

            }
        }

        void ControllerLayer::onSwitchEmbeddingView(){
            if(_multiscale_embedder.get() != nullptr){
                if(_feature_view){
                    _multiscale_embedder->onActivateInfluenceMode(hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type(0,0));
                    _feature_view = false;
                }else{
                    _multiscale_embedder->onActivateUserDefinedMode(hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type(0,0));
                    _feature_view = true;
                }
            }
        }



        void ControllerLayer::onLRMultChanged(double value){
            utils::secureLogValue(_log,"LR Multiplier changed", value);
            const auto& net = _solver->_solver->net();
            const auto& layer = net->layers()[_layer->id()];
            auto& layer_param =  layer->layer_param();

            int num_mutable_params = layer_param.param_size();
            assert(num_mutable_params == 2);

            layer_param.mutable_param(0)->set_lr_mult(value);
            layer_param.mutable_param(1)->set_lr_mult(value*2);

        }



        void ControllerLayer::collectData(){
            const auto& net = _solver->_solver->net();
            const auto& net_layers = net->layers();
            const auto& layer = net->layers()[_layer->id()];
            const caffe::LayerParameter& layer_param = layer->layer_param();

            std::string type = layer_param.type();
            if(layer_param.has_convolution_param()){
                collectDataConvolution();
                return;
            }
            if(layer_param.has_inner_product_param()){
                collectDataFullyConnected();
                return;
            }

        }

        void ControllerLayer::collectDataConvolution(){
            const auto& net = _solver->_solver->net();
            const auto& net_layers = net->layers();
            const auto& layer = net->layers()[_layer->id()];
            const auto& bot_vec = net->bottom_vecs()[_layer->id()];
            const auto& top_vec = net->top_vecs()[_layer->id()];
            const caffe::LayerParameter& layer_param = layer->layer_param();
            const caffe::ConvolutionParameter& conv_param = layer_param.convolution_param();

            int kernel_size = conv_param.kernel_size(0);
            int pad = conv_param.pad_size()==0?0:conv_param.pad().Get(0);
            int stride = 1;
            if(conv_param.stride_size()){
                stride = conv_param.stride(0);
            }
            int group = 1;
            if(conv_param.has_group()){
                group = conv_param.group();
            }

            //only limited to a single one for now
            auto shape_top = top_vec[0]->shape();
            auto shape_bottom = bot_vec[0]->shape();

            assert(shape_top[0] == shape_bottom[0]);
            for(int b = 0; b < shape_top[0]; ++b){
                int label = net->top_vecs()[0][1]->data_at(b,0,0,0);
                if(label == 0){
                    //continue;
                }
                for(int s = 0; s < _samples_per_input; ++s){
                    int i = rand()%shape_top[3];
                    int j = rand()%shape_top[2];

                    std::vector<scalar_type> data;
                    bool valid = true; //DO NOT FILTER
                    //v.reserve(layer._input_data.numDimensions());
                    for(int f = 0; f < shape_bottom[1]; ++f){//features
                        for(int fj = 0; fj < kernel_size; ++fj){
                            for(int fi = 0; fi < kernel_size; ++fi){
                                int x = i*stride + fi - pad;
                                int y = j*stride + fj - pad;

                                scalar_type v = 0;
                                if(x >= 0 && y >= 0 && x < shape_bottom[3] && y < shape_bottom[2]){
                                    v = bot_vec[0]->data_at(b,f,y,x);
                                }else{
                                    //avoid padding
                                    valid = false;
                                }
                                if(v > 0){
                                    valid = true;
                                }
                                data.push_back(v);
                            }
                        }
                    }

                    if(valid){
                        auto data_shape = net->top_vecs()[0][0]->shape();
                        QImage img(_layer->_rf_size,_layer->_rf_size,QImage::Format_ARGB32);

                        //get image
                        {
                            auto shape_img = net->top_vecs()[0][0]->shape();
                            double mult = _rf_scale;
                            double avg = _rf_zero_value;
                            if(data_shape[1] == 1){
                                for(int l = 0; l < _layer->_rf_size; ++l){
                                    for(int k = 0; k < _layer->_rf_size; ++k){
                                        int x = i*_layer->_rf_stride + _layer->_rf_offset + k;
                                        int y = j*_layer->_rf_stride + _layer->_rf_offset + l;
                                        scalar_type intensity = avg;
                                        if(x >= 0 && y >= 0 && x < shape_img[3] && y < shape_img[2]){
                                            intensity = avg + net->top_vecs()[0][0]->data_at(b,0,y,x)*mult;
                                        }
                                        if(intensity > 255) {intensity = 255;}
                                        if(intensity < 0) {intensity = 0;}
                                        img.setPixel(k,l,qRgb(intensity,intensity,intensity));
                                    }
                                }
                            }
                            if(data_shape[1] == 3){
                                for(int l = 0; l < _layer->_rf_size; ++l){
                                    for(int k = 0; k < _layer->_rf_size; ++k){
                                        int x = i*_layer->_rf_stride + _layer->_rf_offset + k;
                                        int y = j*_layer->_rf_stride + _layer->_rf_offset + l;
                                        scalar_type red = avg;
                                        scalar_type green = avg;
                                        scalar_type blue = avg;

                                        if(x >= 0 && y >= 0 && x < shape_img[3] && y < shape_img[2]){
                                            red = avg + net->top_vecs()[0][0]->data_at(b,2,y,x)*mult;
                                            green = avg + net->top_vecs()[0][0]->data_at(b,1,y,x)*mult;
                                            blue = avg + net->top_vecs()[0][0]->data_at(b,0,y,x)*mult;
                                        }
                                        if(red > 255)   {red = 255;}
                                        if(red < 0)     {red = 0;}
                                        if(green > 255) {green = 255;}
                                        if(green < 0)   {green = 0;}
                                        if(blue > 255)  {blue = 255;}
                                        if(blue < 0)    {blue = 0;}
                                        img.setPixel(k,l,qRgb(red,green,blue));
                                    }
                                }
                            }

                        }


                        _layer->_input_data.addDataPoint(std::shared_ptr<data::ImageData>(new data::ImageData(img)),data);
                        //Set properties
                        std::vector<scalar_type> activations(shape_top[1]);
                        double max_activation = 0;

                        int id_pnt = _layer->_input_data.numDataPoints()-1;
                        for(int f = 0; f < shape_top[1]; ++f){//features
                            double activation = top_vec[0]->data_at(b,f,j,i);
                            std::string feature_name = QString("Filter_%1").arg(f).toStdString();
                            _layer->_input_data.getProperty(feature_name)[id_pnt] = activation;
                            //if(activation < 0){activation = 0;}
                            max_activation = std::max(max_activation,activation);
                            activations[f] = activation;
                        }
                        //utils::normalizeL1(activations);
                        utils::softMax(activations);
                        double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                        double perplexity_norm = (perplexity-1.)/(shape_top[1]-1.);

                        _layer->_input_data.getProperty("Label")[id_pnt] = label;
                        _layer->_input_data.getProperty("No Features")[id_pnt] = 0.2;
                        _layer->_input_data.getProperty("Perplexity")[id_pnt] = perplexity_norm;
                        _layer->_input_data.getProperty("Perplexity Inverse")[id_pnt] = 1-perplexity_norm;
                        _layer->_input_data.getProperty("Max Activation")[id_pnt] = max_activation;
                        _layer->_input_data.getProperty("Max Activation Inverse")[id_pnt] = _layer->_layer_max_activation.second - max_activation;
                    }
                }
            }

        }

        void ControllerLayer::collectDataFullyConnected(){
            const auto& net = _solver->_solver->net();
            const auto& net_layers = net->layers();
            const auto& layer = net->layers()[_layer->id()];
            const auto& bot_vec = net->bottom_vecs()[_layer->id()];
            const auto& top_vec = net->top_vecs()[_layer->id()];
            const caffe::LayerParameter& layer_param = layer->layer_param();
            const caffe::InnerProductParameter& fc_param = layer_param.inner_product_param();

            int size = fc_param.num_output();

            //only limited to a single one for now
            auto shape_top = top_vec[0]->shape();
            auto shape_bottom = bot_vec[0]->shape();
            auto learn_params = net_layers[_layer->id()]->blobs();
            auto param_shape = learn_params[0]->shape();

            assert(shape_top[0] == shape_bottom[0]);
            for(int b = 0; b < shape_top[0]; ++b){
                int label = net->top_vecs()[0][1]->data_at(b,0,0,0);
                if(label == 0){
                    //continue;
                }
                std::vector<scalar_type> data;
                if(shape_bottom.size() == 2){
                    data.reserve(shape_bottom[1]);
                    for(int i = 0; i < shape_bottom[1]; ++i){
                        data.push_back(bot_vec[0]->data_at(b,i,0,0));
                    }
                }
                if(shape_bottom.size() == 4){
                    for(int f = 0; f < shape_bottom[1]; ++f){//features
                        for(int j = 0; j < shape_bottom[2]; ++j){
                            for(int i = 0; i < shape_bottom[3]; ++i){
                                data.push_back(bot_vec[0]->data_at(b,f,j,i));
                            }
                        }
                    }
                }
                { //COPY&PASTE IS NOT GOOD
                    auto data_shape = net->top_vecs()[0][0]->shape();
                    QImage img(data_shape[3],data_shape[2],QImage::Format_ARGB32);

                    //get image
                    {
                        auto shape_img = net->top_vecs()[0][0]->shape();
                        double mult = _rf_scale;
                        double avg = _rf_zero_value;
                        if(data_shape[1] == 1){
                            for(int j = 0; j < data_shape[2]; ++j){
                                for(int i = 0; i < data_shape[3]; ++i){
                                    scalar_type intensity = intensity = avg + net->top_vecs()[0][0]->data_at(b,0,j,i)*mult;
                                    if(intensity > 255) {intensity = 255;}
                                    if(intensity < 0) {intensity = 0;}
                                    img.setPixel(i,j,qRgb(intensity,intensity,intensity));
                                }
                            }
                        }
                        if(data_shape[1] == 3){
                            for(int j = 0; j < data_shape[2]; ++j){
                                for(int i = 0; i < data_shape[3]; ++i){
                                    scalar_type red = avg + net->top_vecs()[0][0]->data_at(b,0,j,i)*mult;
                                    scalar_type green = avg + net->top_vecs()[0][0]->data_at(b,1,j,i)*mult;
                                    scalar_type blue = avg + net->top_vecs()[0][0]->data_at(b,2,j,i)*mult;

                                    if(red > 255)   {red = 255;}
                                    if(red < 0)     {red = 0;}
                                    if(green > 255) {green = 255;}
                                    if(green < 0)   {green = 0;}
                                    if(blue > 255)  {blue = 255;}
                                    if(blue < 0)    {blue = 0;}
                                    img.setPixel(i,j,qRgb(red,green,blue));
                                }
                            }
                        }

                    }


                    _layer->_input_data.addDataPoint(std::shared_ptr<data::ImageData>(new data::ImageData(img)),data);
                    //Set properties
                    std::vector<scalar_type> activations(shape_top[1]);
                    double max_activation = 0;
                    int id_pnt = _layer->_input_data.numDataPoints()-1;
                    for(int f = 0; f < shape_top[1]; ++f){//features
                        auto activation = top_vec[0]->data_at(b,f,0,0);
                        std::string feature_name = QString("Filter_%1").arg(f).toStdString();
                        _layer->_input_data.getProperty(feature_name)[id_pnt] = activation;
                        max_activation = std::max<double>(max_activation,activation);
                        //if(activation < 0){activation = 0;}
                        //activations[f] = activation;
                    }
                    //utils::normalizeL1(activations);
                    utils::softMax(activations);
                    double perplexity = utils::computePerplexity(activations.begin(),activations.end());
                    double perplexity_norm = (perplexity-1.)/(shape_top[1]-1.);
                    _layer->_input_data.getProperty("Label")[id_pnt] = label;
                    _layer->_input_data.getProperty("No Features")[id_pnt] = 0.2;
                    _layer->_input_data.getProperty("Perplexity")[id_pnt] = perplexity_norm;
                    _layer->_input_data.getProperty("Perplexity Inverse")[id_pnt] = 1-perplexity_norm;
                    _layer->_input_data.getProperty("Max Activation")[id_pnt] = max_activation;

                }
            }
        }


        void ControllerLayer::onClearInputData(){
            _layer->_input_data.removeData();
        }

        void ControllerLayer::onClearHSNEInterface(){
            _landscape_analysis_tabwdg->clear();
        }

        void ControllerLayer::onComputeEmbedding(){
            {
                _similarity_confidence.resize(_layer->size(),1);
                _similarity_colors.resize(_layer->size(),1);
                for(int f = 0; f < _layer->size(); ++f){
                    scalar_vector_type v(_layer->_activation_sum_per_label[f]);
                    hdi::utils::normalizeL1(v);
                    scalar_type perplexity = hdi::utils::computePerplexity(v.begin(),v.end());
                    _similarity_confidence[f] = 1 - perplexity/_layer->_num_labels;

                    scalar_type max_value = 0;
                    scalar_type max_label = 0;
                    for(int l = 0; l < _layer->_num_labels; ++l){
                        if(v[l] > max_value){
                            max_value = v[l];
                            max_label = l;
                        }
                    }
                    _similarity_colors[f] = (*_palette_labels)[max_label];
                }
            }


            //compute similarity embedding
            _collect_inputs_btn->setChecked(false);

            _embedding = std::shared_ptr<data::Embedding<scalar_type>>(new data::Embedding<scalar_type>());
            std::vector<hdi::data::MapMemEff<unsigned int, float> > distribution(_layer->size());
            std::vector<scalar_type> distances(_layer->size()*_layer->size());
            std::vector<scalar_type> weights(_layer->size(),0);

            for(int j = 0; j < _layer->size(); ++j){
                for(int i = 0; i < _layer->size(); ++i){
                    distances[j*_layer->size()+i] = 1-_layer->_filter_similarity[j][i];
                    distances[j*_layer->size()+i] *= distances[j*_layer->size()+i]; //squared
                    weights[j] += distances[j*_layer->size()+i];
                }
            }

            dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
            dr::HDJointProbabilityGenerator<scalar_type>::Parameters params_gen;
            params_gen._perplexity = std::min(_layer->size()/3,30);
            prob_gen.computeProbabilityDistributionsFromDistanceMatrix(distances,_layer->size(),distribution,params_gen);


            hdi::utils::imageFromSparseMatrix(distribution).save("sim_mat.png");

            dr::WeightedTSNE<> tsne;
            dr::WeightedTSNE<>::Parameters params;
            params._exaggeration_factor = 1;
            //params._mom_switching_iter = 200;
            //params._exponential_decay_iter = 2000;
            //params._remove_exaggeration_iter = 2000;
            tsne.initialize(distribution, _embedding.get(),params);
            tsne.setTheta(0.05);
            //tsne.setWeights(weights);


            _viewer = std::shared_ptr<hdi::viz::ScatterplotCanvas>(new hdi::viz::ScatterplotCanvas());
            _viewer->setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            _viewer->setSelectionColor(qRgb(50,50,50));
            //_viewer->setFixedSize(400,300);
            _viewer->show();

            _viewer->setWindowTitle(_layer->name().c_str());
            _drawer = std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttributeWithColors>(new hdi::viz::ScatterplotDrawerScalarAttributeWithColors());
            _drawer->setData(_embedding->getContainer().data(), _similarity_confidence.data(), _similarity_colors, _layer->_filter_data.getFlagsDataPoints().data(), _layer->size());
            _drawer->initialize(_viewer->context());
            _drawer->setAlpha(0.2);
            _drawer->setPointSize(50);
            _drawer->setLimits(-0.25,1);
            _viewer->addDrawer(_drawer.get());

            _main_gridlayout->addWidget(_viewer.get(),2,2);

            //_main_gridlayout->addWidget(_viewer.get());
            //_main_gridlayout->addWidget(_view.get());

            _controller_selection = std::shared_ptr<viz::ControllerSelectionEmbedding>(new viz::ControllerSelectionEmbedding());
            _controller_selection->setActors(&(_layer->_filter_data),_embedding.get(),_viewer.get());
            _controller_selection->initialize();

            int iter = 0;
            while(iter < 2000){
                tsne.doAnIteration();

                {//limits
                    std::vector<scalar_type> limits;
                    _embedding->computeEmbeddingBBox(limits,0.25);
                    auto  tr = QVector2D(limits[1],limits[3]);
                    auto bl = QVector2D(limits[0],limits[2]);
                    _viewer->setTopRightCoordinates(tr);
                    _viewer->setBottomLeftCoordinates(bl);
                }

                _viewer->updateGL();
                _view_layer_detailed->update();
                QApplication::processEvents();
                ++iter;
            }


        }

        void ControllerLayer::onComputeHSNEEmbedding(){
            _collect_inputs_btn->setChecked(false);
            utils::secureLogValue(_log,"Computing HSNE embedding for layer",_layer->name());
            onClearHSNEInterface();
            _multiscale_embedder = std::shared_ptr<hdi::analytics::MultiscaleEmbedderSystem>(new hdi::analytics::MultiscaleEmbedderSystem());
            _multiscale_embedder->setLogger(_log);
            hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = _multiscale_embedder->getPanelData();
            panel_data = _layer->_input_data;

            hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
            params._seed = -1;
            params._mcmcs_landmark_thresh = 1.5;
            params._num_neighbors = 30;
            params._aknn_num_trees = 4;
            params._aknn_num_checks = 1024;
            params._transition_matrix_prune_thresh = 1.5;
            params._mcmcs_num_walks = 200;
            params._num_walks_per_landmark = 200;

            params._monte_carlo_sampling = true;
            params._out_of_core_computation = true;

            int num_scales = std::log10(panel_data.numDataPoints());
            //int max_points = 5000;
            int max_points = 15000;

            _interface_initializer = std::shared_ptr<ControllerLayerHSNEIntializer>(new ControllerLayerHSNEIntializer());
            _interface_initializer->_tab_widget = _landscape_analysis_tabwdg.get();
            _interface_initializer->_layer = _layer;
            _interface_initializer->_ids = &_id_embeddings;
            _interface_initializer->_palette_labels = _palette_labels;
            connect(_interface_initializer.get(),&ControllerLayerHSNEIntializer::sgnSelectionOnEmbedder,this,&ControllerLayer::onSelectionOnEmbedding);

            _multiscale_embedder->setName("name");
            _multiscale_embedder->setInterfaceInitializer(_interface_initializer.get());
            _multiscale_embedder->initializeWithMaxPoints(max_points,params);
            _multiscale_embedder->createTopLevelEmbedder();

            //std::thread first (&ControllerLayer::hsneGradientDescentThread,this);
            //first.join();
            hsneGradientDescentThreadWorker();
            utils::secureLogValue(_log,"HSNE computation is finished for layer",_layer->name());
        }
        void ControllerLayer::hsneGradientDescentThreadWorker(){
            int iter = 0;
            while(iter < 2000){
                _multiscale_embedder->doAnIterateOnAllEmbedder();
                QApplication::processEvents();
                ++iter;
            }
        }

        void ControllerLayer::onHSNEDrill(){
            if(_multiscale_embedder.get() == nullptr){
                return;
            }
            _multiscale_embedder->onNewAnalysisTriggered(_id_embeddings[_landscape_analysis_tabwdg->currentIndex()]);
        }

        void computeWeightsMeanAndVariance(const caffe::Blob<float>& weights, int f, double& mean, double& variance);


        void ControllerLayer::onOptimizeForSelection(){
            if(_multiscale_embedder.get() == nullptr){
                return;
            }

            bool ok;
            int f = QInputDialog::getInt(nullptr, tr("Filter to optimize"),tr("Filter number:"), 0, 0, _layer->size()-1, 1, &ok);
            if(!ok){
                return;
            }

            std::vector<scalar_type> mean;
            //data::computeSelectionMean(_layer->_input_data,mean);
            data::computeSelectionMean(_multiscale_embedder->getEmbedder(_id_embeddings[_landscape_analysis_tabwdg->currentIndex()]).getPanelData(),mean);

            auto net = _solver->_solver->net();
            auto bot_vec = net->bottom_vecs()[_layer->id()];
            auto net_layers = net->layers();
            auto learn_params = net_layers[_layer->id()]->blobs();
            auto param_shape = learn_params[0]->shape();

            if(param_shape.size() == 4){
                double filter_mean,filter_variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, filter_mean, filter_variance);


                double max_w_abs = 0;

                for(int g = 0; g < param_shape[0]; ++g){
                    for(int d = 0; d < param_shape[1]; ++d){
                        for(int j = 0; j < param_shape[2]; ++j){
                            for(int i = 0; i < param_shape[3]; ++i){
                                auto learn_offset = learn_params[0]->offset(g,d,j,i);
                                double weight = learn_params[0]->mutable_cpu_data()[learn_offset];
                                max_w_abs = std::max(std::abs(weight),max_w_abs);
                            }
                        }
                    }
                }

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[learn_offset] = mean[mean_offset];
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] -= new_mean;
                        }
                    }
                }

                double max_new_w_abs = 0;
                double activation = 0;
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            double weight = learn_params[0]->mutable_cpu_data()[learn_offset];
                            max_new_w_abs = std::max(std::abs(weight),max_new_w_abs);
                            activation += weight*mean[mean_offset];
                        }
                    }
                }
                std::cout << f << ": Activation (" << activation << ") max(" << _layer->_layer_max_activation.second << ") \n";

                double new_activation = 0;
                double scale_factor = std::min(_layer->_layer_max_activation.second/activation*0.8,max_w_abs/max_new_w_abs);
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[learn_offset] *= scale_factor;
                            new_activation += learn_params[0]->mutable_cpu_data()[learn_offset]* mean[mean_offset];
                            //assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }
                std::cout << f << ": Scaled activation (" << new_activation << ") max(" << _layer->_layer_max_activation.second << ") \n";

/*
                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);

                //double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                double target_variance = 0.5/(learn_params[0]-1>shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                //double target_variance = variance;
                double target_std_dev = std::sqrt(target_variance);
                double p = new_variance / target_variance;
                double sqrt_p = std::sqrt(p);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] /= sqrt_p;
                            assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }
                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                std::cout << f << ": mean (" << filter_mean << ") new_mean(" << new_mean << ") \n";
                std::cout << f << ": var (" << filter_variance << ") new_var(" << new_variance << ") \n";*/
                double bias = _layer->_layer_max_activation.second - new_activation;
                learn_params[1]->mutable_cpu_data()[f] = bias;

                learn_params[0]->Update();
                learn_params[1]->Update();
            }
            if(param_shape.size() == 2){
                /*
                //VAFFANCULOOOOOOOOOOOOOOOOOOOOO
                double filter_mean,filter_variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, filter_mean, filter_variance);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[learn_offset] = mean[mean_offset];
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] -= new_mean;
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);

                //double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                //double target_variance = variance;
                double target_std_dev = std::sqrt(target_variance);
                double p = new_variance / target_variance;
                double sqrt_p = std::sqrt(p);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] /= sqrt_p;
                            assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }
                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                std::cout << f << ": mean (" << filter_mean << ") new_mean(" << new_mean << ") \n";
                std::cout << f << ": var (" << filter_variance << ") new_var(" << new_variance << ") \n";
                learn_params[1]->mutable_cpu_data()[f] = 0;

                learn_params[0]->Update();
                learn_params[1]->Update();
                */
            }
        }

#if 0
        void ControllerLayer::onOptimizeForSelection(){
            bool ok;
            int f = QInputDialog::getInt(nullptr, tr("Filter to optimize"),tr("Filter number:"), 0, 0, _layer->size()-1, 1, &ok);
            if(!ok){
                return;
            }

            std::vector<scalar_type> mean;
            //data::computeSelectionMean(_layer->_input_data,mean);
            data::computeSelectionMean(_multiscale_embedder->getEmbedder(_id_embeddings[_landscape_analysis_tabwdg->currentIndex()]).getPanelData(),mean);

            auto net = _solver->_solver->net();
            auto bot_vec = net->bottom_vecs()[_layer->id()];
            auto net_layers = net->layers();
            auto learn_params = net_layers[_layer->id()]->blobs();
            auto param_shape = learn_params[0]->shape();

            if(param_shape.size() == 4){
                double filter_mean,filter_variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, filter_mean, filter_variance);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[learn_offset] = mean[mean_offset];
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] -= new_mean;
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);

                //double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                double target_variance = 0.5/(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                //double target_variance = variance;
                double target_std_dev = std::sqrt(target_variance);
                double p = new_variance / target_variance;
                double sqrt_p = std::sqrt(p);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] /= sqrt_p;
                            assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }
                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                std::cout << f << ": mean (" << filter_mean << ") new_mean(" << new_mean << ") \n";
                std::cout << f << ": var (" << filter_variance << ") new_var(" << new_variance << ") \n";
                learn_params[1]->mutable_cpu_data()[f] = 0;

                learn_params[0]->Update();
                learn_params[1]->Update();
            }
            if(param_shape.size() == 2){
                /*
                //VAFFANCULOOOOOOOOOOOOOOOOOOOOO
                double filter_mean,filter_variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, filter_mean, filter_variance);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto mean_offset = d*(param_shape[2]*param_shape[3]) + j*(param_shape[3]) + i;
                            auto learn_offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[learn_offset] = mean[mean_offset];
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] -= new_mean;
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);

                //double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                //double target_variance = variance;
                double target_std_dev = std::sqrt(target_variance);
                double p = new_variance / target_variance;
                double sqrt_p = std::sqrt(p);

                for(int d = 0; d < param_shape[1]; ++d){
                    for(int j = 0; j < param_shape[2]; ++j){
                        for(int i = 0; i < param_shape[3]; ++i){
                            auto offset = learn_params[0]->offset(f,d,j,i);
                            learn_params[0]->mutable_cpu_data()[offset] /= sqrt_p;
                            assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }
                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                std::cout << f << ": mean (" << filter_mean << ") new_mean(" << new_mean << ") \n";
                std::cout << f << ": var (" << filter_variance << ") new_var(" << new_variance << ") \n";
                learn_params[1]->mutable_cpu_data()[f] = 0;

                learn_params[0]->Update();
                learn_params[1]->Update();
                */
            }
        }
#endif
    }
}
