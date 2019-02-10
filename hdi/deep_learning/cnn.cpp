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

namespace hdi{
    namespace dl{

    void Classifier::initialize(const std::string& model_file, const std::string& trained_file, const std::string& mean_file, const std::string& label_file){
        //#ifdef CPU_ONLY
        //Caffe::set_mode(Caffe::CPU);
        //#elses
        caffe::Caffe::set_mode(caffe::Caffe::GPU);
        //#endif

        utils::secureLog(_logger,"Loading the network...");
        _net.reset(new caffe::Net<scalar_type>(model_file, caffe::TEST));
        _net->CopyTrainedLayersFrom(trained_file);

        utils::secureLog(_logger,"Layer names");
        auto& layer_names = _net->layer_names();
        utils::secureLogVector(_logger,"Layer names", layer_names);

        hdi::checkAndThrowRuntime(_net->num_inputs() == 1, "Network should have exactly one input.");
        hdi::checkAndThrowRuntime(_net->num_outputs() == 1, "Network should have exactly one output.");
        caffe::Blob<scalar_type>* input_layer = _net->input_blobs()[0];
        _num_channels = input_layer->channels();
        hdi::checkAndThrowRuntime(_num_channels == 3 || _num_channels == 1, "Input layer should have 1 or 3 channels.");
        _input_geometry = cv::Size(input_layer->width(), input_layer->height());

        utils::secureLog(_logger,"Loading the binaryproto mean file...");
        setMean(mean_file);

        utils::secureLog(_logger,"Loading labels...");
        {
            std::ifstream labels(label_file.c_str());
            CHECK(labels) << "Unable to open labels file " << label_file;
            std::string line;
            while (std::getline(labels, line)){
                _labels.push_back(line);
            }
        }

        caffe::Blob<scalar_type>* output_layer = _net->output_blobs()[0];
        hdi::checkAndThrowRuntime(_labels.size() == output_layer->channels(), "Number of labels is different from the output layer dimension.");
    }

    void Classifier::initialize(std::shared_ptr<caffe::Net<scalar_type> > net){
        utils::secureLog(_logger,"Get shared network...");
        _net = net;

        utils::secureLog(_logger,"Layer names");
        auto& layer_names = _net->layer_names();
        utils::secureLogVector(_logger,"Layer names", layer_names);

        hdi::checkAndThrowRuntime(_net->num_inputs() == 1, "Network should have exactly one input.");
        hdi::checkAndThrowRuntime(_net->num_outputs() == 1, "Network should have exactly one output.");
        caffe::Blob<scalar_type>* input_layer = _net->input_blobs()[0];
        _num_channels = input_layer->channels();
        hdi::checkAndThrowRuntime(_num_channels == 3 || _num_channels == 1, "Input layer should have 1 or 3 channels.");
        _input_geometry = cv::Size(input_layer->width(), input_layer->height());

        /*
        utils::secureLog(_logger,"Loading the binaryproto mean file...");
        setMean(mean_file);
        */

        _labels.clear();
        for(int i = 0; i < _net->output_blobs()[0]->channels(); ++i){
            _labels.push_back(QString("label %1").arg(i).toStdString());
        }
    }

    void Classifier::setMean(const std::string& mean_file) {
        caffe::BlobProto blob_proto;
        caffe::ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

        /* Convert from BlobProto to Blob<scalar_type> */
        caffe::Blob<scalar_type> mean_blob;
        mean_blob.FromProto(blob_proto);
        hdi::checkAndThrowRuntime(mean_blob.channels() == _num_channels, "Number of channels of mean file doesn't match input layer.");

        /* The format of the mean file is planar 32-bit scalar_type BGR or grayscale. */
        std::vector<cv::Mat> channels;
        scalar_type* data = mean_blob.mutable_cpu_data();
        for (int i = 0; i < _num_channels; ++i) {
            /* Extract an individual channel. */
            cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
            channels.push_back(channel);
            data += mean_blob.height() * mean_blob.width();
        }

        /* Merge the separate channels into a single image. */
        cv::Mat mean;
        cv::merge(channels, mean);

        /* Compute the global mean pixel value and create a mean image filled with this value. */
        cv::Scalar channel_mean = cv::mean(mean);
        _mean = cv::Mat(_input_geometry, mean.type(), channel_mean);
    }


    std::vector<Classifier::scalar_type> Classifier::classify(const cv::Mat& img){
        caffe::Blob<float>* input_layer = _net->input_blobs()[0];
        input_layer->Reshape(1, _num_channels, _input_geometry.height, _input_geometry.width);
        _net->Reshape();

        std::vector<cv::Mat> input_channels;
        wrapInputLayer(&input_channels);
        preprocess(img, &input_channels);

        _net->ForwardPrefilled();

        caffe::Blob<float>* output_layer = _net->output_blobs()[0];
        const float* begin = output_layer->cpu_data();
        const float* end = begin + output_layer->channels();
        return std::vector<scalar_type>(begin, end);
    }

    std::vector<Classifier::prediction_type> Classifier::classify(const cv::Mat& img, int n){
        std::vector<scalar_type> output = classify(img);
        std::vector<prediction_type> predictions;
        predictions.reserve(output.size());
        for(int i = 0; i < output.size(); ++i){
            predictions.push_back(prediction_type(_labels[i],output[i]));
        }
        std::partial_sort(predictions.begin(), predictions.begin()+n, predictions.end(), [](const prediction_type& a, const prediction_type& b){return a.second > b.second;});
        predictions.resize(n);

        return predictions;
    }


    /////////////////////////////////////
    void Classifier::wrapInputLayer(std::vector<cv::Mat>* input_channels){
        caffe::Blob<float>* input_layer = _net->input_blobs()[0];
        int width = input_layer->width();
        int height = input_layer->height();
        float* input_data = input_layer->mutable_cpu_data();
        for (int i = 0; i < input_layer->channels(); ++i) {
            cv::Mat channel(height, width, CV_32FC1, input_data);
            input_channels->push_back(channel);
            input_data += width * height;
        }
    }

    void Classifier::preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels){
        /* Convert the input image to the input image format of the network. */
        cv::Mat sample;
        if (img.channels() == 3 && _num_channels == 1){
            cv::cvtColor(img, sample, CV_BGR2GRAY);
        }else if (img.channels() == 4 && _num_channels == 1){
            cv::cvtColor(img, sample, CV_BGRA2GRAY);
        }else if (img.channels() == 4 && _num_channels == 3){
            cv::cvtColor(img, sample, CV_BGRA2BGR);
        }else if (img.channels() == 1 && _num_channels == 3){
            cv::cvtColor(img, sample, CV_GRAY2BGR);
        }else{
            sample = img;
        }

        cv::Mat sample_resized;
        if (sample.size() != _input_geometry){
            cv::resize(sample, sample_resized, _input_geometry);
        }else{
            sample_resized = sample;
        }

        //cv::imwrite("wuuu_resized.png",sample_resized);
        cv::Mat sample_float;
        if (_num_channels == 3)
            sample_resized.convertTo(sample_float, CV_32FC3);
        else
            sample_resized.convertTo(sample_float, CV_32FC1);
        //cv::imwrite("wuuu_channels.png",sample_resized);

        cv::Mat sample_normalized;
        cv::subtract(sample_float, _mean, sample_normalized);
        //cv::imwrite("wuuu_normalized.png",sample_resized);

        /* This operation will write the separate BGR planes directly to the
        * input layer of the network because it is wrapped by the cv::Mat
        * objects in input_channels. */
        cv::split(sample_normalized, *input_channels);

        hdi::checkAndThrowRuntime(reinterpret_cast<float*>(input_channels->at(0).data) == _net->input_blobs()[0]->cpu_data(), "Input channels are not wrapping the input layer of the network.");
    }

    }
}
