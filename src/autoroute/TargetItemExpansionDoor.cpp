#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/ItemAutorouteInfo.h"

namespace freerouting {

bool TargetItemExpansionDoor::isDestinationDoor() const {
  if (!item) {
    return false;
  }

  const ItemAutorouteInfo& itemInfo = item->getAutorouteInfo();
  return !itemInfo.isStartInfo();
}

} // namespace freerouting
