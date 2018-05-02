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
#include <qimage.h>
#include <QApplication>
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"
#include "hdi/data/embedding.h"

int main(int argc, char *argv[]){
  typedef float scalar_type;

  QApplication app(argc, argv);
  QIcon icon;
  icon.addFile(":/brick32.png");
  icon.addFile(":/brick128.png");
  app.setWindowIcon(icon);

  hdi::viz::ScatterplotCanvas viewer;
  viewer.setBackgroundColors(qRgb(240,240,240),qRgb(200,200,200));
  viewer.setSelectionColor(qRgb(50,50,50));
  viewer.resize(800,800);
  viewer.show();


  const int n_points = 1000;
  const int n_dims = 20;
  hdi::dr::TSNE<scalar_type> tSNE;
  tSNE.setDimensionality(n_dims);
  std::vector<std::vector<scalar_type> > hd_data;
  for(int i = 0; i < n_points/2; ++i){
    std::vector<scalar_type>pnt(n_dims, i);
    hd_data.push_back(pnt);
    tSNE.addDataPoint(hd_data[hd_data.size()-1].data());
  }
  for(int i = 0; i < n_points/2; ++i){
    std::vector<scalar_type>pnt(n_dims);
    int range = std::max(n_points / 10, 10);
    for(auto& v : pnt)
      v = (n_points/4) + (rand() % range)-range/2;
    hd_data.push_back(pnt);
    tSNE.addDataPoint(hd_data[hd_data.size()-1].data());
  }

  hdi::dr::TSNE<scalar_type>::InitParams params;
  params._perplexity = 250;

  hdi::data::Embedding<scalar_type> embedding;

  tSNE.initialize(&embedding,params);
  {//Draw distances
    auto& distances_squared = tSNE.getDistancesSquared();
    scalar_type max_value(0);
    for(int i = 0; i < n_points; ++i){
      max_value = std::max<scalar_type>(distances_squared[i], max_value);
    }

    QImage image(n_points, n_points, QImage::Format::Format_RGB32);
    for(int i = 0; i < n_points*n_points; ++i){
      scalar_type v = distances_squared[i]/max_value*255.;
      image.setPixel(i%n_points, i / n_points, qRgb(v, v, v));
    }
    image.save("tsne_line_distances.png");
  }

  {//Draw Sigmas
    auto& sigmas = tSNE.getSigmas();
    scalar_type max_value(0);
    for(int i = 0; i < n_points; ++i){
      max_value = std::max<scalar_type>(sigmas[i], max_value);
    }

    int width = 50;
    QImage image(width, n_points, QImage::Format::Format_RGB32);
    for(int i = 0; i < n_points; ++i){
      scalar_type v = sigmas[i]/max_value*255.;
      for(int j = 0; j < width; ++j){
        image.setPixel(j, i, qRgb(v, v, v));
      }
    }
    image.save("tsne_line_sigmas.png");
  }

  {//Draw P
    auto& P = tSNE.getDistributionP();
    scalar_type max_value(0);
    for(int i = 0; i < n_points*n_points; ++i){
      max_value = std::max<scalar_type>(P[i], max_value);
    }

    QImage image(n_points, n_points, QImage::Format::Format_RGB32);
    for(int i = 0; i < n_points*n_points; ++i){
      scalar_type v = P[i]/max_value*255.;
      image.setPixel(i%n_points, i / n_points, qRgb(v, v, v));
    }
    image.save("tsne_line_P.png");
  }

  tSNE.doAnIteration();
  {//Draw Q
    auto& Q = tSNE.getDistributionQ();
    scalar_type max_value(0);
    scalar_type min_value(std::numeric_limits<scalar_type>::max());
    for(int i = 0; i < n_points*n_points; ++i){
      max_value = std::max<scalar_type>(Q[i], max_value);
      if(Q[i]!=0)
        min_value = std::min<scalar_type>(Q[i], min_value);
    }

    QImage image(n_points, n_points, QImage::Format::Format_RGB32);
    for(int i = 0; i < n_points*n_points; ++i){
      scalar_type v = (Q[i]-min_value)/(max_value-min_value)*255.;
      image.setPixel(i%n_points, i / n_points, qRgb(v, v, v));
    }
    image.save("tsne_line_Q.png");
  }

  std::vector<uint32_t> flags(n_points,0);
  hdi::viz::ScatterplotDrawerFixedColor drawer;
  drawer.initialize(viewer.context());
  drawer.setData(embedding.getContainer().data(), flags.data(), n_points);
  drawer.setAlpha(1);
  viewer.addDrawer(&drawer);
  for(int i = 0; i < 1000; ++i){
    tSNE.doAnIteration();
    {//limits
      std::vector<scalar_type> limits;
      embedding.computeEmbeddingBBox(limits,0.25);
      auto tr = QVector2D(limits[1],limits[3]);
      auto bl = QVector2D(limits[0],limits[2]);
      viewer.setTopRightCoordinates(tr);
      viewer.setBottomLeftCoordinates(bl);
    }

    viewer.updateGL();
    viewer.show();
    scalar_type err = tSNE.computeKullbackLeiblerDivergence();
    printf("%d: %f\n", i,err);
    QApplication::processEvents();
  }
  {//Draw Q
    auto& Q = tSNE.getDistributionQ();
    scalar_type max_value(0);
    scalar_type min_value(std::numeric_limits<scalar_type>::max());
    for(int i = 0; i < n_points*n_points; ++i){
      max_value = std::max<scalar_type>(Q[i], max_value);
      if(Q[i]!=0)
        min_value = std::min<scalar_type>(Q[i], min_value);
    }

    QImage image(n_points, n_points, QImage::Format::Format_RGB32);
    for(int i = 0; i < n_points*n_points; ++i){
      scalar_type v = (Q[i]-min_value)/(max_value-min_value)*255.;
      image.setPixel(i%n_points, i / n_points, qRgb(v, v, v));
    }
    image.save("tsne_line_Q_final.png");
  }

  return app.exec();
}
