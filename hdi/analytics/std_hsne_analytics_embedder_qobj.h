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

#ifndef STD_HSNE_ANALYTICS_EMBEDDER_H
#define STD_HSNE_ANALYTICS_EMBEDDER_H

#include <QObject>
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/visualization/abstract_view.h"
#include "hdi/visualization/abstract_scatterplot_drawer.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"


#include "hsne_analytics_interfaces_qobj.h"

namespace hdi{
    namespace analytics{

        class StdHSNEEmbedderFactory: public AbstractHSNEEmbedderFactory{
            Q_OBJECT
        public:
            typedef dr::SparseTSNEUserDefProbabilities<scalar_type,sparse_scalar_matrix_type> tsne_type;

        public:
            virtual ~StdHSNEEmbedderFactory(){};
            virtual std::shared_ptr<AbstractHSNEEmbedder> createEmbedder();

        public:
            tsne_type::Parameters _tsne_parameters;
        };

        class StdHSNEEmbedder: public AbstractHSNEEmbedder{
            Q_OBJECT
        public:
            typedef hdi::dr::SparseTSNEUserDefProbabilities<scalar_type,sparse_scalar_matrix_type> tsne_type;
            typedef tsne_type::Parameters tsne_param_type;
            typedef hdi::data::Embedding<scalar_type> embedding_type;

        public:
            virtual ~StdHSNEEmbedder(){};
            virtual void initialize(const sparse_scalar_matrix_type& probabilities);

        signals:
            void sgnNewEmbedder(id_type id);

        public slots:
            virtual void onIterate();

        private:
            tsne_type _tsne;
            tsne_param_type _tsne_params;
            embedding_type _embedding;
            hdi::viz::ScatterplotCanvas _viewer;
            hdi::viz::ScatterplotDrawerFixedColor _drawer;
            std::vector<uint32_t> _flags;
            hdi::viz::ControllerSelectionEmbedding _selection_controller;
        };
    }
}

#endif
