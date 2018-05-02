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


#include "text_view_qobj.h"
#include "hdi/data/text_data.h"
#include <set>
#include <fstream>
#include <assert.h>
#include <QFileDialog>
#include "ui_text_view_qobj.h"

namespace hdi{
  namespace viz{

  TextView::TextView(QWidget* parent)
  {
    _ui = new Ui::TextView();
    _ui->setupUi(this);
    _ui->_selected_text_wdg->setSelectionMode(QAbstractItemView::ContiguousSelection);
    _ui->_unselected_text_wdg->setSelectionMode(QAbstractItemView::ContiguousSelection);
    QObject::connect(_ui->_export_btn, &QPushButton::clicked, this, &TextView::onExportTexts);
    QObject::connect(_ui->_select_btn, &QPushButton::clicked, this, &TextView::onSelectTexts);
    QObject::connect(_ui->_unselect_btn, &QPushButton::clicked, this, &TextView::onUnselectTexts);
    QObject::connect(_ui->_import_selection_btn, &QPushButton::clicked, this, &TextView::onImportTextSelection);
  }

  void TextView::onImportTextSelection(){
    assert(_panel_data != nullptr);
    auto str = QFileDialog::getOpenFileName(0,"Load texts file as...");
    std::ifstream file(str.toStdString());
    std::string line;
    std::set<std::string> text_names;
    while(std::getline(file,line)){
    text_names.insert(line);
    }

    const auto& data = _panel_data->getDataPoints();
    auto& flags = _panel_data->getFlagsDataPoints();
    assert(data.size() == flags.size());

    for(int i = 0; i < data.size(); ++i){
    auto data_ptr = dynamic_cast<data::TextData*>(data[i].get());
    if(data_ptr == nullptr){
      continue;
    }
    if(text_names.find(data_ptr->text()) != text_names.end()){
      flags[i] |= panel_data_type::Selected;
    }else{
      flags[i] &= ~panel_data_type::Selected;
    }
    }

    updateView();
    emit sgnSelectionChanged();
  }

  void TextView::onSelectionChanged(){
    assert(_panel_data != nullptr);
    updateView();
  }

  void TextView::onSelectTexts(){
    assert(_panel_data != nullptr);
    std::set<std::string> text_names;
    for (int i = 0; i < _ui->_unselected_text_wdg->count(); ++i) {
    if(_ui->_unselected_text_wdg->item(i)->isSelected())
      text_names.insert(_ui->_unselected_text_wdg->item(i)->text().toStdString());
    }

    const auto& data = _panel_data->getDataPoints();
    auto& flags = _panel_data->getFlagsDataPoints();
    assert(data.size() == flags.size());

    for(int i = 0; i < data.size(); ++i){
    auto data_ptr = dynamic_cast<data::TextData*>(data[i].get());
    if(data_ptr == nullptr){
      continue;
    }
    if(text_names.find(data_ptr->text()) != text_names.end()){
      flags[i] |= panel_data_type::Selected;
    }
    }

    updateView();
    emit sgnSelectionChanged();
  }

  void TextView::onUnselectTexts(){
    assert(_panel_data != nullptr);
    std::set<std::string> text_names;
    for (int i = 0; i < _ui->_selected_text_wdg->count(); ++i) {
    if(_ui->_selected_text_wdg->item(i)->isSelected())
      text_names.insert(_ui->_selected_text_wdg->item(i)->text().toStdString());
    }

    const auto& data = _panel_data->getDataPoints();
    auto& flags = _panel_data->getFlagsDataPoints();
    assert(data.size() == flags.size());

    for(int i = 0; i < data.size(); ++i){
    auto data_ptr = dynamic_cast<data::TextData*>(data[i].get());
    if(data_ptr == nullptr){
      continue;
    }
    if(text_names.find(data_ptr->text()) != text_names.end()){
      flags[i] &= ~panel_data_type::Selected;
    }
    }

    updateView();
    emit sgnSelectionChanged();
  }

  void TextView::onExportTexts()const{
    assert(_panel_data != nullptr);
    auto str = QFileDialog::getSaveFileName(0,"Save as...");

    std::ofstream file(str.toStdString());
    for (int i = 0; i < _ui->_selected_text_wdg->count(); ++i) {
    file << _ui->_selected_text_wdg->item(i)->text().toStdString() << std::endl;
    }
  }

  void TextView::updateView(){
    assert(_panel_data != nullptr);
    _ui->_selected_text_wdg->clear();
    _ui->_unselected_text_wdg->clear();

    const auto& data = _panel_data->getDataPoints();
    const auto& flags = _panel_data->getFlagsDataPoints();
    assert(data.size() == flags.size());

    int num_selected(0);
    int num_valid(0);
    for(int i = 0; i < data.size(); ++i){
    auto data_ptr = dynamic_cast<data::TextData*>(data[i].get());
    if(data_ptr == nullptr){
      continue;
    }
    ++num_valid;

    if((flags[i]&panel_data_type::Selected) == panel_data_type::Selected){
      _ui->_selected_text_wdg->addItem(QString::fromStdString(data_ptr->text()));
      ++num_selected;
    }else{
      _ui->_unselected_text_wdg->addItem(QString::fromStdString(data_ptr->text()));
    }
    }

    _ui->_num_elem_lbl->setText(QString("# Elements: %1").arg(num_valid));
    _ui->_num_sel_elem_lbl->setText(QString("# Selected elements: %1").arg(num_selected));
  }

  }
}
