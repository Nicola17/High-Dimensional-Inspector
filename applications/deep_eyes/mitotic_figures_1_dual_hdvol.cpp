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
#include "hdi/data/pixel_data.h"
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
#include "hdi/utils/dataset_utils.h"
#include "hdi/dimensionality_reduction/evaluation.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/dimensionality_reduction/wtsne.h"
#include "hdi/visualization/heatmap_view_qobj.h"

class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
        std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
        embedder->addView(image_view);
        image_view->_text_data_as_os_path = true;
        image_view->show();

        std::shared_ptr<hdi::viz::HeatMapView> heatmap_view (new hdi::viz::HeatMapView);
        embedder->addView(heatmap_view);
        heatmap_view->resize(QSize(400,800));
        heatmap_view->setPanelData(&embedder->getPanelData());
        heatmap_view->show();

        std::vector<unsigned int> current_labels;
        current_labels.reserve(idxes_to_orig_data.size());
        for(auto id: idxes_to_orig_data)
            current_labels.push_back((*_labels)[id]);

        {
            std::shared_ptr<hdi::viz::ScatterplotDrawerLabels> drawer(new hdi::viz::ScatterplotDrawerLabels());
            embedder->addUserDefinedDrawer(drawer);
            drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), current_labels.data(), *_palette, embedder->getPanelData().numDataPoints());
            drawer->setPointSize(7.5);
            drawer->setSelectionColor(qRgb(200,200,200));
            drawer->setAlpha(0.5);
            drawer->setSelectionPointSizeMult(5);
        }
    }
    virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());

        embedder->addAreaOfInfluenceDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), influence, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->updateLimitsFromData();
        drawer->setPointSize(25);
        drawer->setAlpha(1);
    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());
        embedder->addSelectionDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), selection, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setLimits(0,0.1);
        drawer->setPointSize(25);
        drawer->setAlpha(1);
    }

    virtual void updateSelection(embedder_type* embedder, scalar_type* selection){
    }

    virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){}

public:
    std::map<unsigned int, QColor>* _palette;
    std::vector<unsigned int>* _labels;
};

void function(std::tuple<unsigned int, unsigned int> embedder_id_type, int key){

}

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

        std::map<unsigned int, QColor> palette;
        palette[0] = qRgb(150,0,0);
        palette[1] = qRgb(0,150,255);

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type transposed_panel_data;

        std::vector<unsigned int> labels;
        hdi::utils::IO::loadMitoticFiguresAndVolume(transposed_panel_data,labels,argv[1],argv[2]);

        hdi::data::transposePanelData(transposed_panel_data,panel_data);

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        panel_data.requestDimProperty("selected_avg");
        panel_data.requestDimProperty("selected_std_dev");
        panel_data.requestDimProperty("selected_max");
        panel_data.requestDimProperty("selected_min");

        //IMAGES
        hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
        params._seed = 2;
        params._num_neighbors = 90;
        params._num_walks_per_landmark = 100;
        params._aknn_num_trees = 4;
        params._aknn_num_checks = 1024;

        params._monte_carlo_sampling = true;
        params._rs_reduction_factor_per_layer = 0.2;


        MyInterfaceInitializer interface_initializer;
        interface_initializer._palette = &palette;
        interface_initializer._labels = &labels;
        multiscale_embedder.setInterfaceInitializer(&interface_initializer);
        multiscale_embedder.initialize(2,params);
        multiscale_embedder.createTopLevelEmbedder();

        //NEURONS
        hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type probability;
        hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
        hdi::dr::HDJointProbabilityGenerator<scalar_type>::Parameters prob_gen_params;
        prob_gen_params._num_trees = 20;
        prob_gen_params._num_checks = 20000;
        prob_gen.setLogger(&log);
        prob_gen.computeJointProbabilityDistribution(transposed_panel_data.getData().data(),transposed_panel_data.numDimensions(),transposed_panel_data.numDataPoints(),probability);

        hdi::utils::secureLogValue(&log,"TPD dim",transposed_panel_data.numDimensions());
        hdi::utils::secureLogValue(&log,"TPD dp",transposed_panel_data.numDataPoints());
        hdi::utils::secureLogValue(&log,"Neuron P size",probability.size());
        prob_gen.statistics().log(&log);

        hdi::utils::imageFromSparseMatrix(probability).save("neurons_P.png");
        hdi::utils::imageFromSparseMatrix(multiscale_embedder.hSNE().scale(0)._transition_matrix).save("images_P.png");
        hdi::data::Embedding<scalar_type> embedding;
        //hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
        //hdi::dr::SparseTSNEUserDefProbabilities<scalar_type>::Parameters tSNE_params;
        hdi::dr::WeightedTSNE<scalar_type> tSNE;
        hdi::dr::WeightedTSNE<scalar_type>::Parameters tSNE_params;

        tSNE.setLogger(&log);
        tSNE_params._seed = 1;
        tSNE.setTheta(0.7);
        tSNE.initializeWithJointProbabilityDistribution(probability,&embedding,tSNE_params);

        std::vector<uint32_t> flags(transposed_panel_data.numDataPoints(),0);

        hdi::viz::ScatterplotCanvas viewer;
        viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
        viewer.setSelectionColor(qRgb(50,50,50));
        viewer.resize(600,600);
        viewer.show();
        viewer.setWindowTitle("Mean");
        hdi::viz::ScatterplotDrawerScalarAttribute drawer;
        drawer.initialize(viewer.context());
        drawer.setData(embedding.getContainer().data(), panel_data.getDimProperty("selected_avg").data(), flags.data(), transposed_panel_data.numDataPoints());
        drawer.setAlpha(0.7);
        drawer.setPointSize(70);
        viewer.addDrawer(&drawer);
        drawer.setLimits(-0.1,50);

        hdi::viz::ScatterplotCanvas viewer_max;
        viewer_max.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
        viewer_max.setSelectionColor(qRgb(50,50,50));
        viewer_max.resize(600,600);
        viewer_max.show();
        viewer_max.setWindowTitle("Max");
        hdi::viz::ScatterplotDrawerScalarAttribute drawer_max;
        drawer_max.initialize(viewer_max.context());
        drawer_max.setData(embedding.getContainer().data(), panel_data.getDimProperty("selected_max").data(), flags.data(), transposed_panel_data.numDataPoints());
        drawer_max.setAlpha(0.7);
        drawer_max.setPointSize(70);
        viewer_max.addDrawer(&drawer_max);
        drawer_max.setLimits(-0.1,50);



        //GRADIENT DESCENT
        int iter = 0;
        while(true){
            multiscale_embedder.doAnIterateOnAllEmbedder();
            if(iter < 600){
                if(((iter+1)%100) == 0){
                    hdi::utils::secureLogValue(&log,"Iter",iter+1);
                }
                tSNE.doAnIteration();
            }

            {//limits
                std::vector<scalar_type> limits;
                embedding.computeEmbeddingBBox(limits,0.25);
                auto tr = QVector2D(limits[1],limits[3]);
                auto bl = QVector2D(limits[0],limits[2]);
                viewer.setTopRightCoordinates(tr);
                viewer.setBottomLeftCoordinates(bl);
                viewer_max.setTopRightCoordinates(tr);
                viewer_max.setBottomLeftCoordinates(bl);
            }

            viewer.updateGL();
            viewer_max.updateGL();
            QApplication::processEvents();
            ++iter;
        }

		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
