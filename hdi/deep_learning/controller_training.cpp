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

#include "hdi/deep_learning/controller_training.h"
#include "hdi/deep_learning/view_layer_detailed_qobj.h"
#include "hdi/utils/log_helper_functions.h"
#include <chrono>
#include <thread>

#include "hdi/utils/visual_utils.h"
#include <QCoreApplication>
#include "hdi/utils/vector_utils.h"
#include "hdi/utils/math_utils.h"

#include "hdi/data/panel_data.h"
#include "hdi/data/image_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/data/embedding.h"

#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"

#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"

#include "caffe/layers/base_conv_layer.hpp"
#include "caffe/layers/conv_layer.hpp"
namespace hdi{
    namespace dl{

    void computeWeightsMeanAndVariance(const caffe::Blob<float>& weights, int f, double& mean, double& variance){
        mean = 0;
        variance = 0;
        double num_elem = 0;
        auto shape = weights.shape();
        assert(shape.size()==2 || shape.size()==4);
        if(shape.size() == 4){
            num_elem = shape[1]*shape[2]*shape[3];
            for(int d = 0; d < shape[1]; ++d){
                for(int j = 0; j < shape[2]; ++j){
                    for(int i = 0; i < shape[3]; ++i){
                        auto v =weights.data_at(f,d,j,i);
                        mean += v;
                        variance += v*v;
                    }
                }
            }
        }
        if(shape.size() == 2){
            num_elem = shape[1];
            for(int d = 0; d < shape[1]; ++d){
                auto v =weights.data_at(f,d,0,0);
                mean += v;
                variance += v*v;
            }
        }

        mean /= num_elem;
        variance /= num_elem;
        variance = variance - mean*mean;
    }


        void ControllerTraining::initialize(std::shared_ptr<ModelCaffeSolver> solver, std::shared_ptr<hdi::dl::ModelTraining> model_training, std::shared_ptr<hdi::dl::ViewTraining> view_training){
            _solver = solver;
            _model_training = model_training;
            _view_training = view_training;
            _view_training->_model = _model_training.get();

            _layers.clear();
            _layer_views.clear();
            _iter = 0;
            hdi::utils::secureLog(_log,"Initializing training controller...");

            //GLOBAL SETTINGS
            QVBoxLayout *layout = new QVBoxLayout();
            _settings_train = new QCheckBox("Train network",_settings_wdg);
            layout->addWidget(_settings_train);
            _settings_train->setChecked(true);
            _settings_wdg->setLayout(layout);


            //_solver->_solver->net_real_ptr() = boost::shared_ptr<caffe::Net<float> >(new caffe::Net<float>(_solver->_solver->param().net_param()));
            //_solver->_solver = std::shared_ptr<caffe::Solver<float> >(caffe::SolverRegistry<float>::CreateSolver(_solver->_solver_param));
            auto net = _solver->_solver->net();

            //net->Init(net->param_copy_); //TEST
            auto& net_layers = net->mutable_layers();
            //auto net_layers = net->layers();
            auto top_vecs = net->top_vecs();
            auto bottom_vecs = net->bottom_vecs();

            ////// mhhhh
            int total_stride = 1;
            int input_size = net->top_vecs()[0][0]->shape()[3]; //data shape
            int isz = input_size;
            int output_size = 0;
            int rf_size = 0;
            int group_param = 1;

            int my_offset = 0;
            int my_kernel = 0;
            int my_stride = 0;

            //not sure...
            auto shape_last_layer = top_vecs[top_vecs.size()-2][0]->shape();
            int num_labels = shape_last_layer[1];
            _palette_labels = std::shared_ptr<std::map<scalar_type,QColor>>(new std::map<scalar_type,QColor>());

            for(int i = 0; i < net_layers.size(); ++i){

                {
                    int kernel_size = 0;
                    int stride = 1;
                    int pad = 0;
                    if(net_layers[i]->layer_param().has_convolution_param()){
                        const caffe::ConvolutionParameter& conv_param = net_layers[i]->layer_param().convolution_param();
                        kernel_size = conv_param.kernel_size(0);
                        if(conv_param.stride_size()){
                            stride = conv_param.stride(0);
                        }
                        pad = conv_param.pad_size()==0?0:conv_param.pad().Get(0);
                        if(my_kernel == 0){
                            my_kernel = kernel_size;
                            my_stride = stride;
                            my_offset = pad;
                        }else{
                            my_stride *= stride;
                            my_offset += pad*my_stride;
                            my_kernel = kernel_size*my_stride+my_stride*2;
                        }
                        if(conv_param.has_group()){
                            group_param = conv_param.group();
                        }
                    }
                    if(net_layers[i]->layer_param().has_pooling_param()){
                        const caffe::PoolingParameter& pooling_param = net_layers[i]->layer_param().pooling_param();
                        kernel_size = pooling_param.kernel_size();
                        stride = pooling_param.has_stride()?pooling_param.stride():1;
                        pad = pooling_param.has_pad()?pooling_param.pad():0;

                        my_stride *= stride;
                        my_kernel += (kernel_size-1)*my_stride;
                        my_offset += pad*my_stride;
                    }
                    if(kernel_size != 0){
                        output_size = (input_size - kernel_size + 2*pad) / stride + 1;
                        input_size = output_size;
                        total_stride = total_stride*stride;
                        rf_size = isz - (output_size - 1) * total_stride;
//assert(output_size == top_vecs[i][0]->shape()[3]);
                        hdi::utils::secureLog(_log,QString("%1 - output: %2 - stride: %3 - RF: %4 ").arg(net->layer_names()[i].c_str()).arg(output_size).arg(total_stride).arg(rf_size).toStdString());
                        hdi::utils::secureLog(_log,QString("%1 - my_kernel: %2 - my_stride: %3 - my_offset: %4 ").arg(net->layer_names()[i].c_str()).arg(my_kernel).arg(my_stride).arg(my_offset).toStdString());
                    }
                }

                // if I don't have learnable parameters I'm not really interested in this layer
                if(net_layers[i]->blobs().size() == 0){
                    continue;
                }



                if(i == 1 && false){
//int new_size = 20;
int new_size = 4;

{
                    auto net = _solver->_solver->net();
                    auto& net_layers = net->mutable_layers();
                    caffe::LayerParameter layer_param = net_layers[i]->layer_param();
                    int pippo = layer_param.convolution_param().num_output();
                    float pluto = layer_param.param(0).lr_mult();
                    layer_param.mutable_convolution_param()->set_num_output(new_size);

                    {
                        auto pre_learning = net_layers[i]->blobs()[0];
                        auto shape = net_layers[i]->blobs()[0]->shape();
/*
                        for(int a = 0; a < shape[0]; ++a){
                            for(int b = 0; b < shape[1]; ++b){
                                for(int c = 0; c < shape[2]; ++c){
                                    for(int d = 0; d < shape[3]; ++d){
                                        auto offset = pre_learning->offset(a,b,c,d);
                                        pre_learning->mutable_cpu_data()[offset] = 0;
                                    }
                                }
                            }
                        }
*/
                        std::cout << "---------------" << pluto << std::endl;
                        std::cout << pre_learning->data_at(0,0,0,0) << std::endl;
                        std::cout << pre_learning->data_at(1,0,1,1) << std::endl;
                        std::cout << pre_learning->data_at(2,0,2,2) << std::endl;
                        std::cout << pre_learning->data_at(3,0,3,3) << std::endl;
                        std::cout << pre_learning->data_at(4,0,4,4) << std::endl;
                    }

                    //net_layers[i] = boost::shared_ptr<caffe::ConvolutionLayer<float>>(new caffe::ConvolutionLayer<float>(layer_param));
                    {
                        net_layers[i]->layer_param().mutable_convolution_param()->set_num_output(new_size);


                        auto uuu = net_layers[i]->blobs()[0]->shape();
                        uuu[0] = new_size;
                        net_layers[i]->blobs()[0]->Reshape(uuu);
                        net_layers[i]->blobs()[1]->Reshape(std::vector<int>(1,new_size));

                        net_layers[i]->LayerSetUp(bottom_vecs[i],top_vecs[i]);
                        net_layers[i]->Reshape(bottom_vecs[i],top_vecs[i]);

                    }
                    auto zzz = net_layers[i+2]->blobs()[0]->shape();
                    zzz[1] = new_size;
                    net_layers[i+2]->blobs()[0]->Reshape(zzz);
                    pluto = layer_param.param(0).lr_mult();

                    {
                        auto post_learning = net_layers[i]->blobs()[0];
                        auto shape = net_layers[i]->blobs()[0]->shape();
/*
                        for(int a = 0; a < shape[0]; ++a){
                            for(int b = 0; b < shape[1]; ++b){
                                for(int c = 0; c < shape[2]; ++c){
                                    for(int d = 0; d < shape[3]; ++d){
                                        auto offset = post_learning->offset(a,b,c,d);
                                        post_learning->mutable_cpu_data()[offset] = 0;
                                    }
                                }
                            }
                        }
*/
                        std::cout << "---------------" << pluto << std::endl;
                        std::cout << post_learning->data_at(0,0,0,0) << std::endl;
                        std::cout << post_learning->data_at(1,0,1,1) << std::endl;
                        std::cout << post_learning->data_at(2,0,2,2) << std::endl;
                        std::cout << post_learning->data_at(3,0,3,3) << std::endl;
                        std::cout << post_learning->data_at(4,0,4,4) << std::endl;
                    }

                    net_layers[i+1]->Reshape(bottom_vecs[i+1],top_vecs[i+1]);
                    net_layers[i+2]->LayerSetUp(bottom_vecs[i+2],top_vecs[i+2]);

                    //auto top_vecs = net->top_vecs();
                    //auto bottom_vecs = net->bottom_vecs();
                    net->Reshape();
                    //net = _solver->_solver->net();
                    //net_layers = net->layers();
                    //top_vecs = net->top_vecs();
                    //bottom_vecs = net->bottom_vecs();
}


{
                    auto test_nets = _solver->_solver->test_nets();
                    auto net = test_nets[0];
                    auto& net_layers = net->mutable_layers();
                    auto top_vecs = net->top_vecs();
                    auto bottom_vecs = net->bottom_vecs();

                    int layer_id = i+1;

                    caffe::LayerParameter layer_param = net_layers[layer_id]->layer_param();
                    int pippo = layer_param.convolution_param().num_output();
                    layer_param.mutable_convolution_param()->set_num_output(new_size);
                    net_layers[layer_id] = boost::shared_ptr<caffe::ConvolutionLayer<float>>(new caffe::ConvolutionLayer<float>(layer_param));
                    net_layers[layer_id]->LayerSetUp(bottom_vecs[layer_id],top_vecs[layer_id]);
                    net_layers[layer_id]->Reshape(bottom_vecs[layer_id],top_vecs[layer_id]);
                    auto zzz = net_layers[layer_id+2]->blobs()[0]->shape();
                    zzz[1] = new_size;
                    net_layers[layer_id+2]->blobs()[0]->Reshape(zzz);

                    net_layers[layer_id+1]->Reshape(bottom_vecs[layer_id+1],top_vecs[layer_id+1]);
                    net_layers[layer_id+2]->LayerSetUp(bottom_vecs[layer_id+2],top_vecs[layer_id+2]);

                    //auto top_vecs = net->top_vecs();
                    //auto bottom_vecs = net->bottom_vecs();
                    net->Reshape();
                    //net = _solver->_solver->net();
                    //net_layers = net->layers();
                    //top_vecs = net->top_vecs();
                    //bottom_vecs = net->bottom_vecs();
}

                }
                auto& learn_params = net_layers[i]->blobs();

                _solver->_solver->net()->Forward();
                auto shape0 = learn_params[0]->shape();
                auto shape1 = bottom_vecs[i][0]->shape();
                auto shape2 = top_vecs[i][0]->shape();
                auto shape3 = bottom_vecs[i+1][0]->shape();
                auto shape4 = top_vecs[i+1][0]->shape();
                //TEST

                /*
                caffe::LayerParameter& layer_param = net_layers[i]->layer_param();
                int pippo = layer_param.param_size();
                int lr_mult = layer_param.mutable_param(0)->lr_mult();
                layer_param.mutable_param(0)->set_lr_mult(2000);
                lr_mult = layer_param.mutable_param(0)->lr_mult();
                //layer_params.has_
                if(layer_param.has_convolution_param()){
                    const caffe::ConvolutionParameter& conv_param = layer_param.convolution_param();
                    //conv_param.has
                    //conv_param.has
                }
                */

                std::shared_ptr<ModelLayer> model_layer(new ModelLayer);
                model_layer->name() = net->layer_names()[i];
                model_layer->initialize(top_vecs[i][0]->shape()[1],num_labels);
                model_layer->id() = i;

                if(net_layers[i]->layer_param().has_convolution_param()){
                    model_layer->_type = ModelLayer::Convolutional;
                } else if(net_layers[i]->layer_param().has_inner_product_param()){
                    model_layer->_type = ModelLayer::InnerProduct;
                } else if(net_layers[i]->layer_param().has_softmax_param()){
                    model_layer->_type = ModelLayer::SoftMax;
                }

                //Receptive field
                model_layer->_rf_offset = my_offset;
                model_layer->_rf_size = my_kernel;
                model_layer->_rf_stride = my_stride;

                {
                    auto learn_shape = learn_params[0]->shape();
                    int size = 1;
                    for(int i = 1; i < learn_shape.size(); ++i){
                        size *= learn_shape[i];
                    }

                    size*=group_param;
                    model_layer->_input_data.initializeWithEmptyDimensions(size);
                }

                std::shared_ptr<ViewLayerOverview> view_layer_overview(new ViewLayerOverview);
                view_layer_overview->_model = model_layer;
                view_layer_overview->update();

                std::shared_ptr<ViewLayerDetailed> view_layer_detailed(new ViewLayerDetailed);
                view_layer_detailed->_model = model_layer;
                view_layer_detailed->update();
                _views_layer_detailed.push_back(view_layer_detailed);

                _overview_vlayout->addWidget(view_layer_overview.get());

                QWidget* layer_view_wdg = new QWidget();
                _layer_wdgs.push_back(layer_view_wdg);
                _layer_tab_wdg->addTab(layer_view_wdg,model_layer->name().c_str());

                _layers.push_back(model_layer);
                _layer_views.push_back(view_layer_overview);


                // Controller Emebdding
                auto view_embedding  = std::shared_ptr<ViewEmbedding>(new ViewEmbedding());
                _views_embedding.push_back(view_embedding);


                auto controller_embedding  = std::shared_ptr<ControllerLayer>(new ControllerLayer());
                controller_embedding->_palette_labels = _palette_labels;
                controller_embedding->initialize(solver, model_layer, view_embedding, view_layer_detailed, view_layer_overview, layer_view_wdg);
                controller_embedding->_log = _log;
                _controllers_embedding.push_back(controller_embedding);
            }

            if(num_labels == 2){
                (*_palette_labels)[0] = qRgb(55,126,184);
                (*_palette_labels)[1] = qRgb(228,26,28);
            }else{
                (*_palette_labels)[0] = qRgb(166,206,227);
                (*_palette_labels)[1] = qRgb(31,120,180);
                (*_palette_labels)[2] = qRgb(178,223,138);
                (*_palette_labels)[3] = qRgb(51,160,44);
                (*_palette_labels)[4] = qRgb(251,154,153);
                (*_palette_labels)[5] = qRgb(227,26,28);
                (*_palette_labels)[6] = qRgb(253,191,111);
                (*_palette_labels)[7] = qRgb(202,178,214);
                (*_palette_labels)[8] = qRgb(106,61,154);
                (*_palette_labels)[9] = qRgb(255,255,153);
                (*_palette_labels)[10] = qRgb(255,127,0);
            }
            {
                for(int i = 0; i < 1000; ++i){
                    (*_palette_labels)[11+i] = qRgb(rand()%226+30,rand()%226+30,rand()%226+30);
                }
            }



            _view_training->update();

        }

        void ControllerTraining::iterate(){
            if(_settings_train->isChecked()){
                auto net = _solver->_solver->net();
                auto top_vecs = net->top_vecs();
                _solver->iterate(1);
                top_vecs = net->top_vecs();
            }else{
                _solver->_solver->net()->Forward();
            }
            /*
            {
                auto net = _solver->_solver->net();
                auto& net_layers = net->mutable_layers();
                auto pre_learning = net_layers[1]->blobs()[0];
                std::cout << "---------------" << std::endl;
                std::cout << pre_learning->data_at(0,0,0,0) << std::endl;
                std::cout << pre_learning->data_at(1,0,1,1) << std::endl;
                std::cout << pre_learning->data_at(2,0,2,2) << std::endl;
                std::cout << pre_learning->data_at(3,0,3,3) << std::endl;
                std::cout << pre_learning->data_at(4,0,4,4) << std::endl;
            }
            */

            if((_iter%30) == 0){
              //  checkFilters(*_solver);
            }

            for(int i = 0; i < _layers.size(); ++i){
                /*
                if((_iter%50) == 0){
                  checkBotVecs(*_solver, *_layers[i]);
                }
                */

                _layer_views[i]->update();
                _views_layer_detailed[i]->update();

                _controllers_embedding[i]->onNewData();

                QCoreApplication::processEvents();
            }
            if(_settings_train->isChecked()){
                _model_training->_loss.push_back(std::pair<scalar_type,scalar_type>(_model_training->_iter,_solver->_solver->smoothed_loss()));
                _model_training->_max_loss = std::max(_model_training->_max_loss,_solver->_solver->smoothed_loss());
                //std::cout << _solver->_solver->accuracy().first << " - " << _solver->_solver->accuracy().second;
                auto accuracy = _solver->_solver->accuracy();
                if(accuracy.first == _model_training->_iter){
                    accuracy.second *= 100;
                    _model_training->_accuracy.push_back(accuracy);

                    utils::secureLogValue(_log,"Accuracy",QString("%1% (iter: %2)").arg(accuracy.second).arg(accuracy.first).toStdString());
                }

                std::cout << "************ "  <<  _iter << " ************" << std::endl;
                ++_model_training->_iter;
                _view_training->update();
            }
            ++_iter;
        }

        void ControllerTraining::checkData(const ModelCaffeSolver& solver){
            auto net = _solver->_solver->net();
            auto top_vecs = net->top_vecs();
            auto data_size = top_vecs[0][0]->shape();

            if(data_size[1] == 3){
                QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                for(int j = 0; j < data_size[2]; ++j){
                    for(int i = 0; i < data_size[3]; ++i){
                        auto r = 150. + top_vecs[0][0]->data_at(0,0,j,i)*255;
                        auto g = 150. + top_vecs[0][0]->data_at(0,1,j,i)*255;
                        auto b = 150. + top_vecs[0][0]->data_at(0,2,j,i)*255;
                        if(r < 0) r = 0;
                        if(r > 255) r = 255;
                        if(g < 0) g = 0;
                        if(g > 255) g = 255;
                        if(b < 0) b = 0;
                        if(b > 255) b = 255;
                        img.setPixel(i,j,qRgb(r,g,b));
                    }
                }
                img.save("data.png");
            }
            if(data_size[1] == 1){
                QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                for(int j = 0; j < data_size[2]; ++j){
                    for(int i = 0; i < data_size[3]; ++i){
                        auto v = 150. + top_vecs[0][0]->data_at(0,0,j,i)*255;
                        if(v < 0) v = 0;
                        if(v > 255) v = 255;
                        img.setPixel(i,j,qRgb(v,v,v));
                    }
                }
                img.save("data.png");
            }
        }
        void ControllerTraining::checkFilters(const ModelCaffeSolver& solver){
            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto learn_params = net_layers[1]->blobs();
            auto data_size = learn_params[0]->shape();

            double std_dev = std::sqrt(2./(data_size[1]*data_size[2]*data_size[3]));
            double scale = 50/std_dev;
            if(data_size[1] == 3){
                for(int f = 0; f < data_size[0]; ++f){
                    double mean,variance;
                    computeWeightsMeanAndVariance(*(learn_params[0]), f, mean, variance);
                    QImage img(data_size[3]+1,data_size[2],QImage::Format_ARGB32);
                    for(int j = 0; j < data_size[2]; ++j){
                        for(int i = 0; i < data_size[3]; ++i){
                            auto kkkk = learn_params[0]->data_at(f,0,j,i);
                            auto r = 125. + learn_params[0]->data_at(f,0,j,i)*scale;
                            auto g = 125. + learn_params[0]->data_at(f,1,j,i)*scale;
                            auto b = 125. + learn_params[0]->data_at(f,2,j,i)*scale;
                            if(r < 0) r = 0;
                            if(r > 255) r = 255;
                            if(g < 0) g = 0;
                            if(g > 255) g = 255;
                            if(b < 0) b = 0;
                            if(b > 255) b = 255;
                            img.setPixel(i,j,qRgb(r,g,b));
                        }
                    }
                    for(int j = 0; j < data_size[2]; ++j){
                        auto v = learn_params[1]->data_at(f,0,0,0);
                        v*=scale*10;
                        if(v > 0){
                            img.setPixel(data_size[3],j,qRgb(v,0,0));
                        }else{
                            v *= -1;
                            img.setPixel(data_size[3],j,qRgb(0,0,v));
                        }
                    }
                    img.scaledToWidth(100).save(QString("filter%1.png").arg(f));
                }
            }
            if(data_size[1] == 1){
                for(int f = 0; f < data_size[0]; ++f){
                    QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                    for(int j = 0; j < data_size[2]; ++j){
                        for(int i = 0; i < data_size[3]; ++i){
                            auto v = 150. + learn_params[0]->data_at(f,0,j,i)*255;
                            if(v < 0) v = 0;
                            if(v > 255) v = 255;
                            img.setPixel(i,j,qRgb(v,v,v));
                        }
                    }
                    img.scaledToWidth(100).save(QString("filter%1.png").arg(f));
                }
            }
        }

        void ControllerTraining::checkBotVecs(const ModelCaffeSolver& solver, ModelLayer& layer){

            auto net = _solver->_solver->net();
            auto bot_vecs = net->bottom_vecs()[layer.id()];
            auto data_size = bot_vecs[0]->shape();
            if(data_size.size()==2)
                return;
            /*
            if(data_size[1] == 3){
                for(int f = 0; f < data_size[0]; ++f){
                    QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                    for(int j = 0; j < data_size[2]; ++j){
                        for(int i = 0; i < data_size[3]; ++i){
                            auto r = 150. + learn_params[0]->data_at(f,0,j,i)*255;
                            auto g = 150. + learn_params[0]->data_at(f,1,j,i)*255;
                            auto b = 150. + learn_params[0]->data_at(f,2,j,i)*255;
                            if(r < 0) r = 0;
                            if(r > 255) r = 255;
                            if(g < 0) g = 0;
                            if(g > 255) g = 255;
                            if(b < 0) b = 0;
                            if(b > 255) b = 255;
                            img.setPixel(i,j,qRgb(r,g,b));
                        }
                    }
                    img.scaledToWidth(100).save(QString("filter%1.png").arg(f));
                }
                return;
            }
            */
            /*
            if(data_size[1] == 1){
                for(int f = 0; f < data_size[0]; ++f){
                    QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                    for(int j = 0; j < data_size[2]; ++j){
                        for(int i = 0; i < data_size[3]; ++i){
                            auto v = 150. + bot_vecs[0]->data_at(0,0,j,i)*255;
                            if(v < 0) v = 0;
                            if(v > 255) v = 255;
                            img.setPixel(i,j,qRgb(v,v,v));
                        }
                    }
                    img.scaledToWidth(100).save(QString("layer_%1_%2.png").arg(layer.name().c_str()).arg(f));
                }
                return;
            }
            */

            for(int f = 0; f < data_size[1]; ++f){
                QImage img(data_size[3],data_size[2],QImage::Format_ARGB32);
                for(int j = 0; j < data_size[2]; ++j){
                    for(int i = 0; i < data_size[3]; ++i){
                        auto v = 150. + bot_vecs[0]->data_at(0,f,j,i)*70;
                        if(v < 0) v = 0;
                        if(v > 255) v = 255;
                        img.setPixel(i,j,qRgb(v,v,v));
                    }
                }
                img.scaledToWidth(100).save(QString("layer_%1_%2.png").arg(layer.name().c_str()).arg(f));
            }


        }

        void ControllerTraining::killLayer(const ModelCaffeSolver& solver, ModelLayer& layer){
            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto learn_params = net_layers[layer.id()]->blobs();
            /*
            {
                auto data_size = learn_params[0]->shape();
                if(data_size.size()!= 4)return;
                for(int k = 0; k < data_size[0]; ++k)
                    for(int l = 0; l < data_size[1]; ++l)
                        for(int j = 0; j < data_size[2]; ++j)
                            for(int i = 0; i < data_size[3]; ++i){
                                auto offset = learn_params[0]->offset(k,l,j,i);
                                auto v = learn_params[0]->cpu_data()[offset];
                                learn_params[0]->mutable_cpu_data()[offset] = 0;
                            }
                learn_params[0]->Update();
            }

            {
                auto data_size = learn_params[1]->shape();
                for(int k = 0; k < data_size[0]; ++k){
                    auto v = learn_params[1]->cpu_data()[k];
                    learn_params[1]->mutable_cpu_data()[k] = 0;
                }
                learn_params[1]->Update();
            }
            */

            {
                auto data_size = learn_params[0]->shape();
                if(data_size.size()!= 4)return;
                int k = 0;
                for(int l = 0; l < data_size[1]; ++l)
                        for(int j = 0; j < data_size[2]; ++j)
                            for(int i = 0; i < data_size[3]; ++i){
                                auto offset = learn_params[0]->offset(k,l,j,i);
                                auto v = learn_params[0]->cpu_data()[offset];
                                //learn_params[0]->mutable_cpu_data()[offset] = 0;
                                learn_params[0]->mutable_cpu_data()[offset] = 0.9;
                            }
                learn_params[0]->Update();
            }
            {
                auto data_size = learn_params[0]->shape();
                if(data_size.size()!= 4)return;
                int k = 1;
                for(int l = 0; l < data_size[1]; ++l)
                        for(int j = 0; j < data_size[2]; ++j)
                            for(int i = 0; i < data_size[3]; ++i){
                                auto offset = learn_params[0]->offset(k,l,j,i);
                                auto v = learn_params[0]->cpu_data()[offset];
                                //learn_params[0]->mutable_cpu_data()[offset] = 0;
                                learn_params[0]->mutable_cpu_data()[offset] = -0.9;
                            }
                learn_params[0]->Update();
            }
            {
                auto data_size = learn_params[0]->shape();
                if(data_size.size()!= 4)return;
                int k = 2;
                for(int l = 0; l < data_size[1]; ++l)
                        for(int j = 0; j < data_size[2]; ++j)
                            for(int i = 0; i < data_size[3]; ++i){
                                auto offset = learn_params[0]->offset(k,l,j,i);
                                auto v = learn_params[0]->cpu_data()[offset];
                                //learn_params[0]->mutable_cpu_data()[offset] = 0;
                                if(i > j){
                                    learn_params[0]->mutable_cpu_data()[offset] = -0.9;
                                }else{
                                    learn_params[0]->mutable_cpu_data()[offset] = -0.9;
                                    if(l==0)
                                        learn_params[0]->mutable_cpu_data()[offset] = 0.9;
                                }
                            }
                learn_params[0]->Update();
            }
        }


        void ControllerTraining::collectData(const ModelCaffeSolver& solver, ModelLayer& layer){

            auto net = _solver->_solver->net();
            auto bot_vec = net->bottom_vecs()[layer.id()];
            auto net_layers = net->layers();
            auto learn_params = net_layers[layer.id()]->blobs();

            assert(bot_vec[0]->shape()[1] == learn_params[0]->shape()[1]);
            int n_samples = 10;
            //every layer is optimized in a greedy way
            for(int s = 0; s < n_samples; ++s){
                int b = rand()%(bot_vec[0]->shape()[0]);
                int i = rand()%(bot_vec[0]->shape()[3]-learn_params[0]->shape()[3]);
                int j = rand()%(bot_vec[0]->shape()[2]-learn_params[0]->shape()[2]);

                std::vector<scalar_type> v;
                v.reserve(layer._input_data.numDimensions());
                for(int m = 0; m < bot_vec[0]->shape()[1]; ++m){
                    for(int k = 0; k < learn_params[0]->shape()[2]; ++k){
                        for(int l = 0; l < learn_params[0]->shape()[3]; ++l){
                            v.push_back(bot_vec[0]->data_at(b,m,i+k,j+l));
                        }
                    }
                }
                layer._input_data.addDataPoint(std::shared_ptr<data::EmptyData>(new data::EmptyData()),v);
            }
        }

        void ControllerTraining::optimizeLayer(const ModelCaffeSolver& solver, ModelLayer& layer){

            auto net = _solver->_solver->net();
            auto bot_vec = net->bottom_vecs()[layer.id()];
            auto net_layers = net->layers();
            auto learn_params = net_layers[layer.id()]->blobs();

            assert(bot_vec[0]->shape()[1] == learn_params[0]->shape()[1]);
            //every layer is optimized in a greedy way
            for(int f = 0; f < layer.size(); ++f){
                int b = rand()%(bot_vec[0]->shape()[0]);
                int i = rand()%(bot_vec[0]->shape()[3]-learn_params[0]->shape()[3]);
                int j = rand()%(bot_vec[0]->shape()[2]-learn_params[0]->shape()[2]);

                double mean,variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, mean, variance);

                for(int m = 0; m < bot_vec[0]->shape()[1]; ++m){
                    for(int k = 0; k < learn_params[0]->shape()[2]; ++k){
                        for(int l = 0; l < learn_params[0]->shape()[3]; ++l){
                            auto offset = learn_params[0]->offset(f,m,k,l);
                            learn_params[0]->mutable_cpu_data()[offset] = bot_vec[0]->data_at(b,m,i+k,j+l);
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                if(new_variance == 0){
                    --f;
                    continue;
                }

                for(int m = 0; m < bot_vec[0]->shape()[1]; ++m){
                    for(int k = 0; k < learn_params[0]->shape()[2]; ++k){
                        for(int l = 0; l < learn_params[0]->shape()[3]; ++l){
                            auto offset = learn_params[0]->offset(f,m,k,l);
                            learn_params[0]->mutable_cpu_data()[offset] -= new_mean;
                        }
                    }
                }

                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);

                //double target_variance = 2./(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                double target_variance = 0.5/(learn_params[0]->shape()[1]*learn_params[0]->shape()[2]*learn_params[0]->shape()[3]);
                //double target_variance = variance;
                double target_std_dev = std::sqrt(target_variance);
                double p = new_variance / target_variance;
                double sqrt_p = std::sqrt(p);

                for(int m = 0; m < bot_vec[0]->shape()[1]; ++m){
                    for(int k = 0; k < learn_params[0]->shape()[2]; ++k){
                        for(int l = 0; l < learn_params[0]->shape()[3]; ++l){
                            auto offset = learn_params[0]->offset(f,m,k,l);
                            learn_params[0]->mutable_cpu_data()[offset] /= sqrt_p;
                            //assert(!isnan(learn_params[0]->mutable_cpu_data()[offset]));
                        }
                    }
                }


                computeWeightsMeanAndVariance(*(learn_params[0]), f, new_mean, new_variance);
                std::cout << f << ": mean (" << mean << ") new_mean(" << new_mean << ") \n";
                std::cout << f << ": var (" << variance << ") new_var(" << new_variance << ") \n";

            }
            {
                auto data_size = learn_params[1]->shape();
                for(int k = 0; k < data_size[0]; ++k){
                    auto v = learn_params[1]->cpu_data()[k];
                    learn_params[1]->mutable_cpu_data()[k] = 0;
                }
            }
            learn_params[0]->Update();
            learn_params[1]->Update();

        }
        /*
            auto net = _solver->_solver->net();
            auto bot_vec = net->bottom_vecs()[layer.id()];
            auto net_layers = net->layers();
            auto learn_params = net_layers[layer.id()]->blobs();

            assert(bot_vec[0]->shape()[1] == learn_params[0]->shape()[1]);

            int f = 0;
            //every layer is optimized in a greedy way
                if(layer._avg_activations[f] != 0){
                    //continue;
                }
                int b = rand()%(bot_vec[0]->shape()[0]);
                int i = rand()%(bot_vec[0]->shape()[3]-learn_params[0]->shape()[3]);
                int j = rand()%(bot_vec[0]->shape()[2]-learn_params[0]->shape()[2]);

                double mean,variance;
                double new_mean,new_variance;
                computeWeightsMeanAndVariance(*(learn_params[0]), f, mean, variance);

                for(int m = 0; m < bot_vec[0]->shape()[1]; ++m){
                    for(int k = 0; k < learn_params[0]->shape()[2]; ++k){
                        for(int l = 0; l < learn_params[0]->shape()[3]; ++l){
                            auto offset = learn_params[0]->offset(f,m,k,l);
                            learn_params[0]->mutable_cpu_data()[offset] = -1;
                        }
                    }
                }

            {
                auto data_size = learn_params[1]->shape();
                for(int k = 0; k < data_size[0]; ++k){
                    auto v = learn_params[1]->cpu_data()[k];
                    learn_params[1]->mutable_cpu_data()[k] = 1;
                }
            }
            learn_params[0]->Update();
            learn_params[1]->Update();
        }
            */


        void ControllerTraining::testEmbeddingGeneration(const ModelCaffeSolver& solver, ModelLayer& layer){
            /*
            auto size = layer.size();

            auto net = _solver->_solver->net();
            auto net_layers = net->layers();
            auto top_vecs = net->top_vecs()[layer.id()];
            auto bot_vecs = net->bottom_vecs()[layer.id()];
            auto top_vecs_shape = top_vecs[0]->shape();
            auto bot_vecs_shape = bot_vecs[0]->shape();

            auto batch_size = top_vecs_shape[0];
            auto height     = top_vecs_shape.size()==4?top_vecs_shape[2]:0;
            auto width      = top_vecs_shape.size()==4?top_vecs_shape[3]:0;
            int n_samples = 15;

            auto learn_params = net_layers[1]->blobs();
            auto learn_size = learn_params[0]->shape();
            int num_dim = learn_size[1]*learn_size[2]*learn_size[3];

            if(height == 0){
                return;
            }
            data::PanelData<scalar_type> data;
            data.initializeWithEmptyDimensions(num_dim);

            std::vector<scalar_type> perplexities;
            std::vector<std::vector<scalar_type>> activations_dp(size,std::vector<scalar_type>(batch_size*n_samples));


            int tot = 0;
            for(int b = 0; b < batch_size; ++b){
                for(int s = 0; s < n_samples; ++s){
                    int i = rand()%width;
                    int j = rand()%height;
                    std::vector<scalar_type> activations(size);
                    for(int l = 0; l < size; ++l){
                        activations[l] = top_vecs[0]->data_at(b,l,j,i);
                    }
                    utils::softMax(activations);
                    scalar_type perplexity = utils::computePerplexity(activations.begin(),activations.end());
                    perplexities.push_back((size - perplexity)/size);
                    for(int l = 0; l < size; ++l){
                        activations_dp[l][tot] = activations[l];
                    }


                    QImage image(learn_size[3],learn_size[2],QImage::Format_ARGB32);
                    std::vector<scalar_type> dimensions;
                    for(int lj = 0; lj < learn_size[2]; ++lj){
                        for(int li = 0; li < learn_size[3]; ++li){
                            int bi = i+li;
                            int bj = j+lj;

                            for(int d = 0; d < learn_size[1]; ++d){
                               scalar_type v = bot_vecs[0]->data_at(b,d,bj,bi);
                               dimensions.push_back(v);
                            }
                            QColor color;
                            if(learn_size[1] != 3){
                                scalar_type v = bot_vecs[0]->data_at(b,0,bj,bi);
                                v = 150 + v*40;
                                color = qRgb(v,v,v);
                            }else{
                                //double scale = 0.5;
                                double scale = 100;
                                scalar_type R = bot_vecs[0]->data_at(b,0,bj,bi);
                                scalar_type G = bot_vecs[0]->data_at(b,1,bj,bi);
                                scalar_type B = bot_vecs[0]->data_at(b,2,bj,bi);
                                R = 150 + R*scale;
                                G = 150 + G*scale;
                                B = 150 + B*scale;
                                if(R < 0) R = 0;
                                if(R > 255) R = 255;
                                if(G < 0) G = 0;
                                if(G > 255) G = 255;
                                if(B < 0) B = 0;
                                if(B > 255) B = 255;
                                color = qRgb(R,G,B);
                            }

                            image.setPixel(li,lj,color.rgb());
                        }
                    }


                    data.addDataPoint(std::shared_ptr<data::ImageData>(new data::ImageData(image)),dimensions);
                    ++tot;
                }
            }

            data::Embedding<scalar_type> embedding;
            std::vector<hdi::data::MapMemEff<unsigned int, float> > distribution;
            dr::HDJointProbabilityGenerator<> prob_gen;
            prob_gen.computeProbabilityDistributions(data.getData().data(),data.numDimensions(),data.numDataPoints(),distribution);

            dr::SparseTSNEUserDefProbabilities<> tsne;
            tsne.initialize(distribution, &embedding);
            tsne.setTheta(0.5);

            viz::MultipleImageView view;
            view.setPanelData(&data);
            view.show();
            view.updateView();



            hdi::viz::ScatterplotCanvas viewer;
            viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer.setSelectionColor(qRgb(50,50,50));
            viewer.resize(600,600);
            viewer.show();
            viewer.setWindowTitle(layer.name().c_str());
            hdi::viz::ScatterplotDrawerScalarAttribute drawer;
            drawer.initialize(viewer.context());
            drawer.setData(embedding.getContainer().data(), perplexities.data(), data.getFlagsDataPoints().data(), data.numDataPoints());
            drawer.setAlpha(0.7);
            drawer.setPointSize(10);
            viewer.addDrawer(&drawer);
            drawer.setLimits(-0.05,1);

            viz::ControllerSelectionEmbedding controller_selection;
            controller_selection.setActors(&data,&embedding,&viewer);
            controller_selection.addView(&view);
            controller_selection.initialize();

            int iter = 0;
            int id_actv = 0;
            while(iter < 2000){
                if(((iter+1)%25) == 0){
                    //hdi::utils::secureLogValue(&log,"Iter",iter+1);
                    drawer.setData(embedding.getContainer().data(), activations_dp[id_actv].data(), data.getFlagsDataPoints().data(), data.numDataPoints());
                    ++id_actv;
                    id_actv = id_actv%activations_dp.size();
                }
                tsne.doAnIteration();

                {//limits
                    std::vector<scalar_type> limits;
                    embedding.computeEmbeddingBBox(limits,0.25);
                    auto tr = QVector2D(limits[1],limits[3]);
                    auto bl = QVector2D(limits[0],limits[2]);
                    viewer.setTopRightCoordinates(tr);
                    viewer.setBottomLeftCoordinates(bl);
                }

                viewer.updateGL();
                QApplication::processEvents();
                ++iter;
            }
            */

        }


    }
}
