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
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/visualization/scatterplot_drawer_user_defined_colors.h"
#include <iostream>
#include <set>
#include <map>
#include <random>
#include "hdi/clustering/kmeans.h"
#include "hdi/utils/timing_utils.h"

template <typename scalar_type>
void highDimVersor(std::vector<scalar_type>& versor){
  scalar_type length(0);
  for(auto& v : versor){
    v = (rand()%1000)/1000.-0.5;
    length += v*v;
  }
  length = std::sqrt(length);
  for(auto& v : versor){
    v /= length;

  }
}

template <typename scalar_type>
void multiplyVector(std::vector<scalar_type>& vector, scalar_type length){
  for (auto& v : vector){
    v *= length;
  }
}

template <typename scalar_type>
void addVector(std::vector<scalar_type>& vector, const std::vector<scalar_type>& add){
  for(int i = 0; i < vector.size(); ++i){
    vector[i] += add[i];
  }
}


int main(int argc, char *argv[]){
  try{
    typedef double scalar_type;
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/brick32.png");
    icon.addFile(":/brick128.png");
    app.setWindowIcon(icon);

    hdi::utils::CoutLog log;

    hdi::viz::ScatterplotCanvas viewer;
    viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
    viewer.setSelectionColor(qRgb(50,50,50));
    viewer.resize(500,500);
    viewer.show();

    const int n_dims(2);
    const int n_hyperspheres(10);
    const int points_per_sphere(500);
    const int outliers(300);
    const int n_points(n_hyperspheres*points_per_sphere+outliers);
    const int iterations = 5000;
    const double radius = 20;
    const double outlier_mult = 10;
    const double inter_sphere_mult = 10;




////////////////////////////////////////

    std::map<int,QColor> color_per_sphere;
    std::vector<std::vector<scalar_type> > hd_data;

    std::default_random_engine generator;
    std::normal_distribution<double> distribution(0,radius);

    for (int i = 0; i < outliers; ++i){
      std::vector<scalar_type> point(n_dims, 0);
      highDimVersor(point);
      multiplyVector(point, (rand() % 1000) / 1000. * radius * outlier_mult);
      hd_data.push_back(point);
    }
    for (int i = 0; i < n_hyperspheres; ++i){
      std::vector<scalar_type> origin(n_dims, 0);
      highDimVersor(origin);
      multiplyVector(origin, (rand()%1000)/1000.*radius*inter_sphere_mult);

      for (int j = 0; j < points_per_sphere; ++j){
        std::vector<scalar_type> point(origin);
        std::vector<scalar_type> to_add(n_dims, n_dims);
        highDimVersor(to_add);
        multiplyVector(to_add, distribution(generator));
        addVector(point, to_add);

        hd_data.push_back(point);
      }
      color_per_sphere[i] = qRgb(rand()%180,rand()%180,rand()%180);
    }

////////////////////////////////////////

    hdi::viz::ScatterplotDrawerUsedDefinedColors drawer_centroids;
    hdi::viz::ScatterplotDrawerUsedDefinedColors drawer;
    std::vector<float> data_points(n_points*2,0);
    std::vector<float> centroids(n_hyperspheres*2,0);
    std::vector<float> colors_cluster(n_points*3,0);
    std::vector<float> colors_centroids(n_hyperspheres*3,0);
    std::vector<uint32_t> flags(n_points,0);

    float min_x(std::numeric_limits<float>::max());
    float max_x(-std::numeric_limits<float>::max());
    float min_y(std::numeric_limits<float>::max());
    float max_y(-std::numeric_limits<float>::max());
    for(int i = 0; i < n_points; ++i){
      data_points[i*2] = hd_data[i][0];
      data_points[i*2+1] = hd_data[i][1];
      colors_cluster[i*3] = (rand()%1000)/1000.;
      colors_cluster[i*3+1]= (rand()%1000)/1000.;
      colors_cluster[i*3+2]= (rand()%1000)/1000.;
      min_x = std::min<float>(min_x,hd_data[i][0]);
      max_x = std::max<float>(max_x,hd_data[i][0]);
      min_y = std::min<float>(min_y,hd_data[i][1]);
      max_y = std::max<float>(max_y,hd_data[i][1]);
    }
    for(int i = 0; i < n_hyperspheres; ++i){
      colors_centroids[i*3] = color_per_sphere[i].redF()*0.7;
      colors_centroids[i*3+1] = color_per_sphere[i].greenF()*0.7;
      colors_centroids[i*3+2] = color_per_sphere[i].blueF()*0.7;
    }
    viewer.setBottomLeftCoordinates(QVector2D(min_x,min_y));
    viewer.setTopRightCoordinates(QVector2D(max_x,max_y));

    drawer.initialize(viewer.context());
    drawer.setData(data_points.data(), colors_cluster.data(), flags.data(), n_points);
    drawer.setAlpha(0.5);
    drawer.setPointSize(2.5);
    viewer.addDrawer(&drawer);

    drawer_centroids.initialize(viewer.context());
    drawer_centroids.setData(centroids.data(), colors_centroids.data(), flags.data(), n_hyperspheres);
    drawer_centroids.setAlpha(.7);
    drawer_centroids.setPointSize(15);
    viewer.addDrawer(&drawer_centroids);


    hdi::clustering::KMeans<float> kmeans;
    kmeans._num_clusters = n_hyperspheres;
    kmeans._num_points = n_points;
    kmeans._dimensionality = 2;
    kmeans._data = data_points.data();
    kmeans._centroids = centroids.data();

    kmeans.initialize();
    for(int i = 0; i < iterations; ++i){
      kmeans.doAnIteration();
      for(int i = 0; i < n_points; ++i){
        colors_cluster[i*3]   = color_per_sphere[kmeans._clusters[i]].redF();
        colors_cluster[i*3+1]   = color_per_sphere[kmeans._clusters[i]].greenF();
        colors_cluster[i*3+2]   = color_per_sphere[kmeans._clusters[i]].blueF();
      }
      viewer.updateGL();
      QApplication::processEvents();
      hdi::utils::sleepFor<hdi::utils::Milliseconds>(100);
    }

    return app.exec();
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
