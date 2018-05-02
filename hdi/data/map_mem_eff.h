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

#ifndef MAP_MEM_EFF_H
#define MAP_MEM_EFF_H

#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>
#include "hdi/utils/assert_by_exception.h"

namespace hdi{
  namespace data{

    //DO NOT CHANGE THE INDICES OUT SIDE
    template <class Key, class T>
    class MapMemEff{
    public:
      typedef Key         key_type;
      typedef T           mapped_type;
      typedef std::pair<Key,T>  value_type;
      typedef std::vector<value_type> storage_type;

    public:
      typedef typename storage_type::iterator iterator;
      typedef typename storage_type::const_iterator const_iterator;

      void clear(){_memory.clear();}
      size_t size()const{return _memory.size();}
      size_t capacity()const{return _memory.capacity();}
      void shrink_to_fit(){_memory.shrink_to_fit();}

      //iterators are always constant
      iterator begin() {return _memory.begin();}
      iterator end() {return _memory.end();}
      const_iterator begin()const {return _memory.begin();}
      const_iterator end()const {return _memory.end();}
      const_iterator cbegin()const {return _memory.cbegin();}
      const_iterator cend()const {return _memory.cend();}

      //access
      mapped_type& operator[](const key_type& k);

      //find
      const_iterator find (const key_type& k) const;

      //initialize
      template <typename It>
      void initialize(It begin, It end, mapped_type thresh = 0);

      //!MEMORY ACCESS: With great power comes great responsibility!
      storage_type& memory(){return _memory;}
      //!MEMORY ACCESS: With great power comes great responsibility!
      const storage_type& memory()const{return _memory;}

    private:
      storage_type _memory;
    };


    template <class Key, class T>
    typename MapMemEff<Key,T>::mapped_type& MapMemEff<Key,T>::operator[](const key_type& k){
      int l = 0;
      int r = _memory.size()-1;
      int m = 0;

      if(size()==0){
        _memory.push_back(std::make_pair(k,mapped_type(0)));
        return _memory[0].second;
      }

      bool found(false);
      while(l<=r){
        m = (l+r)*0.5;
        if(_memory[m].first == k){
          found = true;
          break;
        }
        if(_memory[m].first > k){
          r = m-1;
        }else{
          l = m+1;
        }
      }

      assert(m>=0);
      if(!found){
        assert(m<=_memory.size());
        if(_memory[m].first < k){
          m++;
        }
        _memory.insert(_memory.begin()+m,std::make_pair(k,mapped_type(0)));
      }
      assert(m<_memory.size());
      return _memory[m].second;
    }

    template <class Key, class T>
    typename MapMemEff<Key,T>::const_iterator MapMemEff<Key,T>::find(const key_type& k)const {
      int l = 0;
      int r = _memory.size()-1;
      int m = 0;

      if(size()==0){
        return end();
      }

      bool found(false);
      while(l<=r){
        m = (l+r)*0.5;
        if(_memory[m].first == k){
          found = true;
          break;
        }
        if(_memory[m].first > k){
          r = m-1;
        }else{
          l = m+1;
        }
      }

      assert(m>=0);
      if(!found){
        assert(m<=_memory.size());
        return end();
      }

      return begin()+m;
    }

    template <class Key, class T>
    template <typename It>
    void MapMemEff<Key,T>::initialize(It begin, It end, mapped_type thresh){
      checkAndThrowLogic(_memory.size() == 0, "MapMemEff::initialize: map is not empty");
      if(begin==end)
        return;

      int num_elem = begin->second > thresh?1:0;
      {//check for ordered element
        Key v = begin->first;
        It it = begin;
        for(++it; it != end; ++it){
           checkAndThrowLogic(it->first > v, "MapHelpers::initialize: input data is not ordered");
           v = it->first;
           if(it->second > thresh){
            ++num_elem;
           }
        }
      }
      _memory.reserve(num_elem);
      for(It it = begin; it != end; ++it){
        if(it->second > thresh){
          _memory.push_back(*it);
        }
      }
    }

  }
}

#endif
