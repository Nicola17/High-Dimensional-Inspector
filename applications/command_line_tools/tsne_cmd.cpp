/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *    its contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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
#include <QIcon>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "hdi/data/embedding.h"
#include "hdi/data/panel_data.h"

//#include "log/cout_log.h"

int main(int argc, char *argv[])
{
  try{
    QApplication app(argc, argv);
    QIcon icon;
    icon.addFile(":/hdi16.png");
    icon.addFile(":/hdi32.png");
    icon.addFile(":/hdi64.png");
    icon.addFile(":/hdi128.png");
    icon.addFile(":/hdi256.png");
    app.setWindowIcon(icon);

    QCoreApplication::setApplicationName("tSNE");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Command line version of the tSNE algorithm");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("data", QCoreApplication::translate("main", "High dimensional data."));
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Output file."));
    parser.addPositionalArgument("num_data_points", QCoreApplication::translate("main", "Num of data-points."));
    parser.addPositionalArgument("num_dimensions", QCoreApplication::translate("main", "Num of dimensions."));

  ////////////////////////////////////////////////
  ///////////////   Arguments    /////////////////
  ////////////////////////////////////////////////
    // Verbose
    QCommandLineOption verbose_option(QStringList() << "o" << "verbose",
                     QCoreApplication::translate("main", "Verbose"));
    parser.addOption(verbose_option);

    // Iterations
    QCommandLineOption iterations_option(QStringList() << "i" << "iterations",
        QCoreApplication::translate("main", "Run the gradient for <iterations>."),
        QCoreApplication::translate("main", "iterations"));
    parser.addOption(iterations_option);

    //Dimensions
    QCommandLineOption target_dimensions_option(QStringList() << "d" << "target_dimensions",
        QCoreApplication::translate("main", "Reduce the dimensionality to <target_dimensions>."),
        QCoreApplication::translate("main", "target_dimensions"));
    parser.addOption(target_dimensions_option);

    //Exaggeration iter
    QCommandLineOption exaggeration_iter_option(QStringList() << "x" << "exaggeration_iter",
        QCoreApplication::translate("main", "Remove the exaggeration factor after <exaggeration_iter> iterations."),
        QCoreApplication::translate("main", "exaggeration_iter"));
    parser.addOption(exaggeration_iter_option);

    //Perplexity
    QCommandLineOption perplexity_option(QStringList() << "p" << "perplexity",
        QCoreApplication::translate("main", "Use perplexity value of <perplexity>."),
        QCoreApplication::translate("main", "perplexity"));
    parser.addOption(perplexity_option);

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    if(args.size()!=4){
      std::cout << "Not enough arguments!" << std::endl;
      return -1;
    }

    int num_data_points         = atoi(args.at(2).toStdString().c_str());
    int num_dimensions          = atoi(args.at(3).toStdString().c_str());

    bool verbose                = false;
    int iterations              = 1000;
    int exaggeration_iter       = 250;
    int perplexity              = 30;
    int num_dimensions_output   = 2;


    verbose     = parser.isSet(verbose_option);
    if(parser.isSet(iterations_option)){
      iterations  = atoi(parser.value(iterations_option).toStdString().c_str());
    }
    if(parser.isSet(exaggeration_iter_option)){
      exaggeration_iter = atoi(parser.value(exaggeration_iter_option).toStdString().c_str());
    }
    if(parser.isSet(perplexity_option)){
      perplexity = atoi(parser.value(perplexity_option).toStdString().c_str());
    }
    if(parser.isSet(target_dimensions_option)){
      num_dimensions_output = atoi(parser.value(target_dimensions_option).toStdString().c_str());
    }
    if(verbose){
      std::cout << "===============================================" << std::endl;
      std::cout << "Arguments" << std::endl;
      std::cout << "\tHigh-Dim:\t\t" << args.at(0).toStdString() << std::endl;
      std::cout << "\tOutput:\t\t\t" << args.at(1).toStdString() << std::endl;
      std::cout << "\t# data-points:\t\t" << num_data_points << std::endl;
      std::cout << "\t# dimensions:\t\t" << num_dimensions << std::endl;
      std::cout << "Parameters" << std::endl;
      std::cout << "\tOutput dimensions:\t" << num_dimensions_output << std::endl;
      std::cout << "\tIterations:\t\t" << iterations << std::endl;
      std::cout << "\tExaggeration iter:\t" << exaggeration_iter << std::endl;
      std::cout << "\tPerplexity:\t\t" << perplexity << std::endl;
      std::cout << "===============================================" << std::endl;
    }

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    typedef float scalar_type;
    //Input
    std::vector<scalar_type> data;
    data.resize(num_data_points * num_dimensions);

    {
      std::ifstream input_file (args.at(0).toStdString(), std::ios::in|std::ios::binary|std::ios::ate);
      if(int(input_file.tellg()) != int(sizeof(scalar_type) * num_dimensions * num_data_points)){
        std::cout << "Input file size doesn't agree with input parameters!" << std::endl;
        return 1;
      }
      input_file.seekg (0, std::ios::beg);
      input_file.read (reinterpret_cast<char*>(data.data()), sizeof(scalar_type) * num_dimensions * num_data_points);
      input_file.close();
    }

    for(int d = 0; d < num_dimensions; ++d){
        double min(std::numeric_limits<double>::max());
        double max(-std::numeric_limits<double>::max());
        for(int p = 0; p < num_data_points; ++p){
            min  = std::min<double>(min,data[p*num_dimensions+d]);
            max  = std::max<double>(max,data[p*num_dimensions+d]);
        }
        if(min != max){
            for(int p = 0; p < num_data_points; ++p){
                data[p*num_dimensions+d] = (data[p*num_dimensions+d]-min)/(max-min);
            }
        }
    }

  ////////////////////////////////////////////////
  ////////////////////////////////////////////////
  ////////////////////////////////////////////////

    hdi::utils::CoutLog log;
    hdi::dr::TSNE<scalar_type> tSNE;
    hdi::data::Embedding<scalar_type> embedding;
    tSNE.setLogger(&log);

    tSNE.setDimensionality(num_dimensions);

    for(int i = 0; i < num_data_points; ++i){
      tSNE.addDataPoint(data.data()+i*num_dimensions);
    }
    tSNE.initialize(&embedding);

    hdi::utils::secureLog(&log,"Computing gradient descent...");
    for(int iter = 0; iter < iterations; ++iter){
      tSNE.doAnIteration();
      hdi::utils::secureLogValue(&log,"Iter",iter,verbose);
    }
    hdi::utils::secureLog(&log,"... done!");

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    //Output
    {
      std::ofstream output_file (args.at(1).toStdString(), std::ios::out|std::ios::binary);
      output_file.write(reinterpret_cast<const char*>(embedding.getContainer().data()),sizeof(scalar_type)*embedding.getContainer().size());
    }
  }
  catch(std::logic_error& ex){ std::cout << "Logic error: " << ex.what();}
  catch(std::runtime_error& ex){ std::cout << "Runtime error: " << ex.what();}
  catch(...){ std::cout << "An unknown error occurred";}
}
