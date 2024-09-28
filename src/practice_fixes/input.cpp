#include "practice_fixes.hpp"

void InputPracticeFixes::applyFixes(PlayLayer* pl, PlayerData p1Data, PlayerData p2Data, int frame) {
	if (!pl) return;
	
	auto& g = Global::get();
	bool twoPlayers = pl->m_levelSettings->m_twoPlayerMode;

	eraseActions(frame);

	std::vector<button> foundButtons = findButtons();

	std::vector<int> notFoundButtons = fixInputs(foundButtons, pl, p1Data, p2Data, frame);
	for (auto& it : notFoundButtons) {
		if ((g.heldButtons[it] && twoPlayers) || ((g.heldButtons[it] || g.heldButtons[it + (it >= 3 ? -3 : 3)]) && !twoPlayers)) {
			if (it == 0 || it == 3)
				g.delayedFrameInput[static_cast<int>(it == 3 && twoPlayers)] = frame + 1;
			else
				g.macro.inputs.push_back(input(frame, indexButton[it], !(it > 2), true));

		}
	}
}

void InputPracticeFixes::eraseActions(int frame) {
	auto& g = Global::get();

	auto& inputs = g.macro.inputs;

	if (!inputs.empty()) {
		while (inputs.back().frame >= frame && !inputs.empty())
        	inputs.pop_back();
	}

	auto& frameFixes = g.macro.frameFixes;

	if (frameFixes.empty()) return;

	while (frameFixes.back().frame >= frame && !frameFixes.empty())
        frameFixes.pop_back();
}

std::vector<button> InputPracticeFixes::findButtons() {
	auto& g = Global::get();
	std::vector<button> result;

	for (auto it = g.macro.inputs.rbegin(); it != g.macro.inputs.rend(); ++it) {
		bool breakIteration = false;
		for (auto& el : result) {
			if (el.button == it->button && el.player2 == it->player2) {
				breakIteration = true;
				break;
			}
		}
		if (breakIteration) continue;

		result.push_back({ it->button, it->player2, it->down });
		if (result.size() >= 6) break;
	}
	return result;
}

std::vector<int> InputPracticeFixes::fixInputs(std::vector<button> foundButtons, PlayLayer* pl, PlayerData p1Data, PlayerData p2Data, int frame) {
	auto& g = Global::get();

	bool twoPlayers = pl->m_levelSettings->m_twoPlayerMode;
	std::vector<int> notFoundButtons;

	if (twoPlayers)
		notFoundButtons = { 0, 1, 2, 3, 4, 5 };
	else
		notFoundButtons = { 0, 1, 2 };

	for (auto& it : foundButtons) {
		int btnIndex = twoPlayers ? (it.button - 1 + (3 * static_cast<int>(!it.player2))) : (it.button - 1);
		bool player2 = twoPlayers ? !it.player2 : false;
		bool jumpBtn = btnIndex == 0 || btnIndex == 3;

		if (it.down && jumpBtn && (p1Data.m_isRobot || p2Data.m_isRobot)) {
			g.heldButtons[0] = false;
			g.heldButtons[3] = false;
		}

		if ((p1Data.m_isDashing || p2Data.m_isDashing) && jumpBtn) {
			g.heldButtons[0] = false;
			g.heldButtons[3] = false;
		}

		if ((p1Data.m_currentSlope2 || p2Data.m_currentSlope2) && pl->m_levelSettings->m_platformerMode) {
			g.heldButtons[1] = false;
			g.heldButtons[2] = false;
			g.heldButtons[4] = false;
			g.heldButtons[5] = false;

			g.ignoreFrame = frame + 3;
		}

		if (!twoPlayers) {
			if ((g.heldButtons[1] || g.heldButtons[4]) && (g.heldButtons[2] || g.heldButtons[5])) {
				g.heldButtons[1] = false;
				g.heldButtons[4] = false;
			}
		}
		else {
			if (g.heldButtons[1] && g.heldButtons[2])
				g.heldButtons[1] = false;

			if (g.heldButtons[4] && g.heldButtons[5])
				g.heldButtons[4] = false;
		}

		bool holdingButton = ((g.heldButtons[btnIndex] && twoPlayers) || (!twoPlayers && (g.heldButtons[btnIndex] || g.heldButtons[btnIndex + 3])));
		
		if (!it.down && holdingButton) {
			if (btnIndex != 0 && btnIndex != 3) {
				g.macro.inputs.push_back(input(frame, it.button, !player2, true));
			}
			else
				g.delayedFrameInput[static_cast<int>(btnIndex == 3)] = frame + 1;
		}
		else if (it.down && holdingButton && (btnIndex == 0 || btnIndex == 3))
			g.delayedFrameInput[static_cast<int>(btnIndex == 3)] = frame + 1;

		if (it.down && !holdingButton) {
			if (btnIndex == 0 || btnIndex == 3)
				g.delayedFrameReleaseMain[player2] = frame + 1;
			else {
				bool rightKey = static_cast<bool>(btnIndex > 3 ? btnIndex - 4 : btnIndex - 1);
				if (g.heldButtons[btnIndex + (rightKey ? -1 : 1)]) {
					g.macro.inputs.push_back(input(frame, it.button, !player2, false));
				}
				else {
					g.delayedFrameRelease[static_cast<int>(player2)][static_cast<int>(rightKey)] = frame + 1;
					g.addSideHoldingMembers[static_cast<int>(player2)] = true;
				}
			}
		}

		for (int i = 0; i < notFoundButtons.size(); i++) {
			if (notFoundButtons[i] == btnIndex)
				notFoundButtons.erase(notFoundButtons.begin() + i);
		}
	}

	return notFoundButtons;
}