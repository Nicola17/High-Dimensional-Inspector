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
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include "hdi/visualization/scatterplot_drawer_images.h"

#include <QApplication>
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include <iostream>

using namespace hdi;

int main(int argc, char *argv[]){
  try{
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    typedef float scalar_type;
    typedef uint32_t flag_type;

    utils::CoutLog log;
    viz::ScatterplotCanvas viewer;
    
    
    viewer.setBackgroundColors(qRgb(230,230,230),qRgb(250,150,150));
    viewer.setSelectionColor(qRgb(50,50,50));
    viewer.resize(600,600);

    viewer.setLogger(&log);
    viewer.setVerbose(true);
    viewer.show();

    int n_points(5000);
    std::vector<scalar_type> embedding_sin(n_points*2);
    std::vector<scalar_type> embedding_cos(n_points*2);
    

    std::vector<flag_type> flags(n_points,0);
    scalar_type max_x(2*3.1415);
    {
      for(int i = 0; i < n_points; ++i){
        scalar_type x(max_x*i/n_points);
        scalar_type y_sin(std::sin(x));
        scalar_type y_cos(std::cos(x));
        embedding_sin[i*2] = x;
        embedding_sin[i*2+1] = y_sin;
        embedding_cos[i*2] = x;
        embedding_cos[i*2+1] = y_cos;
        if(i > n_points*0.75){
          flags[i] = 1;
        }
      }
    }

    std::vector<scalar_type> user_defined_points;
    std::vector<scalar_type> user_defined_colors;
    std::vector<flag_type> user_defined_flags;
    {
      int n_points_ud = 5;
      user_defined_points.resize(n_points_ud*2);
      user_defined_colors.resize(n_points_ud*3);
      user_defined_flags.resize(n_points_ud);

      for(int i = 0; i < n_points_ud; ++i){
        user_defined_points[i*2]  = float(i)/(n_points_ud-1)*max_x;
        user_defined_points[i*2+1]  = -2;

        user_defined_colors[i*3]  = float(i)/(n_points_ud-1);
        user_defined_colors[i*3+1]  = 0;
        user_defined_colors[i*3+2]  = 0;

        user_defined_flags[i] = 0;
      }
      user_defined_flags[3] = 1;
    }

    std::vector<scalar_type> images_user_defined_points;
    std::vector<flag_type> images_user_defined_flags;
    std::vector<QImage> images;
    {
      int n_points_ud = 5;
      images_user_defined_points.resize(n_points_ud*2);
      images_user_defined_flags.resize(n_points_ud);

      for(int i = 0; i < n_points_ud; ++i){
        images_user_defined_points[i*2]  = float(i)/(n_points_ud-1)*max_x;
        images_user_defined_points[i*2+1]  = -3;
        images_user_defined_flags[i] = 0;
        auto image  = QImage(30,30,QImage::Format_ARGB32);
        for(int i = 0; i < 30; ++i)
          for(int j = 0; j < 30; ++j)
            //image.setPixel(i,j,qRgb(i/30.*255,j/30.*255,255));
            //image.setPixel(i,j,qRgb(0,0,255));
            image.setPixel(i,j,qRgb(rand()%256,rand()%256,rand()%256));
        images.push_back(image);
      }
    }

    viewer.setTopRightCoordinates(QVector2D(max_x+0.5,1.5));
    viewer.setBottomLeftCoordinates(QVector2D(-0.5,-3.5));

    viz::ScatterplotDrawerFixedColor drawer_sin;
    drawer_sin.initialize(viewer.context());
    drawer_sin.setData(embedding_sin.data(), flags.data(), flags.size());
    drawer_sin.setPointSize(1.5);
    drawer_sin.setColor(qRgb(150,30,30));
    drawer_sin.setSelectionColor(qRgb(30,150,30));

    viz::ScatterplotDrawerFixedColor drawer_cos;
    drawer_cos.initialize(viewer.context());
    drawer_cos.setData(embedding_cos.data(), flags.data(), flags.size());
    drawer_cos.setPointSize(3);
    drawer_cos.setColor(qRgb(150,30,150));
    drawer_cos.setSelectionColor(qRgb(150,150,30));

    viz::ScatterplotDrawerImages drawer_image;
    drawer_image._embedding = images_user_defined_points.data();
    drawer_image._flags = images_user_defined_flags.data();
    drawer_image._num_points = images_user_defined_flags.size();
    drawer_image._images = images;
    drawer_image.initialize(viewer.context());

    viz::ScatterplotDrawerUsedDefinedColors drawer_user_def_colors;
    drawer_user_def_colors.initialize(viewer.context());
    drawer_user_def_colors.setData(user_defined_points.data(), user_defined_colors.data(), user_defined_flags.data(), user_defined_flags.size());
    drawer_user_def_colors.setPointSize(15);
    drawer_user_def_colors.setSelectionColor(qRgb(255,150,0));

    viewer.addDrawer(&drawer_sin);
    viewer.addDrawer(&drawer_cos);
    viewer.addDrawer(&drawer_image);
    viewer.addDrawer(&drawer_user_def_colors);
  


    auto size = viewer.embeddingSize();
    utils::secureLogValue(&log,"Embedding width",size.x());
    utils::secureLogValue(&log,"Embedding height",size.y());
    viewer.resize(size.x()*100,size.y()*100);
    return app.exec();
    
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
