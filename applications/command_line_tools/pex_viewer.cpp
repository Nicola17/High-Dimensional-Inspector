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
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_labels.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"

int main(int argc, char *argv[])
{
  try{
    typedef float scalar_type;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("PEx Viewer");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Visualization of the output of the Projection Explorer");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("embedding", QCoreApplication::translate("main", "Embedding."));

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    if(args.size()!=1){
      std::cout << "Not enough arguments!" << std::endl;
      return -1;
    }

    unsigned int num_data_points = 0;
    {
      std::string line;
      std::ifstream input_file (args[0].toStdString(), std::ios::in);
      while(std::getline(input_file,line)){
        ++num_data_points;
      }
    }


    hdi::data::Embedding<scalar_type> embedding;
    embedding.resize(2,num_data_points);
    std::vector<unsigned int> pivots;
    std::vector<unsigned int> labels;
    std::map<unsigned int, QColor> palette;
    palette[0] = qRgb(16,78,139);
    palette[1] = qRgb(139,90,43);
    palette[2] = qRgb(138,43,226);
    palette[3] = qRgb(0,128,0);
    palette[4] = qRgb(255,150,0);
    palette[5] = qRgb(204,40,40);
    palette[6] = qRgb(131,139,131);
    palette[7] = qRgb(0,205,0);
    palette[8] = qRgb(20,20,20);
    palette[9] = qRgb(0, 150, 255);

    {
      std::map<float, unsigned int> label_map;
      for(int i = 0; i < 10; ++i){
        label_map[i] = i;
      }
      unsigned int used_labels = 0;
      std::string line;
      std::ifstream input_file (args[0].toStdString(), std::ios::in);
      unsigned int id_row = 0;
      while(std::getline(input_file,line)){
        std::stringstream line_stream(line);
        std::string cell;
        unsigned int id_cell = 0;
        while(std::getline(line_stream,cell,' ')){
          if(id_cell == 0){
            embedding.dataAt(id_row,0) = std::atof(cell.c_str());
          }else if(id_cell == 1){
            embedding.dataAt(id_row,1) = std::atof(cell.c_str());
          }else if(id_cell == 2){
            float v = std::atof(cell.c_str());
            auto it = label_map.find(v);

            if(it!=label_map.end()){
              labels.push_back(it->second);
            }else{
              labels.push_back(used_labels);
              label_map[v] = used_labels;
              ++used_labels;
            }
            if(palette.find(label_map[v]) == palette.end()){
              palette[label_map[v]] = qRgb(rand()%256, rand()%256, rand()%256);
            }
          }else if(id_cell == 3){
            if(std::atof(cell.c_str())){
              pivots.push_back(id_row);
            }
          }

          ++id_cell;
        }
        ++id_row;
      }
    }


    std::vector<uint32_t> flags(embedding.numDataPoints(),0);
    hdi::viz::ScatterplotCanvas viewer;
    hdi::viz::ScatterplotDrawerLabels drawer_labels;
    {
      viewer.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
      viewer.resize(900,900);
      viewer.show();
      std::vector<scalar_type> limits;
      embedding.computeEmbeddingBBox(limits,0.2);
      auto tr = QVector2D(limits[1],limits[3]);
      auto bl = QVector2D(limits[0],limits[2]);
      viewer.setTopRightCoordinates(tr);
      viewer.setBottomLeftCoordinates(bl);

      drawer_labels.initialize(viewer.context());
      drawer_labels.setData(embedding.getContainer().data(),flags.data(),labels.data(),palette,embedding.numDataPoints());
      drawer_labels.setPointSize(5);
      viewer.addDrawer(&drawer_labels);

      viewer.updateGL();
      viewer.show();
      QApplication::processEvents();
    }

    hdi::viz::ScatterplotCanvas viewer_fix_color;
    hdi::viz::ScatterplotDrawerFixedColor drawer_fixed;
    {
      viewer_fix_color.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
      viewer_fix_color.resize(900,900);
      viewer_fix_color.show();
      std::vector<scalar_type> limits;
      embedding.computeEmbeddingBBox(limits,0.2);
      auto tr = QVector2D(limits[1],limits[3]);
      auto bl = QVector2D(limits[0],limits[2]);
      viewer_fix_color.setTopRightCoordinates(tr);
      viewer_fix_color.setBottomLeftCoordinates(bl);

      drawer_fixed.initialize(viewer_fix_color.context());
      drawer_fixed.setData(embedding.getContainer().data(),flags.data(),embedding.numDataPoints());
      drawer_fixed.setPointSize(5);
      viewer_fix_color.addDrawer(&drawer_fixed);

      viewer_fix_color.updateGL();
      viewer_fix_color.show();
      QApplication::processEvents();
    }


    hdi::data::Embedding<scalar_type> embedding_pivot;
    embedding_pivot.resize(2,pivots.size());

    std::vector<unsigned int> labels_pivot(pivots.size());
    std::vector<uint32_t> flags_pivot(pivots.size(),0);
    hdi::viz::ScatterplotCanvas viewer_pivot;
    hdi::viz::ScatterplotDrawerLabels drawer_labels_pivot;
    {
      for(int i = 0; i < pivots.size(); ++i){
        embedding_pivot.dataAt(i,0) = embedding.dataAt(pivots[i],0);
        embedding_pivot.dataAt(i,1) = embedding.dataAt(pivots[i],1);
        labels_pivot[i] = labels[pivots[i]];
      }
    }
    std::cout << "#pivots: " << embedding_pivot.numDataPoints() << std::endl;
    {
      viewer_pivot.setBackgroundColors(qRgb(255,255,255),qRgb(255,255,255));
      viewer_pivot.resize(900,900);
      viewer_pivot.show();
      std::vector<scalar_type> limits;
      embedding_pivot.computeEmbeddingBBox(limits,0.2);
      auto tr = QVector2D(limits[1],limits[3]);
      auto bl = QVector2D(limits[0],limits[2]);
      viewer_pivot.setTopRightCoordinates(tr);
      viewer_pivot.setBottomLeftCoordinates(bl);

      drawer_labels_pivot.initialize(viewer.context());
      drawer_labels_pivot.setData(embedding_pivot.getContainer().data(),flags_pivot.data(),labels_pivot.data(),palette,embedding_pivot.numDataPoints());
      drawer_labels_pivot.setPointSize(8);
      viewer_pivot.addDrawer(&drawer_labels_pivot);

      viewer_pivot.updateGL();
      viewer_pivot.show();
      QApplication::processEvents();
    }


    return app.exec();

  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
