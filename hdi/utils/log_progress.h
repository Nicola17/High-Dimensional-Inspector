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

#ifndef LOG_PROGRESS_H
#define LOG_PROGRESS_H

#include "hdi/utils/abstract_log.h"
#include <string>
#include <omp.h>

namespace hdi{
  namespace utils{

    //! shows log on the standar output stream
    class LogProgress{
    public:
      LogProgress(AbstractLog* log):_log(log),_current_step(0),_current_tick(0){}
      //! set the number of steps made by the algorithm
      void setNumSteps(int num_steps){_num_steps = num_steps;}
      //! set the number of ticks made by the progress
      void setNumTicks(int num_ticks){_num_ticks = num_ticks-1;}
      //! set name
      void setName(std::string name){_name = name;}
      //! start
      void start();
      //! end logging
      void finish();

      //! make a step
      inline void step(){
        if(_log == nullptr){return;}
      #pragma omp atomic
        ++_current_step;
        //only the first thread is allowed to generate a tick
        if(omp_get_thread_num() == 0){
          double perc = double(_current_step)/_num_steps;
          int tick = perc*(_num_ticks+1);
          if(tick > _current_tick){
            ++_current_tick;
            std::stringstream ss;
            ss << perc*100 << "%";
            _log->display(ss.str(),true);
          }
        }
      }

    private:
      AbstractLog* _log;
      int _num_steps;
      int _current_step;
      int _num_ticks;
      int _current_tick;
      std::string _name;
    };
  }
}

#endif // COUT_LOG_H
