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

#ifndef EMBEDDING_EQUALIZER_H
#define EMBEDDING_EQUALIZER_H

#include <vector>
#include <stdint.h>
#include "hdi/utils/assert_by_exception.h"
#include "hdi/utils/abstract_log.h"
#include "hdi/data/embedding.h"
#include <unordered_map>

namespace hdi{
  namespace dr{

    template <typename scalar_type = float>
    class EmbeddingEqualizer{
    public:
      typedef std::vector<scalar_type> scalar_vector_type;
      typedef uint32_t data_handle_type;
      typedef std::vector<std::unordered_map<uint32_t,uint32_t>> connections_type;

    public:
      class Params{
      public:
        Params();
      };


    public:
      EmbeddingEqualizer();

      void initialize(data::Embedding<scalar_type>* embedding_master, data::Embedding<scalar_type>* embedding_slave);
      void initialize(data::Embedding<scalar_type>* embedding_master, data::Embedding<scalar_type>* embedding_slave, const connections_type* connections);

          scalar_type& step_size()   {return _step_size;}
      const scalar_type& step_size()const{return _step_size;}
          scalar_type& momentum()    {return _momentum;}
      const scalar_type& momentum()const {return _momentum;}


      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      //! Do an iteration of the gradient descent
      void doAnIteration(double mult = 1);


    private:
      void doAnIteration1Dto1D(double mult);
      void doAnIteration1Dto2D(double mult);
      void doAnIteration2Dto1D(double mult);
      void doAnIteration1Dto1DConnections(double mult);
      void doAnIteration1Dto2DConnections(double mult);
      void doAnIteration2Dto1DConnections(double mult);

    private:
      data::Embedding<scalar_type>* _embedding_master;
      data::Embedding<scalar_type>* _embedding_slave;
      const connections_type*     _connections;
      std::vector<scalar_type>    _previous_update;

      scalar_type _step_size;
      scalar_type _momentum;

      bool _initialized;
      utils::AbstractLog* _logger;
  
    };
  }
}
#endif 
