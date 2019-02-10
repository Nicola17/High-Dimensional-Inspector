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

template <typename scalar_type>
void saveResults(hdi::dr::HierarchicalSNE<scalar_type>& random_walks,std::vector<QImage> images, std::vector<unsigned char> labels, int desired_scale){
	QDir qdir("./L7_RandomWalksResults/");
	qdir.removeRecursively();
	qdir.cdUp();
	qdir.mkdir("L7_RandomWalksResults");
	qdir.cd("L7_RandomWalksResults");
/*
	for(int n_scale = 0; n_scale <= desired_scale; ++n_scale){
		qdir.mkdir(QString("%1").arg(n_scale));
		qdir.mkdir(QString("%1/low_num_visits").arg(n_scale));
		qdir.mkdir(QString("%1/low_num_visit_landmarks").arg(n_scale));
		qdir.mkdir(QString("%1/high_num_visits").arg(n_scale));
		qdir.mkdir(QString("%1/high_num_visit_landmarks").arg(n_scale));
		qdir.mkdir(QString("%1/landmarks").arg(n_scale));

		unsigned int max_visits = 0;
        auto& scale = random_walks.hierarchy()[n_scale];
		for(int i = 0; i < scale._random_walks_passing_by.size(); ++i){
			max_visits = std::max(max_visits,scale._random_walks_passing_by[i]);
			if(scale._random_walks_passing_by[i] < 3 && scale._previous_scale_to_landmark_idx[i] == -1){
				images[i].save(QString("L7_RandomWalksResults/%1/low_num_visits/%2_%3.png").arg(n_scale).arg(scale._random_walks_passing_by[i]).arg(i));
			}
		}
		for(int i = 0; i < scale._random_walks_passing_by.size(); ++i){
			max_visits = std::max(max_visits,scale._random_walks_passing_by[i]);
			if(scale._random_walks_passing_by[i] > max_visits * 0.5){
				images[i].save(QString("L7_RandomWalksResults/%1/high_num_visits/%2_%3.png").arg(n_scale).arg(scale._random_walks_passing_by[i]).arg(i));
			}
		}

		//Landmark
		for(int i = 0; i < scale._landmark_to_original_data_idx.size(); ++i){
			unsigned int lower_lvl_idx = scale._landmark_to_previous_scale_idx[i];
			if(scale._random_walks_passing_by[lower_lvl_idx] < 3){
				images[scale._landmark_to_original_data_idx[i]].save(QString("L7_RandomWalksResults/%1/low_num_visit_landmarks/%2_%3_%4.png").arg(n_scale).arg(scale._random_walks_passing_by[lower_lvl_idx]).arg(i).arg(scale._landmark_to_original_data_idx[i]));
			}
		}
		for(int i = 0; i < scale._landmark_to_original_data_idx.size(); ++i){
			unsigned int lower_lvl_idx = scale._landmark_to_previous_scale_idx[i];
			if(scale._random_walks_passing_by[lower_lvl_idx] > 300){
				images[scale._landmark_to_original_data_idx[i]].save(QString("L7_RandomWalksResults/%1/high_num_visit_landmarks/%2_%3_%4.png").arg(n_scale).arg(scale._random_walks_passing_by[lower_lvl_idx]).arg(i).arg(scale._landmark_to_original_data_idx[i]));
			}
		}
		for(int i = 0; i < scale._landmark_to_original_data_idx.size(); ++i){
			images[scale._landmark_to_original_data_idx[i]].save(QString("L7_RandomWalksResults/%1/landmarks/%2_%3.png").arg(n_scale).arg(i).arg(scale._landmark_to_original_data_idx[i]));
		}

		if(n_scale >= 1){
			QDir qdir(QString("./L7_RandomWalksResults/%1/landmarks").arg(n_scale));
            auto& scale = random_walks.hierarchy()[n_scale];
            auto& previous_scale = random_walks.hierarchy()[n_scale-1];

			for(int i = 0; i < scale._landmark_to_original_data_idx.size(); ++i){
				qdir.mkdir(QString("%1").arg(i));
	
				unsigned int lower_lvl_idx = scale._landmark_to_previous_scale_idx[i];
                for(auto& v: previous_scale._transition_matrix[lower_lvl_idx]){
					if(v.second > 0.05){
						images[previous_scale._landmark_to_original_data_idx[v.first]].save(QString("L7_RandomWalksResults/%1/landmarks/%2/%3_%4.png").arg(n_scale).arg(i).arg(v.second, 5).arg(v.first));
					}
				}
			}
		}

        auto image = hdi::utils::imageFromSparseMatrix(random_walks.hierarchy()[n_scale]._transition_matrix);
		image.save(QString("L7_RandomWalksResults/P_%1.png").arg(n_scale));
	}
	
    */
}

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 6){
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

			const int digit_to_be_selected = 4;
			for(int i = 0; i < images.size(); ++i){
				panel_data.addDataPoint(std::make_shared<hdi::data::ImageData>(hdi::data::ImageData(images[i])), input_data[i]);
				if(labels[i] == digit_to_be_selected){
					panel_data.getFlagsDataPoints()[i] = hdi::data::PanelData<scalar_type>::Selected;
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

        hdi::dr::HierarchicalSNE<scalar_type> random_walks;
		{//initializing
			random_walks.setLogger(&log);
			random_walks.setDimensionality(num_dimensions);
		}
		//Initialization
        hdi::dr::HierarchicalSNE<scalar_type>::Parameters params;
        params._seed = 4;
		params._num_walks_per_landmark = 1000;
        params._rs_reduction_factor_per_layer = 0.1;
        params._rs_outliers_removal_jumps = std::atoi(argv[5]);

		random_walks.initialize(panel_data.getData().data(),panel_data.numDataPoints(),params);
		random_walks.statistics().log(&log);

        const int desired_scale = std::atoi(argv[4]);
		for(int s = 0; s < desired_scale; ++s){
			random_walks.addScale();
		}

		saveResults(random_walks, images, labels, desired_scale);

        hdi::data::Embedding<scalar_type> embedding;
		hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
        tSNE.initialize(random_walks.scale(desired_scale)._transition_matrix,&embedding);

		hdi::viz::ScatterplotCanvas viewer;
		viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
		viewer.setSelectionColor(qRgb(50,50,50));
		viewer.resize(500,500);
		viewer.show();

		std::vector<uint32_t> flags(tSNE.getNumberOfDataPoints(),0);
		std::vector<float> embedding_colors_for_viz(tSNE.getNumberOfDataPoints()*3,0);

        for(int i = 0; i < random_walks.hierarchy()[desired_scale]._landmark_to_original_data_idx.size(); ++i){
            int label = labels[random_walks.hierarchy()[desired_scale]._landmark_to_original_data_idx[i]];
			auto color = color_per_digit[label];
			embedding_colors_for_viz[i*3+0] = color.redF();
			embedding_colors_for_viz[i*3+1] = color.greenF();
			embedding_colors_for_viz[i*3+2] = color.blueF();
		}

		hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
		drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), embedding_colors_for_viz.data(), flags.data(), tSNE.getNumberOfDataPoints());
		drawer.setAlpha(0.5);
		drawer.setPointSize(5);
		viewer.addDrawer(&drawer);

		for(int i = 0; i < 1500*std::max(desired_scale*desired_scale,1); ++i){
			tSNE.doAnIteration();
            {//limits
                std::vector<scalar_type> limits;
                embedding.computeEmbeddingBBox(limits,0.25);
                auto tr = QVector2D(limits[1],limits[3]);
                auto bl = QVector2D(limits[0],limits[2]);
                viewer.setTopRightCoordinates(tr);
                viewer.setBottomLeftCoordinates(bl);
            }
			
			if((i%10) == 0){
				viewer.updateGL();
				viewer.show();
			}
			QApplication::processEvents();
		}

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
