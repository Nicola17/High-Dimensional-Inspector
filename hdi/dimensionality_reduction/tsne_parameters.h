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

#ifndef TSNE_PARAMETERS_H
#define TSNE_PARAMETERS_H

namespace hdi {
  namespace dr {
    //! Parameters used for the initialization of the algorithm
    class TsneParameters {
    public:
      TsneParameters() :
        _seed(-1),
        _rngRange(0.1f),
        _embedding_dimensionality(2),
        _minimum_gain(0.1),
        _eta(200),
        _momentum(0.2),
        _final_momentum(0.5),
        _mom_switching_iter(250),
        _exaggeration_factor(4),
        _remove_exaggeration_iter(250),
        _exponential_decay_iter(150)
      { }

      int _seed;
      float _rngRange;
      int _embedding_dimensionality;

      double _minimum_gain;
      double _eta;                                //! constant multiplicator of the gradient
      double _momentum;
      double _final_momentum;
      double _mom_switching_iter;                 //! momentum switching iteration
      double _exaggeration_factor;                //! exaggeration factor for the attractive forces. Note: it shouldn't be too high when few points are used
      unsigned int _remove_exaggeration_iter;     //! iterations with complete exaggeration of the attractive forces
      unsigned int _exponential_decay_iter;       //! iterations required to remove the exaggeration using an exponential decay
    };
  }
}

#endif
