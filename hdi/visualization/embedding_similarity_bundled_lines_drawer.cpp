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

#include "hdi/visualization/embedding_similarity_bundled_lines_drawer.h"
#include <QOpenGLFunctions>
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
  namespace viz{

    EmbeddingSimilarityBundledLinesDrawer::EmbeddingSimilarityBundledLinesDrawer():
      _color(qRgb(0,0,0)),
      _initialized(false),
      _alpha(0.15),
      _z_coord(0),
      _pnts_per_line(10),
      _fmc_src(nullptr),
      _fmc_dst(nullptr)
    {
      //initialize();

    }

    void EmbeddingSimilarityBundledLinesDrawer::initialize(QGLContext* context){
      _initialized = true;
    }

    void EmbeddingSimilarityBundledLinesDrawer::draw(const point_type& bl, const point_type& tr){
      checkAndThrowLogic(_initialized,"Shader must be initilized");
      //movePoints();
      movePointsSGD();
      ScopedCapabilityEnabler blend_helper(GL_BLEND);
      ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
      ScopedCapabilityEnabler point_smooth_helper(GL_POINT_SMOOTH);
      ScopedCapabilityEnabler multisample_helper(GL_MULTISAMPLE);
      ScopedCapabilityEnabler point_size_helper(GL_PROGRAM_POINT_SIZE);
      glDepthMask(GL_FALSE);
      for(int l = 0; l < _lines.size(); ++l){
        const auto& line = _lines[l];
        glBegin(GL_LINE_STRIP);
          glColor4f(_color.redF(),_color.greenF(),_color.blueF(),_alpha_per_line.size()?_alpha_per_line[l]:_alpha);
          for(int i = 0; i < _pnts_per_line; ++i){
            glVertex3f  (_cntr_pnts[(l*_pnts_per_line+i)*2], _cntr_pnts[(l*_pnts_per_line+i)*2+1], _z_coord); //vertex 1
          }
        glEnd();
      }
    }

    void EmbeddingSimilarityBundledLinesDrawer::setData(const scalar_type* embedding_src, const scalar_type* embedding_dst){
      _embedding_src = embedding_src;
      _embedding_dst = embedding_dst;
    }

    void EmbeddingSimilarityBundledLinesDrawer::setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines){
      _lines = lines;
      _alpha_per_line.clear();
      _cntr_pnts.resize(lines.size()*_pnts_per_line*2);
      _velocity.resize(lines.size()*_pnts_per_line*2);

      for(int l = 0; l < _lines.size(); ++l){
        const auto& line = _lines[l];
        scalar_type x_step = (_embedding_dst[line.second*2]   - _embedding_src[line.first*2])   / (_pnts_per_line - 1);
        scalar_type y_step = (_embedding_dst[line.second*2+1] - _embedding_src[line.first*2+1]) / (_pnts_per_line - 1);
        for(int i = 0; i < _pnts_per_line; ++i){
          _cntr_pnts[(l*_pnts_per_line+i)*2]   = _embedding_src[line.first*2]   + x_step * i;
          _cntr_pnts[(l*_pnts_per_line+i)*2+1]   = _embedding_src[line.first*2+1] + y_step * i;
        }
      }
    }
    void EmbeddingSimilarityBundledLinesDrawer::setLines(const std::vector<std::pair<uint32_t,uint32_t>>& lines, const std::vector<float>& alpha_per_line){
      assert(lines.size() == alpha_per_line.size());
      _lines = lines;
      _alpha_per_line = alpha_per_line;
    }

    void EmbeddingSimilarityBundledLinesDrawer::movePoints(){
      //Resetting the endpoints
      for(int l = 0; l < _lines.size(); ++l){
        const auto& line = _lines[l];
        _cntr_pnts[(l*_pnts_per_line)*2]     = _embedding_src[line.first*2];
        _cntr_pnts[(l*_pnts_per_line)*2+1]     = _embedding_src[line.first*2+1];
        _cntr_pnts[((l+1)*_pnts_per_line)*2-2]   = _embedding_dst[line.second*2];
        _cntr_pnts[((l+1)*_pnts_per_line)*2+1-2] = _embedding_dst[line.second*2+1];
      }

      double dt = 0.15;
      double attr = 0.98;
      //updating velocities
      for(int l0 = 0; l0 < _lines.size(); ++l0){
        for(int i = 1; i < _pnts_per_line-1; ++i){
          //spring forces
          double k = 1;
          double kp = 1*k; //TODO segment length
          double x_spring =  _cntr_pnts[(l0*_pnts_per_line+i-1)*2]
                    +  _cntr_pnts[(l0*_pnts_per_line+i+1)*2]
                    -2*_cntr_pnts[(l0*_pnts_per_line+i)*2];
          double y_spring =  _cntr_pnts[(l0*_pnts_per_line+i-1)*2+1]
                    +  _cntr_pnts[(l0*_pnts_per_line+i+1)*2+1]
                    -2*_cntr_pnts[(l0*_pnts_per_line+i)*2+1];
          x_spring *= kp;
          y_spring *= kp;

          double x_attract = 0;
          double y_attract = 0;
          double ke = 1/_lines.size();
          for(int l1 = 0; l1 < _lines.size(); ++l1){

            double similarity_src = 0;
            double similarity_dst = 0;

            auto it_src = (*_fmc_src)[_lines[l0].first].find(_lines[l1].first);
            if(it_src != (*_fmc_src)[_lines[l0].first].end()){
              similarity_src = it_src->second;
            }
            auto it_dst = (*_fmc_dst)[_lines[l0].second].find(_lines[l1].second);
            if(it_dst != (*_fmc_dst)[_lines[l0].second].end()){
              similarity_dst = it_dst->second;
            }

            double weighted_similarity =  similarity_src * (double(i)/(_pnts_per_line-1))
                           +  similarity_dst * (1-double(i)/(_pnts_per_line-1));

            scalar_type x_l1 = _cntr_pnts[(l1*_pnts_per_line+i)*2]   - _cntr_pnts[(l0*_pnts_per_line+i)*2];
            scalar_type y_l1 = _cntr_pnts[(l1*_pnts_per_line+i)*2+1] - _cntr_pnts[(l0*_pnts_per_line+i)*2+1];

            x_attract += ke*x_l1*weighted_similarity;
            y_attract += ke*y_l1*weighted_similarity;
          }

          //Computing dv
          _velocity[(l0*_pnts_per_line+i)*2]    += dt * (x_spring+x_attract);
          _velocity[(l0*_pnts_per_line+i)*2+1]  += dt * (y_spring+y_attract);

          //Applying attrition
          _velocity[(l0*_pnts_per_line+i)*2]    *= attr;
          _velocity[(l0*_pnts_per_line+i)*2+1]  *= attr;
        }
      }
      //updating positions
      for(int l0 = 0; l0 < _lines.size(); ++l0){
        for(int i = 1; i < _pnts_per_line-1; ++i){
          _cntr_pnts[(l0*_pnts_per_line+i)*2]    += dt * _velocity[(l0*_pnts_per_line+i)*2];
          _cntr_pnts[(l0*_pnts_per_line+i)*2+1]  += dt * _velocity[(l0*_pnts_per_line+i)*2+1];
        }
      }

    }


    void EmbeddingSimilarityBundledLinesDrawer::movePointsSGD(){
      //Resetting the endpoints
      for(int l = 0; l < _lines.size(); ++l){
        const auto& line = _lines[l];
        _cntr_pnts[(l*_pnts_per_line)*2]     = _embedding_src[line.first*2];
        _cntr_pnts[(l*_pnts_per_line)*2+1]     = _embedding_src[line.first*2+1];
        _cntr_pnts[((l+1)*_pnts_per_line)*2-2]   = _embedding_dst[line.second*2];
        _cntr_pnts[((l+1)*_pnts_per_line)*2+1-2] = _embedding_dst[line.second*2+1];
      }

      double dt = 0.15;
      double attr = 0.98;

      int n_samples = 50;
      //updating velocities
      for(int l0 = 0; l0 < _lines.size(); ++l0){
        for(int i = 1; i < _pnts_per_line-1; ++i){
          //spring forces
          double k = 1;
          double kp = 1*k; //TODO segment length
          double x_spring =  _cntr_pnts[(l0*_pnts_per_line+i-1)*2]
                    +  _cntr_pnts[(l0*_pnts_per_line+i+1)*2]
                    -2*_cntr_pnts[(l0*_pnts_per_line+i)*2];
          double y_spring =  _cntr_pnts[(l0*_pnts_per_line+i-1)*2+1]
                    +  _cntr_pnts[(l0*_pnts_per_line+i+1)*2+1]
                    -2*_cntr_pnts[(l0*_pnts_per_line+i)*2+1];
          x_spring *= kp;
          y_spring *= kp;

          double x_attract = 0;
          double y_attract = 0;
          double ke = 0.5*100/n_samples;
          for(int s = 0; s < n_samples; ++s){
            int l1 = rand()%_lines.size();
            double similarity_src = 0;
            double similarity_dst = 0;

            auto it_src = (*_fmc_src)[_lines[l0].first].find(_lines[l1].first);
            if(_lines[l0].first == _lines[l1].first){
              similarity_src = 0.5;
            }else if(it_src != (*_fmc_src)[_lines[l0].first].end()){
              similarity_src = it_src->second;
            }
            auto it_dst = (*_fmc_dst)[_lines[l0].second].find(_lines[l1].second);
            if(_lines[l0].second == _lines[l1].second){
              similarity_dst = 0.5;
            }else if(it_dst != (*_fmc_dst)[_lines[l0].second].end()){
              similarity_dst = it_dst->second;
            }

            double weighted_similarity =  similarity_src * (1-double(i)/(_pnts_per_line-1))
                           +  similarity_dst * (double(i)/(_pnts_per_line-1));

            scalar_type x_l1 = _cntr_pnts[(l1*_pnts_per_line+i)*2]   - _cntr_pnts[(l0*_pnts_per_line+i)*2];
            scalar_type y_l1 = _cntr_pnts[(l1*_pnts_per_line+i)*2+1] - _cntr_pnts[(l0*_pnts_per_line+i)*2+1];

            x_attract += ke*x_l1*weighted_similarity;
            y_attract += ke*y_l1*weighted_similarity;
          }

          //Computing dv
          _velocity[(l0*_pnts_per_line+i)*2]    += dt * (x_spring+x_attract);
          _velocity[(l0*_pnts_per_line+i)*2+1]  += dt * (y_spring+y_attract);

          //Applying attrition
          _velocity[(l0*_pnts_per_line+i)*2]    *= attr;
          _velocity[(l0*_pnts_per_line+i)*2+1]  *= attr;
        }
      }
      //updating positions
      for(int l0 = 0; l0 < _lines.size(); ++l0){
        for(int i = 1; i < _pnts_per_line-1; ++i){
          _cntr_pnts[(l0*_pnts_per_line+i)*2]    += dt * _velocity[(l0*_pnts_per_line+i)*2];
          _cntr_pnts[(l0*_pnts_per_line+i)*2+1]  += dt * _velocity[(l0*_pnts_per_line+i)*2+1];
        }
      }

    }


  }
}
