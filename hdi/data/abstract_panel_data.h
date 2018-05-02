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

#ifndef ABSTRACT_PANEL_DATA_H
#define ABSTRACT_PANEL_DATA_H

#include <memory>
#include <vector>
#include "hdi/data/abstract_data.h"

namespace hdi{
  namespace data{

    //! Abstract class that represents a generic panel data
    /*!
      Abstract class that represents a generic panel data.
      \author Nicola Pezzotti
    */

    class AbstractPanelData{
    public:
      typedef uint32_t          handle_type;
      typedef uint32_t          flag_type;
      typedef std::vector<flag_type>    flag_vector_type;
      typedef std::vector<std::shared_ptr<AbstractData> > data_ptr_vector_type;

      enum Flags { None = 0, Selected = 1, Fixed = 2, Disabled = 4};

    public:
      AbstractPanelData(){}
      virtual ~AbstractPanelData(){}

      virtual int numDataPoints()const = 0;
      virtual int numDimensions()const = 0;
      virtual const data_ptr_vector_type&  getDataPoints()const = 0;
      virtual const data_ptr_vector_type& getDimensions()const = 0;

      virtual const flag_vector_type&  getFlagsDataPoints()const = 0;
      virtual const flag_vector_type& getFlagsDimensions()const = 0;
      virtual flag_vector_type& getFlagsDataPoints() = 0;
      virtual flag_vector_type& getFlagsDimensions() = 0;

      virtual double dataAt(unsigned int data_point, unsigned int dimension)const=0;
    };

  }
}
#endif
