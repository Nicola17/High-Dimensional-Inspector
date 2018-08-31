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

#include "hdi/data/embedding.h"
#include "hdi/data/io.h"
#include "hdi/data/panel_data.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/dimensionality_reduction/tsne.h"
#include "hdi/utils/cout_log.h"
#include "hdi/utils/log_helper_functions.h"
#include "hdi/utils/scoped_timers.h"
#include "hdi/utils/visual_utils.h"
#include "hdi/visualization/scatterplot_canvas_qobj.h"
#include "hdi/visualization/scatterplot_drawer_fixed_color.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>

#include <stdio.h>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
  try {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Approximated tSNE with Progressive Visualization");
    QApplication::setApplicationVersion("0.1");

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
    QCommandLineOption verbose_option(QStringList() << "o"
                                                    << "verbose",
                                      QCoreApplication::translate("main", "Verbose"));
    parser.addOption(verbose_option);

    // Iterations
    QCommandLineOption iterations_option(QStringList() << "i"
                                                       << "iterations",
                                         QCoreApplication::translate("main", "Run the gradient for <iterations>."),
                                         QCoreApplication::translate("main", "iterations"));
    parser.addOption(iterations_option);

    //Dimensions
    QCommandLineOption target_dimensions_option(QStringList() << "d"
                                                              << "target_dimensions",
                                                QCoreApplication::translate("main", "Reduce the dimensionality to <target_dimensions>."),
                                                QCoreApplication::translate("main", "target_dimensions"));
    parser.addOption(target_dimensions_option);

    //Exaggeration iter
    QCommandLineOption exaggeration_iter_option(QStringList() << "x"
                                                              << "exaggeration_iter",
                                                QCoreApplication::translate("main", "Remove the exaggeration factor after <exaggeration_iter> iterations."),
                                                QCoreApplication::translate("main", "exaggeration_iter"));
    parser.addOption(exaggeration_iter_option);

    //Perplexity
    QCommandLineOption perplexity_option(QStringList() << "p"
                                                       << "perplexity",
                                         QCoreApplication::translate("main", "Use perplexity value of <perplexity>."),
                                         QCoreApplication::translate("main", "perplexity"));
    parser.addOption(perplexity_option);

    //Perplexity
    QCommandLineOption theta_option(QStringList() << "t"
                                                  << "theta",
                                    QCoreApplication::translate("main", "Use theta value of <theta> in the BH computation [0 <= t <= 1]."),
                                    QCoreApplication::translate("main", "theta"));
    parser.addOption(theta_option);

    //Save similarities
    QCommandLineOption save_similarities_option(QStringList() << "s"
                                                              << "similarities",
                                                QCoreApplication::translate("main", "Save the similarity matrix P in <similarities>."),
                                                QCoreApplication::translate("main", "similarities"));
    parser.addOption(save_similarities_option);

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    // source is args.at(0), destination is args.at(1)

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    if (args.size() != 4) {
      std::cout << "Not enough arguments!" << std::endl;
      return -1;
    }

    int num_data_points = atoi(args.at(2).toStdString().c_str());
    int num_dimensions = atoi(args.at(3).toStdString().c_str());

    bool verbose = false;
    int iterations = 1000;
    int exaggeration_iter = 250;
    int perplexity = 30;
    double theta = 0.5;
    int num_target_dimensions = 2;

    verbose = parser.isSet(verbose_option);
    if (parser.isSet(iterations_option)) {
      iterations = atoi(parser.value(iterations_option).toStdString().c_str());
    }
    if (parser.isSet(exaggeration_iter_option)) {
      exaggeration_iter = atoi(parser.value(exaggeration_iter_option).toStdString().c_str());
    }
    if (parser.isSet(perplexity_option)) {
      perplexity = atoi(parser.value(perplexity_option).toStdString().c_str());
    }
    if (parser.isSet(theta_option)) {
      theta = atof(parser.value(theta_option).toStdString().c_str());
      hdi::checkAndThrowRuntime(theta >= 0 && theta <= 1, "Invalid theta value");
    }
    if (parser.isSet(target_dimensions_option)) {
      num_target_dimensions = atoi(parser.value(target_dimensions_option).toStdString().c_str());
      hdi::checkAndThrowRuntime(num_target_dimensions >= 1, "Invalid number of target dimensions");
    }
    if (verbose) {
      std::cout << "===============================================" << std::endl;
      std::cout << "Arguments" << std::endl;
      std::cout << "\tHigh-Dim:\t\t" << args.at(0).toStdString() << std::endl;
      std::cout << "\tOutput:\t\t\t" << args.at(1).toStdString() << std::endl;
      std::cout << "\t#target_dimensions:\t\t\t" << num_target_dimensions << std::endl;
      std::cout << "\t# data-points:\t\t" << num_data_points << std::endl;
      std::cout << "Parameters" << std::endl;
      std::cout << "\tIterations:\t\t" << iterations << std::endl;
      std::cout << "\tExaggeration iter:\t" << exaggeration_iter << std::endl;
      std::cout << "\tPerplexity:\t\t" << perplexity << std::endl;
      std::cout << "\tTheta:\t\t" << theta << std::endl;
      std::cout << "===============================================" << std::endl;
    }

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    float data_loading_time = 0;
    float similarities_comp_time = 0;
    float gradient_desc_comp_time = 0;
    float data_saving_time = 0;

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    typedef float scalar_type;
    //Input
    std::vector<scalar_type> data;
    data.resize(num_data_points * num_dimensions);

    {
      hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(data_loading_time);
      std::ifstream input_file(args.at(0).toStdString(), std::ios::in | std::ios::binary | std::ios::ate);
      if (int(input_file.tellg()) != int(sizeof(scalar_type) * num_dimensions * num_data_points)) {
        std::cout << "Input file size doesn't agree with input parameters!" << std::endl;
        return 1;
      }
      input_file.seekg(0, std::ios::beg);
      input_file.read(reinterpret_cast<char*>(data.data()), sizeof(scalar_type) * num_dimensions * num_data_points);
      input_file.close();
    }

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    hdi::utils::CoutLog log;
    hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
    hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type distributions;
    hdi::dr::HDJointProbabilityGenerator<scalar_type>::Parameters prob_gen_param;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> tSNE;
    hdi::dr::SparseTSNEUserDefProbabilities<scalar_type>::Parameters tSNE_param;
    hdi::data::Embedding<scalar_type> embedding;

    {
      hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(similarities_comp_time);
      prob_gen_param._perplexity = perplexity;
      prob_gen.computeProbabilityDistributions(data.data(), num_dimensions, num_data_points, distributions, prob_gen_param);
    }

    tSNE_param._embedding_dimensionality = num_target_dimensions;
    tSNE_param._mom_switching_iter = exaggeration_iter;
    tSNE_param._remove_exaggeration_iter = exaggeration_iter;
    tSNE.initialize(distributions, &embedding, tSNE_param);
    tSNE.setTheta(theta);

    hdi::viz::ScatterplotCanvas canvas;
    canvas.setBackgroundColors(qRgb(255, 255, 255), qRgb(255, 255, 255));
    canvas.setSelectionColor(qRgb(255, 155, 0));
    canvas.resize(800, 800);
    canvas.show();

    std::vector<uint32_t> flags(num_data_points, 0);
    hdi::viz::ScatterplotDrawerFixedColor drawer;
    drawer.initialize(canvas.context());
    drawer.setData(embedding.getContainer().data(), flags.data(), num_data_points);
    drawer.setAlpha(0.5);
    drawer.setPointSize(20);
    canvas.addDrawer(&drawer);
    {
      hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(gradient_desc_comp_time);
      hdi::utils::secureLog(&log, "Computing gradient descent...");
      for (int iter = 0; iter < iterations; ++iter) {
        tSNE.doAnIteration();
        {  //Compute limits
          std::vector<scalar_type> limits;
          embedding.computeEmbeddingBBox(limits, 0.25);
          auto tr = QVector2D(limits[1], limits[3]);
          auto bl = QVector2D(limits[0], limits[2]);
          canvas.setTopRightCoordinates(tr);
          canvas.setBottomLeftCoordinates(bl);
        }
        canvas.updateGL();
        hdi::utils::secureLogValue(&log, "Iter", iter, verbose);
        QApplication::processEvents();
      }
    }
    hdi::utils::secureLog(&log, "... done!");

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    {
      //Output
      hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(data_saving_time);
      {
        std::ofstream output_file(args.at(1).toStdString(), std::ios::out | std::ios::binary);
        output_file.write(reinterpret_cast<const char*>(embedding.getContainer().data()), sizeof(scalar_type) * embedding.getContainer().size());
      }
      if (parser.isSet(save_similarities_option)) {
        std::ofstream output_file(parser.value(save_similarities_option).toStdString().c_str(), std::ios::out | std::ios::binary);
        hdi::data::IO::saveSparseMatrix(distributions, output_file);
      }
    }

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    hdi::utils::secureLogValue(&log, "Data loading (sec)", data_loading_time);
    hdi::utils::secureLogValue(&log, "Similarities computation (sec)", similarities_comp_time);
    hdi::utils::secureLogValue(&log, "Gradient descent (sec)", gradient_desc_comp_time);
    hdi::utils::secureLogValue(&log, "Data saving (sec)", data_saving_time);

    return app.exec();
  } catch (std::logic_error& ex) {
    std::cout << "Logic error: " << ex.what() << std::endl;
  } catch (std::runtime_error& ex) {
    std::cout << "Runtime error: " << ex.what() << std::endl;
  } catch (...) {
    std::cout << "An unknown error occurred" << std::endl;
    ;
  }
}
