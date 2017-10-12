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

#ifndef HSNE_ANALYTICS_INTERFACES_H
#define HSNE_ANALYTICS_INTERFACES_H

#include <QObject>
#include <unordered_map>
#include "hdi/utils/abstract_log.h"
#include "hdi/data/map_mem_eff.h"
#include <memory>

namespace hdi{
    namespace analytics{

        class AbstractHSNEEmbedder;

        class AbstractHSNEEmbedderFactory: public QObject{
            Q_OBJECT
        public:
            typedef float scalar_type;
            typedef uint32_t unsigned_int_type;
            typedef std::vector<hdi::data::MapMemEff<unsigned_int_type,scalar_type>> sparse_scalar_matrix_type;
            typedef std::pair<unsigned_int_type, unsigned_int_type> id_type;

        public:
            AbstractHSNEEmbedderFactory():_logger(nullptr){}
            virtual ~AbstractHSNEEmbedderFactory() = 0;

            //! Return the current log
            utils::AbstractLog* logger()const{return _logger;}
            //! Set a pointer to an existing log
            void setLogger(utils::AbstractLog* logger){_logger = logger;}

            //! Create an empty embedder
            virtual std::shared_ptr<AbstractHSNEEmbedder> createEmbedder() = 0;

        protected:
            utils::AbstractLog* _logger;
        };

        inline AbstractHSNEEmbedderFactory::~AbstractHSNEEmbedderFactory(){}

////////////////////////////////////////////////////////////////////////////////////////////

        class AbstractHSNEEmbedder: public QObject{
            Q_OBJECT
        public:
            typedef float scalar_type;
            typedef uint32_t unsigned_int_type;
            typedef std::vector<hdi::data::MapMemEff<unsigned_int_type,scalar_type>> sparse_scalar_matrix_type;
            typedef std::pair<unsigned_int_type, unsigned_int_type> id_type;

        public:
            AbstractHSNEEmbedder():_logger(nullptr){}
            virtual ~AbstractHSNEEmbedder() = 0;

            void setId(id_type id){_id = id;}
            id_type id()const{return _id;}

            const std::vector<unsigned_int_type>& indicesInScale()const{return _scale_idxes;}
            const std::vector<unsigned_int_type>& indicesInData()const{return _data_idxes;}
            const std::vector<scalar_type>& weights()const{return _weights;}
            std::vector<unsigned_int_type>& indicesInScale(){return _scale_idxes;}
            std::vector<unsigned_int_type>& indicesInData(){return _data_idxes;}
            std::vector<scalar_type>& weights(){return _weights;}

            //! Return the current log
            utils::AbstractLog* logger()const{return _logger;}
            //! Set a pointer to an existing log
            void setLogger(utils::AbstractLog* logger){_logger = logger;}
            //! Initialize the embedder
            virtual void initialize(const sparse_scalar_matrix_type& probabilities) = 0;

        signals:
            void sgnNewAnalysisTriggered(id_type id);

        public slots:
            //! Do an iteration in the embedding
            virtual void onIterate() = 0;

        protected:
            std::vector<unsigned_int_type> _scale_idxes;
            std::vector<unsigned_int_type> _data_idxes;
            std::vector<scalar_type> _weights;
            id_type _id;
            utils::AbstractLog* _logger;

        };
        inline AbstractHSNEEmbedder::~AbstractHSNEEmbedder(){}

    }
}

#endif
