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

#ifndef CONTROLLER_EMBEDDING_H
#define CONTROLLER_EMBEDDING_H

#include <vector>
#include "hdi/deep_learning/model_caffe_solver.h"
#include "hdi/deep_learning/model_layer.h"
#include "hdi/deep_learning/view_layer_overview_qobj.h"
#include "hdi/deep_learning/view_training.h"
#include "hdi/deep_learning/model_training.h"
#include "hdi/deep_learning/view_embedding.h"
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QInputDialog>

#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_multiple_scalar_attributes.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute_with_colors.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/deep_learning/view_layer_detailed_qobj.h"

namespace hdi{
    namespace dl{

        class ControllerLayerHSNEIntializer;

        class ControllerLayer: public QObject{
            Q_OBJECT
        public:
            typedef float scalar_type;
            typedef std::vector<scalar_type> scalar_vector_type;
            typedef std::vector<std::vector<scalar_type> > matrix_type;

        public:
            void initialize(std::shared_ptr<ModelCaffeSolver> solver, std::shared_ptr<ModelLayer> layer, std::shared_ptr<ViewEmbedding> view_input_landscape, std::shared_ptr<ViewLayerDetailed> view_layer_detailed, std::shared_ptr<ViewLayerOverview> view_layer_overview, QWidget* widget);


        //final functions
        public slots:
            //called when new data is fed through the network
            void onNewData();


        public slots:
            void onClearInputData();
            void onComputeEmbedding();
            void onComputeHSNEEmbedding();
            void onOptimizeForSelection();
            void onChangeSelectedProperty(int id){_layer->_selected_property = id; if(_multiscale_embedder.get()!=nullptr)_multiscale_embedder->onUpdateViews();}
            void onClearHSNEInterface();
            void onHSNEDrill();
            void onLRMultChanged(double value);
            void onSwitchEmbeddingView();
            void onSortFilters();
            void onResetFilterTimings();

            void onSelectFilter(int id);
            void onSelectPerplexityBucket(int id);
            void onSelectionOnEmbedding(analytics::MultiscaleEmbedderSingleView* ptr);

        private:
            void collectData();
            void collectDataConvolution();
            void collectDataFullyConnected();

            void hsneGradientDescentThreadWorker();

            void updatePerplexityHistograms();
            void updateActivationValues();
            void updatePerplexityActivationValues();

            void updateWeightedJaccard();

        private:
        public:
            std::shared_ptr<ModelCaffeSolver> _solver;
            std::shared_ptr<ModelLayer> _layer;
            std::shared_ptr<ViewLayerDetailed> _view_layer_detailed;
            std::shared_ptr<ViewLayerOverview> _view_layer_overview;
            std::shared_ptr<ViewEmbedding> _view_input_landscape;


            std::shared_ptr<QGridLayout> _main_gridlayout;
            std::shared_ptr<QGroupBox> _options_grpbx;
            std::shared_ptr<QWidget> _view_wdg;
            std::shared_ptr<QTabWidget> _landscape_analysis_tabwdg;

            //Controls
            std::shared_ptr<QGridLayout> _controls_layout;
            std::shared_ptr<QLabel> _inputs_available_lbl;
            std::shared_ptr<QPushButton> _collect_inputs_btn;
            std::shared_ptr<QPushButton> _reset_data_btn;
            std::shared_ptr<QPushButton> _compute_embedding_btn;
            std::shared_ptr<QPushButton> _compute_hsne_btn;
            std::shared_ptr<QPushButton> _optimize_for_selection_btn;
            std::shared_ptr<QPushButton> _hsne_drill_btn;
            std::shared_ptr<QPushButton> _switch_embedding_view_btn;
            std::shared_ptr<QPushButton> _sort_filters_btn;
            std::shared_ptr<QPushButton> _reset_filter_timings_btn;
            std::shared_ptr<QComboBox> _visible_feature_cmbbx;
            std::shared_ptr<QComboBox> _heat_modes_cmbbx;
            std::shared_ptr<QDoubleSpinBox> _lr_multiplier_dspbx;
            std::shared_ptr<QCheckBox> _filter_similiarities_chbx;

            bool _feature_view;

            //For atsne
            std::shared_ptr<hdi::viz::ScatterplotCanvas> _viewer;
            std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttributeWithColors> _drawer;
            std::shared_ptr<viz::ControllerSelectionEmbedding> _controller_selection;
            std::shared_ptr<hdi::viz::MultipleImageView> _view;
            std::shared_ptr<hdi::data::Embedding<scalar_type>> _embedding;
            std::shared_ptr<ControllerLayerHSNEIntializer> _interface_initializer;

            scalar_vector_type _similarity_confidence;
            std::vector<QColor> _similarity_colors;

            //For HSNE
            std::shared_ptr<hdi::analytics::MultiscaleEmbedderSystem> _multiscale_embedder;
            int _samples_per_input;
            std::vector<hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type> _id_embeddings;
            std::shared_ptr<std::map<scalar_type, QColor>> _palette_labels;

            //For RF creation
            double _rf_zero_value;
            double _rf_scale;

            int _iter;

        public:
            hdi::utils::AbstractLog* _log;
        };


        class ControllerLayerHSNEIntializer: public QObject, public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
            Q_OBJECT
        public:
            virtual ~ControllerLayerHSNEIntializer(){}
            virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
                connect(embedder, &embedder_type::sgnSelectionPtr, this, &ControllerLayerHSNEIntializer::onSelectionOnEmbedder);


                std::shared_ptr<hdi::viz::ScatterplotDrawerLabels> drawer_labels(new hdi::viz::ScatterplotDrawerLabels());
                embedder->addUserDefinedDrawer(drawer_labels);
                drawer_labels->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(),embedder->getPanelData().getProperty("Label").data(), *_palette_labels, embedder->getPanelData().numDataPoints());
                drawer_labels->setPointSize(30);
                drawer_labels->setAlpha(0.01);


                //I save the reference of the current embedder
                _ids->push_back(embedder->getId());
                std::shared_ptr<hdi::viz::ScatterplotDrawerMultipleScalarAttributes> drawer(new hdi::viz::ScatterplotDrawerMultipleScalarAttributes());
                embedder->addUserDefinedDrawer(drawer);

                drawer->setLimits(0,_layer->_layer_max_activation.second);
                drawer->setAlpha(0.7);
                //drawer->setPointSize(30);
                drawer->setPointSize(60);
                {
                    std::vector<std::string> names;
                    embedder->getPanelData().getAvailableProperties(names);
                    std::vector<const scalar_type*> attributes;
                    for(auto name: names){
                        attributes.push_back(embedder->getPanelData().getProperty(name).data());
                    }
                    _dummy_flags.resize(embedder->getPanelData().numDataPoints(),0);
                    drawer->setData(embedder->getEmbedding().getContainer().data(), attributes, &_layer->_selected_property, _dummy_flags.data(), embedder->getPanelData().numDataPoints());
                }

               std::shared_ptr<hdi::viz::MultipleImageView> view = std::shared_ptr<hdi::viz::MultipleImageView>(new hdi::viz::MultipleImageView());
               embedder->addView(view);
               view->setDetailsVisible(false);
               view->show();

               QString text = QString("Scale %1 - Embedding %2").arg(1+std::get<0>(embedder->getId())).arg(1+std::get<1>(embedder->getId()));
               QWidget* wdg = new QWidget(_tab_widget);
               QHBoxLayout* layout = new QHBoxLayout(wdg);
               //_tab_widget->addTab(embedder->getCanvas(),text);
               _tab_widget->addTab(wdg,text);
               _tab_widget->setCurrentIndex(_tab_widget->count()-1);

               wdg->setLayout(layout);
               layout->addWidget(embedder->getCanvas());
               layout->addWidget(view.get());
               embedder->getCanvas()->setFixedSize(500,300);

               //text = QInputDialog::getText(nullptr, "HSNE analysis name", "Name:", QLineEdit::Normal);
               _tab_widget->setTabText(_tab_widget->count()-1,text);
            }
            virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence){
                std::shared_ptr<hdi::viz::ScatterplotDrawerLabels> drawer_labels(new hdi::viz::ScatterplotDrawerLabels());
                embedder->addAreaOfInfluenceDrawer(drawer_labels);
                drawer_labels->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(),embedder->getPanelData().getProperty("Label").data(), *_palette_labels, embedder->getPanelData().numDataPoints());
                drawer_labels->setPointSize(7.5);
            }
            virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){}
            virtual void updateSelection(embedder_type* embedder, scalar_type* selection){}

            virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){}

        private slots:
            void onSelectionOnEmbedder(embedder_type* ptr){emit sgnSelectionOnEmbedder(ptr);}

        signals:
            void sgnSelectionOnEmbedder(embedder_type* ptr);

        public:
            QTabWidget* _tab_widget;
            std::shared_ptr<ModelLayer> _layer;
            std::vector<hdi::analytics::MultiscaleEmbedderSystem::embedder_id_type>* _ids;
            std::shared_ptr<std::map<scalar_type, QColor>> _palette_labels;
            std::vector<unsigned int> _dummy_flags;

        };
    }
}
#endif
