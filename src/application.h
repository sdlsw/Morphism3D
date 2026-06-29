#pragma once

#include "camera_control.h"
#include "container.h"
#include "expression.h"
#include "graph.h"
#include "primitive.h"
#include "temporal.h"
#include "statistics.h"
#include "ui/all.h"
#include "vk/renderer.h"
#include "window.h"

namespace g3d {
class ExpressionFunc {
private:
	TokenRegistry _tokenRegistry;
	VariableStore _vars;
	std::chrono::time_point<std::chrono::high_resolution_clock> _startTime;
	std::unique_ptr<ParseNode> _parsedExpression;
	bool _animated = false;
	bool _updated = false;

public:
	ExpressionFunc()
	: _tokenRegistry { makeTokenRegistry() },
	  _startTime { now() }
	{}

	bool animated() const { return _animated; }
	bool updated() const { return _updated; }
	void setUpdated() { _updated = true; }
	void resetUpdated() { _updated = false; }
	auto& vars() { return _vars; }

	float eval(float x, float y);
	void update();
	void updateAnimated();
	void updateExpression(const std::string& expression);
};

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

	ExpressionFunc _f;
	Graph<ExpressionFunc> _graph;

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
	_graph { _renderer, _f, _initialCells, _initialRange, _perfTimers },
	_axes { buildAxes() },
	_frame { buildFrame() },
	_lightObject { buildLightObject() },
	_testObject { _renderer }
	{
		// Set up UI
		_ui.addWindow<CameraWindow>(_camController);
		_ui.addWindow<RenderWindow>(_renderSettings, _light, _graph.surfaceMaterial());
		_ui.addWindow<GraphWindow<ExpressionFunc>>(_graph, _window);
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
