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

float readFloatBigEndian(std::ifstream& stream){
    char buff[4];
    char right_buff[4];
    stream.read(buff,4);
    right_buff[0] = buff[3];
    right_buff[1] = buff[2];
    right_buff[2] = buff[1];
    right_buff[3] = buff[0];
    float val = *((float*)right_buff);
    return val;
}

template <typename scalar_type>
void loadData(std::string filename, hdi::data::PanelData<scalar_type>& panel_data){
    std::ifstream file(filename, std::ios::in|std::ios::binary);
    if (!file.is_open()){
        throw std::runtime_error("data file cannot be found");
    }
    const int width = 144;
    const int height = 616;
    const int depth = 25;
    std::vector<std::vector<std::vector<float>>> input_data(11);
    file.seekg(4,std::ios_base::cur);

    {
        int x = 87, y = 388, z = 10;
        {//Phosphate
            input_data[0].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[0][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[0][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[0][z][y*width+x] << std::endl;
            }
        }
        {//Nitrate
            input_data[1].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[1][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[1][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[1][z][y*width+x] << std::endl;
            }
        }
        {//Ammonia
            input_data[2].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[2][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[2][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[2][z][y*width+x] << std::endl;
            }
        }
        {//Silicate
            input_data[3].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[3][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[3][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[3][z][y*width+x] << std::endl;
            }
        }

        {//skip 11

            for(int to_skip = 0; to_skip < 11; ++ to_skip){
                for(int k = 0; k < depth; ++k){
                    for(int j = 0; j < height; ++j){
                        for(int i = 0; i < width; ++i){
                            float val = readFloatBigEndian(file);
                        }
                    }
                    file.seekg(8,std::ios_base::cur);
                }
            }
        }
        {//Chlorophyll
            input_data[4].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[4][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[4][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):((val<0.001)?(0.001):(val));
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[4][z][y*width+x] << std::endl;
            }
        }
        {//Primary production
            input_data[5].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[5][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[5][k][j*width+i] = (val<-10)?(std::numeric_limits<float>::min()):((val<0.001)?(0.001):(val));
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[5][z][y*width+x] << std::endl;
            }
        }
        {//skip 3

            for(int to_skip = 0; to_skip < 3; ++ to_skip){
                for(int k = 0; k < depth; ++k){
                    for(int j = 0; j < height; ++j){
                        for(int i = 0; i < width; ++i){
                            float val = readFloatBigEndian(file);
                        }
                    }
                    file.seekg(8,std::ios_base::cur);
                }
            }
        }
        {//skip 6 2D

            for(int to_skip = 0; to_skip < 6; ++ to_skip){
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
        }
        {//u velocity
            input_data[6].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[6][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[6][k][j*width+i] = (val<-1000000.)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[6][z][y*width+x] << std::endl;
            }
        }
        {//v velocity
            input_data[7].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[7][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[7][k][j*width+i] = (val<-1000000.)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[7][z][y*width+x] << std::endl;
            }
        }
        {//temperature
            input_data[8].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[8][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[8][k][j*width+i] = (val<-1000000.)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[8][z][y*width+x] << std::endl;
            }
        }
        {//salinity
            input_data[9].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[9][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[9][k][j*width+i] = (val<-1000000.)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[9][z][y*width+x] << std::endl;
            }
        }
        {//vertical diffusivity
            input_data[10].resize(depth);
            for(int k = 0; k < depth; ++k){
                input_data[10][k].resize(width*height,0);
                for(int j = 0; j < height; ++j){
                    for(int i = 0; i < width; ++i){
                        float val = readFloatBigEndian(file);
                        input_data[10][k][j*width+i] = (val<-1000000.)?(std::numeric_limits<float>::min()):(val);
                    }
                }
                file.seekg(8,std::ios_base::cur);
            }
            //test
            {
                std::cout << input_data[10][z][y*width+x] << std::endl;
            }
        }
    }

    for(int i = 0; i < 11; ++i){
        panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
    }
    panel_data.initialize();
    for(int k = 0; k < depth; ++k){
        for(int j = 0; j < height; ++j){
            for(int i = 0; i < width; ++i){
                bool valid = true;
                std::vector<scalar_type> input(panel_data.numDimensions());
                for(int d = 0; d < panel_data.numDimensions(); ++d){
                    input[d] = input_data[d][k][j*width+i];
                    if(input[d] == std::numeric_limits<float>::min()){
                        valid = false;
                    }
                }
                if(valid){
                    panel_data.addDataPoint(std::make_shared<hdi::data::VoxelData>(hdi::data::VoxelData(i,j,k)), input);
                }
            }
        }
    }

    hdi::data::zScoreNormalization(panel_data);
}


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


        loadData(argv[1],panel_data);
        int num_points(panel_data.numDataPoints());

        voxel_view->updateLimits();
        voxel_view->setResMultiplier(1);
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
