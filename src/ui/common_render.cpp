#include "ui/common_render.h"

namespace g3d {
void resettableLightPanel(WithInitial<Light>& light) {
	resettableSlider("Mix",
		&light.current.mix,
		light.initial().mix,
		0.0f, 1.0f
	);
	resettableSlider("Color",
		&light.current.color,
		light.initial().color,
		0.0f, 1.0f
	);
	resettableDrag("Position",
		&light.current.position,
		light.initial().position,
		0.025f
	);
	resetAllButton("Light", light);
}

void resettableMaterialPanel(WithInitial<Material>& material) {
	resettableSlider("Ambient",
		&material.current.ambient,
		material.initial().ambient,
		0.0f, 1.0f
	);
	resettableSlider("Diffuse",
		&material.current.diffuse,
		material.initial().diffuse,
		0.0f, 1.0f
	);
	resettableSlider("Specular",
		&material.current.specular,
		material.initial().specular,
		0.0f, 1.0f
	);
	resettableSlider("Shine",
		&material.current.shine,
		material.initial().shine,
		1.0f, 64.0f
	);
	resetAllButton("Material", material);
}
}
