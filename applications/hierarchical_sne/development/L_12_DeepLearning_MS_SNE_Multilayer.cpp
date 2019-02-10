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


//Test for the single layer of the multiscale embedder
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

        std::string line;
        std::getline(file_pictures, line);

        const int num_pics(std::atoi(line.c_str()));
        const int num_dimensions(file_hd_space.tellg()/sizeof(float)/num_pics);

        hdi::utils::secureLogValue(&log,"#pics",num_pics);
        hdi::utils::secureLogValue(&log,"#dimensions",num_dimensions);
		
        file_hd_space.seekg (0, std::ios::beg);

        hdi::utils::secureLog(&log,"Reading data...");
		hdi::data::PanelData<scalar_type> panel_data;
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
            for(int i = 0; i < panel_data.numDataPoints(); ++i){
                //panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
            }
		}



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        //const int iterations = 15000;
        hdi::dr::HierarchicalSNE<scalar_type> multiscale_random_walks;
        {//initializing
            multiscale_random_walks.setLogger(&log);
            multiscale_random_walks.setDimensionality(num_dimensions);
            hdi::dr::HierarchicalSNE<scalar_type>::Parameters params;
            params._seed = 1;
            params._num_walks_per_landmark = 1000;
            params._rs_reduction_factor_per_layer = 0.3;
            params._rs_outliers_removal_jumps = 0;//Check for infinite loops
            multiscale_random_walks.initialize(panel_data.getData().data(),panel_data.numDataPoints(),params);
            multiscale_random_walks.statistics().log(&log);
        }
        const int desired_scale = 1;
        for(int s = 0; s < desired_scale; ++s){
            multiscale_random_walks.addScale();
        }
        for(int s = 0; s <= desired_scale; ++s){
            auto image = hdi::utils::imageFromSparseMatrix(multiscale_random_walks.hierarchy()[s]._transition_matrix);
            image.save(QString("P%1.png").arg(s));
        }

        std::cout << "# DATA POINTS: " << multiscale_random_walks.hierarchy()[desired_scale]._transition_matrix.size() << std::endl;
        hdi::analytics::MultiscaleEmbedderSingleView top_level_embedding;
        hdi::analytics::MultiscaleEmbedderSingleView::tsne_type::Parameters tsne_params;
        tsne_params._seed = 1;
        newPanelDataFromIndexes(panel_data, top_level_embedding.getPanelData(), multiscale_random_walks.hierarchy()[desired_scale]._landmark_to_original_data_idx);
        top_level_embedding.initialize(multiscale_random_walks.hierarchy()[desired_scale]._transition_matrix,std::tuple<unsigned int,unsigned int>(0,0),tsne_params);
        top_level_embedding.setLogger(&log);
        {
            std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
            top_level_embedding.addView(image_view);
            image_view->_text_data_as_os_path = true;
            std::shared_ptr<hdi::viz::MultipleHeatmapsView> heatmaps_view(new hdi::viz::MultipleHeatmapsView());
            top_level_embedding.addView(heatmaps_view);
            heatmaps_view->setAuxData(top_level_embedding.getPanelData().numDimensions(),top_level_embedding.getPanelData().getData().data());
            image_view->show();
            heatmaps_view->show();

            std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
            top_level_embedding.addUserDefinedDrawer(drawer);
            drawer->setData(top_level_embedding.getEmbedding().getContainer().data(), top_level_embedding.getPanelData().getFlagsDataPoints().data(), top_level_embedding.getPanelData().numDataPoints());
            drawer->setAlpha(0.5);
            drawer->setPointSize(10);
        }

        while(true){
            top_level_embedding.doAnIteration();

            QApplication::processEvents();
        }
		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
