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

#include "hdi/deep_learning/cnn.h"
#include <QString>

#include "hdi/deep_learning/view_layer_overview_qobj.h"

namespace hdi{
    namespace dl{

        ViewLayerOverview::ViewLayerOverview(QWidget* parent):QWidget(parent),_model(nullptr){
           _main_layout = std::shared_ptr<QGridLayout>(new QGridLayout(this));
           _name_lbl = std::shared_ptr<QLabel>(new QLabel(this));

           _patch_hist_wdg = std::shared_ptr<hdi::viz::HistogramView>(new hdi::viz::HistogramView(nullptr));
           _patch_hist_wdg->setFixedSize(QSize(300,60)); //TODO
           _patch_hist_wdg->onUpdate();

           _changes_hist_wdg = std::shared_ptr<hdi::viz::HistogramZCentView>(new hdi::viz::HistogramZCentView(nullptr));
           _changes_hist_wdg->setFixedSize(QSize(300,60));//TODO
           _changes_hist_wdg->onUpdate();

           _main_layout->addWidget(_name_lbl.get());
           _main_layout->addWidget(_patch_hist_wdg.get());
           _main_layout->addWidget(_changes_hist_wdg.get());

           connect(_patch_hist_wdg.get(),&hdi::viz::HistogramView::sgnClickOnBucket,this,&ViewLayerOverview::onClickOnPerplexityBucket);
        }

        void ViewLayerOverview::update(){
            assert(_model != nullptr);

            _name_lbl->setText(_model->name().c_str());

            _patch_hist_wdg->setObjectName(_model->name().c_str());
            _patch_hist_wdg->setData(_model->smoothedPatchPerplexity());
            switch(_model->_type){
            case hdi::dl::ModelLayer::Convolutional:  _patch_hist_wdg->setColorName("steelblue"); break;
            case hdi::dl::ModelLayer::InnerProduct:   _patch_hist_wdg->setColorName("lightpink"); break;
            case hdi::dl::ModelLayer::SoftMax:        _patch_hist_wdg->setColorName("green"); break;
            };
            _patch_hist_wdg->onUpdate();


            _changes_hist_wdg->setObjectName(_model->name().c_str());
            _changes_hist_wdg->setData(_model->learningHistogram());
            _changes_hist_wdg->onUpdate();
        }

    }
}
