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

#include <assert.h>
#include "hdi/visualization/scalable_pcp_view_qobj.h"

#include <QApplication>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include <iostream>
#include "hdi/utils/timing_utils.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/dataset_utils.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/controller_embedding_selection_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/utils/visual_utils.h"

int main(int argc, char *argv[]){
  try{
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    typedef float scalar_type;
    typedef uint32_t flag_type;

    hdi::utils::CoutLog log;

    int n_dim = 10;
    scalar_type max = 1;
    scalar_type min = 0;
    hdi::viz::ScalablePCPView pcp_view;
    hdi::viz::ScalablePCPView pcp_view_avg_sorted;
    hdi::viz::ScalablePCPView pcp_view_std_dev_sorted;
    {
      pcp_view.setLogger(&log);
      pcp_view.show();

      QCoreApplication::processEvents();

      pcp_view.setMaxY(max);
      pcp_view.setMinY(min);
      pcp_view.setNumDims(n_dim);
      pcp_view.setSorting(hdi::viz::ScalablePCPView::sorting_type::NO_SORTING);
      pcp_view.resize(QSize(600,200));
      pcp_view.onUpdate();
      pcp_view.resize(QSize(600,200));

      QCoreApplication::processEvents();
      pcp_view.resize(QSize(600,200));
    }
    {
      pcp_view_avg_sorted.setLogger(&log);
      pcp_view_avg_sorted.show();

      QCoreApplication::processEvents();

      pcp_view_avg_sorted.setMaxY(max);
      pcp_view_avg_sorted.setMinY(min);
      pcp_view_avg_sorted.setNumDims(n_dim);
      pcp_view_avg_sorted.setSorting(hdi::viz::ScalablePCPView::sorting_type::AVG_SORTING);
      pcp_view_avg_sorted.resize(QSize(600,200));
      pcp_view_avg_sorted.onUpdate();
      pcp_view_avg_sorted.resize(QSize(600,200));

      QCoreApplication::processEvents();
      pcp_view_avg_sorted.resize(QSize(600,200));
    }
    {
      pcp_view_std_dev_sorted.setLogger(&log);
      pcp_view_std_dev_sorted.show();

      QCoreApplication::processEvents();

      pcp_view_std_dev_sorted.setMaxY(max);
      pcp_view_std_dev_sorted.setMinY(min);
      pcp_view_std_dev_sorted.setNumDims(n_dim);
      pcp_view_std_dev_sorted.setSorting(hdi::viz::ScalablePCPView::sorting_type::STD_DEV_SORTING);
      pcp_view_std_dev_sorted.resize(QSize(600,200));
      pcp_view_std_dev_sorted.onUpdate();
      pcp_view_std_dev_sorted.resize(QSize(600,200));

      QCoreApplication::processEvents();
      pcp_view_std_dev_sorted.resize(QSize(600,200));
    }


    int n_points = 250;
    for(int p = 0; p < n_points; ++p){
      std::vector<scalar_type> point;
      for(int i = 0; i < n_dim; ++i){
        point.push_back((rand()%1000)/1000.);
      }
      pcp_view.addDataPointAndDraw(point);
      pcp_view_std_dev_sorted.addDataPointAndDraw(point);
      pcp_view_avg_sorted.addDataPointAndDraw(point);
    }


    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
