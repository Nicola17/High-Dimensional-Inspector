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
#include "hdi/data/pixel_data.h"
#include "hyperspectral_images_app.h"
#include "ui_hyperspectral_images_app.h"
#include "QTableWidget"
#include "QInputDialog"

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
        typedef hdi::analytics::MultiscaleEmbedderSystem::panel_data_type panel_data_type;
        const auto& data = _panel_data->getDataPoints();

        _selection_image = QImage(_width,_height,QImage::Format_ARGB32);
        for(int i = 0; i < _width; ++i)
            for(int j = 0; j < _height; ++j)
                _selection_image.setPixel(i,j,qRgb(50,0,0));


        for(int i = 0; i < selection.size(); ++i){
            auto data_ptr = dynamic_cast<hdi::data::PixelData*>(data[i].get());
            if(data_ptr == nullptr){
                continue;
            }
            if(data_ptr->_height != _selection_image.height() || data_ptr->_width != _selection_image.width()){
                continue;
            }
            scalar_type sv(selection[i]);
            _selection_image.setPixel(data_ptr->_u,data_ptr->_v,qRgb(sv*255,sv*255,sv*255));
            //_selection_image.setPixel(0,0,qRgb(sv*255,sv*255,sv*255));
        }
        updateSelectionWidget();
    }

    virtual void updateSelectionWidget(){
        _image_label->setPixmap(QPixmap::fromImage(_selection_image.scaledToHeight(512,Qt::SmoothTransformation)));
        _image_label->show();
    }

public:
    int _width,_height;
    QImage _selection_image;
    QLabel* _image_label;
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;
    QTabWidget* _embeddings_widget;
    std::vector<std::tuple<unsigned int, unsigned int>>* _idxes;
};

int main(int argc, char *argv[]){
	try{
		typedef float scalar_type;
		QApplication app(argc, argv);

		hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
        if(argc < 4){
			hdi::utils::secureLog(&log,"Not enough input parameters...");
			return 1;
		}

        hyperspectral_images_app application;

        application.ui->_images_widget->removeTab(0);
        application.ui->_images_widget->removeTab(0);
        application.ui->_embeddings_widget->removeTab(0);
        application.ui->_embeddings_widget->removeTab(0);

        const int num_dimensions(argc-5);
        const int img_width(std::atoi(argv[1]));
        const int img_height(std::atoi(argv[2]));
        int num_points(num_dimensions*img_height*img_width);

        QLabel* new_label = new QLabel(&application);
        new_label->setPixmap(QPixmap::fromImage(QImage(argv[3]).scaledToHeight(512,Qt::SmoothTransformation)));
        application.ui->_images_widget->addTab(new_label,QString("Ref"));
{
        QLabel* new_label = new QLabel(&application);
        new_label->setPixmap(QPixmap::fromImage(QImage(argv[4]).scaledToHeight(512,Qt::SmoothTransformation)));
        application.ui->_images_widget->addTab(new_label,QString("Equal"));
}
        std::vector<QImage> images(num_dimensions);
        for(int i = 0; i < num_dimensions; ++i){
            hdi::utils::secureLogValue(&log,"Image",i);
            images[i] = QImage(argv[5+i]);
            hdi::utils::secureLogValue(&log,"\twidth",images[i].width());
            hdi::utils::secureLogValue(&log,"\theight",images[i].height());
            if(images[i].width() != img_width || images[i].width() != img_height){
                images[i] = images[i].scaled(img_width,img_height);
            }
            hdi::utils::secureLogValue(&log,"\twidth",images[i].width());
            hdi::utils::secureLogValue(&log,"\theight",images[i].height());

            QLabel* new_label = new QLabel(&application);
            new_label->setPixmap(QPixmap::fromImage(images[i].scaledToHeight(512,Qt::SmoothTransformation)));
            application.ui->_images_widget->addTab(new_label,QString("HI%1").arg(i));
        }

        //return 1;
        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
        application._panel_data = &panel_data;
        application._width = img_width;
        application._height = img_height;

		{//initializing panel data
            for(int i = 0; i < num_dimensions; ++i){
                panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
            }
			panel_data.initialize();
		}

        {
            for(int j = 0; j < img_height; ++j){
                for(int i = 0; i < img_width; ++i){
                    std::vector<float> input_data(num_dimensions);
                    for(int d = 0; d < num_dimensions; ++d){
                        auto color = images[d].pixel(i,j);
                        input_data[d] = qGray(color);
                        if(input_data[d]!=0){
                         //   input_data[d] += (rand()%1000)/1000.*0.5;
                        }
                    }
                    bool ok_flag = false;
                    for(int f = 1; f < input_data.size(); ++f){
                        if(input_data[f] != input_data[f-1]){
                            ok_flag = true;
                        }
                    }
                    if(!ok_flag)
                        continue;
                    panel_data.addDataPoint(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(i,j,img_width,img_height)), input_data);
                }
			}
		}

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
        //params._aknn_num_trees = 4;
        //params._aknn_num_checks = 128;
        params._monte_carlo_sampling = true;
        params._num_walks_per_landmark = 10;

        double scale = std::log10(num_points/100);
        hdi::utils::secureLogValue(&log,"Scale",scale);

        MyInterfaceInitializer interface_initializer;
        interface_initializer._idxes = &application._idxes;
        interface_initializer._image_label = application.ui->_aoi_label;
        interface_initializer._embeddings_widget = application.ui->_embeddings_widget;
        {
            QImage temp(512,512,QImage::Format_ARGB32);
            for(int i = 0; i < 512; ++i)
                for(int j = 0; j < 512; ++j)
                    temp.setPixel(i,j,qRgb(0,0,0));
            interface_initializer._selection_image = temp;
        }
        interface_initializer.updateSelectionWidget();
        interface_initializer._panel_data = &panel_data;
        interface_initializer._width = img_width;
        interface_initializer._height = img_height;

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
