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

#ifndef CONTROLLER_TRAINING_H
#define CONTROLLER_TRAINING_H

#include <vector>
#include "hdi/deep_learning/model_caffe_solver.h"
#include "hdi/deep_learning/model_layer.h"
#include "hdi/deep_learning/view_layer_overview_qobj.h"
#include "hdi/deep_learning/view_layer_detailed_qobj.h"
#include "hdi/deep_learning/view_training.h"
#include "hdi/deep_learning/model_training.h"
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTabWidget>
#include "hdi/deep_learning/controller_layer_qobj.h"
#include "hdi/deep_learning/view_embedding.h"


namespace hdi{
    namespace dl{

        class ControllerTraining: public QObject{
        public:
            typedef float scalar_type;
            typedef std::vector<scalar_type> scalar_vector_type;
            typedef std::vector<std::vector<scalar_type> > matrix_type;

        public:
            void initialize(std::shared_ptr<ModelCaffeSolver> solver, std::shared_ptr<hdi::dl::ModelTraining> model_training, std::shared_ptr<hdi::dl::ViewTraining> view_training);
            void iterate();

        private:
            void updateSimilarities(const ModelCaffeSolver& solver, ModelLayer& layer);
            void collectData(const ModelCaffeSolver& solver, ModelLayer& layer);

            void checkData(const ModelCaffeSolver& solver);
            void checkFilters(const ModelCaffeSolver& solver);
            void checkBotVecs(const ModelCaffeSolver& solver, ModelLayer& layer);
            void killLayer(const ModelCaffeSolver& solver, ModelLayer& layer);

            void optimizeLayer(const ModelCaffeSolver& solver, ModelLayer& layer);

            void testEmbeddingGeneration(const ModelCaffeSolver& solver, ModelLayer& layer);


        //final data
        public:
            //logger
            hdi::utils::AbstractLog* _log;
            //model of the solver
            std::shared_ptr<ModelCaffeSolver> _solver;
            //model for the training
            std::shared_ptr<hdi::dl::ModelTraining> _model_training;
            //view for the training (loss and accuracy curves)
            std::shared_ptr<hdi::dl::ViewTraining> _view_training;
            //controllers for every layer
            std::vector<std::shared_ptr<ControllerLayer>> _controllers_embedding;
            //models for the layers (shared with the layer controllers)
            std::vector<std::shared_ptr<ModelLayer>>   _layers;
            //Overviews for the every layer
            std::vector<std::shared_ptr<ViewLayerOverview>> _layer_views;
            //Detailed view of evry layer
            std::vector<std::shared_ptr<ViewLayerDetailed>> _views_layer_detailed;
            //Label's palette
            std::shared_ptr<std::map<scalar_type,QColor>> _palette_labels;

            //training iteration
            int _iter;

        public:
            QWidget* _settings_wdg;
            QVBoxLayout* _overview_vlayout;

            QTabWidget* _layer_tab_wdg;
            std::vector<QWidget*> _layer_wdgs;

            //QTabWidget* _data_embedding_tab_wdg;
            //std::vector<QWidget*> _data_embedding_wdgs;

            //TODO
            QCheckBox* _settings_train;



            std::vector<std::shared_ptr<ViewEmbedding>> _views_embedding;


        };

    }
}
#endif
