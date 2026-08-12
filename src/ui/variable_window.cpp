#include "ui/variable_window.h"

namespace g3d {
bool RearrangeFrame::showHeader() {
	_upPressed = ImGui::ArrowButton(_upArrowId.c_str(), ImGuiDir_Up);
	ImGui::SameLine(0.0f, 0.0f);
	_downPressed = ImGui::ArrowButton(_downArrowId.c_str(), ImGuiDir_Down);
	ImGui::SameLine();

	return ImGui::CollapsingHeader(
		headerId().c_str(),
		&exists,
		(
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_OpenOnArrow
		)
	);
}

void RearrangeFrame::showBody() {
	_elem->show();
}

bool SliderElement::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}

void SliderElement::updateStore() {
	char v = var();
	if (v != '\0') {
		_variableStore->set(v, _value);
	}
}

void SliderElement::show() {
	// Top row
	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(80);
	ImGui::InputFloat(_minEntryId.c_str(), &_min);
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = var();
	bool varChanged = ImGui::InputText(_varEntryId.c_str(), _entryBuffer.data(), _entryBuffer.size());
	if (varChanged && !varValid(var())) {
		// Don't allow the user to enter an invalid variable name
		_entryBuffer[0] = lastVar;
		varChanged = false;
	}

	if (var() != lastVar) {
		_variableStore->set(lastVar, 0.0f);
	}

	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::InputFloat(_maxEntryId.c_str(), &_max);
	ImGui::PopItemWidth();

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(_valueEntryId.c_str(), &_value, _min, _max);

	if (varChanged || valChanged) updateStore();
}

char SliderElement::var() const {
	return _entryBuffer[0];
}

RearrangeFrame& SliderPanel::getFrame(size_t i) {
	return _frames.at(_frameOrder[i]);
}

void SliderPanel::showFrame(size_t i) {
	auto& frame = getFrame(i);

	ImVec2 start = ImGui::GetCursorScreenPos();
	ImGui::BeginGroup();
	bool headerShown = frame.showHeader();

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
		ImGui::SetDragDropPayload("SLIDER_REARRANGE", &i, sizeof(size_t));
		ImGui::Text(frame.fromTooltip().c_str());
		ImGui::EndDragDropSource();
	}

	if (headerShown) frame.showBody();
	ImGui::EndGroup();
	ImVec2 end = ImGui::GetCursorScreenPos();

	_heights[frame.elem().id()] = static_cast<unsigned int>(end.y - start.y);

	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
			"SLIDER_REARRANGE",
			ImGuiDragDropFlags_AcceptBeforeDelivery |
			ImGuiDragDropFlags_AcceptNoPreviewTooltip
		);

		if (payload) {
			ImGui::SetTooltip(frame.toTooltip().c_str());
			if (payload->IsDelivery()) {
				_rearrangeFrom = *reinterpret_cast<const size_t*>(payload->Data);
				_rearrangeTo = i;
				_rearrange = true;
			}
		}

		ImGui::EndDragDropTarget();
	}

	if (frame.upPressed() && i > 0) {
		_rearrangeFrom = i;
		_rearrangeTo = i-1;
		_rearrange = true;

		// Make cursor follow the up/down arrows, so you can click them
		// many times in succession to move items quickly
		auto& prevFrame = getFrame(i-1);
		double yinc = -static_cast<double>(_heights[prevFrame.elem().id()]);
		_window->incCursorPosition(0, yinc);
	}

	if (frame.downPressed() && i < _frameOrder.size() - 1) {
		_rearrangeFrom = i;
		_rearrangeTo = i+1;
		_rearrange = true;

		auto& nextFrame = getFrame(i+1);
		double yinc = static_cast<double>(_heights[nextFrame.elem().id()]);
		_window->incCursorPosition(0, yinc);
	}
}

void SliderPanel::showFrames() {
	// Handle slider removal
	std::erase_if(_frameOrder, [this](unsigned int id) {
		auto& frame = _frames.at(id);

		if (!frame.exists) {
			SliderRemovedEvent e { id };
			_vars->eventRouter().routeEvent(e);
			_sliders.erase(id);
			_frames.erase(id);
			_heights.erase(id);
			return true;
		}

		return false;
	});

	for (size_t i = 0; i < _frameOrder.size(); i++) {
		showFrame(i);
	}

	if (_rearrange) {
		int from = static_cast<int>(_rearrangeFrom);
		int to = static_cast<int>(_rearrangeTo);
		if (std::abs(from - to) == 1) {
			// slightly more efficient than erase/insert
			std::swap(_frameOrder[_rearrangeFrom], _frameOrder[_rearrangeTo]);
		} else {
			auto id = _frameOrder[_rearrangeFrom];
			_frameOrder.erase(_frameOrder.begin() + _rearrangeFrom);
			_frameOrder.insert(_frameOrder.begin() + _rearrangeTo, id);
		}

		_rearrange = false;
	}
}

bool SliderPanel::hasSlider(char c) {
	for (const auto& [id, slider] : _sliders) {
		if (slider.var() == c) return true;
	}

	return false;
}

char SliderPanel::findFirstAvailableVar() {
	for (char c = 'a'; c <= 'z'; c++) {
		// NOTE: This is a bit slow (iterating over all sliders every
		// time we check) but in practice it doesn't seem to matter, so
		// stick with simpler algorithm
		if (!hasSlider(c) && SliderElement::varValid(c)) return c;
	}

	for (char c = 'A'; c <= 'Z'; c++) {
		if (!hasSlider(c) && SliderElement::varValid(c)) return c;
	}

	return '\0';
}

void SliderPanel::show() {
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		for (auto& [_, frame] : _frames) {
			frame.exists = false;
		}
	}

	showFrames();
}

void SliderPanel::addSlider(char c) {
	_sliders.emplace(_nextSliderId, SliderElement(*_vars, _nextSliderId, c));
	auto& newSlider = _sliders.at(_nextSliderId);

	_frames.emplace(_nextSliderId, RearrangeFrame(newSlider));
	_heights[_nextSliderId] = 0;
	_frameOrder.push_back(_nextSliderId);


	_nextSliderId++;
}

void VariableWindow::drawUi() {
	_sliders.show();
}
}
