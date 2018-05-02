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
#include "hdi/visualization/dl_filter_viz_qobj.h"

#include <QApplication>
#include <QIcon>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include <iostream>
#include "hdi/utils/timing_utils.h"
#include "hdi/data/panel_data.h"
#include "hdi/utils/dataset_utils.h"

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


    std::vector<hdi::viz::DLFilterViz::timed_scalar_type> values;
    std::vector<hdi::viz::DLFilterViz::unsigned_int_type> order;
    std::vector<std::string> names;

    int num_filters = 60;
    for(int i = 0; i < num_filters; ++i){
      names.push_back(QString("F%1").arg(i).toStdString());
      order.push_back(i);

      hdi::viz::DLFilterViz::scalar_type v = rand()%1000;
      hdi::viz::DLFilterViz::unsigned_int_type it = 200 + rand()%400;
      values.push_back(hdi::viz::DLFilterViz::timed_scalar_type(it,v));
    }

    hdi::viz::DLFilterViz filter_viz;
    filter_viz.setData(values);
    filter_viz.setFilterNames(names);
    filter_viz.setFilterOrder(order);
    filter_viz.setMaxValue(1000);
    filter_viz.setLogger(&log);
    filter_viz.show();
    filter_viz.resize(500,500);

    QCoreApplication::processEvents();
    filter_viz.updateView();



    QCoreApplication::processEvents();

    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
