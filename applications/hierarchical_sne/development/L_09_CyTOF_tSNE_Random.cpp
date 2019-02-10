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
#include "hdi/dimensionality_reduction/hierarchical_sne.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
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
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/utils/math_utils.h"
#include <set>
#include "hdi/dimensionality_reduction/tsne.h"

class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder){
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
    virtual void initializeInfluenceVisualization(embedder_type* embedder, scalar_type* influence){

    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, scalar_type* selection){

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
        if(argc != 5){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}
		
		const int num_data(std::atoi(argv[2]));
		const int num_dimensions(std::atoi(argv[3]));
        const int desired_scale(std::atoi(argv[4]));
		const int picture_size = std::sqrt(num_dimensions)+1;

		std::ifstream file_data(argv[1], std::ios::in|std::ios::binary);
		if (!file_data.is_open()){
			throw std::runtime_error("data file cannot be found");
		}
		

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();

		{//initializing panel data
			for(int j = 0; j < picture_size; ++j){
				for(int i = 0; i < picture_size; ++i){
					if(j*picture_size+i < num_dimensions){
						panel_data.addDimension(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(j,i,picture_size,picture_size)));
					}
				}
			}
			panel_data.initialize();
		}
			

		std::vector<QImage> images;
		std::vector<std::vector<scalar_type> > input_data;
		std::vector<unsigned char> labels;

		{//reading data
			images.reserve(num_data);
			input_data.reserve(num_data);
			labels.reserve(num_data);
			
			for(int i = 0; i < num_data; ++i){
				//still some pics to read for this digit
				input_data.push_back(std::vector<scalar_type>(num_dimensions));
				images.push_back(QImage(picture_size,picture_size,QImage::Format::Format_ARGB32));
				labels.push_back(0);
				const int idx = int(input_data.size()-1);
				for(int i = 0; i < num_dimensions; ++i){
					float value;
					file_data.read((char*)&value,4);
					input_data[idx][i] = value;
					images[idx].setPixel(i%picture_size,i/picture_size,qRgb(value,value,value));
				}
			}

			for(int i = 0; i < num_data; ++i){
				panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(images[i])), input_data[i]);
			}
		}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        std::vector<unsigned int> idxes;
        const unsigned int num_landmarks(num_data/(std::pow(10,desired_scale)));
        {
            std::set<unsigned int> set_idxes;
            std::default_random_engine generator;
            std::uniform_int_distribution<> distribution_int(0, num_landmarks-1);
            hdi::utils::secureLogValue(&log,"#L\n",num_landmarks);
            for(int i = 0; i < num_landmarks; ++i){
                unsigned int v = distribution_int(generator);
                if(set_idxes.find(v) == set_idxes.end()){
                    set_idxes.insert(v);
                    idxes.push_back(v);
                }else{
                    --i;
                }
            }
        }

        hdi::data::PanelData<scalar_type> new_panel_data;
        hdi::data::newPanelDataFromIndexes<scalar_type>(panel_data,new_panel_data,idxes);

        hdi::dr::TSNE<scalar_type> tsne;
        tsne.setDimensionality(num_dimensions);
        for(int i = 0; i < num_landmarks; ++i){
            tsne.addDataPoint(new_panel_data.getData().data() + i*num_dimensions);
        }
        hdi::data::Embedding<scalar_type> embedding;
        tsne.initialize(&embedding);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();

        std::vector<uint32_t> flags(num_landmarks,0);
        hdi::viz::ScatterplotDrawerFixedColor drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), flags.data(), num_landmarks);
        drawer.setAlpha(0.5);
        drawer.setPointSize(5);
        viewer.addDrawer(&drawer);

        while(true){
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
            viewer.show();
            QApplication::processEvents();
        }
        return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
