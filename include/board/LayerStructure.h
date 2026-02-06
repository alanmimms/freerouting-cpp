#ifndef FREEROUTING_BOARD_LAYERSTRUCTURE_H
#define FREEROUTING_BOARD_LAYERSTRUCTURE_H

#include "Layer.h"
#include "core/Types.h"
#include <vector>
#include <string_view>

namespace freerouting {

// Describes the complete layer structure of a PCB board
// Manages the collection of layers and provides lookup/indexing functions
class LayerStructure {
public:
  // Default constructor - empty layer structure
  LayerStructure() = default;

  // Constructor from vector of layers
  explicit LayerStructure(std::vector<Layer> layers)
    : layers_(std::move(layers)) {}

  // Constructor from initializer list for convenience
  LayerStructure(std::initializer_list<Layer> layers)
    : layers_(layers) {}

  // Get layer count
  int count() const {
    return static_cast<int>(layers_.size());
  }

  // Get layer by index
  const Layer& operator[](int index) const {
    FR_ASSERT(index >= 0 && index < count());
    return layers_[index];
  }

  // Get layer by index (mutable)
  Layer& operator[](int index) {
    FR_ASSERT(index >= 0 && index < count());
    return layers_[index];
  }

  // Find layer index by name
  // Returns -1 if not found
  int getLayerNumber(std::string_view name) const {
    for (int i = 0; i < count(); i++) {
      if (name == layers_[i].name) {
        return i;
      }
    }
    return -1;
  }

  // Find layer index by layer reference
  // Returns -1 if not found
  int getLayerNumber(const Layer& layer) const {
    for (int i = 0; i < count(); i++) {
      if (&layer == &layers_[i]) {
        return i;
      }
    }
    return -1;
  }

  // Count signal layers
  int signalLayerCount() const {
    int count = 0;
    for (const auto& layer : layers_) {
      if (layer.isSignal) {
        ++count;
      }
    }
    return count;
  }

  // Get the n-th signal layer
  // Returns reference to last layer if index out of range
  const Layer& getSignalLayer(int signalLayerIndex) const {
    int foundSignalLayers = 0;
    for (int i = 0; i < count(); i++) {
      if (layers_[i].isSignal) {
        if (signalLayerIndex == foundSignalLayers) {
          return layers_[i];
        }
        ++foundSignalLayers;
      }
    }
    // Fallback - return last layer
    FR_ASSERT(!layers_.empty());
    return layers_.back();
  }

  // Get signal layer number (index among signal layers only)
  // Returns -1 if layer not found or is not a signal layer
  int getSignalLayerNumber(const Layer& layer) const {
    int foundSignalLayers = 0;
    for (int i = 0; i < count(); i++) {
      if (&layers_[i] == &layer) {
        // Return the signal layer index only if this layer is a signal layer
        return layers_[i].isSignal ? foundSignalLayers : -1;
      }
      if (layers_[i].isSignal) {
        ++foundSignalLayers;
      }
    }
    return -1;
  }

  // Get absolute layer number from signal layer index
  // Returns the layer number in the full layer structure
  int getLayerNumberFromSignalLayer(int signalLayerIndex) const {
    const Layer& signalLayer = getSignalLayer(signalLayerIndex);
    return getLayerNumber(signalLayer);
  }

  // Add a layer
  void addLayer(const Layer& layer) {
    layers_.push_back(layer);
  }

  // Iterator support for range-based for loops
  auto begin() const { return layers_.begin(); }
  auto end() const { return layers_.end(); }
  auto begin() { return layers_.begin(); }
  auto end() { return layers_.end(); }

private:
  std::vector<Layer> layers_;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_LAYERSTRUCTURE_H
