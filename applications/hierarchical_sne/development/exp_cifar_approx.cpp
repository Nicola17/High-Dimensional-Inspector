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

class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
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
        if(argc != 5){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}
        std::string analysis_name(argv[4]);
        QDir().mkdir(argv[4]);

        std::map<unsigned int, QColor> palette;
        palette[0] = qRgb(16,78,139);
        palette[1] = qRgb(139,90,43);
        palette[2] = qRgb(138,43,226);
        palette[3] = qRgb(0,128,0);
        palette[4] = qRgb(255,150,0);
        palette[5] = qRgb(204,40,40);
        palette[6] = qRgb(238,180,180);
        palette[7] = qRgb(0,205,0);
        palette[8] = qRgb(20,20,20);
        palette[9] = qRgb(0, 150, 255);

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder_exact;
        multiscale_embedder.setLogger(&log);

        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data_exact = multiscale_embedder_exact.getPanelData();

        std::vector<unsigned int> labels;
        hdi::utils::IO::loadCifar10(panel_data,labels,argv[1],argv[2],std::atoi(argv[3]));
        hdi::utils::IO::loadCifar10(panel_data_exact,labels,argv[1],argv[2],std::atoi(argv[3]));



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
        params._seed = 2;
        params._num_neighbors = 90;
        params._num_walks_per_landmark = 100;
        params._aknn_num_trees = 4;
        params._aknn_num_checks = 5024;

        MyInterfaceInitializer interface_initializer;
        interface_initializer._palette = &palette;
        interface_initializer._labels = &labels;

        multiscale_embedder.setInterfaceInitializer(&interface_initializer);
        multiscale_embedder.initialize(3,params);
        multiscale_embedder.createTopLevelEmbedder();

        std::ofstream latex_chart_stream(QString("%1/pr.tex").arg(analysis_name.c_str()).toStdString().c_str());

        int iter = 0;
        while(iter < 5000){
            if(((iter+1)%50) == 0){
                hdi::utils::secureLogValue(&log,"Iter",iter+1);
            }
            multiscale_embedder.doAnIterateOnAllEmbedder();
            QApplication::processEvents();
            ++iter;
        }
        hdi::utils::secureLog(&log,"Stop computation");

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        auto& hsne = multiscale_embedder.hSNE();
        auto& analysis = multiscale_embedder.analysis();

        bool valid = true;
        for(int i = 0; i < analysis.size(); ++i){
            if(analysis[i].size() != 1){
                valid = false;
            }
        }
        multiscale_embedder.saveImagesToFile(QString("%1/hSNE").arg(QString::fromStdString(analysis_name)).toStdString());

        if(valid){
            hdi::utils::secureLog(&log,"Performing precision/recall analysis");

            {
                const auto& idxes = analysis[0][0]._scale_idxes;
                std::vector<unsigned int> pnts_to_evaluate(analysis[0][0]._embedder->getEmbedding().numDataPoints());
                std::iota(pnts_to_evaluate.begin(),pnts_to_evaluate.end(),0);
                {
                    latex_chart_stream << "\\addplot[blue,dashed]coordinates{" << std::endl;
                    //lowest level
                    std::vector<scalar_type> precision;
                    std::vector<scalar_type> recall;

                    const hdi::data::Embedding<scalar_type>& embedding = analysis[0][0]._embedder->getEmbedding();
                    std::vector<unsigned int> pnts_to_evaluate(embedding.numDataPoints());
                    std::iota(pnts_to_evaluate.begin(),pnts_to_evaluate.end(),0);
                    hdi::dr::computePrecisionRecall(panel_data,embedding,pnts_to_evaluate,idxes,precision,recall,30);

                    std::cout << std::endl << std::endl;
                    for(int i = 0; i < precision.size(); ++i){
                        std::cout << precision[i] << "\t" << recall[i] << std::endl;
                        latex_chart_stream << "(" << precision[i] << "," << recall[i] << ")" << std::endl;
                    }
                    latex_chart_stream << "};\\label{plots:HSNE0}" << std::endl;
                }
                for(int l = 1; l < analysis.size(); ++l){
                    latex_chart_stream << "\\addplot[blue]coordinates{" << std::endl;
                    //lowest level
                    std::vector<scalar_type> precision;
                    std::vector<scalar_type> recall;

                    hdi::dr::HierarchicalSNE<scalar_type>::sparse_scalar_matrix_type weights;
                    const hdi::data::Embedding<scalar_type>& embedding_lnd = analysis[l][0]._embedder->getEmbedding();
                    hsne.getInterpolationWeights(idxes,weights,l);
                    hdi::utils::removeEdgesToUnselectedVertices(weights,analysis[l][0]._scale_idxes);

                    hdi::data::Embedding<scalar_type> embedding;
                    hdi::data::interpolateEmbeddingPositions(embedding_lnd,embedding,weights);
                    hdi::dr::computePrecisionRecall(panel_data,embedding,pnts_to_evaluate,idxes,precision,recall,30);


                    std::cout << std::endl << std::endl << l << std::endl << std::endl;
                    for(int i = 0; i < precision.size(); ++i){
                        std::cout << precision[i] << "\t" << recall[i] << std::endl;
                        latex_chart_stream << "(" << precision[i] << "," << recall[i] << ")" << std::endl;
                    }
                    latex_chart_stream << "};\\label{plots:HSNE"<<l<<"}" << std::endl;
                }
            }
        }



		return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
