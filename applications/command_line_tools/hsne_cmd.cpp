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

#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QIcon>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "hdi/data/embedding.h"
#include "hdi/data/panel_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include "hdi/analytics/multiscale_embedder_system_qobj.h"




class MyInterfaceInitializer: public hdi::analytics::AbstractInterfaceInitializer{
public:
    virtual ~MyInterfaceInitializer(){}
    virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
        std::shared_ptr<hdi::viz::ScatterplotDrawerFixedColor> drawer(new hdi::viz::ScatterplotDrawerFixedColor());
        embedder->addUserDefinedDrawer(drawer);
        drawer->setData(embedder->getEmbedding().getContainer().data(), embedder->getPanelData().getFlagsDataPoints().data(), embedder->getPanelData().numDataPoints());
        drawer->setAlpha(0.5);
        drawer->setPointSize(10);
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

    virtual void dataPointSelectionChanged(const std::vector<scalar_type>& selection){}

public:
};

int main(int argc, char *argv[])
{
  try{
    hdi::utils::CoutLog log;
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/hdi16.png");
    icon.addFile(":/hdi32.png");
    icon.addFile(":/hdi64.png");
    icon.addFile(":/hdi128.png");
    icon.addFile(":/hdi256.png");
    app.setWindowIcon(icon);

    QApplication::setApplicationName("HSNE");
    QApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Command line version of the HSNE algorithm");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("data", QCoreApplication::translate("main", "High dimensional data."));
    parser.addPositionalArgument("num_data_points", QCoreApplication::translate("main", "Num of data-points."));
    parser.addPositionalArgument("num_dimensions", QCoreApplication::translate("main", "Num of dimensions."));

    QCommandLineOption beta_option(QStringList() << "b" << "beta",
            QCoreApplication::translate("main", "Beta treshold for landmark selection (default: 1.5)."),
            QCoreApplication::translate("main", "beta"));
    parser.addOption(beta_option);

    QCommandLineOption neigh_option(QStringList() << "n" << "num_neighbors",
            QCoreApplication::translate("main", "Number of neighbors considered in the first level, aka perplexity*3 (default: 30)."),
            QCoreApplication::translate("main", "num_neighbors"));
    parser.addOption(neigh_option);

    QCommandLineOption trees_option(QStringList() << "t" << "num_trees",
            QCoreApplication::translate("main", "Number of trees used for the AKNN computation (default: 4)."),
            QCoreApplication::translate("main", "num_trees"));
    parser.addOption(trees_option);

    QCommandLineOption checks_option(QStringList() << "c" << "num_checks",
            QCoreApplication::translate("main", "Number of checks used for the AKNN computation (default: 1024)."),
            QCoreApplication::translate("main", "num_checks"));
    parser.addOption(checks_option);

    QCommandLineOption prune_option(QStringList() << "u" << "prune",
            QCoreApplication::translate("main", "Pruning treshold for FMC computation, aka saves memory if greater than 0 (default: 1.5, be careful in increasing it, prbly max 5)."),
            QCoreApplication::translate("main", "prune"));
    parser.addOption(prune_option);

    QCommandLineOption walks_landmark_option(QStringList() << "wl" << "num_walks_selection",
            QCoreApplication::translate("main", "Number of walks used for the selection of landmarks (default: 200)."),
            QCoreApplication::translate("main", "num_walks_selection"));
    parser.addOption(walks_landmark_option);

    QCommandLineOption walks_similarities_option(QStringList() << "ws" << "num_walks_similarities",
            QCoreApplication::translate("main", "Number of walks used for the selection of landmarks (default: 200)."),
            QCoreApplication::translate("main", "num_walks_similarities"));
    parser.addOption(walks_similarities_option);

    QCommandLineOption scale_option(QStringList() << "s" << "scales",
            QCoreApplication::translate("main", "Number of scales (default: log10(num_data_points))."),
            QCoreApplication::translate("main", "scales"));
    parser.addOption(scale_option);

    QCommandLineOption name_option(QStringList() << "a" << "analysis_name",
            QCoreApplication::translate("main", "Number of scales (default: \"HSNE_Analysis\")."),
            QCoreApplication::translate("main", "analysis_name"));
    parser.addOption(name_option);

    QCommandLineOption normalization_option(QStringList() << "z" << "normalization",
            QCoreApplication::translate("main", "Apply a min-max normalization."));
    parser.addOption(normalization_option);

  ////////////////////////////////////////////////
  ///////////////   Arguments    /////////////////
  ////////////////////////////////////////////////

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    if(args.size()!=3){
      std::cout << "Not enough arguments!" << std::endl;
      return -1;
    }

    int num_data_points         = atoi(args.at(1).toStdString().c_str());
    int num_dimensions          = atoi(args.at(2).toStdString().c_str());

    hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
    multiscale_embedder.setLogger(&log);
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    typedef float scalar_type;
    //Input

    {//initializing panel data
      for(int j = 0; j < num_dimensions; ++j){
          panel_data.addDimension(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()));
      }
      panel_data.initialize();
      panel_data.reserve(num_data_points);
    }
    {
      std::ifstream input_file (args.at(0).toStdString(), std::ios::in|std::ios::binary);
      for(int j = 0; j < num_data_points; ++j){
          std::vector<scalar_type> data(num_dimensions);
          input_file.read (reinterpret_cast<char*>(data.data()), sizeof(scalar_type) * num_dimensions);
          panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(hdi::data::EmptyData()),data);
      }
      input_file.close();
    }
    if(parser.isSet(normalization_option)){
      std::cout << "Applying a min-max normalization" << std::endl;
      hdi::data::minMaxNormalization(panel_data);
    }

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
    params._seed = -1;
    params._mcmcs_landmark_thresh = 1.5;
    params._num_neighbors = 30;
    params._aknn_num_trees = 4;
    params._aknn_num_checks = 1024;
    params._transition_matrix_prune_thresh = 1.5;
    params._mcmcs_num_walks = 200;
    params._num_walks_per_landmark = 200;

    params._monte_carlo_sampling = true;
    params._out_of_core_computation = true;

    int num_scales = std::log10(num_data_points);
    std::string name("HSNE_Analysis");

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
        params._transition_matrix_prune_thresh = std::atof(parser.value(prune_option).toStdString().c_str());
    }
    if(parser.isSet(walks_landmark_option)){
        params._mcmcs_num_walks = std::atoi(parser.value(walks_landmark_option).toStdString().c_str());
    }
    if(parser.isSet(walks_similarities_option)){
        params._num_walks_per_landmark = std::atoi(parser.value(walks_similarities_option).toStdString().c_str());
    }
    if(parser.isSet(scale_option)){
        num_scales = std::atoi(parser.value(scale_option).toStdString().c_str());
    }
    if(parser.isSet(name_option)){
        name = parser.value(scale_option).toStdString();
    }

    std::cout << "Scales: " << num_scales << std::endl;

    MyInterfaceInitializer interface_initializer;
    multiscale_embedder.setName(name);
    multiscale_embedder.setInterfaceInitializer(&interface_initializer);
    multiscale_embedder.initialize(num_scales,params);
    multiscale_embedder.createTopLevelEmbedder();

    while(true){
        multiscale_embedder.doAnIterateOnAllEmbedder();
        QApplication::processEvents();
    }
    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what() << std::endl;}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what() << std::endl;}
  catch(std::bad_alloc& ba){ std::cerr << "bad_alloc caught: " << ba.what() << std::endl; }
  catch(...){ std::cout << "An unknown error occurred" << std::endl;}
}
