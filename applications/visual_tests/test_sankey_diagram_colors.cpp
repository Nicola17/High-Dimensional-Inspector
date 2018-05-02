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
#include "hdi/visualization/sankey_diagram_qobj.h"
#include "hdi/data/flow_model.h"

#include <QApplication>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/assert_by_exception.h"
#include <iostream>
#include "hdi/utils/timing_utils.h"
#include <QIcon>


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
    hdi::viz::SankeyDiagram diagram;
    diagram.setLogger(&log);
    diagram.setVerbose(true);
    diagram.show();
    diagram.resize(QSize(750,350));

    for(int i = 0; i < 5; ++i){
      typedef hdi::data::ColoredFlowModelTrait::node_type node_type;
      typedef hdi::data::ColoredFlowModelTrait::flow_type flow_type;

      node_type a,b;
      a.id() = 0;
      b.id() = 1;
      hdi::checkAndThrowLogic(a!=b,"0");
      hdi::checkAndThrowLogic(!(a==b),"1");
      hdi::checkAndThrowLogic(a<b,"2");
      hdi::checkAndThrowLogic(!(a>b),"3");
      hdi::checkAndThrowLogic(a<=b,"4");
      hdi::checkAndThrowLogic(!(a>=b),"5");


      hdi::data::FlowModel<hdi::data::ColoredFlowModelTrait> model;
      model.addNode(node_type(0,"D0",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(1,"D1",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(2,"D2",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(3,"D3",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(4,"D4",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(5,"D5",qRgb(rand()%156,rand()%156,rand()%156)));

      model.addNode(node_type(6,"L0",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(7,"L1",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(8,"L2",qRgb(rand()%156,rand()%156,rand()%156)));
      model.addNode(node_type(9,"L3",qRgb(rand()%156,rand()%156,rand()%156)));

      const unsigned int num_flows(30);
      for(int i = 0; i < num_flows; ++i){
        model.addFlow(flow_type(i,rand()%6,6+rand()%4,(rand()%100)/100.,qRgb(rand()%156,rand()%156,rand()%156)));
      }

      diagram.visualizeFlow(model);

      for(int i = 0; i < 250; ++i){//OMG are you really doing this!? :P
        hdi::utils::sleepFor<hdi::utils::Milliseconds>(10);
        QApplication::processEvents();
      }
    }


    return app.exec();

  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
