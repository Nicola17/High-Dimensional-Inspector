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
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"

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
            throw std::runtime_error("picture file cannot be found");
        }
        if (!file_hd_space.is_open()){
            throw std::runtime_error("hd space file cannot be found");
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
                QImage img(QString::fromStdString(line));
                file_hd_space.read(reinterpret_cast<char*>(data.data()), sizeof(float) * num_dimensions);
                panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(img)), data);
            }
            for(int i = 0; i < panel_data.numDataPoints(); ++i){
                //panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
            }
		}

        hdi::viz::MultipleImageView image_view;
        image_view.setPanelData(&panel_data);
        image_view.show();
        image_view.updateView();

        hdi::viz::MultipleHeatmapsView heatmaps_view;
        heatmaps_view.setPanelData(&panel_data);
        heatmaps_view.setAuxData(panel_data.numDimensions(),panel_data.getData().data());
        heatmaps_view.show();
        heatmaps_view.updateView();

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        const int iterations = 1000;

        hdi::data::Embedding<scalar_type> embedding;
        hdi::dr::TSNE<scalar_type> tSNE;
        tSNE.setLogger(&log);
        tSNE.setDimensionality(num_dimensions);

        for(int i = 0; i < panel_data.numDataPoints(); ++i){
            tSNE.addDataPoint(panel_data.getData().data()+i*num_dimensions);
        }
        tSNE.initialize(&embedding);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();

        hdi::viz::ScatterplotDrawerFixedColor drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), panel_data.getFlagsDataPoints().data(), panel_data.numDataPoints());
        drawer.setAlpha(0.5);
        drawer.setPointSize(5);
        viewer.addDrawer(&drawer);

        hdi::viz::ControllerSelectionEmbedding selection_controller;
        selection_controller.setActors(&panel_data,&embedding,&viewer);
        selection_controller.setLogger(&log);
        selection_controller.initialize();
        selection_controller.addView(&image_view);
        selection_controller.addView(&heatmaps_view);

        hdi::utils::secureLog(&log,"Computing gradient descent...");
        for(int iter = 0; iter < iterations; ++iter){
            tSNE.doAnIteration();

            {//limits
                std::vector<scalar_type> limits;
                embedding.computeEmbeddingBBox(limits,0.25);
                auto tr = QVector2D(limits[1],limits[3]);
                auto bl = QVector2D(limits[0],limits[2]);
                viewer.setTopRightCoordinates(tr);
                viewer.setBottomLeftCoordinates(bl);
            }

            hdi::utils::secureLogValue(&log,"Iter",iter,true);
            if((iter%10) == 0){
                viewer.updateGL();
                viewer.show();
            }
            QApplication::processEvents();
        }
        hdi::utils::secureLog(&log,"... done!");

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
