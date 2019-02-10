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

        hdi::data::PanelData<scalar_type> panel_data;
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
        std::vector<unsigned int> labels;

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
                    //panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
                }
            }
        }

        hdi::viz::ImageView image_view;
        image_view.setImageSize(28, 28);
        image_view.setResMultiplier(5);
        image_view.setPanelData(&panel_data);
        image_view.show();
        image_view.updateView();

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


        hdi::dr::TSNERandomWalks<scalar_type> tSNE_random_walks;
		{//initializing
			tSNE_random_walks.setLogger(&log);
			tSNE_random_walks.setDimensionality(num_dimensions);
		}
		//Initialization
        hdi::dr::TSNERandomWalks<scalar_type>::Parameters params;
        //params._number_of_landmarks = panel_data.numDataPoints() / 10;
        params._num_neighbors = 100;
        params._number_of_landmarks = 1400;
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


		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
