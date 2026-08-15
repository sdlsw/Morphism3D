#include "ui/common.h"

namespace g3d {
template<>
void resettableSlider<float, float>(
	const std::string& label,
	float* setting,
	const float& _default,
	float min,
	float max
) {
	ImGui::SliderFloat(label.c_str(), setting, min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableSlider<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float min,
	float max
) {
	ImGui::SliderFloat3(label.c_str(), glm::value_ptr(*setting), min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableDrag<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float inc
) {
	ImGui::DragFloat3(label.c_str(), glm::value_ptr(*setting), inc);
	resetButton(label, setting, _default);
}

void UiWindow::show() {
	if (!open) return;

	ImGui::Begin(title().c_str(), &open);
	ImGui::PushItemWidth(200.0f);
	drawUi();
	ImGui::PopItemWidth();
	ImGui::End();
}

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

unsigned int RearrangeablePanel::_nextPanelId = 1;

RearrangeFrame& RearrangeablePanel::getFrame(size_t i) {
	return _frames.at(_frameOrder[i]);
}

void RearrangeablePanel::showFrame(size_t i) {
	auto& frame = getFrame(i);

	ImVec2 start = ImGui::GetCursorScreenPos();
	ImGui::BeginGroup();
	bool headerShown = frame.showHeader();

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
		ImGui::SetDragDropPayload(_dragDropTarget.c_str(), &i, sizeof(size_t));
		ImGui::Text(frame.fromTooltip().c_str());
		ImGui::EndDragDropSource();
	}

	if (headerShown) frame.showBody();
	ImGui::EndGroup();
	ImVec2 end = ImGui::GetCursorScreenPos();

	_heights[frame.elem().id()] = static_cast<unsigned int>(end.y - start.y);

	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
			_dragDropTarget.c_str(),
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

void RearrangeablePanel::showFrames() {
	// Handle frame removal
	std::erase_if(_frameOrder, [this](unsigned int id) {
		auto& frame = _frames.at(id);

		if (!frame.exists) {
			frame.elem().shouldShow = false;
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

void RearrangeablePanel::addFrame(UiElement& elem) {
	auto id = elem.id();
	_frames.emplace(id, RearrangeFrame(elem));
	_heights[id] = 0;
	_frameOrder.push_back(id);
}

void RearrangeablePanel::removeAllFrames() {
	for (auto& [_, frame] : _frames) {
		frame.exists = false;
	}
}

void RearrangeablePanel::show() {
	showFrames();
}
}
