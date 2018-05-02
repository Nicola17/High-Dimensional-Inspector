/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
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

#include <assert.h>
#include "hdi/visualization/heatmap_view_qobj.h"

#include <QApplication>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include <iostream>
#include "hdi/utils/timing_utils.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/utils/visual_utils.h"

int main(int argc, char *argv[]){
  try{
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    typedef float scalar_type;
    typedef uint32_t flag_type;

    hdi::utils::CoutLog log;

    std::vector<unsigned int> labels;
    hdi::data::PanelData<scalar_type> panel_data;
    hdi::utils::loadGaussianSpheres(panel_data,labels,10,1000,10);

    hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
    hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
    prob_gen.setLogger(&log);
    prob_gen.computeJointProbabilityDistribution(panel_data.getData().data(),panel_data.numDimensions(),panel_data.numDataPoints(),probability);

    hdi::data::Embedding<scalar_type> embedding;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type>::Parameters tSNE_params;
    tSNE.setLogger(&log);
    tSNE_params._seed = 1;
    tSNE_params._exaggeration_factor = 4;
    tSNE.setTheta(0.5);
    {
      tSNE.initialize(probability,&embedding,tSNE_params);
    }

    std::map<unsigned int, QColor> palette;
    for(int i = 0; i < 100; ++i){
      palette[i] = qRgb(rand()%256,rand()%256,rand()%256);
    }

    hdi::viz::ScatterplotCanvas viewer;
    viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
    viewer.setSelectionColor(qRgb(50,50,50));
    viewer.resize(500,500);
    viewer.show();
    hdi::viz::ScatterplotDrawerLabels drawer;
    drawer.initialize(viewer.context());
    drawer.setData(embedding.getContainer().data(),panel_data.getFlagsDataPoints().data(),labels.data(),palette, panel_data.numDataPoints());
    drawer.setAlpha(0.5);
    drawer.setSelectionColor(qRgb(170,170,170));
    drawer.setSelectionPointSizeMult(5);
    drawer.setPointSize(5);
    viewer.addDrawer(&drawer);

    hdi::viz::HeatMapView heatmap_view;
    heatmap_view.setPanelData(&panel_data);
    heatmap_view.resize(QSize(400,800));
    heatmap_view.show();

    hdi::viz::ControllerSelectionEmbedding selection_controller;
    selection_controller.setActors(&panel_data,&embedding,&viewer);
    //selection_controller.setLogger(&log);
    selection_controller.initialize();
    selection_controller.addView(&heatmap_view);

    {
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

    return app.exec();

  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
