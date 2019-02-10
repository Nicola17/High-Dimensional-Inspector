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
#include "gene_analysis_app.h"
#include "ui_gene_analysis_app.h"
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
        _table->setRowCount(0);

        double thresh  = 0.01;

        std::vector<std::pair<double,int>> sel;
        for(int i = 0; i < selection.size(); ++i){
            if(selection[i] > thresh){
                sel.push_back(std::pair<double,int>(selection[i],i));
            }
        }
        std::sort(sel.begin(),sel.end());

        _table->setRowCount(sel.size());
        for(int i = 0; i < sel.size() ; ++i){
            _table->setItem(sel.size()-i-1, 0, new QTableWidgetItem(QString("%1").arg(std::get<0>(sel[i])*100)));
            _table->setItem(sel.size()-i-1, 1, new QTableWidgetItem(QString::fromStdString((*_gene_names)[std::get<1>(sel[i])])));
        }


    }

    virtual void updateSelectionWidget(){
    }

public:
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;
    QTabWidget* _embeddings_widget;
    std::vector<std::tuple<unsigned int, unsigned int>>* _idxes;
    std::vector<std::string>* _gene_names;
    QTableWidget* _table;
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
            hdi::utils::secureLog(&log,"Wrong number of parameters...");
			return 1;
		}

        gene_analysis_app application;
        application.ui->_embeddings_widget->removeTab(0);
        application.ui->_embeddings_widget->removeTab(0);

        application.ui->_table_widget->setItem(0,0,new QTableWidgetItem("pippo"));
        application.ui->_table_widget->setRowCount(10);
        application.ui->_table_widget->setColumnCount(2);

        //m_TableHeader<<"#"<<"Name"<<"Text";
        //application.ui->_table_widget->setHorizontalHeaderLabels(m_TableHeader);
        //application.ui->_table_widget->setItem(0, 1, new QTableWidgetItem("Hello"));
        application.ui->_table_widget->verticalHeader()->setVisible(false);

        const int num_dimensions(std::atoi(argv[1]));
        const int num_points(std::atoi(argv[2]));
        std::ifstream data_file(argv[3], std::ios::in);
        std::ifstream gene_file(argv[4], std::ios::in);

        std::vector<std::string> genes;
        // ... reading gene descriptions
        {
            std::string line;
            std::getline(gene_file,line);
            for(int i = 0; i < num_points; ++i){
                std::getline(gene_file,line);
                genes.push_back(line);
            }
        }
        // ... initalizing panel data
        hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
        multiscale_embedder.setLogger(&log);
        hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
        application._panel_data = &panel_data;
        {
            for(int i = 0; i < num_dimensions; ++i){
                panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
            }
            panel_data.initialize();
        }

        // ... reading data
        {

            std::string line;
            for(int i = 0; i < num_points; ++i){
                std::getline(data_file,line);
                std::stringstream line_stream(line);
                std::string cell;
                std::vector<float> input_data(panel_data.numDimensions());
                int i_dim = 0;
                int x,y,z;
                while(std::getline(line_stream,cell,',')){
                    input_data[i_dim] = std::atof(cell.c_str())*std::atof(cell.c_str());
                    ++i_dim;
                }
                panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()), input_data);
            }
        }

        hdi::data::zScoreNormalization(panel_data);
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
        params._num_walks_per_landmark = 100;
        params._mcmcs_num_walks = 100;
        params._mcmcs_landmark_thresh = 1.5;  //1.5

        double scale = std::log10(num_points/10);
        hdi::utils::secureLogValue(&log,"Scale",scale);

        MyInterfaceInitializer interface_initializer;
        interface_initializer._idxes = &application._idxes;
        interface_initializer._embeddings_widget = application.ui->_embeddings_widget;
        interface_initializer.updateSelectionWidget();
        interface_initializer._panel_data = &panel_data;
        interface_initializer._table = application.ui->_table_widget;
        interface_initializer._gene_names = &genes;

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
