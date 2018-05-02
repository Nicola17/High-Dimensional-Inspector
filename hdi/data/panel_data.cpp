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

#include "panel_data_inl.h"

namespace hdi{
  namespace data{
    template class PanelData<float>;
    template class PanelData<double>;
    template void newPanelDataFromIndexes(const PanelData<float>& ori_panel_data, PanelData<float>& dst_panel_data, const std::vector<unsigned int>& idxes);
    template void newPanelDataFromIndexes(const PanelData<double>& ori_panel_data, PanelData<double>& dst_panel_data, const std::vector<unsigned int>& idxes);
    template void zScoreNormalization(PanelData<float>& panel_data);
    template void zScoreNormalization(PanelData<double>& panel_data);
    template void minMaxNormalization(PanelData<float>& panel_data);
    template void minMaxNormalization(PanelData<double>& panel_data);
    template void transposePanelData(const PanelData<float>& panel_data, PanelData<float>& transpose_panel_data);
    template void transposePanelData(const PanelData<double>& panel_data, PanelData<double>& transpose_panel_data);
    template double computePanelDataSparsity(const PanelData<double>& panel_data);
    template double computePanelDataSparsity(const PanelData<float>& panel_data);
    template void getMaxPerDimension(const PanelData<float>& panel_data, std::vector<float>& max);
    template void getMaxPerDimension(const PanelData<double>& panel_data, std::vector<double>& max);
    template void getMinPerDimension(const PanelData<float>& panel_data, std::vector<float>& min);
    template void getMinPerDimension(const PanelData<double>& panel_data, std::vector<double>& min);
    template void computeMean(const PanelData<float>& panel_data, std::vector<float>& mean);
    template void computeMean(const PanelData<double>& panel_data, std::vector<double>& mean);
    template void computeSelectionMean(const PanelData<float>& panel_data, std::vector<float>& mean);
    template void computeSelectionMean(const PanelData<double>& panel_data, std::vector<double>& mean);
    template void computeWeightedMean(const PanelData<float>& panel_data, const std::vector<float>& weights, std::vector<float>& mean);
    template void computeWeightedMean(const PanelData<double>& panel_data, const std::vector<double>& weights, std::vector<double>& mean);
    template void computeWeightedStddev(const PanelData <float>& panel_data, const std::vector<float>& weights, std::vector<float>& mean, std::vector<float>& std_dev);
    template void computeWeightedStddev(const PanelData <double>& panel_data, const std::vector<double>& weights, std::vector<double>& mean, std::vector<double>& std_dev);
  }
}
