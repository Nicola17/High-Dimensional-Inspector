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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>

#include "hdi/utils/vector_utils.h"
#include "hdi/deep_learning/cnn.h"
#include "hdi/utils/cout_log.h"

#include "caffe/util/io.hpp"
#include "caffe/util/signal_handler.h"
#include <QImage>

// Parse GPU ids or use all available devices
void get_gpus(std::vector<int>* gpus) {
    int count = 0;
#ifndef CPU_ONLY
    CUDA_CHECK(cudaGetDeviceCount(&count));
#else
    NO_GPU;
#endif
    for (int i = 0; i < count; ++i) {
      gpus->push_back(i);
    }
}

int main(int argc, char** argv) {
    try{
        hdi::utils::CoutLog logger;

        caffe::SolverParameter solver_param;
        caffe::ReadProtoFromTextFileOrDie(argv[1], &solver_param);

        std::vector<int> gpus;
        get_gpus(&gpus);
        if(gpus.size() == 0){
          logger.display("Use CPU");
          caffe::Caffe::set_mode(caffe::Caffe::CPU);
        }else{
          std::ostringstream s;
          for (int i = 0; i < gpus.size(); ++i) {
            s << (i ? ", " : "") << gpus[i];
          }
          hdi::utils::secureLogValue(&logger, "Using GPUs ", s.str());

          solver_param.set_device_id(gpus[0]);
          caffe::Caffe::SetDevice(gpus[0]);
          caffe::Caffe::set_mode(caffe::Caffe::GPU);
          caffe::Caffe::set_solver_count(gpus.size());
        }

        //?
        //caffe::SignalHandler signal_handler(GetRequestedAction(FLAGS_sigint_effect), GetRequestedAction(FLAGS_sighup_effect));

        std::shared_ptr<caffe::Solver<float> > solver(caffe::SolverRegistry<float>::CreateSolver(solver_param));
        //solver->Solve();
        //auto& net = *(solver->net().get());
        std::shared_ptr<caffe::Net<float> > net(solver->net().get());
        auto& net_layers = net->layers();
/*
        for(int i = 0; i < net_layers.size(); ++i){
            auto& learn_params = net_layers[i]->blobs();
            for(int p = 0; p < learn_params.size(); ++p){
                auto shape = learn_params[p]->shape();
                for(int x = 0; x < shape[0]; ++x)
                    for(int y = 0; y < shape[1]; ++y)
                        for(int w = 0; w < shape[2]; ++w)
                            for(int z = 0; z < shape[3]; ++z){
                                learn_params[p]->data()
                            }
            }
        }
*/
        for(int n = 0; n < 10000; ++n){
            solver->Step(1);
/*
            {//activations
                auto blobs = net->top_vecs();
                //hdi::utils::secureLogValue(&logger,"size",blobs.size());
                //hdi::utils::secureLogVector(&logger,"size",blobs[0][0]->shape());
                //hdi::utils::secureLogVector(&logger,"size",blobs[0][1]->shape());
                //hdi::utils::secureLogVector(&logger,"size",blobs[1][0]->shape());

                for(int d = 0; d < 64; ++d){
                    QImage img(28,28,QImage::Format_ARGB32);
                    for(int j = 0; j < 28; ++j){
                        for(int i = 0; i < 28; ++i){
                            double v = blobs[0][0]->data_at(d,0,j,i);
                            v = v*150;
                            img.setPixel(i,j,qRgb(v,v,v));
                        }
                    }
                    img.save(QString("pippo_%1.png").arg(d));
                }
            }
            */
            if((n%100) == 0){//activations
                auto blobs = net->learnable_params();
                //hdi::utils::secureLogValue(&logger,"size",blobs.size());
                //hdi::utils::secureLogVector(&logger,"size",blobs[0]->shape());
                //hdi::utils::secureLogVector(&logger,"size",blobs[1]->shape());
                hdi::utils::secureLogValue(&logger,"v",blobs[0]->data_at(15,0,2,2));

                QImage img(5,5,QImage::Format_ARGB32);
                for(int j = 0; j < 5; ++j){
                    for(int i = 0; i < 5; ++i){
                        double v = blobs[0]->data_at(15,0,j,i);
                        v = 150 + v*150;
                        img.setPixel(i,j,qRgb(v,v,v));
                    }
                }
                img.scaledToWidth(200).save(QString("pluto_%1.png").arg(n));
            }
        }

        hdi::dl::Classifier classifier;
        classifier.setLogger(&logger);
        classifier.initialize(net);
/*
        cv::Mat img = cv::imread(file, -1);
        std::vector<hdi::dl::Classifier::prediction_type> predictions = classifier.classify(img,5);
        for (size_t i = 0; i < predictions.size(); ++i) {
            hdi::dl::Classifier::prediction_type p = predictions[i];
            std::cout << std::fixed << std::setprecision(4) << p.second << " - \"" << p.first << "\"" << std::endl;
        }
*/
    }
    catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
    catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
    catch(...){ std::cout << "An unknown error occurred";}
}
