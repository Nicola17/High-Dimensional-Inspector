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
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/data/nano_flann.h"
#include "hdi/data/vptree.h"
#include "hdi/dimensionality_reduction/evaluation.h"
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
        std::srand(2);
        for(int i = 0; i < 4000; ++i){color_per_digit.push_back(qRgb(std::rand()%255, std::rand()%255, std::rand()%255));}

        hdi::data::PanelData<scalar_type> panel_data;
        std::vector<unsigned int> labels;

        hdi::utils::IO::loadTimit(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));
        hdi::utils::secureLog(&log,"Data loaded...");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        double time(0);

        hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
        hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
        prob_gen.setLogger(&log);
        prob_gen.computeJointProbabilityDistribution(panel_data.getData().data(),panel_data.numDimensions(),panel_data.numDataPoints(),probability);

        prob_gen.statistics().log(&log);

        hdi::data::Embedding<scalar_type> embedding;
        hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
        hdi::dr::SparseTSNEUserDefProbabilities<scalar_type>::Parameters tSNE_params;
        tSNE.setLogger(&log);
        tSNE_params._seed = 1;
        tSNE.setTheta(0.5);
        {
            hdi::utils::ScopedIncrementalTimer<double> timer(time);
            tSNE.initializeWithJointProbabilityDistribution(probability,&embedding,tSNE_params);
        }

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(500,500);
        viewer.show();

        hdi::viz::MultipleImageView image_view;
        image_view.setPanelData(&panel_data);
        image_view.show();
        image_view.updateView();

        hdi::viz::ControllerSelectionEmbedding selection_controller;
        selection_controller.setActors(&panel_data,&embedding,&viewer);
        //selection_controller.setLogger(&log);
        selection_controller.initialize();
        selection_controller.addView(&image_view);

        std::vector<uint32_t> flags(panel_data.numDataPoints(),0);
        std::vector<scalar_type> embedding_colors_for_viz(panel_data.numDataPoints()*3,0);

        for(int i = 0; i < panel_data.numDataPoints(); ++i){
            int label = labels[i];
            auto color = color_per_digit[label];
            embedding_colors_for_viz[i*3+0] = color.redF();
            embedding_colors_for_viz[i*3+1] = color.greenF();
            embedding_colors_for_viz[i*3+2] = color.blueF();
        }

        hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), embedding_colors_for_viz.data(), flags.data(), panel_data.numDataPoints());
        drawer.setAlpha(0.8);
        drawer.setPointSize(5);
        viewer.addDrawer(&drawer);

        {
            hdi::utils::ScopedIncrementalTimer<double> timer(time);
            int iter = 0;
            while(iter < 1000){
                tSNE.doAnIteration();

                {//limits
                    std::vector<scalar_type> limits;
                    embedding.computeEmbeddingBBox(limits,0.25);
                    auto tr = QVector2D(limits[1],limits[3]);
                    auto bl = QVector2D(limits[0],limits[2]);
                    viewer.setTopRightCoordinates(tr);
                    viewer.setBottomLeftCoordinates(bl);
                }

                if(((iter+1)%50) == 0){
                    hdi::utils::secureLogValue(&log,"Iter",iter+1);
                }
                viewer.updateGL();
                QApplication::processEvents();
                ++iter;
            }
        }

        std::cout << "Comp time:\t" << time << std::endl;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


        std::vector<scalar_type> precision;
        std::vector<scalar_type> recall;
        std::vector<unsigned int> pnts_to_evaluate(panel_data.numDataPoints());
        std::iota(pnts_to_evaluate.begin(),pnts_to_evaluate.end(),0);

        hdi::dr::computePrecisionRecall(panel_data,embedding,pnts_to_evaluate,precision,recall,30);

        std::cout << std::endl << std::endl;
        std::ofstream latex_chart_stream("timit_tsne_pr.tex");
        latex_chart_stream << "\\addplot[red]coordinates{" << std::endl;
        for(int i = 0; i < precision.size(); ++i){
            std::cout << precision[i] << "\t" << recall[i] << std::endl;
            latex_chart_stream << "(" << precision[i] << "," << recall[i] << ")" << std::endl;
        }
        latex_chart_stream << "};\\label{plots:tSNE:TIMIT}" << std::endl;

        viewer.saveToFile("timit_tsne.png");
        return app.exec();
    }
    catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
    catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
    catch (std::bad_alloc& ex){ std::cout << "Bad alloc: " << ex.what();}
    catch(...){ std::cout << "An unknown error occurred";}
}
