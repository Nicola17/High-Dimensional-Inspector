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

#include "hdi/deep_learning/view_layer_detailed_qobj.h"
#include "hdi/utils/visual_utils.h"

namespace hdi{
    namespace dl{


        ViewLayerDetailed::ViewLayerDetailed(QWidget* parent):QWidget(parent),_model(nullptr){
           _main_layout = std::shared_ptr<QGridLayout>(new QGridLayout(this));
           //_similarity_lbl = std::shared_ptr<QLabel>(new QLabel(this));
           _filter_activations_viz = std::shared_ptr<hdi::viz::DLFilterViz>(new hdi::viz::DLFilterViz());
           _filter_activations_perplexity_viz = std::shared_ptr<hdi::viz::DLFilterViz>(new hdi::viz::DLFilterViz());

           _filter_activations_viz->show();
           //_filter_activations_perplexity_viz->show();

           _main_layout->addWidget(_filter_activations_viz.get(),0,0);
           //_main_layout->addWidget(_filter_activations_perplexity_viz.get(),1,0);
           //_main_layout->addWidget(_similarity_lbl.get(),2,0);

           //Connections
           connect(_filter_activations_viz.get(),&hdi::viz::DLFilterViz::sgnClickOnFilter,this,&ViewLayerDetailed::onClickOnFilter);
           connect(_filter_activations_perplexity_viz.get(),&hdi::viz::DLFilterViz::sgnClickOnFilter,this,&ViewLayerDetailed::onClickOnFilter);
        }

        void ViewLayerDetailed::update(){
            assert(_model != nullptr);
            updateFilterVisualization();
            //auto img = getFilterSimilarityImage();
            //img.save(QString("filter_sim_%1.png").arg(_model->name().c_str()));
            //_similarity_lbl->setPixmap(QPixmap::fromImage(img.scaledToWidth(100)));
        }

        void ViewLayerDetailed::updateFilterVisualization(){
            //setting the data
            _filter_activations_viz->setCurrentIteration(_model->_last_refresh);
            _filter_activations_viz->setData(_model->_max_activations);
            _filter_activations_viz->setFilterNames(_model->_filter_names);
            _filter_activations_viz->setFilterOrder(_model->_order_in_visualization);
            _filter_activations_viz->setFilterFlags(_model->_filter_data.getFlagsDataPoints());
            _filter_activations_viz->setMaxValue(_model->_layer_max_activation.second);
            _filter_activations_viz->setFrequencies(_model->_activation_freq);
            _filter_activations_viz->updateView();

            /*
            _filter_activations_perplexity_viz->setCurrentIteration(_model->_last_refresh);
            _filter_activations_perplexity_viz->setData(_model->_max_activations_perplexity);
            _filter_activations_perplexity_viz->setFilterNames(_model->_filter_names);
            _filter_activations_perplexity_viz->setFilterFlags(_model->_filter_data.getFlagsDataPoints());
            _filter_activations_perplexity_viz->setFilterOrder(_model->_order_in_visualization);
            //_filter_activations_perplexity_viz->setMaxValue(_model->_layer_max_activation.second);
            _filter_activations_perplexity_viz->updateView();
            */
        }

        QImage ViewLayerDetailed::getFilterMaxActivationImage()const{
            QRgb bgColor(qRgb(100,50,50));
            int elem_per_row(std::ceil(std::sqrt(_model->size())));
            QImage img(elem_per_row,elem_per_row,QImage::Format_ARGB32);
            for(int j = 0; j < elem_per_row; ++j){
                for(int i = 0; i < elem_per_row; ++i){
                    img.setPixel(i,j,bgColor);
                }
            }
            double max = _model->_layer_max_activation.second;
            for(int i = 0; i < _model->size(); ++i){
                int id_viz = _model->_order_in_visualization[i];
                if(_model->_max_activations[i].second == 0){
                    img.setPixel(id_viz%elem_per_row,id_viz/elem_per_row,qRgb(50,100,50));
                }else{
                    auto v = _model->_max_activations[i].second/max * 255;
                    img.setPixel(id_viz%elem_per_row,id_viz/elem_per_row,qRgb(v,v,v));
                }
            }
            int desired_size = 200;

            return img.scaledToWidth(int(desired_size/elem_per_row)*elem_per_row);
        }
        QImage ViewLayerDetailed::getFilterRefreshActivationImage()const{
            QRgb bgColor(qRgb(100,50,50));
            int elem_per_row(std::ceil(std::sqrt(_model->size())));
            QImage img(elem_per_row,elem_per_row,QImage::Format_ARGB32);
            for(int j = 0; j < elem_per_row; ++j){
                for(int i = 0; i < elem_per_row; ++i){
                    img.setPixel(i,j,bgColor);
                }
            }
            double max_iter = 500;
            for(int i = 0; i < _model->size(); ++i){
                double v = (_model->_last_refresh - _model->_max_activations[i].first);
                v /= max_iter;
                if(v < 0){v = 0;}
                if(v > 1){v = 1;}
                v = (1-v) * 255;
                int id_viz = _model->_order_in_visualization[i];
                img.setPixel(id_viz%elem_per_row,id_viz/elem_per_row,qRgb(v,v,v));
            }
            int desired_size = 200;

            return img.scaledToWidth(int(desired_size/elem_per_row)*elem_per_row);
        }

        QImage ViewLayerDetailed::getFilterSimilarityImage()const{
            return hdi::utils::imageFromMatrix<scalar_type>(_model->_filter_similarity,1);
        }

    }
}
