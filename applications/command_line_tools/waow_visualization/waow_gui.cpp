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
#include "hdi/analytics/waow_vis_qobj.h"
#include "hdi/visualization/embedding_lines_drawer.h"
#include "hdi/visualization/histogram_view_qobj.h"
#include "hdi/visualization/word_cloud_qobj.h"

#include "application.h"
#include <QInputDialog>

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

    QApplication::setApplicationName("WAOW");
    QApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Command line version of WAOW visualization");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("data", QCoreApplication::translate("main", "Input folder."));

  ////////////////////////////////////////////////
  ///////////////   Arguments  /////////////////
  ////////////////////////////////////////////////

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    if(!(args.size()==1 || args.size()==2)){
      std::cout << "Wrong number of arguments!" << std::endl;
      return -1;
    }


    WAOWApplication application;
    application.resize(1900,750);
    application._ui->centralwidget->setStyleSheet("background-color:white;");
    application.show();
    application._log = &log;

    hdi::analytics::WAOWVis waow_vis;
    waow_vis.setLogger(&log);

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    std::unordered_map<std::string,uint32_t> follower_id;
    std::unordered_map<std::string,uint32_t> target_id;
    std::shared_ptr<std::vector<Roaring>> follower_to_target(std::make_shared<std::vector<Roaring>>());
    std::shared_ptr<std::vector<Roaring>> target_to_follower(std::make_shared<std::vector<Roaring>>());
    follower_to_target->reserve(1000000);
    target_to_follower->reserve(3000);

    hdi::utils::IO::loadTwitterFollowers(argv[1],*follower_to_target,*target_to_follower,follower_id,target_id);

    std::vector<std::shared_ptr<hdi::data::AbstractData>> data_followers(follower_to_target->size());
    std::vector<std::shared_ptr<hdi::data::AbstractData>> data_target(target_to_follower->size());

    for(const auto& target: target_id){
      data_target[target.second] = std::make_shared<hdi::data::TextData>(target.first);
    }


  ////////////////////////////////////////////////
  //////////   TESTING  ////////////////////////
  ////////////////////////////////////////////////
/*
{
  follower_to_target->resize(10000);
  data_followers.resize(10000);

  target_to_follower->resize(target_to_follower->size()/2*2);
  data_target.resize(target_to_follower->size());

  int half_followers = follower_to_target->size()/2;
  int half_target  = target_to_follower->size()/2;
  for(auto& bmap: *follower_to_target){
    bmap = Roaring();
  }
  for(auto& bmap: *target_to_follower){
    bmap = Roaring();
  }
  for(int f = 0; f < follower_to_target->size(); ++f){
    for(int l = 0; l < 20; ++l){
      int r = rand()%half_target;
      if(f < half_followers){
        (*follower_to_target)[f].add(r);
        (*target_to_follower)[r].add(f);
      }else{
        (*follower_to_target)[f].add(r+half_target);
        (*target_to_follower)[r+half_target].add(f);
      }
    }
  }
}
*/

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    hdi::analytics::WAOWVis::hsne_type::Parameters params;
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


    application._waow_vis  = &waow_vis;
    waow_vis.setInterfaceInitializer(&application);
    waow_vis.setVisualizationMode("Default");
    if(args.size()==2){
      //Load from disk
      waow_vis.initialize(follower_to_target,target_to_follower,data_followers,data_target,argv[2]);
    }else{
      waow_vis.initialize(follower_to_target,target_to_follower,data_followers,data_target);
    }

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    hdi::viz::HistogramView histogram_view_before_A;
    hdi::viz::HistogramView histogram_view_after_A;
    hdi::viz::HistogramView histogram_view_before_B;
    hdi::viz::HistogramView histogram_view_after_B;

    histogram_view_before_A.setData(waow_vis.histogramA());
    histogram_view_after_A.setData(waow_vis.histogramUniqueA());

    histogram_view_before_B.setColorName("green");
    histogram_view_after_B.setColorName("green");
    histogram_view_before_B.setData(waow_vis.histogramB());
    histogram_view_after_B.setData(waow_vis.histogramUniqueB());


    application._ui->_histograms_setA_layout->addWidget(histogram_view_before_A.widgetPtr());
    application._ui->_histograms_setA_layout->addWidget(histogram_view_after_A.widgetPtr());
    application._ui->_histograms_setB_layout->addWidget(histogram_view_before_B.widgetPtr());
    application._ui->_histograms_setB_layout->addWidget(histogram_view_after_B.widgetPtr());

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    application.resize(1800,750);
    waow_vis.createOverview();
    waow_vis.setVisualizationMode("Weights");

    //waow_vis.save("WAOW_SAVE");

    int iter = 0;
    while(true){
      //waow_vis.doAnIterateOnAllViews();
      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
      application.iterate();
      ++iter;
    }
    return app.exec();


  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what() << std::endl;}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what() << std::endl;}
  catch(std::bad_alloc& ba){ std::cerr << "bad_alloc caught: " << ba.what() << std::endl; }
  catch(...){ std::cout << "An unknown error occurred" << std::endl;}
}
