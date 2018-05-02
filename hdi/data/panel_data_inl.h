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

#ifndef PANEL_DATA_INL
#define PANEL_DATA_INL

#include "hdi/data/panel_data.h"
#include "hdi/data/empty_data.h"
#include "hdi/utils/assert_by_exception.h"
#include <cmath>
#include <limits>
#include <algorithm>

namespace hdi{
  namespace data{

    //! Initialize the class with the number of dimensions already inserted
    template <typename scalar_type>
    void PanelData<scalar_type>::initialize(){
      checkAndThrowLogic(_dimensions.size() != 0,"At least a dimension must be present before the initialization of the technique");
      _initialized = true;
    }
    //! Initialize the class with the number of dimensions already inserted
    template <typename scalar_type>
    void PanelData<scalar_type>::initializeWithEmptyDimensions(int n){
      checkAndThrowLogic(_dimensions.size() == 0,"No dimensions shoudl be already present");
      for(int d = 0; d < n; ++d){
        addDimension(std::shared_ptr<EmptyData>(new EmptyData()));
      }
      _initialized = true;
    }

    //! Add a data-point and returns a handle that can be used to refer to this data-point
    template <typename scalar_type>
    typename PanelData<scalar_type>::handle_type PanelData<scalar_type>::addDataPoint(std::shared_ptr<AbstractData> data_point, const scalar_vector_type& data){
      checkAndThrowLogic(_initialized,"Panel data must be initialized before a data-point can be inserted");
      checkAndThrowLogic(data.size() == numDimensions(),"wrong dimensionality for the data point"); //assert?

      _data_points.push_back(data_point);
      _flags_data_points.push_back(0);
      _data.insert(_data.end(),data.begin(),data.end());

      for(auto& property: _properties){
        property.second.push_back(0);
      }

      return numDataPoints()-1;
    }
    //! Add a dimension and returns a handle that can be used to refer to this dimension
    template <typename scalar_type>
    typename PanelData<scalar_type>::handle_type PanelData<scalar_type>::addDimension(std::shared_ptr<AbstractData> dimension){
      checkAndThrowLogic(!_initialized,"Panel data must not be initialized in order to insert dimensions");
      _dimensions.push_back(dimension);
      _flags_dimensions.push_back(0);
      return numDimensions()-1;
    }

    //! Uninitialize the data and clear the memory
    template <typename scalar_type>
    void PanelData<scalar_type>::clear(){
      _dimensions.clear();
      _data_points.clear();
      _data.clear();
      _flags_data_points.clear();
      _flags_dimensions.clear();
    }

    //! Reserve memory for n data-points
    template <typename scalar_type>
    void PanelData<scalar_type>::reserve(int n){
      checkAndThrowLogic(_initialized,"Panel data must be initialized before a reserve operation can be computed");
      _data_points.reserve(n);
      _data.reserve(n*numDimensions());
    }

    //! Uninitialize the data and clear the memory
    template <typename scalar_type>
    void PanelData<scalar_type>::removeData(){
      checkAndThrowLogic(_initialized,"Panel data must be initialized before a reserve operation can be computed");
      _data_points.clear();
      _data.clear();
      _flags_data_points.clear();
    }

    template <typename scalar_type>
    void PanelData<scalar_type>::requestProperty(std::string name){
      if(!hasProperty(name)){
        _properties[name] = scalar_vector_type(numDataPoints(),0);
      }
    }

    template <typename scalar_type>
    bool PanelData<scalar_type>::hasProperty(std::string name)const{
      return _properties.find(name) != _properties.end();
    }

    template <typename scalar_type>
    void PanelData<scalar_type>::releaseProperty(std::string name){
      auto it = _properties.find(name);
      if(it != _properties.end()){
        _properties.erase(it);
      }
    }

    template <typename scalar_type>
    typename PanelData<scalar_type>::scalar_vector_type& PanelData<scalar_type>::getProperty(std::string name){
      if(!hasProperty(name)){
        throw std::logic_error("PanelData: property not available");
      }
      return _properties[name];
    }

    template <typename scalar_type>
    const typename PanelData<scalar_type>::scalar_vector_type& PanelData<scalar_type>::getProperty(std::string name)const{
      if(!hasProperty(name)){
        throw std::logic_error("PanelData: property not available");
      }
      return _properties.find(name)->second;
    }

    template <typename scalar_type>
    void PanelData<scalar_type>::getAvailableProperties(std::vector<std::string>& property_names)const{
      for(const auto& p: _properties){
        property_names.push_back(p.first);
      }
    }


    template <typename scalar_type>
    void PanelData<scalar_type>::requestDimProperty(std::string name){
      checkAndThrowLogic(_initialized,"Panel data must be initialized before requesting a dimension property");
      if(!hasDimProperty(name)){
        _dim_properties[name] = scalar_vector_type(numDimensions(),0);
      }
    }

    template <typename scalar_type>
    bool PanelData<scalar_type>::hasDimProperty(std::string name)const{
      return _dim_properties.find(name) != _dim_properties.end();
    }

    template <typename scalar_type>
    void PanelData<scalar_type>::releaseDimProperty(std::string name){
      checkAndThrowLogic(_initialized,"Panel data must be initialized before releasing a dimension property");
      auto it = _dim_properties.find(name);
      if(it != _dim_properties.end()){
        _dim_properties.erase(it);
      }
    }

    template <typename scalar_type>
    typename PanelData<scalar_type>::scalar_vector_type& PanelData<scalar_type>::getDimProperty(std::string name){
      if(!hasDimProperty(name)){
        throw std::logic_error("PanelData: property not available");
      }
      return _dim_properties[name];
    }

    template <typename scalar_type>
    const typename PanelData<scalar_type>::scalar_vector_type& PanelData<scalar_type>::getDimProperty(std::string name)const{
      if(!hasDimProperty(name)){
        throw std::logic_error("PanelData: property not available");
      }
      return _dim_properties.find(name)->second;
    }

    template <typename scalar_type>
    void PanelData<scalar_type>::getAvailableDimProperties(std::vector<std::string>& property_names)const{
      for(const auto& p: _dim_properties){
        property_names.push_back(p.first);
      }
    }




    template <typename scalar_type>
    void newPanelDataFromIndexes(const PanelData<scalar_type>& ori_panel_data, PanelData<scalar_type>& dst_panel_data, const std::vector<unsigned int>& idxes){
      auto& dimensions = ori_panel_data.getDimensions();
      auto& data_points = ori_panel_data.getDataPoints();
      for(auto& d: dimensions){
        dst_panel_data.addDimension(d);
      }
      dst_panel_data.initialize();

      for(auto idx: idxes){
        std::vector<scalar_type> data(dimensions.size());
        for(int d = 0; d < dimensions.size(); ++d){
          data[d] = ori_panel_data.dataAt(idx,d);
        }
        dst_panel_data.addDataPoint(data_points[idx],data);
      }

      //properties
      {
        std::vector<std::string> names;
        ori_panel_data.getAvailableProperties(names);
        for(const auto& name: names){
          dst_panel_data.requestProperty(name);
          const auto& ori_property = ori_panel_data.getProperty(name);
              auto& dst_property = dst_panel_data.getProperty(name);
          for(int i = 0; i < idxes.size(); ++i){
            dst_property[i] = ori_property[idxes[i]];
          }
        }
      }

      //dim
      {
        std::vector<std::string> names;
        ori_panel_data.getAvailableDimProperties(names);
        for(const auto& name: names){
          dst_panel_data.requestDimProperty(name);
          dst_panel_data.getDimProperty(name) = ori_panel_data.getDimProperty(name);
        }
      }

    }

    template <typename scalar_type>
    void zScoreNormalization(PanelData<scalar_type>& panel_data){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();
      auto& data = panel_data.getData();

      for(int d = 0; d < num_dimensions; ++d){
        double avg(0);
        double avg_sq(0);
        for(int p = 0; p < num_data_points; ++p){
          avg   += data[p*num_dimensions+d];
          avg_sq  += data[p*num_dimensions+d]*data[p*num_dimensions+d];
        }
        avg /= num_data_points;
        avg_sq /= num_data_points;
        double std_dev = std::sqrt(avg_sq-avg*avg);
        for(int p = 0; p < num_data_points; ++p){
          data[p*num_dimensions+d] = (data[p*num_dimensions+d]-avg)/((std_dev!=0)?std_dev:1);
        }
      }
    }
    template <typename scalar_type>
    void minMaxNormalization(PanelData<scalar_type>& panel_data){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();
      auto& data = panel_data.getData();

      for(int d = 0; d < num_dimensions; ++d){
        double min(std::numeric_limits<double>::max());
        double max(-std::numeric_limits<double>::max());
        for(int p = 0; p < num_data_points; ++p){
          min  = std::min<double>(min,data[p*num_dimensions+d]);
          max  = std::max<double>(max,data[p*num_dimensions+d]);
        }
        for(int p = 0; p < num_data_points; ++p){
          if(max!=min){
            data[p*num_dimensions+d] = (data[p*num_dimensions+d]-min)/(max-min);
          }else{
            data[p*num_dimensions+d] = 0.5;
          }
        }
      }
    }

    template <typename scalar_type>
    void transposePanelData(const PanelData<scalar_type>& panel_data, PanelData<scalar_type>& transpose_panel_data){
      transpose_panel_data.clear();
      for(int i = 0; i < panel_data.getDataPoints().size(); ++i){
        transpose_panel_data.addDimension(panel_data.getDataPoints()[i]);
      }
      transpose_panel_data.initialize();

      for(int i = 0; i < panel_data.numDimensions(); ++i){
        std::vector<scalar_type> input(transpose_panel_data.numDimensions(),0);
        for(int f = 0; f < panel_data.numDataPoints(); ++f){
          input[f] = panel_data.dataAt(f,i);
        }
        transpose_panel_data.addDataPoint(panel_data.getDimensions()[i],input);
      }
    }

    template <typename scalar_type>
    double computePanelDataSparsity(const PanelData<scalar_type>& panel_data){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();
      auto& data = panel_data.getData();

      double sparsity = 0;
      for(int d = 0; d < num_dimensions; ++d){
        for(int p = 0; p < num_data_points; ++p){
          if(data[p*num_dimensions+d] == scalar_type(0)){
            ++sparsity;
          }
        }
      }
      return sparsity/num_data_points/num_dimensions;
    }

    template <typename scalar_type>
    void getMaxPerDimension(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& max){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();

      max.clear();
      max.resize(num_dimensions,-std::numeric_limits<scalar_type>::max());

      for(int d = 0; d < num_dimensions; ++d){
        for(int p = 0; p < num_data_points; ++p){
          max[d] = std::max<scalar_type>(max[d],panel_data.dataAt(p,d));
        }
      }
    }

    template <typename scalar_type>
    void getMinPerDimension(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& min){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();

      min.clear();
      min.resize(num_dimensions,std::numeric_limits<scalar_type>::max());

      for(int d = 0; d < num_dimensions; ++d){
        for(int p = 0; p < num_data_points; ++p){
          min[d] = std::min<scalar_type>(min[d],panel_data.dataAt(p,d));
        }
      }
    }

    template <typename scalar_type>
    void computeMean(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& mean){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();
      auto& data = panel_data.getData();

      mean.clear();
      mean.resize(num_dimensions,0);

      if(panel_data.hasProperty("weights")){
        auto& weights = panel_data.getProperty("weights");
        scalar_type sum_weights = 0;
        for(int p = 0; p < num_data_points; ++p){
          sum_weights += weights[p];
          for(int d = 0; d < num_dimensions; ++d){
            mean[d] += data[p*num_dimensions+d]*weights[p];

          }
        }
        for(int d = 0; d < num_dimensions; ++d){
          mean[d] /= sum_weights;
        }
      }else{
        for(int p = 0; p < num_data_points; ++p){
          for(int d = 0; d < num_dimensions; ++d){
            mean[d] += data[p*num_dimensions+d];
          }
        }
        for(int d = 0; d < num_dimensions; ++d){
          mean[d] /= num_data_points;
        }
      }
    }

    template <typename scalar_type>
    void computeSelectionMean(const PanelData<scalar_type>& panel_data, std::vector<scalar_type>& mean){
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();
      const auto& flags = panel_data.getFlagsDataPoints();
      auto& data = panel_data.getData();

      mean.clear();
      mean.resize(num_dimensions,0);

      if(panel_data.hasProperty("weights")){
        auto& weights = panel_data.getProperty("weights");
        scalar_type sum_weights = 0;
        for(int p = 0; p < num_data_points; ++p){
          if((flags[p]&PanelData<scalar_type>::Selected) == PanelData<scalar_type>::Selected){
            sum_weights += weights[p];
            for(int d = 0; d < num_dimensions; ++d){
              mean[d] += data[p*num_dimensions+d]*weights[p];

            }
          }
        }
        for(int d = 0; d < num_dimensions; ++d){
          mean[d] /= sum_weights;
        }
      }else{
        int selected = 0;
        for(int p = 0; p < num_data_points; ++p){
          if((flags[p]&PanelData<scalar_type>::Selected) == PanelData<scalar_type>::Selected){
            for(int d = 0; d < num_dimensions; ++d){
              mean[d] += data[p*num_dimensions+d];
            }
            ++selected;
          }
        }
        for(int d = 0; d < num_dimensions; ++d){
          mean[d] /= selected;
        }
      }
    }

    template <typename scalar_type>
    void computeWeightedMean(const PanelData<scalar_type>& panel_data, const std::vector<scalar_type>& weights, std::vector<scalar_type>& mean) {
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();

      auto& data = panel_data.getData();

      mean.resize(num_dimensions);

      for (int d = 0; d < num_dimensions; ++d) {
        scalar_type sum_weights = 0;
        for (int p = 0; p < num_data_points; ++p) {
          sum_weights += weights[p];
          mean[d] += data[p*num_dimensions + d] * weights[p];
        }
        mean[d] /= sum_weights;
      }
    }

    template <typename scalar_type>
    void computeWeightedStddev(const PanelData <scalar_type>& panel_data, const std::vector<scalar_type>& weights, std::vector<scalar_type>& mean, std::vector<scalar_type>& std_dev) {
      const unsigned int num_dimensions = panel_data.numDimensions();
      const unsigned int num_data_points = panel_data.numDataPoints();

      auto& data = panel_data.getData();

      std_dev.resize(num_dimensions);

      for (int d = 0; d < num_dimensions; ++d) {
        scalar_type sum_weights = 0;
        for (int p = 0; p < num_data_points; ++p) {
          sum_weights += weights[p];
          scalar_type deviation = (data[p*num_dimensions + d] - mean[d]);
          std_dev[d] += deviation * deviation * weights[p];
        }
        std_dev[d] /= sum_weights;
        std_dev[d] = sqrt(std_dev[d]);
      }
    }

  }
}
#endif
