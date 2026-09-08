#pragma once

#include "ui/common.h"
#include "vk/datatypes.h"

namespace g3d {
void resettableLightPanel(WithInitial<Light>& light);
void resettableMaterialPanel(WithInitial<Material>& material);
}
