#ifndef CLUSTER_H
#define CLUSTER_H

#include <unordered_set>
#include <QColor>

namespace  hdi {
  namespace data {

    class Cluster {
    public:
      Cluster() : color(0, 0, 0) { }
      Cluster(QColor color) : color(color) { }

      const std::unordered_set<unsigned int>& getPointIndices() const { return point_ids; }

      QColor getColor() const { return color; }

      void add(unsigned int id) {
        point_ids.insert(id);
      }

      void remove(unsigned int id) {
        point_ids.erase(id);
      }

      bool contains(unsigned int id) const {
        return point_ids.find(id) != point_ids.end();
      }
    private:
      std::unordered_set<unsigned int> point_ids;
      QColor color;
    };

  }
}

#endif // CLUSTER_H
