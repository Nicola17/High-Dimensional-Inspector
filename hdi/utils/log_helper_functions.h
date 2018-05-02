#ifndef LOG_HELPER_FUNCTIONS_H
#define LOG_HELPER_FUNCTIONS_H

#include <string>
#include <sstream>
#include <vector>
#include <cmath>
#include <limits>

namespace hdi{
  namespace utils{
    template <class Logger>
    inline void secureLog(Logger* logPtr, const std::string& text, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          logPtr->display(text);
        }
      }
    }

    template <class Logger, class Data>
    inline void secureLogValue(Logger* logPtr, const std::string& text, const Data& v, bool enabled = true, unsigned int num_tabs = 1){
      if(enabled){
        if(logPtr != nullptr){
          std::stringstream ss;
          ss << text;
          ss << ":";
          for(int i = 0; i < num_tabs; ++i){ss << "\t";}
          ss << v;
          logPtr->display(ss.str());
        }
      }
    }

    template <class Logger, class Data>
    inline void secureLogVector(Logger* logPtr, const std::string& text, const std::vector<Data>& vector, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          for(int i = 0; i < vector.size(); ++i){
            std::stringstream ss;
            ss << text << "[" << i << "]";
            secureLogValue(logPtr,ss.str(),vector[i],enabled);
          }
        }
      }
    }

    template <class Logger, class Data>
    inline void secureLogVectorWithStats(Logger* logPtr, const std::string& text, const std::vector<Data>& vector, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          double avg(0);
          double avg_sq(0);
          double sum(0);
          double min(std::numeric_limits<double>::max());
          double max(-std::numeric_limits<double>::max());
          for(int i = 0; i < vector.size(); ++i){
            std::stringstream ss;
            ss << text << "[" << i << "]";
            secureLogValue(logPtr,ss.str(),vector[i],enabled);
            avg += static_cast<double>(vector[i]);
            avg_sq += static_cast<double>(vector[i]*vector[i]);
            min = std::min(min,static_cast<double>(vector[i]));
            max = std::max(max,static_cast<double>(vector[i]));
          }
          sum = avg;
          avg /= vector.size();
          avg_sq /= vector.size();
          {
            std::stringstream ss;
            ss << "MIN(" << text << ")";
            secureLogValue(logPtr,ss.str(),min,enabled);
          }
          {
            std::stringstream ss;
            ss << "MAX(" << text << ")";
            secureLogValue(logPtr,ss.str(),max,enabled);
          }
          {
            std::stringstream ss;
            ss << "SUM(" << text << ")";
            secureLogValue(logPtr,ss.str(),sum,enabled);
          }
          {
            std::stringstream ss;
            ss << "AVG(" << text << ")";
            secureLogValue(logPtr,ss.str(),avg,enabled);
          }
          {
            std::stringstream ss;
            ss << "STDDEV(" << text << ")";
            secureLogValue(logPtr,ss.str(),std::sqrt(avg_sq-avg*avg),enabled);
          }
        }
      }
    }

    template <class Logger, class Data>
    inline void secureLogVectorStats(Logger* logPtr, const std::string& text, const std::vector<Data>& vector, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          double avg(0);
          double avg_sq(0);
          double sum(0);
          double min(std::numeric_limits<double>::max());
          double max(-std::numeric_limits<double>::max());
          for(int i = 0; i < vector.size(); ++i){
            avg += static_cast<double>(vector[i]);
            avg_sq += static_cast<double>(vector[i]*vector[i]);
            min = std::min(min,static_cast<double>(vector[i]));
            max = std::max(max,static_cast<double>(vector[i]));
          }
          sum = avg;
          avg /= vector.size();
          avg_sq /= vector.size();
          {
            std::stringstream ss;
            ss << "MIN(" << text << ")";
            secureLogValue(logPtr,ss.str(),min,enabled);
          }
          {
            std::stringstream ss;
            ss << "MAX(" << text << ")";
            secureLogValue(logPtr,ss.str(),max,enabled);
          }
          {
            std::stringstream ss;
            ss << "SUM(" << text << ")";
            secureLogValue(logPtr,ss.str(),sum,enabled);
          }
          {
            std::stringstream ss;
            ss << "AVG(" << text << ")";
            secureLogValue(logPtr,ss.str(),avg,enabled);
          }
          {
            std::stringstream ss;
            ss << "STDDEV(" << text << ")";
            secureLogValue(logPtr,ss.str(),std::sqrt(avg_sq-avg*avg),enabled);
          }
        }
      }
    }

    template <class Logger, class Data>
    inline void secureLogElementsInVector(Logger* logPtr, const std::string& text, const std::vector<Data>& vector, Data data, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          double sum(0);
          for(int i = 0; i < vector.size(); ++i){
            if(vector[i] == data){
              ++sum;
            }
          }
          {
            std::stringstream ss;
            ss << "SUM(" << text << " == " << data << "): \t" << sum;
            secureLog(logPtr,ss.str(),enabled);
          }
        }
      }
    }

    template <class Logger, class SparseMatrix>
    inline void secureLogSparseMatrixStats(Logger* logPtr, const std::string& text, const SparseMatrix& matrix, bool enabled = true){
      if(enabled){
        if(logPtr != nullptr){
          unsigned int num_rows(matrix.size());
          unsigned int num_elements(0);
          double avg_elements_per_row(0);
          double avg_elements_per_row_sq(0);
          for(auto& row: matrix){
            num_elements += row.size();
            avg_elements_per_row += static_cast<double>(row.size());
            avg_elements_per_row_sq += static_cast<double>(row.size()*row.size());
          }
          avg_elements_per_row /= num_rows;
          avg_elements_per_row_sq /= num_rows;
          const double sparsity(1.-double(num_elements)/(num_rows*num_rows));

          {
            std::stringstream ss;
            ss << "ROWS(" << text << ")";
            secureLogValue(logPtr,ss.str(),num_rows,enabled);
          }
          {
            std::stringstream ss;
            ss << "SUM(" << text << " != 0)";
            secureLogValue(logPtr,ss.str(),num_elements,enabled);
          }
          {
            std::stringstream ss;
            ss << "SPARSITY(" << text << ")";
            secureLogValue(logPtr,ss.str(),sparsity,enabled);
          }
          {
            std::stringstream ss;
            ss << "AVG_ROW(" << text << " != 0)";
            secureLogValue(logPtr,ss.str(),avg_elements_per_row,enabled);
          }
          {
            std::stringstream ss;
            ss << "STDDEV_ROW(" << text << ")";
            secureLogValue(logPtr,ss.str(),std::sqrt(avg_elements_per_row_sq-avg_elements_per_row*avg_elements_per_row),enabled);
          }
        }
      }
    }

    template <class Logger>
    inline void secureClear(Logger* logPtr){
      if(logPtr != nullptr){
        logPtr->clear();
      }
    }
  }
}

#endif // LOG_HELPER_FUNCTIONS_H
