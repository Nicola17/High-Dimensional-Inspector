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
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/utils/visual_utils.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/data/embedding.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include <QDir>
#include "hdi/utils/math_utils.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include "hdi/data/nano_flann.h"
#include "hdi/dimensionality_reduction/evaluation.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/utils/scoped_timers.h"

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

        hdi::data::PanelData<scalar_type> panel_data;
        std::vector<unsigned int> labels;

        hdi::utils::IO::loadMNIST(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));
        hdi::utils::secureLog(&log,"Data loaded...");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::dr::HierarchicalSNE<scalar_type> hsne;
		{//initializing
            hsne.setLogger(&log);
            hsne.setDimensionality(panel_data.numDimensions());
			auto& data = panel_data.getData();
		}
		//Initialization
        hdi::dr::HierarchicalSNE<scalar_type>::Parameters params;
        params._seed = -2;
        params._num_neighbors = 90;
        params._num_walks_per_landmark = 100;
        params._aknn_num_trees = 4;
        params._aknn_num_checks = 1024;

        params._monte_carlo_sampling = false;
        params._rs_reduction_factor_per_layer = 0.2;

        double time(0);
        const int desired_scale = 2;
        {
            hdi::utils::ScopedIncrementalTimer<double> timer(time);
            hsne.initialize(panel_data.getData().data(),panel_data.numDataPoints(),params);
            hsne.statistics().log(&log);

            for(int s = 0; s < desired_scale; ++s){
                hsne.addScale();
                hdi::utils::secureLog(&log,"--------------\n");
            }
        }

        hdi::dr::HierarchicalSNE<scalar_type>::sparse_scalar_matrix_type closeness;
        hsne.getInterpolationWeights(closeness);

        hdi::data::Embedding<scalar_type> embedding;
		hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
        tSNE.initialize(hsne.hierarchy()[desired_scale]._transition_matrix,&embedding);

        hdi::data::Embedding<scalar_type> embedding_full;
        embedding_full.resize(2,panel_data.numDataPoints());

		hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
		viewer.setSelectionColor(qRgb(50,50,50));
		viewer.resize(500,500);
		viewer.show();

        hdi::data::PanelData<scalar_type> new_panel_data;
        hdi::data::newPanelDataFromIndexes(panel_data,new_panel_data,hsne.hierarchy()[desired_scale]._landmark_to_original_data_idx);

        hdi::viz::MultipleImageView image_view;
        image_view.setPanelData(&panel_data);
        image_view.show();
        image_view.updateView();

        hdi::viz::ControllerSelectionEmbedding selection_controller;
        selection_controller.setActors(&panel_data,&embedding_full,&viewer);
        selection_controller.setLogger(&log);
        selection_controller.initialize();
        selection_controller.addView(&image_view);

        std::vector<uint32_t> flags(panel_data.numDataPoints(),0);
        std::vector<float> embedding_colors_for_viz(panel_data.numDataPoints()*3,0);

        for(int i = 0; i < panel_data.numDataPoints(); ++i){
            int label = labels[i];
			auto color = color_per_digit[label];
			embedding_colors_for_viz[i*3+0] = color.redF();
			embedding_colors_for_viz[i*3+1] = color.greenF();
			embedding_colors_for_viz[i*3+2] = color.blueF();
		}

		hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
		drawer.initialize(viewer.context());
        drawer.setData(embedding_full.getContainer().data(), embedding_colors_for_viz.data(), flags.data(), panel_data.numDataPoints());
        drawer.setAlpha(0.15);
        drawer.setPointSize(5);
		viewer.addDrawer(&drawer);

        {
            hdi::utils::ScopedIncrementalTimer<double> timer(time);
            int iter = 0;
            while(iter < 1000){
                tSNE.doAnIteration();

                if((iter%50) == 0){
                    std::cout << iter << std::endl;
                }
                ++iter;
            }
            hdi::data::interpolateEmbeddingPositions(embedding,embedding_full,closeness);
        }
        std::cout << "Comp time:\t" << time << std::endl;
/*
        {//precision/recall
            std::vector<scalar_type> precision;
            std::vector<scalar_type> recall;
            std::vector<unsigned int> pnts_to_evaluate(panel_data.numDataPoints());
            std::iota(pnts_to_evaluate.begin(),pnts_to_evaluate.end(),0);
            hdi::dr::computePrecisionRecall(panel_data,embedding_full,pnts_to_evaluate,precision,recall,30);

            std::cout << std::endl << std::endl;
            for(int i = 0; i < precision.size(); ++i){
                std::cout << precision[i] << "\t" << recall[i] << std::endl;
            }
        }
        */

        std::map<unsigned int, QColor> palette;
        palette[0] = qRgb(16,78,139);
        palette[1] = qRgb(139,90,43);
        palette[2] = qRgb(138,43,226);
        palette[3] = qRgb(0,128,0);
        palette[4] = qRgb(255,150,0);
        palette[5] = qRgb(204,40,40);
        palette[6] = qRgb(131,139,131);
        palette[7] = qRgb(0,205,0);
        palette[8] = qRgb(20,20,20);
        palette[9] = qRgb(0, 150, 255);

        hdi::viz::ScatterplotCanvas viewer_full;
        hdi::viz::ScatterplotDrawerLabels drawer_labels;
        {
            viewer_full.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_full.resize(500,500);
            viewer_full.show();
            std::vector<scalar_type> limits;
            embedding_full.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_full.setTopRightCoordinates(tr);
            viewer_full.setBottomLeftCoordinates(bl);

            drawer_labels.initialize(viewer_full.context());
            drawer_labels.setData(embedding_full.getContainer().data(),flags.data(),labels.data(),palette,embedding_full.numDataPoints());
            drawer_labels.setPointSize(5);
            viewer_full.addDrawer(&drawer_labels);

            viewer_full.updateGL();
            viewer_full.show();
            QApplication::processEvents();
        }

        hdi::viz::ScatterplotCanvas viewer_fix_color;
        hdi::viz::ScatterplotDrawerFixedColor drawer_fixed;
        {
            viewer_fix_color.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_fix_color.resize(500,500);
            viewer_fix_color.show();
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_fix_color.setTopRightCoordinates(tr);
            viewer_fix_color.setBottomLeftCoordinates(bl);

            drawer_fixed.initialize(viewer_fix_color.context());
            drawer_fixed.setData(embedding_full.getContainer().data(),flags.data(),embedding_full.numDataPoints());
            drawer_fixed.setPointSize(5);
            drawer_fixed.setAlpha(0.2);
            viewer_fix_color.addDrawer(&drawer_fixed);

            viewer_fix_color.updateGL();
            viewer_fix_color.show();
            QApplication::processEvents();
        }


        std::vector<unsigned int> labels_pivot(hsne.hierarchy()[desired_scale]._landmark_to_original_data_idx.size());
        std::vector<uint32_t> flags_pivot(hsne.hierarchy()[desired_scale]._landmark_to_original_data_idx.size(),0);
        hdi::viz::ScatterplotCanvas viewer_pivot;
        hdi::viz::ScatterplotDrawerLabels drawer_labels_pivot;
        {
            for(int i = 0; i < hsne.hierarchy()[desired_scale]._landmark_to_original_data_idx.size(); ++i){
                labels_pivot[i] = labels[hsne.hierarchy()[desired_scale]._landmark_to_original_data_idx[i]];
            }
        }
        {
            viewer_pivot.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
            viewer_pivot.resize(500,500);
            viewer_pivot.show();
            std::vector<scalar_type> limits;
            embedding.computeEmbeddingBBox(limits,0.2);
            auto tr = QVector2D(limits[1],limits[3]);
            auto bl = QVector2D(limits[0],limits[2]);
            viewer_pivot.setTopRightCoordinates(tr);
            viewer_pivot.setBottomLeftCoordinates(bl);

            drawer_labels_pivot.initialize(viewer.context());
            drawer_labels_pivot.setData(embedding.getContainer().data(),flags_pivot.data(),labels_pivot.data(),palette,embedding.numDataPoints());
            drawer_labels_pivot.setPointSize(8);
            viewer_pivot.addDrawer(&drawer_labels_pivot);

            viewer_pivot.updateGL();
            viewer_pivot.show();
            QApplication::processEvents();
        }

        int it = 0;
        while(++it<10){
            viewer_fix_color.saveToFile("mnist_hsne_fixed_color.png");
            viewer_full.saveToFile("mnist_hsne_full.png");
            viewer_pivot.saveToFile("mnist_hsne_landmarks.png");
            QApplication::processEvents();
        }

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
