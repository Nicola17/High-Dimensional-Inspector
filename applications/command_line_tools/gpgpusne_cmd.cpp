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
#include "hdi/utils/dataset_utils.h"
#include "hdi/data/embedding.h"
#include "hdi/data/panel_data.h"
#include "hdi/data/io.h"
#include "hdi/dimensionality_reduction/hd_joint_probability_generator.h"
#include "hdi/dimensionality_reduction/gradient_descent_tsne_texture.h"
#include "hdi/dimensionality_reduction/sparse_tsne_user_def_probabilities.h"
#include "hdi/utils/visual_utils.h"
#include "hdi/utils/scoped_timers.h"
#include "hdi/dimensionality_reduction/evaluation.h"

#include <QApplication>
#include <QIcon>
#include <QCommandLineParser>

#include <iostream>
#include <fstream>
#include <stdio.h>

#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QDebug>

typedef float scalar_type;

enum Technique
{
  TSNE,
  BH05,
  BH01,
  GPGPUSNE
};

class OffscreenBuffer : public QWindow
{
public:
  OffscreenBuffer()
  {
    setSurfaceType(QWindow::OpenGLSurface);

    _context = new QOpenGLContext(this);
    _context->setFormat(requestedFormat());

    if (!_context->create())
      qFatal("Cannot create requested OpenGL context.");

    create();
  }

  QOpenGLContext* getContext() { return _context; }

  void bindContext()
  {
    _context->makeCurrent(this);
    if (!gladLoadGL()) {
      qFatal("No OpenGL context is currently bound, therefore OpenGL function loading has failed.");
    }
  }

  void releaseContext()
  {
    _context->doneCurrent();
  }

private:
  QOpenGLContext* _context;
};

void loadData(hdi::data::PanelData<scalar_type>& panel_data, std::string filename_data, unsigned int num_points, unsigned int num_dims)
{
  panel_data.clear();
  std::ifstream file_data(filename_data, std::ios::in | std::ios::binary);

  if (!file_data.is_open()) {
    throw std::runtime_error("data file cannot be found");
  }

  for (int j = 0; j < num_dims; ++j) {
    panel_data.addDimension(std::make_shared<hdi::data::EmptyData>());
  }
  panel_data.initialize();

  {
    for (int j = 0; j < num_points; ++j) {
      std::vector<scalar_type> input_data(num_dims);
      for (int i = 0; i < num_dims; ++i) {
        float appo;
        file_data.read((char*)&appo, 4);
        input_data[i] = scalar_type(appo);
      }
      panel_data.addDataPoint(std::make_shared<hdi::data::EmptyData>(), input_data);
    }
  }
}

int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);

        QCommandLineParser parser;
        parser.setApplicationDescription("Compute embedding from given raw input data");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addPositionalArgument("data", QApplication::translate("main", "Name of input file."));

        // Process the actual command line arguments given by the user
        parser.process(app);

        const QStringList args = parser.positionalArguments();

        if (args.size() != 3) {
          std::cout << "Not enough arguments!" << std::endl;
          return -1;
        }

        unsigned num_data_points;
        unsigned num_dimensions;
        unsigned embedding_dimensions = 2;

        std::ifstream inputFile(args[0].toStdString(), std::ios::in | std::ios::binary | std::ios::ate);

        std::string inputDataName = args[0].toStdString();
        /*std::string probDistName = inputDataName + "_P";*/
        num_data_points = args[1].toInt();
        num_dimensions = args[2].toInt();

        int iterations = 1000;
        int exaggeration_iter = 250;
        int perplexity = 30;
        double theta = 0.5;
        int num_target_dimensions = 2;

        float data_loading_time = 0;
        float similarities_comp_time = 0;
        float gradient_desc_comp_time = 0;
        float data_saving_time = 0;

        typedef float scalar_type;

        hdi::utils::CoutLog log;
        hdi::dr::HDJointProbabilityGenerator<scalar_type> prob_gen;
        hdi::dr::HDJointProbabilityGenerator<scalar_type>::sparse_scalar_matrix_type distributions;
        hdi::dr::HDJointProbabilityGenerator<scalar_type>::Parameters prob_gen_param;
        hdi::dr::GradientDescentTSNETexture tSNE;
        hdi::dr::SparseTSNEUserDefProbabilities<scalar_type> bhTSNE;
        hdi::dr::TsneParameters tsne_params;

        hdi::data::Embedding<scalar_type> embedding;

        hdi::data::PanelData<scalar_type> panel_data;
        std::vector<uint32_t> labels;

        Technique technique = GPGPUSNE;

        std::string outputFileName = "gpgpu_emb_" + inputDataName;
        std::string probDistFileName = inputDataName + "_P";

        // Set a fixed seed
        tsne_params._seed = 1;

        // Set Barnes-Hut parameters
        if (technique == BH05)
          bhTSNE.setTheta(0.5);
        else if (technique == BH01)
          bhTSNE.setTheta(0.1);
        else if (technique == TSNE)
          bhTSNE.setTheta(0);

        // Compute probability distribution or load from file if it is cached
        std::ifstream input_file(probDistFileName, std::ios::binary);
        if (input_file.is_open())
        {
          hdi::utils::secureLog(&log, "Loading probability distribution from file...");
          hdi::data::IO::loadSparseMatrix(distributions, input_file);
        }
        else
        {
          // Load the input data
          hdi::utils::secureLog(&log, "Loading original data...");
          loadData(panel_data, inputDataName, num_data_points, num_dimensions);

          hdi::utils::secureLog(&log, "Computing probability distribution...");
          hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(similarities_comp_time);
          prob_gen_param._perplexity = perplexity;
          prob_gen.computeProbabilityDistributions(panel_data.getData().data(), panel_data.numDimensions(), panel_data.numDataPoints(), distributions, prob_gen_param);

          std::ofstream output_file(probDistFileName, std::ios::binary);
          hdi::data::IO::saveSparseMatrix(distributions, output_file);
        }

        // Create offscreen buffer for OpenGL context
        OffscreenBuffer offscreen;
        offscreen.bindContext();

        tsne_params._embedding_dimensionality = num_target_dimensions;
        tsne_params._mom_switching_iter = exaggeration_iter;
        tsne_params._remove_exaggeration_iter = exaggeration_iter;

        hdi::utils::secureLog(&log, "Initializing t-SNE...");
        if (technique == BH05 || technique == BH01 || technique == TSNE)
        {
          bhTSNE.initialize(distributions, &embedding, tsne_params);
        }
        else if (technique == GPGPUSNE)
        {
          tSNE.initialize(distributions, &embedding, tsne_params);
        }

        // Compute embedding
        {
          hdi::utils::secureLog(&log, "Computing gradient descent...");

          if (technique == BH05 || technique == BH01 || technique == TSNE)
          {
            hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(gradient_desc_comp_time);
            for (int iter = 0; iter < iterations; ++iter) {
              bhTSNE.doAnIteration();
            }
          }
          else if (technique == GPGPUSNE)
          {
            hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(gradient_desc_comp_time);
            for (int iter = 0; iter < iterations; ++iter) {
              tSNE.doAnIteration();
            }
          }
        }

        //double KL = tSNE.computeKullbackLeiblerDivergence();
        //std::cout << "KL-Divergence: " << KL << std::endl;
        //Output
        hdi::utils::secureLog(&log, "Writing embedding to file...");
        {
          hdi::utils::ScopedTimer<float, hdi::utils::Seconds> timer(data_saving_time);
          {
            std::ofstream output_file(outputFileName, std::ios::out | std::ios::binary);
            output_file.write(reinterpret_cast<const char*>(embedding.getContainer().data()), sizeof(scalar_type)*embedding.getContainer().size());
          }
        }

        offscreen.releaseContext();

        hdi::utils::secureLogValue(&log, "Data loading (sec)", data_loading_time);
        hdi::utils::secureLogValue(&log, "Similarities computation (sec)", similarities_comp_time);
        hdi::utils::secureLogValue(&log, "Gradient descent (sec)", gradient_desc_comp_time);
        hdi::utils::secureLogValue(&log, "Data saving (sec)", data_saving_time);

        app.exec();
    }
    catch (std::logic_error& ex) { std::cout << "Logic error: " << ex.what() << std::endl; }
    catch (std::runtime_error& ex) { std::cout << "Runtime error: " << ex.what() << std::endl; }
    catch (...) { std::cout << "An unknown error occurred" << std::endl;; }
}
