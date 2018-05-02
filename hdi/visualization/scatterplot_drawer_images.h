#ifndef SCATTERPLOT_DRAWER_IMAGES_H
#define SCATTERPLOT_DRAWER_IMAGES_H

#include <QGLShaderProgram>
#include <QGLShader>
#include <QColor>
#include <stdint.h>
#include <memory>
#include <array>
#include "hdi/visualization/abstract_scatterplot_drawer.h"

namespace hdi{
  namespace viz{

  //QUICK AND DIRT
    class ScatterplotDrawerImages: public AbstractScatterplotDrawer{
    public:
      ScatterplotDrawerImages();
      virtual void initialize(QGLContext* context);
      virtual void draw(const point_type& bl, const point_type& tr);

    private:
      void initializePointView();

    public:
      std::vector<QImage> _images; //OMG
      scalar_type* _embedding;
      scalar_type* _weight;
      uint32_t* _flags;
      unsigned int _num_points;
      scalar_type _red_factor;
      scalar_type _samplig_factor;
      scalar_type _min_val;
      scalar_type _max_val;
      std::vector<bool> _visible;

    private:
      std::vector<GLuint> _textures_id;
    };
  }
}

#endif
