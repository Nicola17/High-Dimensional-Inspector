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
#include "hdi/visualization/linechart_dual_view_qobj.h"

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

    std::vector<std::pair<scalar_type,scalar_type>> points;
    std::vector<std::pair<scalar_type,scalar_type>> points_dual;
    {
      int n_points = 1000;
      scalar_type val = 100;
      for(int i = 0; i < n_points; ++i){
        val = 5*((rand()%1000)/1000.-0.4) + val;
        points.push_back(std::pair<scalar_type,scalar_type>(i,val));
      }
    }
    {
      int n_points = 100;
      scalar_type val = 20;
      for(int i = 0; i < n_points; ++i){
        val = 6*((rand()%1000)/1000.-0.4) + val;
        points_dual.push_back(std::pair<scalar_type,scalar_type>(i*10,val));
      }
    }
    hdi::viz::LineChartDualView linechart_dual_view;
    linechart_dual_view.setLogger(&log);
    linechart_dual_view.show();

    QCoreApplication::processEvents();

    linechart_dual_view.setMaxX(1000);
    linechart_dual_view.setMinX(0);
    linechart_dual_view.setMaxY(2000);
    linechart_dual_view.setMinY(0);
    linechart_dual_view.setData(points);
    linechart_dual_view.setDualMaxY(100);
    linechart_dual_view.setDualMinY(0);
    linechart_dual_view.setDualData(points_dual);
    linechart_dual_view.resize(QSize(600,200));
    linechart_dual_view.onUpdate();

    QCoreApplication::processEvents();

    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
