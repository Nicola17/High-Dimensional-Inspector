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


#ifndef EMBEDDING_EQUALIZER_INL
#define EMBEDDING_EQUALIZER_INL

#include "hdi/dimensionality_reduction/embedding_equalizer.h"
#include "hdi/utils/math_utils.h"
#include "hdi/utils/log_helper_functions.h"

#include <time.h>
#include <cmath>



namespace hdi{
  namespace dr{

    template <typename scalar_type>
    EmbeddingEqualizer<scalar_type>::EmbeddingEqualizer():
      _initialized(false),
      _embedding_master(nullptr),
      _embedding_slave(nullptr),
      _step_size(0.02),
      _momentum(0.8),
      _connections(nullptr)
    {

    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::initialize(data::Embedding<scalar_type>* embedding_master, data::Embedding<scalar_type>* embedding_slave){
      checkAndThrowLogic(embedding_master->numDataPoints() == embedding_slave->numDataPoints(), "Embedding size must agree");
      //checkAndThrowLogic(_embedding_master->numDimensions() == _embedding_slave->numDimensions(), "Embedding size must agree");

      _embedding_master   = embedding_master;
      _embedding_slave  = embedding_slave;
      _previous_update.clear();
      _previous_update.resize(embedding_slave->numDataPoints() * embedding_slave->numDimensions(),0);

      _initialized = true;
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::initialize(data::Embedding<scalar_type>* embedding_master, data::Embedding<scalar_type>* embedding_slave, const connections_type* connections){
      checkAndThrowLogic(embedding_slave->numDataPoints() == connections->size(), "Embedding size must agree");

      _embedding_master   = embedding_master;
      _embedding_slave  = embedding_slave;
      _connections    = connections;
      _previous_update.clear();
      _previous_update.resize(embedding_slave->numDataPoints() * embedding_slave->numDimensions(),0);

      _initialized = true;
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration(double mult){
      if(_embedding_master->numDimensions() == 1 && _embedding_slave->numDimensions() == 2){
        if(_connections){
          doAnIteration1Dto2DConnections(mult);
        }else{
          doAnIteration1Dto2D(mult);
        }
      }else if(_embedding_master->numDimensions() == 2 && _embedding_slave->numDimensions() == 1){
        if(_connections){
          doAnIteration2Dto1DConnections(mult);
        }else{
          doAnIteration2Dto1D(mult);
        }
      }else if(_embedding_master->numDimensions() == 1 && _embedding_slave->numDimensions() == 1){
        if(_connections){
          doAnIteration1Dto1DConnections(mult);
        }else{
          doAnIteration1Dto1D(mult);
        }
      }else{
        throw std::logic_error("embedding equalizer not supported");
      }
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration1Dto1D(double mult){
      const int num_pnts = _embedding_master->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      for(int i = 0; i < num_pnts; ++i){
        double master_position = (master_data[i]-master_bb[0])/(master_bb[1]-master_bb[0]);
        double slave_position  = (slave_data [i]-slave_bb [0])/(slave_bb [1]-slave_bb [0]);

        double delta = _previous_update[i]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i] += delta*(slave_bb [1]-slave_bb [0]);
        _previous_update[i] = delta;
      }
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration1Dto2D(double mult){
      const int num_pnts = _embedding_master->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      //for preserving the aspect ratio
      bool x_bigger = false;
      double ratio = 0;
      if(slave_bb [3]-slave_bb [2] < slave_bb [1]-slave_bb [0]){
        x_bigger = true;
        ratio = (slave_bb [3]-slave_bb [2])/(slave_bb [1]-slave_bb [0]);
      }

      for(int i = 0; i < num_pnts; ++i){
        double master_position = (master_data[i]  -master_bb[0])/(master_bb[1]-master_bb[0]);
        double slave_position  = (slave_data [i*2+1]-slave_bb [2])/(slave_bb [3]-slave_bb [2]);

        if(x_bigger){
          slave_position = slave_position*ratio + (1-ratio)/2;
        }

        double delta = _previous_update[i*2+1]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i*2+1] += delta*(slave_bb [3]-slave_bb [2]);
        _previous_update[i*2+1] = delta;
      }
    }


    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration2Dto1D(double mult){
      const int num_pnts = _embedding_master->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      for(int i = 0; i < num_pnts; ++i){
        double master_position = (master_data[i*2+1]-master_bb[2])/(master_bb[3]-master_bb[2]);
        double slave_position  = (slave_data [i]  -slave_bb [0])/(slave_bb [1]-slave_bb [0]);

        double delta = _previous_update[i]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i] += delta*(slave_bb [1]-slave_bb [0]);
        _previous_update[i] = delta;
      }
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration1Dto1DConnections(double mult){
      const int num_pnts = _embedding_master->numDataPoints();
      const int num_pnts_slave = _embedding_slave->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      for(int i = 0; i < num_pnts_slave; ++i){
        double slave_position = (slave_data[i]  -slave_bb[0])/(slave_bb[1]-slave_bb[0]);
        //The master is computed as the weighted average of the connected elements
        double master_position  = 0;
        double weight = 0;
        for(auto e: (*_connections)[i]){
          assert(e.first < num_pnts);
          master_position += (master_data [e.first]-master_bb [0])/(master_bb [1]-master_bb [0])*e.second;
          weight += e.second;
        }
        if(weight == 0){
          continue;
        }
        master_position /= weight;

        double delta = _previous_update[i]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i] += delta*(slave_bb [1]-slave_bb [0]);
        _previous_update[i] = delta;
      }
    }

    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration1Dto2DConnections(double mult){
      const int num_pnts =     _embedding_master->numDataPoints();
      const int num_pnts_slave = _embedding_slave->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      //for preserving the aspect ratio
      bool x_bigger = false;
      double ratio = 0;
      if(slave_bb [3]-slave_bb [2] < slave_bb [1]-slave_bb [0]){
        x_bigger = true;
        ratio = (slave_bb [3]-slave_bb [2])/(slave_bb [1]-slave_bb [0]);
      }

      for(int i = 0; i < num_pnts_slave; ++i){
        double slave_position = (slave_data[i*2+1]  -slave_bb[2])/(slave_bb[3]-slave_bb[2]);
        if(x_bigger){
          slave_position = slave_position*ratio + (1-ratio)/2;
        }

        //The master is computed as the weighted average of the connected elements
        double master_position  = 0;
        double weight = 0;
        for(auto e: (*_connections)[i]){
          assert(e.first < num_pnts);
          master_position += (master_data [e.first]-master_bb[0])/(master_bb[1]-master_bb[0])*e.second;
          weight += e.second;
        }
        if(weight == 0){
          continue;
        }
        master_position /= weight;

        double delta = _previous_update[i*2+1]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i*2+1] += delta*(slave_bb [3]-slave_bb[2]);
        _previous_update[i*2+1] = delta;
      }
    }


    template <typename scalar_type>
    void EmbeddingEqualizer<scalar_type>::doAnIteration2Dto1DConnections(double mult){
      const int num_pnts = _embedding_master->numDataPoints();
      const int num_pnts_slave = _embedding_slave->numDataPoints();

      std::vector<scalar_type> slave_bb, master_bb;
      _embedding_slave  -> computeEmbeddingBBox(slave_bb);
      _embedding_master   -> computeEmbeddingBBox(master_bb);

      const auto& master_data = _embedding_master->getContainer();
          auto& slave_data  = _embedding_slave->getContainer();

      for(int i = 0; i < num_pnts_slave; ++i){
        double slave_position = (slave_data[i]-slave_bb[0])/(slave_bb[1]-slave_bb[0]);
        //The master is computed as the weighted average of the connected elements
        double master_position  = 0;
        double weight = 0;
        for(auto e: (*_connections)[i]){
          assert(e.first < num_pnts);
          master_position += (master_data [e.first*2+1]-master_bb[2])/(master_bb[3]-master_bb[2])*e.second;
          weight += e.second;
        }
        if(weight == 0){
          continue;
        }
        master_position /= weight;

        double delta = _previous_update[i]*_momentum + (master_position-slave_position)*_step_size*mult;
        slave_data[i] += delta*(slave_bb [1]-slave_bb [0]);
        _previous_update[i] = delta;
      }
    }

  }
}
#endif 

