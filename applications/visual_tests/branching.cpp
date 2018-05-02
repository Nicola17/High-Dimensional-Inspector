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

class MyInterfaceInitializer: public hdi::analytics::AbstractInterfaceInitializer{
public:
  virtual ~MyInterfaceInitializer(){}
  virtual void initializeStandardVisualization(embedder_type* embedder, const std::vector<unsigned int>& idxes_to_orig_data){
    std::shared_ptr<hdi::viz::MultipleImageView> image_view(new hdi::viz::MultipleImageView());
    embedder->addView(image_view);
    image_view->_text_data_as_os_path = true;
    image_view->show();

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
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    hdi::utils::CoutLog log;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
    if(argc != 2){
      hdi::utils::secureLog(&log,"Not enough input parameters...");
      return 1;
    }

    std::map<unsigned int, QColor> palette;
    std::map<QRgb, unsigned int> rev_palette;

    hdi::analytics::MultiscaleEmbedderSystem multiscale_embedder;
    multiscale_embedder.setLogger(&log);
    hdi::analytics::MultiscaleEmbedderSystem::panel_data_type& panel_data = multiscale_embedder.getPanelData();
    std::vector<unsigned int> labels;
    QImage image(argv[1]);

    panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
    panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
    panel_data.initialize();

    for(int j = 0; j < image.height(); ++j){
      for(int i = 0; i < image.width(); ++i){
        auto pixel = QColor(image.pixel(i,j));
        if(pixel.red() != 255 && pixel.green() != 255 && pixel.blue() != 255){
          std::vector<scalar_type> data(2,0);
          data[0] = i;
          data[1] = j;
          panel_data.addDataPoint(std::make_shared<hdi::data::PixelData>(hdi::data::PixelData(i,j,image.width(),image.height())),data);

          if(rev_palette.find(pixel.rgb()) != rev_palette.end()){
            labels.push_back(rev_palette[pixel.rgb()]);
          }else{
            rev_palette[pixel.rgb()] = palette.size();
            palette[palette.size()] = pixel;
            labels.push_back(rev_palette[pixel.rgb()]);
          }
        }
      }
    }

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

    hdi::analytics::MultiscaleEmbedderSystem::hsne_type::Parameters params;
    params._seed = 2;
    params._num_neighbors = 90;
    params._num_walks_per_landmark = 100;
    params._aknn_num_trees = 4;
    params._aknn_num_checks = 1024;

    params._monte_carlo_sampling = true;
    //params._rs_reduction_factor_per_layer = 0.2;


    MyInterfaceInitializer interface_initializer;
    interface_initializer._palette = &palette;
    interface_initializer._labels = &labels;
    multiscale_embedder.setInterfaceInitializer(&interface_initializer);
    multiscale_embedder.initialize(3,params);
    multiscale_embedder.createTopLevelEmbedder();

    int iter = 0;
    while(true){
      if(((iter+1)%100) == 0){
        hdi::utils::secureLogValue(&log,"Iter",iter+1);
      }
      multiscale_embedder.doAnIterateOnAllEmbedder();
      QApplication::processEvents();
      ++iter;
    }

    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
