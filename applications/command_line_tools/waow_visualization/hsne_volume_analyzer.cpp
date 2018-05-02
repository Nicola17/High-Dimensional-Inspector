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
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
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
#include "hdi/visualization/scalable_pcp_view_qobj.h"
#include "hdi/utils/dataset_utils.h"


class MyInterfaceInitializer: public hdi::analytics::AbstractInterfaceInitializer{
public:
  MyInterfaceInitializer(){
    _scale_colors.push_back(qRgb(228,26,28));
    _scale_colors.push_back(qRgb(55,126,184));
    _scale_colors.push_back(qRgb(77,175,74));
    _scale_colors.push_back(qRgb(152,75,163));
    _scale_colors.push_back(qRgb(255,127,0));
    _scale_colors.push_back(qRgb(255,255,51));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
    _scale_colors.push_back(qRgb(0,0,0));
  }
  virtual ~MyInterfaceInitializer(){}
  virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){

    connect(embedder,&embedder_type::sgnSelectionPtr,this,&MyInterfaceInitializer::onUpdatePCP);

    _analysis_colors.push_back(std::vector<scalar_type>(idxes_to_orig_data.size()*3));
    auto& colors = _analysis_colors[_analysis_colors.size()-1];
    for(int i = 0; i < idxes_to_orig_data.size(); ++i){
      colors[i*3+0] = _colors[idxes_to_orig_data[i]*3+0];
      colors[i*3+1] = _colors[idxes_to_orig_data[i]*3+1];
      colors[i*3+2] = _colors[idxes_to_orig_data[i]*3+2];
    }

    std::shared_ptr<hdi::viz::ScatterplotDrawerUsedDefinedColors> drawer(new hdi::viz::ScatterplotDrawerUsedDefinedColors());
    embedder->addUserDefinedDrawer(drawer);
    drawer->setData(embedder->getEmbedding().getContainer().data(), colors.data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
    drawer->setAlpha(0.25);
    drawer->setPointSize(10);

    auto id = embedder->getId();

    _idxes->push_back(id);
    _embeddings_widget->addTab(embedder->getCanvas(),QString("S%1:A%2").arg(std::get<0>(id)).arg(std::get<1>(id)));
    _embeddings_widget->setCurrentIndex(_embeddings_widget->count()-1);
    embedder->getCanvas()->resize(512,512);
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
    auto id = embedder->getId();
    auto scale_id = std::get<0>(id);

    for(int i = scale_id; i <_hsne->hierarchy().size(); ++i){
      std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
      std::shared_ptr<std::vector<uint32_t>> flags_ptr(new std::vector<uint32_t>(embedder->getPanelData().numDataPoints(),4));

      for(int k = 0; k < idxes_to_orig_data.size(); ++k){
        for(int l = 0; l < _hsne->scale(i)._landmark_to_original_data_idx.size(); ++l){
          if(_hsne->scale(i)._landmark_to_original_data_idx[l] == idxes_to_orig_data[k]){
            (*flags_ptr)[k] = 0;
            break;
          }
        }
      }

      _flags.push_back(flags_ptr);
      embedder->addSelectionDrawer(drawer);
      drawer->setData(embedder->getEmbedding().getContainer().data(), flags_ptr.get()->data(), embedder->getPanelData().numDataPoints());
      int point_size = (i+1)*5;
      drawer->setPointSize(point_size);
      drawer->setAlpha(0.5);
      drawer->setColor(_scale_colors[_hsne->hierarchy().size()-i-1]);
    }

  }

  virtual void updateSelection(embedder_type* embedder, scalar_type* selection){
  }

  virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){
    _view->setSelection(selection);
    _view->updateViewWithSelection();
  }

  virtual void updateSelectionWidget(){}

public slots:

  void onUpdatePCP(hdi::analytics::MultiscaleEmbedderSingleView* ptr){
    _pcp_view->resetAndDraw();
    const auto& panel_data = ptr->getPanelData();

    if(panel_data.numDimensions() > 50){
      return;
    }

    hdi::analytics::MultiscaleEmbedderSingleView::id_type id = ptr->getId();
    const auto& analysis =_embedder_system->analysis()[std::get<0>(id)][std::get<1>(id)];

    int num_selected = 0;
    const auto& flags = panel_data.getFlagsDataPoints();
    for(int i = 0; i < panel_data.numDataPoints(); ++i){
      if((flags[i]&hdi::data::PanelData<scalar_type>::Selected) == hdi::data::PanelData<scalar_type>::Selected){
        std::vector<scalar_type> pnt;
        for(int d = 0; d < panel_data.numDimensions(); ++d){
          int orig_id = _hsne->scale(std::get<0>(id))._landmark_to_original_data_idx[analysis._scale_idxes[i]];
          pnt.push_back(_panel_data->dataAt(orig_id,d));
        }
        _pcp_view->addDataPointAndDraw(pnt);
        ++num_selected;
      }
    }
    std::cout << "num selected: " << num_selected << std::endl;
  }

public:
  int _width,_height;
  hdi::analytics::MultiscaleEmbedderSystem::panel_data_type* _panel_data;
  QTabWidget* _embeddings_widget;
  std::vector<std::tuple<unsigned int, unsigned int>>* _idxes;
  std::vector<scalar_type> _colors;
  std::vector<std::vector<scalar_type>> _analysis_colors;
  std::vector<QRgb> _scale_colors;


  std::vector<std::shared_ptr<std::vector<uint32_t>>> _flags;

  const hdi::analytics::MultiscaleEmbedderSystem* _embedder_system;
  const hdi::analytics::MultiscaleEmbedderSystem::hsne_type* _hsne;

  hdi::viz::AnatomicalPlanesView* _view;
  hdi::viz::ScalablePCPView* _pcp_view;
};

int main(int argc, char *argv[]){
  try{
    typedef float scalar_type;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("HSNE Volume Analyzer");
    QCoreApplication::setApplicationVersion("0.3");

    hdi::utils::CoutLog log;

    QCommandLineParser parser;
    parser.setApplicationDescription("Hierarchical-SNE for the analysis of volumetric high-dimensional data.\nNOTE: options marked with [I] are ignored when a precomputed hierarchy is provided!");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("data", QCoreApplication::translate("main", ".hdvol file."));

    QCommandLineOption hierarchy_option(QStringList() << "p" << "precomputed_hierarchy",
        QCoreApplication::translate("main", "Use a precomputed hierarchy (.hsne file)."),
        QCoreApplication::translate("main", "precomputed_hierarchy"));
    parser.addOption(hierarchy_option);

    QCommandLineOption feature_subset_option(QStringList() << "f" << "feature_subset",
        QCoreApplication::translate("main", "Use a subset of available features (.txt file)."),
        QCoreApplication::translate("main", "feature_subset"));
    parser.addOption(feature_subset_option);

    QCommandLineOption color_option(QStringList() << "c" << "colors",
        QCoreApplication::translate("main", "Add colors to data points (.txt file)."),
        QCoreApplication::translate("main", "colors"));
    parser.addOption(color_option);

    QCommandLineOption res_mult_option(QStringList() << "r" << "resolution_multiplier",
        QCoreApplication::translate("main", "Initial resolution multiplier for images (default: 1)."),
        QCoreApplication::translate("main", "resolution_multiplier"));
    parser.addOption(res_mult_option);

    QCommandLineOption beta_option(QStringList() << "b" << "beta",
        QCoreApplication::translate("main", "[I] Beta treshold for landmark selection (default: 1.5)."),
        QCoreApplication::translate("main", "beta"));
    parser.addOption(beta_option);

    QCommandLineOption neigh_option(QStringList() << "n" << "num_neighbors",
        QCoreApplication::translate("main", "[I] Number of neighbors considered in the first level, aka perplexity*3 (default: 100)."),
        QCoreApplication::translate("main", "num_neighbors"));
    parser.addOption(neigh_option);

    QCommandLineOption trees_option(QStringList() << "t" << "num_trees",
        QCoreApplication::translate("main", "[I] Number of trees used for the AKNN computation (default: 4)."),
        QCoreApplication::translate("main", "num_trees"));
    parser.addOption(trees_option);

    QCommandLineOption checks_option(QStringList() << "c" << "num_checks",
        QCoreApplication::translate("main", "[I] Number of checks used for the AKNN computation (default: 1024)."),
        QCoreApplication::translate("main", "num_checks"));
    parser.addOption(checks_option);

    QCommandLineOption prune_option(QStringList() << "u" << "prune",
        QCoreApplication::translate("main", "[I] Pruning treshold for FMC computation, aka saves memory if greater than 0 (default: 0, be careful in increasing it, prbly max 5)."),
        QCoreApplication::translate("main", "prune"));
    parser.addOption(prune_option);

    QCommandLineOption skip_normalization_option(QStringList() << "s" << "skip_normalization",
                     QCoreApplication::translate("main", "Skip the Min-Max normalization"),
                     QCoreApplication::translate("main", "skip_normalization"));
    parser.addOption(skip_normalization_option);

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
    if(args.size() != 1){
      hdi::utils::secureLog(&log,"Wrong number of parameters! Use -h to see the options");
      return 1;
    }

    hyperspectral_images_app application;

    application.ui->_embeddings_widget->removeTab(0);
    application.ui->_embeddings_widget->removeTab(0);
    application.ui->_view_widget->removeTab(0);
    application.ui->_view_widget->removeTab(0);

    std::string feature_subset_filename = "";
    if(parser.isSet(feature_subset_option)){
      feature_subset_filename = parser.value(feature_subset_option).toStdString();
    }

    hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
    multiscale_embedder.setLogger(&log);
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
    application._panel_data = &panel_data;

    hdi::viz::AnatomicalPlanesView* voxel_view = new hdi::viz::AnatomicalPlanesView(&application);
    application._view = voxel_view;
    voxel_view->setPanelData(&panel_data);
    application.ui->_view_widget->addTab(voxel_view,QString("Selection"));

    std::vector<int> limits;
    hdi::utils::IO::loadHDVOL(panel_data,limits,args[0].toStdString(),feature_subset_filename,&log);
    if(limits[4] == limits[5]){
      application._view->setSingleImageMode(true);
      application._view->_ui._z_SpBx->blockSignals(true);
      application._view->_ui._z_SpBx->setValue(limits[5]);
      application._view->_ui._z_SpBx->blockSignals(false);
    }

    voxel_view->updateLimits();
    voxel_view->setResMultiplier(1);
    if(parser.isSet(res_mult_option)){
      voxel_view->setResMultiplier(std::atof(parser.value(res_mult_option).toStdString().c_str()));
    }
    voxel_view->updateView();

    application._log = & log;
    application._multiscale_embedder = & multiscale_embedder;
    application.show();

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

    hdi::viz::ScalablePCPView pcp_view;
    pcp_view.show();

    QCoreApplication::processEvents();

    pcp_view.setMinY(0);
    pcp_view.setMaxY(1);
    pcp_view.setNumDims(panel_data.numDimensions());
    pcp_view.setSorting(hdi::viz::ScalablePCPView::sorting_type::NO_SORTING);
    pcp_view.resize(QSize(600,200));
    pcp_view.onUpdate();
    pcp_view.resize(QSize(600,200));
    QCoreApplication::processEvents();
    pcp_view.resize(QSize(600,200));


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

    hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
    params._seed = 3;
    params._num_neighbors = 100;
    params._aknn_num_trees = 4;
    params._aknn_num_checks = 1024;
    params._monte_carlo_sampling = true;
    params._mcmcs_landmark_thresh = 1.5;
    params._num_walks_per_landmark = 10;
    params._out_of_core_computation = true;
    params._transition_matrix_prune_thresh = 0;
    //params.

    if(parser.isSet(beta_option)){
      params._mcmcs_landmark_thresh = std::atof(parser.value(beta_option).toStdString().c_str());
    }
    if(parser.isSet(neigh_option)){
      params._num_neighbors = std::atoi(parser.value(neigh_option).toStdString().c_str());
    }
    if(parser.isSet(trees_option)){
      params._aknn_num_trees = std::atoi(parser.value(trees_option).toStdString().c_str());
    }
    if(parser.isSet(checks_option)){
      params._aknn_num_checks = std::atoi(parser.value(checks_option).toStdString().c_str());
    }
    if(parser.isSet(prune_option)){
      params._transition_matrix_prune_thresh = std::atoi(parser.value(prune_option).toStdString().c_str());
    }
    if(!parser.isSet(skip_normalization_option)){
      hdi::utils::secureLog(&log,"Min-Max normalization of the data...");
      hdi::data::minMaxNormalization(panel_data);
    }


    MyInterfaceInitializer interface_initializer;
    interface_initializer._idxes = &application._idxes;
    interface_initializer._embeddings_widget = application.ui->_embeddings_widget;
    interface_initializer._panel_data = &panel_data;
    interface_initializer._view = voxel_view;
    interface_initializer._colors.resize(panel_data.numDataPoints()*3,0);
    interface_initializer._hsne = &(multiscale_embedder.hSNE());
    interface_initializer._pcp_view = &pcp_view;
    interface_initializer._embedder_system = &multiscale_embedder;

    if(parser.isSet(color_option)){
      auto filename = parser.value(color_option).toStdString();
      std::ifstream color_file(filename.c_str());
      std::string line;
      int num_elem = 0;
      while(std::getline(color_file,line)){
        std::stringstream      lineStream(line);
        std::string        cell;
        while(std::getline(lineStream,cell, ',')){
          if(num_elem < interface_initializer._colors.size()){
            interface_initializer._colors[num_elem] = std::atof(cell.c_str())/255.;
          }
          ++num_elem;
        }
      }
    }


    multiscale_embedder.setInterfaceInitializer(&interface_initializer);
    if(!parser.isSet(hierarchy_option)){
      double scale = std::log10(panel_data.numDataPoints()/10);
      hdi::utils::secureLogValue(&log,"Scale",scale);
      multiscale_embedder.initialize(scale,params);
    }else{
      auto filename = parser.value(hierarchy_option).toStdString();
      hdi::utils::secureLogValue(&log,".hsne file provided",filename);
      multiscale_embedder.initializeFromFile(filename);
    }

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
