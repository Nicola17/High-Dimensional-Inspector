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

#ifndef SCATTERPLOT_CANVAS_H
#define SCATTERPLOT_CANVAS_H

#include <QtGui>
#include <QGLWidget>
#include <memory>
#include "hdi/utils/abstract_log.h"
#include "hdi/visualization/abstract_scatterplot_drawer.h"


namespace hdi{
  namespace viz{

    //! Canvas for the visualization of a scatterplot
    /*!
      Canvas for the visualization of a scatterplot
      \note Provides a QGLWidget with the interaction on a scatterplot
      \author Nicola Pezzotti
    */
    class ScatterplotCanvas: public QGLWidget
    {
       Q_OBJECT
    public:
      typedef float    scalar_type;
      typedef QVector2D  point_type;
      typedef QColor    color_type;

    public:
      ScatterplotCanvas(QWidget *parent = 0, QGLWidget *shareWidget = 0);
      ~ScatterplotCanvas();
      
      //! Return the current log
      utils::AbstractLog* logger()const{return _logger;}
      //! Set a pointer to an existing log
      void setLogger(utils::AbstractLog* logger){_logger = logger;}
      //! Is verbose?
      bool isVerbose()const{return _verbose;}
      //! Set a pointer to an existing log
      void setVerbose(bool verbose){_verbose = verbose;}
      
      //! Set coordinates of the top right corner
      void setTopRightCoordinates(const point_type& tr){_embedding_top_right_corner = tr;}
      //! Set coordinates of the top right corner
      const point_type& topRightCoordinates()const {return _embedding_top_right_corner;}
      //! Set coordinates of the bottom left corner
      void setBottomLeftCoordinates(const point_type& bl){_embedding_bottom_left_corner = bl;}
      //! Set coordinates of the bottom left corner
      const point_type& bottomLeftCoordinates()const {return _embedding_bottom_left_corner;}

      //! Return the size of the embedding visualized on the screen
      point_type embeddingSize()const{return (_embedding_top_right_corner - _embedding_bottom_left_corner);}
      
      //! Set background colors
      void setBackgroundColors(const QColor &top, const QColor &bottom);
      //! Get background colors
      void getBackgroundColors(QColor &top, QColor &bottom)const;

      //! Set selection color
      void setSelectionColor(const QColor selection_color){_selection_color = selection_color;}
      //! Get selection color
      const QColor& selectionColor()const {return _selection_color;}
        
      //! Set drawers for this canvas
      void setDrawers(const std::vector<AbstractScatterplotDrawer*>& drawers){_drawers = drawers;}
      //! Add a single drawer for this canvas
      void addDrawer(AbstractScatterplotDrawer* drawer){_drawers.push_back(drawer);}
      //! Add a single drawer for this canvas
      void removeAllDrawers(){_drawers.clear();}

      //! Take a screenshot and save it to a file
      void saveToFile(std::string filename);

    signals:
      //! Mouse clicked on the widget
      void clicked();
      //! A selection occurred in the widget
      void sgnSelection(point_type bl, point_type tr);
      void sgnRightButtonPressed(point_type pnt);
      void sgnLeftButtonPressed(point_type pnt);
      void sgnRightButtonReleased(point_type pnt);
      void sgnLeftButtonReleased(point_type pnt);
      void sgnMouseMove(point_type pnt);
      void sgnLeftDoubleClick(point_type pnt);
      void sgnRightDoubleClick(point_type pnt);
      void sgnKeyPressed(int key);

    private slots:

    protected:
      //! Initialize OpenGL
      void initializeGL();
      //! Update the embedding
      void paintGL();
      //! Resize the widget
      void resizeGL(int width, int height);
      void mousePressEvent(QMouseEvent *event);
      void mouseMoveEvent(QMouseEvent *event);
      void mouseReleaseEvent(QMouseEvent *event);
      void wheelEvent (QWheelEvent* event);
      void mouseDoubleClickEvent(QMouseEvent* e);
      void keyPressEvent(QKeyEvent* e);

 
     private: 
       point_type getMousePositionInSpace(QPoint pnt)const;

    private:
      color_type _bg_top_color;
      color_type _bg_bottom_color;
      color_type _selection_color;

      point_type _embedding_top_right_corner;
      point_type _embedding_bottom_left_corner;
      scalar_type _near_val;
      scalar_type _far_val;

      QPoint  _selection_first_point;
      QPoint  _selection_second_point;
      bool  _selection_in_progress;

      std::vector<AbstractScatterplotDrawer*> _drawers;

    private:
      utils::AbstractLog* _logger;
      bool _verbose;
    };
  }
}

 #endif
