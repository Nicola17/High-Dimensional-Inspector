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

#ifndef FLOW_MODEL_H
#define FLOW_MODEL_H

#include <cstdint>
#include <set>
#include <string>
#include <QString>
#include <QColor>

namespace hdi{
  namespace data{


    class FlowNode{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
    public:
      FlowNode(id_type id = 0, std::string text = ""){_id=id;_text=text;}
      bool operator==(const FlowNode& a)const{return id() == a.id();}
      bool operator!=(const FlowNode& a)const{return id() != a.id();}
      bool operator<=(const FlowNode& a)const{return id() <= a.id();}
      bool operator>=(const FlowNode& a)const{return id() >= a.id();}
      bool operator<(const FlowNode& a)const{return id() < a.id();}
      bool operator>(const FlowNode& a)const{return id() > a.id();}

      id_type& id(){return _id;}
      const id_type& id()const {return _id;}

      static QString getCSVHeader(){return QString("id,text");}
      QString getCSVValues(){return QString("%1,%2").arg(_id).arg(QString::fromStdString(_text));}

    private:
      id_type _id;
      std::string _text;
    };

    class FlowColoredNode{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
      typedef QColor color_type;

    public:
      FlowColoredNode(id_type id = 0, std::string text = "", color_type color = qRgb(50,50,50)){_id=id;_text=text;_color = color;}
      bool operator==(const FlowColoredNode& a)const{return id() == a.id();}
      bool operator!=(const FlowColoredNode& a)const{return id() != a.id();}
      bool operator<=(const FlowColoredNode& a)const{return id() <= a.id();}
      bool operator>=(const FlowColoredNode& a)const{return id() >= a.id();}
      bool operator<(const FlowColoredNode& a)const{return id() < a.id();}
      bool operator>(const FlowColoredNode& a)const{return id() > a.id();}

      id_type& id(){return _id;}
      const id_type& id()const {return _id;}

      static QString getCSVHeader(){return QString("id,text,color");}
      QString getCSVValues(){return QString("%1,%2,%3").arg(_id).arg(QString::fromStdString(_text)).arg(_color.name());}

    private:
      id_type _id;
      std::string _text;
      color_type _color;
    };

    class FlowLink{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
    public:
      FlowLink(id_type id = 0, id_type start_node_id = 0, id_type end_node_id = 0, scalar_type flow = 0){
        _id=id;
        _start_node_id = start_node_id;
        _end_node_id = end_node_id;
        _flow = flow;
      }
      bool operator==(const FlowLink& a)const{return id() == a.id();}
      bool operator!=(const FlowLink& a)const{return id() != a.id();}
      bool operator<=(const FlowLink& a)const{return id() <= a.id();}
      bool operator>=(const FlowLink& a)const{return id() >= a.id();}
      bool operator<(const FlowLink& a)const{return id() < a.id();}
      bool operator>(const FlowLink& a)const{return id() > a.id();}

      id_type& id(){return _id;}
      const id_type& id()const {return _id;}
      id_type& start_node_id(){return _start_node_id;}
      const id_type& start_node_id()const {return _start_node_id;}
      id_type& end_node_id(){return _end_node_id;}
      const id_type& end_node_id()const {return _end_node_id;}
      scalar_type& flow(){return _flow;}
      const scalar_type& flow()const {return _flow;}

      static QString getCSVHeader(){return QString("id,source,target,value");}
      QString getCSVValues(){return QString("%1,%2,%3,%4").arg(_id).arg(_start_node_id).arg(_end_node_id).arg(_flow);}
    private:
      id_type _id;
      id_type _start_node_id;
      id_type _end_node_id;
      scalar_type _flow;
    };

    class FlowColoredLink{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
      typedef QColor color_type;

    public:
      FlowColoredLink(id_type id = 0, id_type start_node_id = 0, id_type end_node_id = 0, scalar_type flow = 0, color_type color = qRgb(0,150,255)){
        _id=id;
        _start_node_id = start_node_id;
        _end_node_id = end_node_id;
        _flow = flow;
        _color = color;
      }
      bool operator==(const FlowColoredLink& a)const{return id() == a.id();}
      bool operator!=(const FlowColoredLink& a)const{return id() != a.id();}
      bool operator<=(const FlowColoredLink& a)const{return id() <= a.id();}
      bool operator>=(const FlowColoredLink& a)const{return id() >= a.id();}
      bool operator<(const FlowColoredLink& a)const{return id() < a.id();}
      bool operator>(const FlowColoredLink& a)const{return id() > a.id();}

      id_type& id(){return _id;}
      const id_type& id()const {return _id;}
      id_type& start_node_id(){return _start_node_id;}
      const id_type& start_node_id()const {return _start_node_id;}
      id_type& end_node_id(){return _end_node_id;}
      const id_type& end_node_id()const {return _end_node_id;}
      scalar_type& flow(){return _flow;}
      const scalar_type& flow()const {return _flow;}

      static QString getCSVHeader(){return QString("id,source,target,value,color");}
      QString getCSVValues(){return QString("%1,%2,%3,%4,%5").arg(_id).arg(_start_node_id).arg(_end_node_id).arg(_flow).arg(_color.name());}
    private:
      id_type _id;
      id_type _start_node_id;
      id_type _end_node_id;
      scalar_type _flow;
      color_type _color;
    };

    class DefaultFlowModelTrait{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
      typedef FlowNode node_type;
      typedef FlowLink flow_type;
    };

    class ColoredFlowModelTrait{
    public:
      typedef float scalar_type;
      typedef uint32_t id_type;
      typedef FlowColoredNode node_type;
      typedef FlowColoredLink flow_type;
    };

    template <typename Traits>
    class FlowModel{
    public:
      typedef typename Traits::scalar_type scalar_type;
      typedef typename Traits::id_type id_type;
      typedef typename Traits::node_type node_type;
      typedef typename Traits::flow_type flow_type;

    public:
      FlowModel(){}

      void addNode(const node_type& node);
      void addFlow(const flow_type& flow);

      std::set<node_type>& nodes(){return _nodes;}
      std::set<flow_type>& flows(){return _flows;}

    private:
      std::set<node_type> _nodes;
      std::set<flow_type> _flows;
    };

    template <class Traits>
    void FlowModel<Traits>::addNode(const node_type& node){
      _nodes.insert(node);
    }
    template <class Traits>
    void FlowModel<Traits>::addFlow(const flow_type& flow){
      _flows.insert(flow);
    }

  }
}

#endif
