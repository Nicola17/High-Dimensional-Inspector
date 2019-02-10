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
#include "hdi/data/empty_data.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/image_view_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/utils/visual_utils.h"
#include <QDir>
#include "hdi/analytics/multiscale_embedder_system_qobj.h"
#include "hdi/visualization/multiple_heatmaps_view_qobj.h"
#include "hdi/visualization/pixel_view_qobj.h"
#include "hdi/utils/graph_algorithms.h"
#include "hdi/utils/math_utils.h"
#include "hdi/data/voxel_data.h"
#include "application.h"
#include "ui_application.h"
#include "QTableWidget"
#include "QInputDialog"
#include "hdi/data/text_data.h"
#include "hdi/visualization/anatomical_planes_view_qobj.h"

class MyInterfaceInitializer: public hdi::analytics::MultiscaleEmbedderSystem::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addUserDefinedDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setAlpha(0.5);
        drawer->setPointSize(10);

        auto id = embedder->getId();

        //auto emb_name = QInputDialog::getText(0,"New embedding","Embedding name");
        //auto name = QString("S%1:A%2:%3").arg(std::get<0>(id)).arg(std::get<1>(id)).arg(emb_name);

        _idxes->push_back(id);
        _embeddings_widget->addTab(embedder->getCanvas(),QString("S%1:A%2").arg(std::get<0>(id)).arg(std::get<1>(id)));
        _embeddings_widget->setCurrentIndex(_embeddings_widget->count()-1);
        embedder->getCanvas()->resize(512,512);
        embedder->onActivateInfluencedMode();
        embedder->doAnIteration();


        auto emb_name = QInputDialog::getText(0,"New embedding","Embedding name");
        auto name = QString("S%1:A%2:%3").arg(std::get<0>(id)).arg(std::get<1>(id)).arg(emb_name);
        //auto name = QString("pippo");
        _embeddings_widget->setTabText(_embeddings_widget->count()-1,name);

    }
    virtual void initializeInfluenceVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* influence){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());

        embedder->addAreaOfInfluenceDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), influence, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->updateLimitsFromData();
        drawer->setPointSize(25);
        drawer->setAlpha(0.8);
    }
    virtual void initializeSelectionVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data, scalar_type* selection){
        std::shared_ptr<hdi::viz::ScatterplotDrawerScalarAttribute> drawer(new hdi::viz::ScatterplotDrawerScalarAttribute());
        embedder->addSelectionDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), selection, embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setLimits(0,0.1);
        drawer->setPointSize(25);
        drawer->setAlpha(0.8);
    }

    virtual void updateSelection(embedder_type* embedder, scalar_type* selection){
    }

    virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){
        _view->setSelection(selection);
        _view->updateViewWithSelection();
    }

    virtual void updateSelectionWidget(){}

public:
    int _width,_height;
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;
    QTabWidget* _embeddings_widget;
    std::vector<std::tuple<unsigned int, unsigned int>>* _idxes;

    hdi::viz::AnatomicalPlanesView* _view;
};

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc != 2){
            hdi::utils::secureLog(&log,"Wrong number of parameters...");
			return 1;
		}

        hyperspectral_images_app application;

        application.ui->_embeddings_widget->removeTab(0);
        application.ui->_embeddings_widget->removeTab(0);
        application.ui->_view_widget->removeTab(0);
        application.ui->_view_widget->removeTab(0);

        int num_points(0);

        std::ifstream in_file(argv[1], std::ios::in);
        if(!in_file.is_open()){
            hdi::utils::secureLog(&log,"Cannot open the input file");
            return 1;
        }

        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
        application._panel_data = &panel_data;

        hdi::viz::AnatomicalPlanesView* voxel_view = new hdi::viz::AnatomicalPlanesView(&application);
        voxel_view->setPanelData(&panel_data);
        application.ui->_view_widget->addTab(voxel_view,QString("Selection"));

        std::string line;
        {
            std::getline(in_file,line);
            std::stringstream line_stream(line);
            std::string cell;
            int i_dim = 0;
            while(std::getline(line_stream,cell,',')){
                if(i_dim >= 3){
                    panel_data.addDimension(std::make_shared<hdi::data::TextData>(hdi::data::TextData(cell)));
                }
                ++i_dim;
            }
            panel_data.initialize();
        }


        {
            while(std::getline(in_file,line)){
                std::stringstream line_stream(line);
                std::string cell;
                std::vector<float> input_data(panel_data.numDimensions());
                int i_dim = 0;
                int x,y,z;
                while(std::getline(line_stream,cell,',')){
                    if(i_dim == 0) x = std::atof(cell.c_str());
                    if(i_dim == 1) y = std::atof(cell.c_str());
                    if(i_dim == 2) z = std::atof(cell.c_str());
                    if(i_dim >= 3){
                        input_data[i_dim-3] = std::atof(cell.c_str());
                    }
                    ++i_dim;
                }
                panel_data.addDataPoint(std::make_shared<hdi::data::VoxelData>(hdi::data::VoxelData(x,y,z)), input_data);
                ++num_points;
			}
		}

        voxel_view->updateLimits();
        voxel_view->setResMultiplier(2);
        voxel_view->updateView();

        //hdi::data::zScoreNormalization(panel_data);
        application._log = & log;
        application._multiscale_embedder = & multiscale_embedder;
        application.show();

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

        hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
        params._seed = 3;
        params._num_neighbors = 100;
        params._monte_carlo_sampling = true;
        params._num_walks_per_landmark = 10;
        params._transition_matrix_prune_thresh = 0;

        double scale = std::log10(num_points/10);
        hdi::utils::secureLogValue(&log,"Scale",scale);

        MyInterfaceInitializer interface_initializer;
        interface_initializer._idxes = &application._idxes;
        interface_initializer._embeddings_widget = application.ui->_embeddings_widget;
        interface_initializer._panel_data = &panel_data;
        interface_initializer._view = voxel_view;

        multiscale_embedder.setInterfaceInitializer(&interface_initializer);
        multiscale_embedder.initialize(scale,params);
        multiscale_embedder.createTopLevelEmbedder();

        application.show();
        application.initTree();

        while(true){
            multiscale_embedder.doAnIterateOnAllEmbedder();
            QApplication::processEvents();
        }



        return app.exec();
	}
	catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
	catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
	catch(...){ std::cout << "An unknown error occurred";}
}
