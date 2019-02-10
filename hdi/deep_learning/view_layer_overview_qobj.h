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

#ifndef VIEW_LAYER_H
#define VIEW_LAYER_H

#include <vector>
#include <string>
#include "hdi/deep_learning/model_layer.h"
#include <QWidget>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "hdi/utils/visual_utils.h"
#include "hdi/visualization/dockable_widget_qobj.h"
#include "hdi/visualization/histogram_view_qobj.h"
#include "hdi/visualization/histogram_zcent_view_qobj.h"

namespace hdi{
    namespace dl{

        class ViewLayerOverview: public QWidget{
            Q_OBJECT
        public:
            typedef float scalar_type;
            typedef std::vector<scalar_type> scalar_vector_type;
            typedef std::vector<std::vector<scalar_type> > matrix_type;

        public:
            ViewLayerOverview(QWidget* parent = nullptr);
            void update();

        public slots:
            void onClickOnPerplexityBucket(int i){emit sgnClickOnPerplexityBucket(i);}
        signals:
            void sgnClickOnPerplexityBucket(int);


        public:
            std::shared_ptr<hdi::dl::ModelLayer> _model;

            std::shared_ptr<QLabel> _name_lbl;
            std::shared_ptr<hdi::viz::HistogramView> _patch_hist_wdg;
            std::shared_ptr<hdi::viz::HistogramZCentView> _changes_hist_wdg;

            std::shared_ptr<QGridLayout> _main_layout;
        };



    }
}
#endif
