#pragma once

#include "camera_control.h"
#include "container.h"
#include "expression.h"
#include "figure/figure.h"
#include "function.h"
#include "graph.h"
#include "primitive.h"
#include "temporal.h"
#include "statistics.h"
#include "ui/all.h"
#include "vk/renderer.h"
#include "window.h"

namespace g3d {
class Application {
private:
	static constexpr unsigned int _initialCells = 80;
	static constexpr float _initialRange = 3.0f;

	static constexpr uint32_t _initialWindowWidth = 800;
	static constexpr uint32_t _initialWindowHeight = 600;

	EventRouter _eventRouter;
	Window _window;
	Ui _ui;
	GraphDevice _graphDevice;
	Renderer _renderer;
	ImGuiWrapper _imGui;

	CameraController _camController;
	TimerCollection _perfTimers { 50 };
	RenderSettings _renderSettings;
	DebugSettings _debugSettings;

	VariableStore _variableStore;
	Graph _graph;

	FigureCollection _figures;

	WithInitial<Light> _light {{
		{0.0f, 0.0f, 1.5f}, // position
		{1.0f, 1.0f, 1.0f}, // color
		1.0f
	}};

	EventPrinter<MousePositionEvent> _posDumper;
	EventPrinter<KeyEvent> _keyDumper;
	EventPrinter<ScrollEvent> _scrollDumper;

	PrimitiveTest _testObject;

	SimpleLitObject _axes;
	SimpleUnlitObject _lightObject;
	SimpleLineObject _frame;

	SimpleLitObject buildAxes();
	SimpleUnlitObject buildLightObject();
	SimpleLineObject buildFrame();

	void update();
	void updateSynchronized();
	void drawObjects();
	bool render();
public:
	Application(VkTop& vkTop) :
	_window {
		_eventRouter,
		vkTop.appInfo().pApplicationName,
		_initialWindowWidth,
		_initialWindowHeight
	},
	_posDumper { _eventRouter },
	_keyDumper { _eventRouter },
	_scrollDumper { _eventRouter },
	_graphDevice { vkTop, _window },
	_renderer { _graphDevice },
	_imGui { _graphDevice, _renderer },
	_camController {
		{2.5f, -2.5f, 2.5f}, // cam position
		{0.0f, 0.0f, 0.0f}, // initial center
		_window
	},
	_variableStore { _eventRouter },
	_graph { _renderer, _variableStore, _initialCells, _initialRange, _perfTimers },
	_figures { _eventRouter },
	_axes { buildAxes() },
	_frame { buildFrame() },
	_lightObject { buildLightObject() },
	_testObject { _renderer }
	{
		// Set up UI
		_ui.addWindow<CameraWindow>(_camController);
		_ui.addWindow<RenderWindow>(_renderSettings, _light, _graph.surfaceMaterial());
		_ui.addWindow<GraphWindow>(_graph, _window);
		_ui.addWindow<FigureWindow>(_figures, _window, _variableStore);
		_ui.addWindow<StatsWindow>(_perfTimers);
		_ui.addWindow<DebugWindow>(_debugSettings);
		_ui.addWindow<AboutWindow>();

		// Set up event dumpers
		_posDumper.enabled = false;
		_keyDumper.enabled = false;
		_scrollDumper.enabled = false;
	}

	void mainloop();
};
}
