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

#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include <iostream>
#include <fstream>
#include "hdi/data/panel_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/data/image_data.h"
#include "hdi/data/pixel_data.h"
#include "hdi/data/text_data.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/analytics/multiscale_embedder_single_view_qobj.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/utils/math_utils.h"

class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
        std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
        embedder->addView(image_view);
        image_view->_text_data_as_os_path = true;
        image_view->show();

        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addUserDefinedDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setAlpha(0.5);
        drawer->setPointSize(10);
    }
    virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());

        embedder->addAreaOfInfluenceDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), influence, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->updateLimitsFromData();
        drawer->setPointSize(25);
        drawer->setAlpha(0.8);
    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());
        embedder->addSelectionDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), selection, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setLimits(0,0.1);
        drawer->setPointSize(25);
        drawer->setAlpha(0.8);
    }

    virtual void updateSelection(embedder_type* embedder, scalar_type* selection){
    }

    virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){}
};

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 4){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();

        {
            const int num_pics(std::atoi(argv[3]));
            const int num_dimensions(784);

            std::ifstream file_data(argv[1], std::ios::in|std::ios::binary);
            std::ifstream file_labels(argv[2], std::ios::in|std::ios::binary);
            if (!file_labels.is_open()){
                throw std::runtime_error("label file cannot be found");
            }
            if (!file_data.is_open()){
                throw std::runtime_error("data file cannot be found");
            }
            {//removing headers
                int32_t appo;
                file_labels.read((char*)&appo,4);
                file_labels.read((char*)&appo,4);
                file_data.read((char*)&appo,4);
                file_data.read((char*)&appo,4);
                file_data.read((char*)&appo,4);
                file_data.read((char*)&appo,4);
            }

            {//initializing panel data
                for(int j = 0; j < 28; ++j){
                    for(int i = 0; i < 28; ++i){
                        panel_data.addDimension(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(j,i,28,28)));
                    }
                }
                panel_data.initialize();
            }


            std::vector<QImage> images;
            std::vector<std::vector<scalar_type> > input_data;
            std::vector<unsigned char> labels;

            {//reading data
                images.reserve(num_pics);
                input_data.reserve(num_pics);
                labels.reserve(num_pics);

                for(int i = 0; i < num_pics; ++i){
                    unsigned char label;
                    file_labels.read((char*)&label,1);
                    labels.push_back(label);

                    //still some pics to read for this digit
                    input_data.push_back(std::vector<scalar_type>(num_dimensions));
                    images.push_back(QImage(28,28,QImage::Format::Format_ARGB32));
                    const int idx = int(input_data.size()-1);
                    for(int i = 0; i < num_dimensions; ++i){
                        unsigned char pixel;
                        file_data.read((char*)&pixel,1);
                        const scalar_type intensity(255.f - pixel);
                        input_data[idx][i] = intensity;
                        images[idx].setPixel(i%28,i/28,qRgb(intensity,intensity,intensity));
                    }
                }

                {
                    //moving a digit at the beginning digits of the vectors
                    const int digit_to_be_moved = 1;
                    int idx_to_be_swapped = 0;
                    for(int i = 0; i < images.size(); ++i){
                        if(labels[i] == digit_to_be_moved){
                            std::swap(images[i],		images[idx_to_be_swapped]);
                            std::swap(input_data[i],	input_data[idx_to_be_swapped]);
                            std::swap(labels[i],		labels[idx_to_be_swapped]);
                            ++idx_to_be_swapped;
                        }
                    }
                }
                const int digit_to_be_selected = 4;
                for(int i = 0; i < images.size(); ++i){
                    panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(images[i])), input_data[i]);
                    if(labels[i] == digit_to_be_selected){
                        panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
                    }
                }
            }
            hdi::utils::secureLog(&log,"done!");
        }

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
        params._seed = 2;
        params._num_walks_per_landmark = 100;
        params._rs_reduction_factor_per_layer = 0.1;
        params._rs_outliers_removal_jumps = 500;

        MyInterfaceInitializer interface_initializer;
        multiscale_embedder.setInterfaceInitializer(&interface_initializer);
        //multiscale_embedder.initialize(4,params);
        multiscale_embedder.initialize(3,params);
        multiscale_embedder.createTopLevelEmbedder();

        hdi::utils::secureLog(&log,"Computing connected components\n");
        for(int s = 0; s < multiscale_embedder.hSNE().hierarchy().size(); ++s){
            std::vector<unsigned int> vertex_to_cluster,cluster_to_vertex,cluster_size;
            hdi::utils::computeConnectedComponents(multiscale_embedder.hSNE().hierarchy()[s]._transition_matrix,vertex_to_cluster,cluster_to_vertex,cluster_size);
            {
                std::vector<double> perplexity(multiscale_embedder.hSNE().hierarchy()[s]._transition_matrix.size());
                for(int i = 0; i < perplexity.size(); ++i){
                    perplexity[i] = hdi::utils::computePerplexity(multiscale_embedder.hSNE().hierarchy()[s]._transition_matrix[i]);
                }
                std::stringstream ss;
                ss << "PRPLXT" << s;
                hdi::utils::secureLogVectorStats(&log,ss.str(),perplexity);
            }
            {
                std::stringstream ss;
                ss << "NEIGH_ON_MANIFOLD" << s;
                hdi::utils::secureLogSparseMatrixStats(&log,ss.str(),multiscale_embedder.hSNE().hierarchy()[s]._transition_matrix);
            }
            {
                std::stringstream ss;
                ss << "CC" << s;
                hdi::utils::secureLogVectorWithStats(&log,ss.str(),cluster_size);
            }
            {
                std::stringstream ss;
                ss << "INFL" << s;
                hdi::utils::secureLogVectorStats(&log,ss.str(),multiscale_embedder.hSNE().hierarchy()[s]._landmark_weight);
            }
            hdi::utils::secureLog(&log,"--------------\n");
        }

        while(true){
            multiscale_embedder.doAnIterateOnAllEmbedder();
            QApplication::processEvents();
        }
		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
