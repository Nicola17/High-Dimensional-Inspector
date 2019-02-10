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

#include "hdi/visualization/scatterplot_drawer_scalar_attribute.h"
#include <QOpenGLFunctions>
#include "opengl_helpers.h"
#include "hdi/utils/assert_by_exception.h"

#define GLSL(version, shader)  "#version " #version "\n" #shader

namespace hdi{
	namespace viz{

        ScatterplotDrawerScalarAttribute::ScatterplotDrawerScalarAttribute():
			_program(nullptr),
			_vshader(nullptr),
			_fshader(nullptr),
			_selection_color(qRgb(255,150,0)),
			_initialized(false),
            _point_size(20),
			_alpha(.7),
			_z_coord(0),
            _z_coord_selection(-0.5),
            _min_val(0),
            _max_val(1)
		{
			//initialize();

		}

        void ScatterplotDrawerScalarAttribute::initialize(QGLContext* context){
			{//Points color
				const char *vsrc = GLSL(130,
					in highp vec4 pos_attribute;				
                    in highp float scalar_attribute;
                    in highp float flag_attribute;
					uniform highp mat4 matrix_uniform;				
					uniform highp float alpha_uniform;
					uniform highp float z_coord_uniform;
                    uniform highp float min_uniform;
                    uniform highp float max_uniform;
                    uniform highp float pnt_size_uniform;
					out lowp vec4 col;						

                    void main() {
                        float v = (scalar_attribute-min_uniform)/(max_uniform-min_uniform+0.0001);
                        if(v > 1)
                            v = 1;
                        if(v < 0)
                            v = 0;
                        //col = vec4(v,0,0,1);
                        //col.a = alpha_uniform;
                        if((int(0.01+flag_attribute)%2) == 1){
                            col = vec4(1,0.6,0,0.2);
                        }else{
                            col = vec4(0,0,0,0.15);
                        }

						gl_Position = matrix_uniform * pos_attribute;
						gl_Position.z = z_coord_uniform;
                        gl_PointSize = v * pnt_size_uniform;
					}										
				);
				const char *fsrc = GLSL(130,
					in lowp vec4 col;						
					void main() {								
					   gl_FragColor = col;
					}											
				);

				_vshader = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Vertex));
				_fshader = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Fragment));

				_vshader->compileSourceCode(vsrc);
				_fshader->compileSourceCode(fsrc);


				_program = std::unique_ptr<QGLShaderProgram>(new QGLShaderProgram());
				_program->addShader(_vshader.get());
				_program->addShader(_fshader.get());
				_program->link();

				_coords_attribute	= _program->attributeLocation("pos_attribute");
				_flags_attribute	= _program->attributeLocation("flag_attribute");
                _scalar_attribute	= _program->attributeLocation("scalar_attribute");

				_z_coord_uniform	= _program->uniformLocation("z_coord_uniform");
				_alpha_uniform		= _program->uniformLocation("alpha_uniform");
				_matrix_uniform		= _program->uniformLocation("matrix_uniform");
                _min_uniform        = _program->uniformLocation("min_uniform");
                _max_uniform        = _program->uniformLocation("max_uniform");
                _pnt_size_uniform   = _program->uniformLocation("pnt_size_uniform");

				_program->release();
			}

			{//selection
				const char *vsrc = GLSL(130,
					in highp vec4 pos_attribute;				
					in highp float flag_attribute;		
					uniform highp mat4 matrix_uniform;				
					uniform highp float z_coord_uniform;			
					uniform highp float alpha_uniform;			
					uniform highp vec4 color_uniform;			
					out lowp vec4 col;						

					void main() {							
						gl_Position = matrix_uniform * pos_attribute;
						gl_Position.z = -0.5;//z_coord_uniform;
                        if((int(0.01+flag_attribute)%2) == 1){
                            col = color_uniform;
							col.a = 1;
                        }else{
                            col.a = 0;
                        }
					}										
				);
				const char *fsrc = GLSL(130,
					in lowp vec4 col;						
					void main() {								
					   gl_FragColor = col;
					}											
				);

				_vshader_selection = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Vertex));
				_fshader_selection = std::unique_ptr<QGLShader>(new QGLShader(QGLShader::Fragment));

				_vshader_selection->compileSourceCode(vsrc);
				_fshader_selection->compileSourceCode(fsrc);


				_program_selection = std::unique_ptr<QGLShaderProgram>(new QGLShaderProgram());
				_program_selection->addShader(_vshader_selection.get());
				_program_selection->addShader(_fshader_selection.get());
				_program_selection->link();

				_coords_attribute_selection	= _program_selection->attributeLocation("pos_attribute");
				_flags_attribute_selection	= _program_selection->attributeLocation("flag_attribute");

				_matrix_uniform_selection	= _program_selection->uniformLocation("matrix_uniform");
				_color_uniform_selection	= _program_selection->uniformLocation("color_uniform");
				_alpha_uniform_selection	= _program_selection->uniformLocation("alpha_uniform");
				_z_coord_uniform_selection	= _program_selection->uniformLocation("z_coord_uniform");

				_program_selection->release();
			}
			_initialized = true;
		}

        void ScatterplotDrawerScalarAttribute::draw(const point_type& bl, const point_type& tr){
			checkAndThrowLogic(_initialized,"Shader must be initilized");
			ScopedCapabilityEnabler blend_helper(GL_BLEND);
			ScopedCapabilityEnabler depth_test_helper(GL_DEPTH_TEST);
			ScopedCapabilityEnabler point_smooth_helper(GL_POINT_SMOOTH);
			ScopedCapabilityEnabler multisample_helper(GL_MULTISAMPLE);
			ScopedCapabilityEnabler point_size_helper(GL_PROGRAM_POINT_SIZE);
			glDepthMask(GL_FALSE);

			{
				glPointSize(_point_size*1.5);
				_program_selection->bind();
					QMatrix4x4 matrix;
					matrix.ortho(bl.x(), tr.x(), bl.y(), tr.y(), 1, -1);


					_program_selection->setUniformValue(_matrix_uniform, matrix);
					_program_selection->setUniformValue(_alpha_uniform, _alpha);
					_program_selection->setUniformValue(_z_coord_uniform, _z_coord_selection);
					_program_selection->setUniformValue(_color_uniform_selection, _selection_color);

					QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
					glFuncs.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
					_program_selection->enableAttributeArray(_coords_attribute_selection);
					glFuncs.glVertexAttribPointer(_coords_attribute_selection, 2, GL_FLOAT, GL_FALSE, 0, _embedding);

					_program_selection->enableAttributeArray(_flags_attribute_selection);
					glFuncs.glVertexAttribPointer(_flags_attribute_selection, 1, GL_UNSIGNED_INT, GL_FALSE, 0, _flags);

					glDrawArrays(GL_POINTS, 0, _num_points);
				_program_selection->release();
			}

			{
				glPointSize(_point_size);
				_program->bind();
					QMatrix4x4 matrix;
					matrix.ortho(bl.x(), tr.x(), bl.y(), tr.y(), 1, -1);

                    _program->setUniformValue(_min_uniform, _min_val);
                    _program->setUniformValue(_max_uniform, _max_val);
                    _program->setUniformValue(_pnt_size_uniform, _point_size);
					_program->setUniformValue(_matrix_uniform, matrix);
					_program->setUniformValue(_alpha_uniform, _alpha);
					_program->setUniformValue(_z_coord_uniform, _z_coord);

					QOpenGLFunctions glFuncs(QOpenGLContext::currentContext());
					glFuncs.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
					_program->enableAttributeArray(_coords_attribute);
					glFuncs.glVertexAttribPointer(_coords_attribute, 2, GL_FLOAT, GL_FALSE, 0, _embedding);

                    _program->enableAttributeArray(_scalar_attribute);
                    glFuncs.glVertexAttribPointer(_scalar_attribute, 1, GL_FLOAT, GL_FALSE, 0, _attribute);

					_program->enableAttributeArray(_flags_attribute);
					glFuncs.glVertexAttribPointer(_flags_attribute, 1, GL_UNSIGNED_INT, GL_FALSE, 0, _flags);

					glDrawArrays(GL_POINTS, 0, _num_points);
				_program->release();
			}

			glDepthMask(GL_TRUE);
		}

        void ScatterplotDrawerScalarAttribute::setData(const scalar_type* embedding, const scalar_type* attribute, const flag_type* flags, int num_points){
			_embedding = embedding;
            _attribute = attribute;
			_flags = flags;
			_num_points = num_points;
		}

        void ScatterplotDrawerScalarAttribute::updateLimitsFromData(){
            _min_val = std::numeric_limits<scalar_type>::max();
            _max_val = -std::numeric_limits<scalar_type>::max();
            scalar_type avg = 0; //QUICKPAPER
            for(int i = 0; i < _num_points; ++i){
                _min_val = std::min(_attribute[i],_min_val);
                _max_val = std::max(_attribute[i],_max_val);
                avg += _attribute[i];//QUICKPAPER
            }

            //avg /= _num_points;
            //_min_val = avg*0.3;
            //_max_val = std::min<double>(avg*1.3,_max_val);

        }

	}
}
