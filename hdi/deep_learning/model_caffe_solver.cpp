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

#include "hdi/deep_learning/model_caffe_solver.h"
#include "hdi/utils/log_helper_functions.h"

namespace hdi{
    namespace dl{

        void get_gpus(std::vector<int>* gpus) {
            int count = 0;
        #ifndef CPU_ONLY
            CUDA_CHECK(cudaGetDeviceCount(&count));
        #else
            NO_GPU;
        #endif
            for (int i = 0; i < count; ++i) {
              gpus->push_back(i);
            }
        }

        void ModelCaffeSolver::initialize(std::string& file_name){
            caffe::ReadProtoFromTextFileOrDie(file_name, &_solver_param);

//            std::vector<int> gpus;
//            get_gpus(&gpus);
//            if(gpus.size() == 0){
              hdi::utils::secureLog(_log,"Caffe - using CPU");
              caffe::Caffe::set_mode(caffe::Caffe::CPU);
//            }else{
//              std::ostringstream s;
//              for (int i = 0; i < gpus.size(); ++i) {
//                s << (i ? ", " : "") << gpus[i];
//              }
//              hdi::utils::secureLogValue(_log, "Caffe - Using GPUs ", s.str());
//              _solver_param.set_device_id(gpus[0]);
//              caffe::Caffe::SetDevice(gpus[0]);
//              caffe::Caffe::set_mode(caffe::Caffe::GPU);
//              caffe::Caffe::set_solver_count(gpus.size());
//           }

            _solver = std::shared_ptr<caffe::Solver<float> >(caffe::SolverRegistry<float>::CreateSolver(_solver_param));
            auto param = _solver->param();
            hdi::utils::secureLog(_log,"Model loaded");
            hdi::utils::secureLogVector(_log,"Layers",_solver->net()->layer_names());
        }

        void ModelCaffeSolver::iterate(int n){
            _solver->Step(n);

        }

    }
}
