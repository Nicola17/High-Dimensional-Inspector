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
        /*std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
        embedder->addView(image_view);
        image_view->_text_data_as_os_path = true;
        image_view->show();
*/
        std::shared_ptr<hdi::viz::MultipleHeatmapsView> heatmaps_view(new hdi::viz::MultipleHeatmapsView());
        embedder->addView(heatmaps_view);
        heatmaps_view->setAuxData(embedder->getPanelData().numDimensions(),embedder->getPanelData().getData().data());
        heatmaps_view->show();

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
        drawer->setPointSize(15);
        drawer->setAlpha(0.8);
    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){
        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addSelectionDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setPointSize(7.5);
        drawer->setColor(qRgb(255,0,150));
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
        if(argc != 3){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}

        std::ifstream file_pictures(argv[1], std::ios::in);
        std::ifstream file_hd_space(argv[2], std::ios::in|std::ios::binary|std::ios::ate);
        if (!file_pictures.is_open()){
            throw std::runtime_error("label file cannot be found");
        }
        if (!file_hd_space.is_open()){
            throw std::runtime_error("data file cannot be found");
        }

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();

        std::string line;
        std::getline(file_pictures, line);

        const int num_pics(std::atoi(line.c_str()));
        const int num_dimensions(file_hd_space.tellg()/sizeof(float)/num_pics);

        hdi::utils::secureLogValue(&log,"#pics",num_pics);
        hdi::utils::secureLogValue(&log,"#dimensions",num_dimensions);
		
        file_hd_space.seekg (0, std::ios::beg);

        hdi::utils::secureLog(&log,"Reading data...");
		{//initializing panel data
            for(int i = 0; i < num_dimensions; ++i){
                panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
            }
			panel_data.initialize();
		}

		{//reading data
            while(std::getline(file_pictures, line)){
                std::vector<scalar_type> data(num_dimensions);
                file_hd_space.read(reinterpret_cast<char*>(data.data()), sizeof(float) * num_dimensions);
                panel_data.addDataPoint(std::make_shared<hdi::data::TextData>(hdi::data::TextData(line)), data);
            }
		}
        hdi::utils::secureLog(&log,"done!");


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
        params._seed = -2;
        params._num_neighbors = 200;
        params._num_walks_per_landmark = 200;
        params._rs_reduction_factor_per_layer = 0.1;
        params._rs_outliers_removal_jumps = 100;

        double scale = std::log10(num_pics/100);
        hdi::utils::secureLogValue(&log,"Scale",scale);

        MyInterfaceInitializer interface_initializer;
        multiscale_embedder.setInterfaceInitializer(&interface_initializer);
        multiscale_embedder.initialize(scale,params);
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
                hdi::utils::secureLogElementsInVector(&log,ss.str(),multiscale_embedder.hSNE().hierarchy()[s]._landmark_weight,static_cast<scalar_type>(0));
            }
            hdi::utils::secureLog(&log,"--------------\n");
        }

        hdi::utils::secureLog(&log,"@@@@@@@@@@@@@@@@");

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
