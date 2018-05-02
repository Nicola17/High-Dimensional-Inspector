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

#include "hdi/data/text_data.h"
#include "hdi/data/panel_data.h"
#include "hdi/data/pixel_data.h"
#include "hdi/data/image_data.h"
#include "hdi/visualization/text_view_qobj.h"
#include "hdi/visualization/image_view_qobj.h"
#include "hdi/visualization/pixel_view_qobj.h"
#include "hdi/data/empty_data.h"
#include "hdi/visualization/multiple_image_view_qobj.h"
#include <assert.h>
#include <QVector3D>

using namespace hdi;

int main(int argc, char *argv[]){
  QApplication app(argc, argv);
  QIcon icon;
  icon.addFile(":/brick32.png");
  icon.addFile(":/brick128.png");
  app.setWindowIcon(icon);

  viz::TextView text_view;
  data::PanelData<float> panel_data_text;

  viz::ImageView image_view;
  data::PanelData<float> panel_data_image;

  viz::PixelView pixel_view;
  data::PanelData<float> panel_data_pixel;

  viz::MultipleImageView multiple_image_view;
  data::PanelData<float> panel_data_multiple_image;

  {// Text view
    int n_points = 300;
    int n_dim = 5;

    for(int i = 0; i < n_dim; ++i){
      panel_data_text.addDimension(std::make_shared<data::TextData>());
    }

    panel_data_text.initialize();

    for(int i = 0; i < n_points; ++i){
      std::vector<float> data(n_dim);
      panel_data_text.addDataPoint(std::make_shared<data::TextData>(QString("data %1").arg(i).toStdString()), data);
    }

    text_view.setPanelData(&panel_data_text);
    text_view.show();
    text_view.updateView();
  }

  {// Image view
    int n_points = 300;
    int width = 30;
    int height = 30;

    for(int j = 0; j < height; ++j){
      for(int i = 0; i < width; ++i){
        panel_data_image.addDimension(std::make_shared<data::PixelData>(width,height,i,j));
      }
    }

    panel_data_image.initialize();

    for(int i = 0; i < n_points; ++i){
      std::vector<float> data(width*height,0);
      QImage image(width,height,QImage::Format::Format_RGB32);

      for(int j = 0; j < height; ++j){
        for(int i = 0; i < width; ++i){
          image.setPixel(i, j, qRgb(255,255.*i/width,255.*j/height));
        }
      }

      panel_data_image.addDataPoint(std::make_shared<data::ImageData>(image), data);
    }

    panel_data_image.getFlagsDataPoints()[0] = data::PanelData<float>::Selected;

    image_view.setImageSize(width, height);
    image_view.setResMultiplier(5);
    image_view.setPanelData(&panel_data_image);
    image_view.show();
    image_view.updateView();
  }

  {// Pixel view
    int n_points = 300;
    int num_dim = 10;
    int width = 30;
    int height = 30;

    for(int i = 0; i < num_dim; ++i){
      panel_data_pixel.addDimension(std::make_shared<data::EmptyData>());
    }

    panel_data_pixel.initialize();

    for(int j = 0; j < height; ++j){
      for(int i = 0; i < width; ++i){
        std::vector<float> data(num_dim,0);
        panel_data_pixel.addDataPoint(std::make_shared<data::PixelData>(i,j,width,height), data);
      }
    }


    panel_data_pixel.getFlagsDataPoints()[0] = data::PanelData<float>::Selected;

    pixel_view.setImageSize(width, height);
    pixel_view.setResMultiplier(5);
    pixel_view.setPanelData(&panel_data_pixel);
    pixel_view.show();
    pixel_view.updateView();
  }

  {// Multiple image view
    int n_points = 300;
    int width = 30;
    int height = 30;

    for(int j = 0; j < height; ++j){
      for(int i = 0; i < width; ++i){
        panel_data_multiple_image.addDimension(std::make_shared<data::PixelData>(width,height,i,j));
      }
    }

    panel_data_multiple_image.initialize();

    for(int i = 0; i < n_points; ++i){
      std::vector<float> data(width*height,0);
      QImage image(width,height,QImage::Format::Format_RGB32);

      QVector3D color_1(rand()%256,rand()%256,rand()%256);
      QVector3D color_2(rand()%256,rand()%256,rand()%256);
      for(int j = 0; j < height; ++j){
        for(int i = 0; i < width; ++i){
          QVector3D pixel_color = (color_1 * float(i)/width + color_2 *float(j)/height)*0.5;
          image.setPixel(i, j, qRgb(pixel_color.x(),pixel_color.y(),pixel_color.z()));
        }
      }

      panel_data_multiple_image.addDataPoint(std::make_shared<data::ImageData>(image), data);
    }

    for(int i = 0; i < n_points/3; ++i){
      panel_data_multiple_image.getFlagsDataPoints()[i] = data::PanelData<float>::Selected;
    }

    multiple_image_view.setPanelData(&panel_data_multiple_image);
    multiple_image_view.show();
    multiple_image_view.updateView();
  }

  return app.exec();
}
