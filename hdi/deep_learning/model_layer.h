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

#ifndef MODEL_CAFFE_LAYER_H
#define MODEL_CAFFE_LAYER_H

#include <vector>
#include <string>
#include "hdi/data/histogram.h"
#include "hdi/data/panel_data.h"
#include "hdi/data/embedding.h"

namespace hdi{
    namespace dl{

        class ModelLayer{
        public:
            typedef float scalar_type;
            typedef uint32_t unsigned_int_type;
            typedef std::pair<unsigned_int_type,scalar_type> timed_activation_type;
            typedef std::vector<scalar_type> scalar_vector_type;
            typedef std::vector<uint32_t> uint_vector_type;
            typedef std::vector<timed_activation_type> timed_activation_vector_type;
            typedef std::vector<std::vector<scalar_type> > matrix_type;
            typedef data::Histogram<scalar_type> histogram_type;
            enum layer_type {Convolutional,InnerProduct,SoftMax};

        public:

            void initialize(int n, int num_labels);
            int size()const{return _filter_similarity.size();}
            int id()const{return _id;}
            int& id(){return _id;}
            std::string& name(){return _name;}
            const std::string& name()const{return _name;}
            int historySize()const{return _smoothed_patch_perplexity_history.size();}

            void updateSmoothedHistogram();
            void computeLearningHistogram();

            histogram_type& patchPerplexity(){return _patch_perplexity;}
            const histogram_type& patchPerplexity()const{return _patch_perplexity;}
            histogram_type& smoothedPatchPerplexity(){return _smoothed_patch_perplexity_history[_smoothed_history_id];}
            const histogram_type& smoothedPatchPerplexity()const{return _smoothed_patch_perplexity_history[_smoothed_history_id];}
            histogram_type& learningHistogram(){return _learning_histogram;}
            const histogram_type& learningHistogram()const{return _learning_histogram;}


        private:
            int circularHistoryId(int id);



            //final data
        public:
            //layer type
            layer_type _type;

            //threshold for refreshing an activation iter
            scalar_type _refresh_thresh;
            //last refresh iteration
            unsigned_int_type _last_refresh;

            //maximum activation for a filter
            timed_activation_vector_type _max_activations;
            //maximum activation for a filter
            timed_activation_vector_type _max_activations_perplexity;
            //max activation ever registered in the net
            timed_activation_type _layer_max_activation;
            //order of the filters in the visualizations
            uint_vector_type _order_in_visualization;

            //used for the creation of the similarity embedding
            matrix_type _activation_sum_per_label;



            //filter names
            std::vector<std::string> _filter_names;
            //data for input landscape
            data::PanelData<scalar_type> _filter_data;

            //similarity between filters
            matrix_type _filter_similarity;

            //Receptive field parameters
            int _rf_offset;
            int _rf_stride;
            int _rf_size;

            //propertiy to be visualized in the embedding view
            unsigned int _selected_property;

            //data for input landscape
            data::PanelData<scalar_type> _input_data;

            //Histograms
            histogram_type _patch_perplexity;
            std::vector<histogram_type> _smoothed_patch_perplexity_history;
            histogram_type _learning_histogram;
            int _selected_histogram_bucket;
            int _num_labels;

        public:
            matrix_type _filter_distance;
            matrix_type _filter_confidence;

            matrix_type _weighted_jaccard_numerator;
            matrix_type _weighted_jaccard_denumerator;
            scalar_vector_type _total_activation;
            scalar_vector_type _activation_freq;
            scalar_vector_type _activation_freq_num;
            scalar_vector_type _activation_freq_den;

            //input landscape (AtSNE)
            data::Embedding<scalar_type> _embedding;

        private:

            std::string _name;
            int _id;
            int _smoothed_history_id;

            bool _valid_history;
        };

    }
}
#endif
