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

#include "hdi/deep_learning/model_layer.h"
#include "hdi/data/empty_data.h"

namespace hdi{
    namespace dl{

        void ModelLayer::initialize(int n, int num_labels){
            _refresh_thresh = 0.7;
            _last_refresh = 0;

            _layer_max_activation = timed_activation_type(0,0);
            _max_activations = timed_activation_vector_type(n,timed_activation_type(0,0));
            _max_activations_perplexity = timed_activation_vector_type(n,timed_activation_type(0,0));
            _num_labels = num_labels;
            _activation_sum_per_label = matrix_type(_max_activations.size(),scalar_vector_type(num_labels,0));

            _selected_histogram_bucket = -1;

            //_filter_flags = uint_vector_type(n);
            _order_in_visualization = uint_vector_type(n);
            std::iota(_order_in_visualization.begin(),_order_in_visualization.end(),0);

            _filter_data.addDimension(std::shared_ptr<hdi::data::EmptyData>(new hdi::data::EmptyData()));
            _filter_data.initialize();

            _filter_names.reserve(n);
            for(int i = 0; i < n; ++i){
                _filter_names.push_back(QString("Filter_%1").arg(i).toStdString());
                _filter_data.addDataPoint(std::shared_ptr<hdi::data::EmptyData>(new hdi::data::EmptyData()),std::vector<scalar_type>(1,0));
            }

            /////////////////////////////////////////////////

            _filter_similarity = matrix_type(n,matrix_type::value_type(n,0));
            _filter_distance = matrix_type(n,matrix_type::value_type(n,0));
            _filter_confidence = matrix_type(n,matrix_type::value_type(n,0));
            _weighted_jaccard_numerator = matrix_type(n,matrix_type::value_type(n,0));
            _weighted_jaccard_denumerator = matrix_type(n,matrix_type::value_type(n,0));
            _total_activation = scalar_vector_type(n,0);
            _activation_freq = scalar_vector_type(n,0);
            _activation_freq_den = scalar_vector_type(n,0);
            _activation_freq_num = scalar_vector_type(n,0);

            _valid_history = false;

            //_patch_perplexity.resize(0,n,n);
            //_patch_perplexity.resize(1,n,std::min(n,20));
            int buckets = 30;
            _patch_perplexity.resize(1,n,buckets);
            _learning_histogram.resize(1,n,buckets);

            _smoothed_history_id = 0;
            _smoothed_patch_perplexity_history.resize(20);
        }

        int ModelLayer::circularHistoryId(int id){
            return id%historySize();
        }

        void ModelLayer::updateSmoothedHistogram(){
            if(!_valid_history){
                for(int i = 0; i < historySize(); ++i){
                    _smoothed_patch_perplexity_history[i] = _patch_perplexity;
                }
                _valid_history = true;
                return;
            }

            double memory_factor = 0.99;

            int old_id = _smoothed_history_id;
            _smoothed_history_id = circularHistoryId(_smoothed_history_id+1);

            _smoothed_patch_perplexity_history[_smoothed_history_id].clear();
            for(int i = 0; i < _smoothed_patch_perplexity_history[_smoothed_history_id].num_buckets(); ++i){
                const auto old_val = _smoothed_patch_perplexity_history[old_id].data()[i];
                const auto new_val = _patch_perplexity.data().data()[i];
                const auto update_val = old_val * memory_factor + new_val * (1-memory_factor);

                _smoothed_patch_perplexity_history[_smoothed_history_id].data()[i] = update_val;
            }


        }
        void ModelLayer::computeLearningHistogram(){
            int oldest_hist = circularHistoryId(_smoothed_history_id+1);
            for(int i = 0; i < _smoothed_patch_perplexity_history[_smoothed_history_id].num_buckets(); ++i){
                const auto old_val = _smoothed_patch_perplexity_history[oldest_hist].data()[i];
                const auto new_val = _smoothed_patch_perplexity_history[_smoothed_history_id].data().data()[i];

                _learning_histogram.data()[i] = new_val-old_val;
            }
        }
    }
}
