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

#ifndef VIEW_TRAINING_H
#define VIEW_TRAINING_H

#include <vector>
#include <string>
#include "hdi/deep_learning/model_training.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include "hdi/visualization/linechart_dual_view_qobj.h"

namespace hdi{
    namespace dl{

        class ViewTraining{
        public:
            typedef float scalar_type;
            typedef std::vector<scalar_type> scalar_pair_vector_type;
            typedef std::vector<std::vector<scalar_type> > matrix_type;

        public:
            ViewTraining(QGridLayout* layout):_model(nullptr),_layout(layout){ //TODO PARENT
                //setStyleSheet( "QWidget{ background-color : rgba( 160, 160, 160, 255); border-radius : 7px;  }" );
                _loss_accuracy_chart = std::shared_ptr<hdi::viz::LineChartDualView>(new hdi::viz::LineChartDualView(nullptr));
                _loss_accuracy_chart->show();
                layout->addWidget(_loss_accuracy_chart.get());
            }

            void update(){
                _loss_accuracy_chart->setData(_model->_loss);
                _loss_accuracy_chart->setDualData(_model->_accuracy);
                _loss_accuracy_chart->setMaxX(_model->_iter+1);
                _loss_accuracy_chart->setMinX(0);
                _loss_accuracy_chart->setMaxY(_model->_max_loss*1.1);
                _loss_accuracy_chart->setMinY(0);
                _loss_accuracy_chart->setDualMaxY(100);
                _loss_accuracy_chart->setDualMinY(0);
                _loss_accuracy_chart->onUpdate();
            }


        public://TEMP
            hdi::dl::ModelTraining* _model;
            std::shared_ptr<hdi::viz::LineChartDualView> _loss_accuracy_chart;
            QGridLayout* _layout;

        };

    }
}
#endif
