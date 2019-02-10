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
#include "hdi/dimensionality_reduction/tsne_random_walks.h"
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include <iostream>
#include <fstream>
#include "hdi/data/panel_data.h"
#include "hdi/data/pixel_data.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/image_view_qobj.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include "hdi/utils/dataset_utils.h"

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
        std::vector<QColor> color_per_digit;
        color_per_digit.push_back(qRgb(16,78,139));
        color_per_digit.push_back(qRgb(139,90,43));
        color_per_digit.push_back(qRgb(138,43,226));
        color_per_digit.push_back(qRgb(0,128,0));
        color_per_digit.push_back(qRgb(255,150,0));
        color_per_digit.push_back(qRgb(204,40,40));
        color_per_digit.push_back(qRgb(131,139,131));
        color_per_digit.push_back(qRgb(0,205,0));
        color_per_digit.push_back(qRgb(20,20,20));
        color_per_digit.push_back(qRgb(0, 150, 255));
        std::srand(2);
        for(int i = 0; i < 4000; ++i){color_per_digit.push_back(qRgb(std::rand()%255, std::rand()%255, std::rand()%255));}


        hdi::data::PanelData<scalar_type> panel_data;
        std::vector<unsigned int> labels;

        hdi::utils::IO::loadTimit(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));
        hdi::utils::secureLog(&log,"Data loaded...");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


		hdi::dr::TSNERandomWalks<scalar_type> tSNE_random_walks;
		{//initializing
			tSNE_random_walks.setLogger(&log);
            tSNE_random_walks.setDimensionality(panel_data.numDimensions());
		}
		//Initialization
		hdi::dr::TSNERandomWalks<scalar_type>::Parameters params;
        //params._number_of_landmarks = panel_data.numDataPoints() / 10;
        params._num_neighbors = 100;
        params._number_of_landmarks = 2493;
		params._distance_weighted_random_walk = true;
		tSNE_random_walks.initialize(panel_data.getData().data(),panel_data.numDataPoints(),params);

		int num_lmks_in_embedding = tSNE_random_walks.getNumberOfLandmarks();

		hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
		viewer.setSelectionColor(qRgb(50,50,50));
		viewer.resize(500,500);
		viewer.show();

		std::vector<uint32_t> flags(num_lmks_in_embedding,0);
		std::vector<float> embedding_for_viz(num_lmks_in_embedding*2);
		std::vector<float> embedding_colors_for_viz(num_lmks_in_embedding*3,0);
		{
			auto& idx_landmarks = tSNE_random_walks.getLandmarksIds();
			for(int i = 0; i < num_lmks_in_embedding; ++i){
                int idx = labels[idx_landmarks[i]];
				embedding_colors_for_viz[i * 3] = color_per_digit[idx].redF();
				embedding_colors_for_viz[i * 3 + 1] = color_per_digit[idx].greenF();
				embedding_colors_for_viz[i * 3 + 2] = color_per_digit[idx].blueF();
			}
		}

		hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
		drawer.initialize(viewer.context());
		drawer.setData(embedding_for_viz.data(), embedding_colors_for_viz.data(), flags.data(), num_lmks_in_embedding);
        drawer.setAlpha(0.5);
		drawer.setPointSize(5);
		viewer.addDrawer(&drawer);

		
		auto& embedding = tSNE_random_walks.getEmbedding();
        for(int i = 0; i < 1000; ++i){
			tSNE_random_walks.doAnIteration();
				
			scalar_type min_x(std::numeric_limits<scalar_type>::max());
			scalar_type max_x(-std::numeric_limits<scalar_type>::max());
			scalar_type min_y(std::numeric_limits<scalar_type>::max());
			scalar_type max_y(-std::numeric_limits<scalar_type>::max());
			{
				for(int i = 0; i < num_lmks_in_embedding; ++i){
					min_x = std::min(min_x,embedding[i*2]);
					max_x = std::max(max_x,embedding[i*2]);
					min_y = std::min(min_y,embedding[i*2+1]);
					max_y = std::max(max_y,embedding[i*2+1]);
					embedding_for_viz[i*2] = embedding[i*2];
					embedding_for_viz[i*2+1] = embedding[i*2+1];
				}
                viewer.setTopRightCoordinates(QVector2D(max_x+10,max_y+10));
                viewer.setBottomLeftCoordinates(QVector2D(min_x-10,min_y-10));
			}
			
			if((i%10) == 0){
				viewer.updateGL();
				viewer.show();
			}
			if((i%100) == 0){
				printf("%d: - rd: %f\n", i,max_x-min_x);
			}
			QApplication::processEvents();
		}

		tSNE_random_walks.statistics().log(&log);

        viewer.saveToFile("timit_lsne.png");


		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
