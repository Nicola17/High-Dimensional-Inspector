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

#ifndef TRAINED_CLASSIFIER_H
#define TRAINED_CLASSIFIER_H

#include <string>
#include "hdi/utils/abstract_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include <caffe/caffe.hpp>

//OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


namespace hdi{
    namespace dl{

        class Classifier{
        public:
            typedef float scalar_type;
            typedef std::pair<std::string, scalar_type> prediction_type;

        public:
            Classifier():_logger(nullptr){}
            void initialize(const std::string& model_file, const std::string& trained_file, const std::string& mean_file, const std::string& label_file);
            void initialize(std::shared_ptr<caffe::Net<scalar_type> > net);

            std::vector<scalar_type> classify(const cv::Mat& img);
            std::vector<prediction_type> classify(const cv::Mat& img, int n);

            //! Return the current log
            utils::AbstractLog* logger()const{return _logger;}
            //! Set a pointer to an existing log
            void setLogger(utils::AbstractLog* logger){_logger = logger;}

        private:
            void setMean(const std::string& mean_file);
            void wrapInputLayer(std::vector<cv::Mat>* input_channels);
            void preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);

        private:
            utils::AbstractLog* _logger;
            std::shared_ptr<caffe::Net<scalar_type> > _net;
            int _num_channels;
            cv::Size _input_geometry;
            std::vector<std::string> _labels;
            cv::Mat _mean;
        };

    }
}
#endif
