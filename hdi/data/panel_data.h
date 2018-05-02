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

#ifndef PANEL_DATA_H
#define PANEL_DATA_H

#include "hdi/data/abstract_data.h"
#include "hdi/data/abstract_panel_data.h"
#include <vector>
#include <memory>
#include <map>

namespace hdi{
  namespace data{
    //! Class that describes a panel data
    /*!
      Class that describes a panel data.
      Rows (Data-Points) and Columns (Dimensions) are described by a generic Abstract Data.
      \author Nicola Pezzotti
      \note Dimensions must be inserted before the initialization of the class
      \note Data-poinst must be inserted after the initialization of the class
    */
    template <typename scalar_type>
    class PanelData: public AbstractPanelData{
      public:
      typedef uint32_t          handle_type;
      typedef uint32_t          flag_type;
      typedef std::vector<scalar_type>  scalar_vector_type;
      typedef std::vector<flag_type>    flag_vector_type;
      typedef std::vector<std::shared_ptr<AbstractData> > data_ptr_vector_type;

    public:
      PanelData():_initialized(false){}

      //! Initialize the class with the number of dimensions already inserted
      void initialize();
      //! Initialize the class with a given number of dimensions
      void initializeWithEmptyDimensions(int n);
      //! Check if the class is initialized
      bool isInitialized(){return _initialized;}

      //! Add a data-point and returns a handle that can be used to refer to this data-point
      virtual handle_type addDataPoint(std::shared_ptr<AbstractData> data_point, const scalar_vector_type& data);
      //! Add a dimension and returns a handle that can be used to refer to this dimension
      handle_type addDimension(std::shared_ptr<AbstractData> dimension);

      //!Return the number of data-points inserted
      virtual int numDataPoints()const{return static_cast<int>(_data_points.size());}
      //!Return the number of dimensions inserted
      int numDimensions()const{return static_cast<int>(_dimensions.size());}

      //! Uninitialize the data and clear the memory
      void clear();
      //! Reserve memory for n data-points
      void reserve(int n);
      //! Remove data
      void removeData();

      //Getter
      const data_ptr_vector_type&  getDimensions()const    {return _dimensions;}
      const data_ptr_vector_type&  getDataPoints()const    {return _data_points;}
      const scalar_vector_type&  getData()const        {return _data;}
      const flag_vector_type&    getFlagsDimensions()const  {return _flags_dimensions;}
      const flag_vector_type&    getFlagsDataPoints()const  {return _flags_data_points;}

      data_ptr_vector_type&  getDimensions()    {return _dimensions;}
      data_ptr_vector_type&  getDataPoints()    {return _data_points;}
      scalar_vector_type&    getData()      {return _data;}
      flag_vector_type&    getFlagsDimensions(){return _flags_dimensions;}
      flag_vector_type&    getFlagsDataPoints(){return _flags_data_points;}

      virtual double dataAt(unsigned int data_point, unsigned int dimension)const{return _data[data_point*numDimensions() + dimension];}


      void requestProperty(std::string name);
      bool hasProperty(std::string name)const;
      void releaseProperty(std::string name);
      scalar_vector_type& getProperty(std::string name);
      const scalar_vector_type& getProperty(std::string name)const;
      void getAvailableProperties(std::vector<std::string>& property_names)const;


      void requestDimProperty(std::string name);
      bool hasDimProperty(std::string name)const;
      void releaseDimProperty(std::string name);
      scalar_vector_type& getDimProperty(std::string name);
      const scalar_vector_type& getDimProperty(std::string name)const;
      void getAvailableDimProperties(std::vector<std::string>& property_names)const;


    private:
      bool _initialized;

      data_ptr_vector_type _dimensions;
      data_ptr_vector_type _data_points;
      scalar_vector_type _data;
      flag_vector_type _flags_dimensions;
      flag_vector_type _flags_data_points;
      std::map<std::string,scalar_vector_type> _properties;
      std::map<std::string,scalar_vector_type> _dim_properties;

    };

    template <typename scalar_type>
    void newPanelDataFromIndexes(const PanelData<scalar_type>& ori_panel_data, PanelData<scalar_type>& dst_panel_data, const std::vector<unsigned int>& idxes);

    template <typename scalar_type>
    void zScoreNormalization(PanelData<scalar_type>& panel_data);

    template <typename scalar_type>
    void minMaxNormalization(PanelData<scalar_type>& panel_data);

    template <typename scalar_type>
    void transposePanelData(const PanelData<scalar_type>& panel_data, PanelData<scalar_type>& transpose_panel_data);

    template <typename scalar_type>
    double computePanelDataSparsity(const PanelData<scalar_type>& panel_data);

    template <typename scalar_type>
    void getMaxPerDimension(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& max);

    template <typename scalar_type>
    void getMinPerDimension(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& min);

    template <typename scalar_type>
    void computeMean(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& mean);

    template <typename scalar_type>
    void computeSelectionMean(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& mean);

    template <typename scalar_type>
    void computeWeightedMean(const PanelData<scalar_type>& panel_data, const std::vector<scalar_type>& weights, std::vector<scalar_type>& mean);

    template <typename scalar_type>
    void computeWeightedStddev(const PanelData <scalar_type>& panel_data, const std::vector<scalar_type>& weights, std::vector<scalar_type>& mean, std::vector<scalar_type>& std_dev);
  }
}

#endif
