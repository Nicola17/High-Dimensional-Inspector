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
#include "hdi/data/embedding.h"
#include "hdi/data/panel_data.h"
#include "hdi/data/io.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/utils/visual_utils.h"
#include "hdi/utils/scoped_timers.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QIcon>

#include <iostream>
#include <fstream>
#include <stdio.h>



int main(int argc, char *argv[])
{
  try{
    hdi::utils::CoutLog log;

    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/hdi16.png");
    icon.addFile(":/hdi32.png");
    icon.addFile(":/hdi64.png");
    icon.addFile(":/hdi128.png");
    icon.addFile(":/hdi256.png");
    app.setWindowIcon(icon);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("data", QCoreApplication::translate("main", "Raw data file."));
    parser.addPositionalArgument("hdvol", QCoreApplication::translate("main", "HDVol data file."));
    parser.addPositionalArgument("width", QCoreApplication::translate("main", "Image width."));
    parser.addPositionalArgument("height", QCoreApplication::translate("main", "Image height."));
    parser.addPositionalArgument("channels", QCoreApplication::translate("main", "Number of channels."));
    parser.addPositionalArgument("bytes", QCoreApplication::translate("main", "Number of bytes per channel."));

    QCommandLineOption desired_channels_option(QStringList() << "d" << "desired_channels",
        QCoreApplication::translate("main", "Generate a hdvol file with <desired_channels> channels (evenly distributed)."),
        QCoreApplication::translate("main", "desired_channels"));
    parser.addOption(desired_channels_option);
    QCommandLineOption save_channels_as_images_option(QStringList() << "s" << "save_channels",
                     QCoreApplication::translate("main", "Save channels as images"));
    parser.addOption(save_channels_as_images_option);

    parser.process(app);
    const QStringList args = parser.positionalArguments();

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    std::ifstream raw_file   (args.at(0).toStdString().c_str(), std::ios::in |std::ios::binary);
    std::ofstream hdvol_file   (args.at(1).toStdString().c_str(), std::ios::out|std::ios::binary);
    if(!raw_file.is_open())   {throw std::runtime_error("unable to open the input file");}
    if(!hdvol_file.is_open())   {throw std::runtime_error("unable to open the output file");}

    const unsigned int width   = std::atoi(args.at(2).toStdString().c_str());
    const unsigned int height  = std::atoi(args.at(3).toStdString().c_str());
    const unsigned int channels  = std::atoi(args.at(4).toStdString().c_str());
    const unsigned int bytes   = std::atoi(args.at(5).toStdString().c_str());
    unsigned int desired_channels = parser.isSet(desired_channels_option)?atoi(parser.value(desired_channels_option).toStdString().c_str()):channels;
    const bool save_images = parser.isSet(save_channels_as_images_option);


  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    std::vector<unsigned int> channel_ids;
    if(false){

    }else{
      channel_ids.resize(desired_channels);
      if(desired_channels == channels){
        std::iota(channel_ids.begin(),channel_ids.end(),0);
      }else{
        for(int c = 0; c < desired_channels; ++c){
          channel_ids[c] = float(channels)/desired_channels*c;
        }
      }
    }
    std::vector<QImage> channel_images(save_images?channel_ids.size():0,QImage(width,height,QImage::Format_ARGB32));


    hdi::utils::secureLogValue(&log, "Width",       width);
    hdi::utils::secureLogValue(&log, "Height",      height);
    hdi::utils::secureLogValue(&log, "Channels",    channels);
    hdi::utils::secureLogValue(&log, "Bytes",       bytes);
    hdi::utils::secureLogValue(&log, "Channels (Out)",  desired_channels);
    hdi::utils::secureLogValue(&log, "Save images",   save_images);

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    const int num_data_points = width*height;
    const int num_dimensions  = desired_channels;
    const int zero = 0;

    hdvol_file.write((char*)&num_data_points,4);
    hdvol_file.write((char*)&num_dimensions,4);
    hdvol_file.write((char*)&zero,4);
    hdvol_file.write((char*)&width,4);
    hdvol_file.write((char*)&zero,4);
    hdvol_file.write((char*)&height,4);
    hdvol_file.write((char*)&zero,4);
    hdvol_file.write((char*)&zero,4);

    double total_values  = 0;
    double num_zeroes    = 0;
    double num_high_values = 0;




    for(unsigned int j = 0; j < height; ++j){
      for(unsigned int i = 0; i < width; ++i){

        hdvol_file.write((char*)&i,4);
        hdvol_file.write((char*)&j,4);
        hdvol_file.write((char*)&zero,4);
        for(int c = 0; c < channel_ids.size(); ++c){
          std::streampos byte_to_read = 0;
          const unsigned int channel_id = channel_ids[c];
          byte_to_read = static_cast<unsigned long>(i+width*j)*channels*bytes+(channel_id*bytes);
          raw_file.seekg(byte_to_read,std::ios::beg);
          /*
          if(desired_channels!=channels){
            const unsigned int channel_id = float(channels)/desired_channels*c;
            //byte_to_read = (i+width*j)*channels*bytes+(channel_id*bytes);
            byte_to_read = static_cast<unsigned long>(i+width*j)*channels*bytes+(channel_id*bytes);
            raw_file.seekg(byte_to_read,std::ios::beg);
          }
          */

          uint32_t temp(0);
          raw_file.read((char*)&temp,bytes);

          if(!(temp < std::pow(256,bytes))){throw std::logic_error("wrong input data");}

          ++total_values;
          if(temp == 0){
            ++num_zeroes;
          }
          if(temp > std::pow(256,bytes)/5.){
            ++num_high_values;
          }

          float val(temp);
          hdvol_file.write((char*)&val,4);

          if(save_images){
            //float intensity = std::min(val/std::pow(256.0,bytes)*255,255.);
            float intensity = std::min(val/100.*255,255.);
            channel_images[c].setPixel(i,j,qRgb(intensity,intensity,intensity));
          }
        }

      }
    }
    hdi::utils::secureLogValue(&log, "% of zeroes",  100.*num_zeroes/total_values);
    hdi::utils::secureLogValue(&log, "% high values",  100.*num_high_values/total_values);

    for(int c = 0; c < channel_images.size(); ++c){
      channel_images[c].save(QString("channel_img_%1.png").arg(channel_ids[c]));
    }
    
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what() << std::endl;}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what() << std::endl;}
  catch(...){ std::cout << "An unknown error occurred" << std::endl;;}
}
