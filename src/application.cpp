#include "application.h"

#include "generated/appinfo.h"

void buildArrow(g3d::MeshBuilder& b) {
	constexpr unsigned int hRes = 10;
	constexpr unsigned int cylVRes = 30;
	constexpr unsigned int coneVRes = hRes;
	constexpr float arrowLength = 1.2f;
	constexpr float arrowTipLength = 0.1f;
	constexpr float arrowRadius = 0.01f;
	constexpr float arrowTipRadius = arrowRadius * 2.0f;

	constexpr float cylLength = arrowLength - arrowTipLength;

	b.beginGroup();
	b.primitive<g3d::Cone>(
		{ arrowTipRadius, arrowTipLength, hRes, coneVRes },
		{ 0.0f, 0.0f, cylLength }
	);
	b.primitive<g3d::Cylinder>(
		{ arrowRadius, cylLength, hRes, cylVRes },
		{ 0.0f, 0.0f, cylLength / 2.0f }
	);
	b.endGroup();
}

namespace g3d {
void ExpressionFunc::_VariableChangedHandler::handle(const VariableChangedEvent& event) {
	char s[] { '\0', '\0' };
	s[0] = event.c;

	if (_this->_parsedExpression && _this->_parsedExpression.get()->hasTokenStr(s)) {
		_this->_updated = true;
	}
}

float ExpressionFunc::eval(float x, float y) {
	if (!_parsedExpression) {
		return 0.0f;
	}

	// Don't want to send event for x/y changes
	// TODO find a less hacky way to do this that doesn't involve updating
	// the variable store on every eval. Other graphs are going to need to
	// share this.
	_vars->set('x', x, false);
	_vars->set('y', y, false);
	return _parsedExpression.get()->eval();
}

void ExpressionFunc::update() {
	_vars->set('t', secondsSince(_startTime));
}

void ExpressionFunc::updateAnimated() {
	_animated = (_parsedExpression && _parsedExpression.get()->hasTokenStr("t"));
}

void ExpressionFunc::updateExpression(const std::string& expression) {
	Parser p { _tokenRegistry, *_vars, expression };

	try {
		_parsedExpression.reset(new ParseNode(p.parse()));
		updateAnimated();
		_updated = true;

		std::cerr << "Expression updated: " << expression << std::endl;
		std::cerr << "Animated: " << _animated << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Failed to parse expression: " << expression << std::endl;
		std::cerr << e.what() << std::endl;
	}
}

SimpleLitObject Application::buildAxes() {
	constexpr Color xcolor { 0.6f, 0.1f, 0.1f };
	constexpr Color ycolor { 0.1f, 0.6f, 0.1f };
	constexpr Color zcolor { 0.1f, 0.1f, 0.6f };
	constexpr Color centerColor { 0.1f, 0.1f, 0.1f };

	auto builder = SimpleLitObject::mkBuilder(
		_renderer,
		MeshBuilderMode::genColors
	);

	builder.setMaterial({ 0.4f, 0.2f, 0.7f, 8.0f });

	builder.setColor(xcolor);
	buildArrow(builder);
	builder.rot90Y(RotateDirection::CCW);

	builder.setColor(ycolor);
	buildArrow(builder);
	builder.rot90X(RotateDirection::CW);

	builder.setColor(zcolor);
	buildArrow(builder);

	builder.setColor(centerColor);
	builder.primitive<Sphere>({0.02f, 20, 20});

	return { builder };
}

SimpleUnlitObject Application::buildLightObject() {
	return {
		SimpleUnlitObject::mkBuilder(_renderer)
			.setColor({1.0f, 1.0f, 1.0f})
			.primitive<Cube>({0.1f})
	};
}

SimpleLineObject Application::buildFrame() {
	return {
		SimpleLineObject::mkBuilder(_renderer)
			.setColor({0.1f, 0.1f, 0.1f})
			.primitive<Cube>({2.0f})
	};
}

void Application::drawObjects() {
	// Note: Graph handles its own draw settings
	_graph.draw();

	if (_renderSettings.renderFrame) {
		_frame.draw();
	}

	if (_renderSettings.renderAxes) {
		_axes.draw();
	}

	if (_renderSettings.renderLightObject) {
		_lightObject.draw();
	}

	if (_debugSettings.renderTestObject) {
		_testObject.draw();
	}
}

void Application::update() {
	Window::pollEvents();
	imGuiNewFrame();
	imGuiHandleControlExclusivity(_window, _camController);

	// UI updates.
	_ui.show();

	// Entity and camera updates.
	_camController.update();
	getTransform(_lightObject.entity()).translation = _light.current.position;
	_f.update();
	getTransform(_axes.entity()).translation = _graph.origin();
}

void Application::updateSynchronized() {
	// Objects with dynamic mesh data need to be updated after
	// beginFrame() to ensure proper synchronization.
	_graph.update();
}

// Renders a frame. Also handles updates that need to be synchronized with
// rendering.
bool Application::render() {
	_renderer.beginFrame(_camController.camera(), _light);
	if (!_renderer.inFrame()) {
		// Indicates we failed to begin the frame for some
		// outside reason - probably window resize
		return false;
	}
	updateSynchronized();
	drawObjects();
	_renderer.recordEntities();
	imGuiRecord(_renderer);
	_renderer.endFrame();

	return true;
}

void Application::mainloop() {
	while (!_window.shouldClose()) {
		_perfTimers.start("frame");
		update();
		bool didRender = render();
		if (!didRender) continue;
		_perfTimers.stop("frame");

		_window.title(std::format(
			"{} | {:.2f} FPS",
			APPLICATION_NAME,
			1.0f / _perfTimers.getAverage("frame")
		));
	}

	_graphDevice.logicalDevice().waitIdle();
}
}
