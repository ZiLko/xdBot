#pragma once

#include <iostream>
#include <vector>
#include <optional>

#include "json.hpp"

#include <Geode/utils/VersionInfo.hpp>

geode::prelude::VersionInfo getVersion(std::vector<std::string> nums);

cocos2d::CCPoint dataFromString(std::string dataString);

std::vector<std::string> splitByChar(std::string str, char splitChar);

const std::string xdBotVersion = "v2.3.6";

namespace gdr {

	using namespace nlohmann;

	struct Bot {
		std::string name;
		std::string version;

		inline Bot(std::string const& name, std::string const& version)
			: name(name), version(version) {}
	};

	struct Level {
		uint32_t id;
		std::string name;

		Level() = default;

		inline Level(std::string const& name, uint32_t id = 0)
			: name(name), id(id) {}
	};

	struct FrameData {
		cocos2d::CCPoint pos = { 0.f, 0.f };
		float rotation = 0.f;
		bool rotate = true;
	};

	struct FrameFix {
		int frame;
		FrameData p1;
		FrameData p2;
	};

	class Input {
	protected:
		Input() = default;
		template <typename, typename>
		friend class Replay;
	public:
		uint32_t frame;
		int button;
		bool player2;
		bool down;

		inline virtual void parseExtension(json::object_t obj) {}
		inline virtual json::object_t saveExtension() const {
			return {};
		}

		inline Input(int frame, int button, bool player2, bool down)
			: frame(frame), button(button), player2(player2), down(down) {}


		inline static Input hold(int frame, int button, bool player2 = false) {
			return Input(frame, button, player2, true);
		}

		inline static Input release(int frame, int button, bool player2 = false) {
			return Input(frame, button, player2, false);
		}

		inline bool operator<(Input const& other) const {
			return frame < other.frame;
		}
	};

	template <typename S = void, typename T = Input>
	class Replay {
		Replay() = default;
	public:
		using InputType = T;
		using Self = std::conditional_t<std::is_same_v<S, void>, Replay<S, T>, S>;

		std::string author;
		std::string description;

		float duration;
		float gameVersion;
		float version = 1.0;

		float framerate = 240.f;

		int seed = 0;
		int coins = 0;

		bool ldm = false;

		Bot botInfo;
		Level levelInfo;

		std::vector<InputType> inputs;
		std::vector<FrameFix> frameFixes;

		uint32_t frameForTime(double time)
		{
			return static_cast<uint32_t>(time * (double)framerate);
		}

		virtual void parseExtension(json::object_t obj) {}
		virtual json::object_t saveExtension() const {
			return {};
		}

		Replay(std::string const& botName, std::string const& botVersion)
			: botInfo(botName, botVersion) {}

		static Self importData(std::vector<uint8_t> const& data, bool importInputs = true) {
			Self replay;
			json replayJson;

			try {
				if (json::accept(data)) {
						replayJson = json::parse(data);
				}
				else {
						replayJson = json::from_msgpack(data);
				}
			} catch(const json::parse_error& e) {
				return replay;
			}

			// try {
			// 	replayJson = json::from_msgpack(data);
			// } catch(std::exception& e) {
			// 	replayJson = json::parse(data);
			// }

			if (!replayJson["gameVersion"].is_null()) replay.gameVersion = replayJson["gameVersion"];
			if (!replayJson["description"].is_null()) replay.description = replayJson["description"];
			if (!replayJson["version"].is_null()) replay.version = replayJson["version"];
			if (!replayJson["duration"].is_null()) replay.duration = replayJson["duration"];
			if (!replayJson["author"].is_null()) replay.author = replayJson["author"];
			if (!replayJson["seed"].is_null()) replay.seed = replayJson["seed"];
			if (!replayJson["coins"].is_null()) replay.coins = replayJson["coins"];
			if (!replayJson["ldm"].is_null()) replay.ldm = replayJson["ldm"];

			if (!replayJson["bot"]["name"].is_null()) replay.botInfo.name = replayJson["bot"]["name"];
			if (!replayJson["bot"]["version"].is_null()) replay.botInfo.version = replayJson["bot"]["version"];
			if (!replayJson["level"]["id"].is_null()) replay.levelInfo.id = replayJson["level"]["id"];
			if (!replayJson["level"]["name"].is_null()) replay.levelInfo.name = replayJson["level"]["name"];

			std::string ver = replay.botInfo.version;
			
			if (replayJson.contains("framerate"))
				replay.framerate = replayJson["framerate"];

			bool rotation = ver.find("beta.") == std::string::npos && ver.find("alpha.") == std::string::npos;
			if (replay.botInfo.name == "xdBot" && ver == "v2.0.0") rotation = true;

			// bool addOffset = false;
			bool addOffset = replay.botInfo.name == "xdBot";

			if (addOffset) {
				if (ver.front() == 'v') ver = ver.substr(1);

				std::vector<std::string> splitVer = splitByChar(ver, '.');

				if (splitVer.size() <= 3) {
					std::vector<std::string> realVer = {"2", "3", "6"};

					geode::prelude::VersionInfo macroVer = getVersion(splitVer);
					geode::prelude::VersionInfo checkVer = getVersion(realVer);

					if (macroVer >= checkVer) addOffset = false;
				}
			}

			replay.parseExtension(replayJson.get<json::object_t>());

			if (!importInputs)
				return replay;

			for (json const& inputJson : replayJson["inputs"]) {
				InputType input;

				if (!inputJson.contains("frame")) continue;
				if (inputJson["frame"].is_null()) continue;

				input.frame = inputJson["frame"].get<int>() + (addOffset ? 1 : 0);
				input.button = inputJson["btn"];
				input.player2 = inputJson["2p"];
				input.down = inputJson["down"];
				input.parseExtension(inputJson.get<json::object_t>());

				replay.inputs.push_back(input);
			}

			if (!replayJson.contains("frameFixes")) return replay;

			for (json const& frameFixJson : replayJson["frameFixes"]) {
				FrameFix frameFix;

				if (!frameFixJson.contains("frame")) continue;
				if (frameFixJson["frame"].is_null()) continue;

				frameFix.frame = frameFixJson["frame"].get<int>() + (addOffset ? 1 : 0);

				if (frameFixJson.contains("player1")) {

					frameFix.p1.pos = dataFromString(frameFixJson["player1"]);
					frameFix.p1.rotate = false;
					
					frameFix.p2.pos = dataFromString(frameFixJson["player2"]);
					frameFix.p2.rotate = false;

				}
				else if (frameFixJson.contains("player1X")) {

					frameFix.p1.pos = ccp(frameFixJson["player1X"], frameFixJson["player1Y"]);
					frameFix.p1.rotate = false;
					
					frameFix.p2.pos = ccp(frameFixJson["player2X"], frameFixJson["player2Y"]);
					frameFix.p2.rotate = false;

				} else if (frameFixJson.contains("p1")) {
					if (replay.botInfo.name != "xdBot") rotation = false;

					if (frameFixJson["p1"].contains("x"))
						frameFix.p1.pos.x = frameFixJson["p1"]["x"];

					if (frameFixJson["p1"].contains("y"))
						frameFix.p1.pos.y = frameFixJson["p1"]["y"];

					if (frameFixJson["p1"].contains("r") && rotation)
						frameFix.p1.rotation = frameFixJson["p1"]["r"];

					if (frameFixJson.contains("p2")) {
						if (frameFixJson["p2"].contains("x"))
							frameFix.p2.pos.x = frameFixJson["p2"]["x"];

						if (frameFixJson["p2"].contains("y"))
							frameFix.p2.pos.y = frameFixJson["p2"]["y"];

						if (frameFixJson["p2"].contains("r") && rotation)
							frameFix.p2.rotation = frameFixJson["p2"]["r"];
					}
				} else continue;

				replay.frameFixes.push_back(frameFix);
			}

			return replay;
		}

		std::vector<uint8_t> exportData(bool exportJson = false) {
			json replayJson = saveExtension();
			replayJson["gameVersion"] = gameVersion;
			replayJson["description"] = description;
			replayJson["version"] = version;
			replayJson["duration"] = duration;
			replayJson["bot"]["name"] = botInfo.name;
			replayJson["bot"]["version"] = botInfo.version;
			replayJson["level"]["id"] = levelInfo.id;
			replayJson["level"]["name"] = levelInfo.name;
			replayJson["author"] = author;
			replayJson["seed"] = seed;
			replayJson["coins"] = coins;
			replayJson["ldm"] = ldm;
			replayJson["framerate"] = framerate;

			for (InputType const& input : inputs) {
				json inputJson = input.saveExtension();
				inputJson["frame"] = input.frame;
				inputJson["btn"] = input.button;
				inputJson["2p"] = input.player2;
				inputJson["down"] = input.down;

				replayJson["inputs"].push_back(inputJson);
			}

			for (FrameFix const& frameFix : frameFixes) {
				json frameFixJson;

				json p1Json;
				json p2Json;

				if (frameFix.p1.pos.x != 0.f) p1Json["x"] = frameFix.p1.pos.x;
				if (frameFix.p1.pos.y != 0.f) p1Json["y"] = frameFix.p1.pos.y;
				if (frameFix.p1.rotation != 0.f) p1Json["r"] = frameFix.p1.rotation;

				if (frameFix.p2.pos.x != 0.f) p2Json["x"] = frameFix.p2.pos.x;
				if (frameFix.p2.pos.y != 0.f) p2Json["y"] = frameFix.p2.pos.y;
				if (frameFix.p2.rotation != 0.f) p2Json["r"] = frameFix.p2.rotation;

				if (p1Json.empty() && p2Json.empty()) continue;

				frameFixJson["frame"] = frameFix.frame;
				frameFixJson["p1"] = p1Json;

				if (frameFix.p2.pos.y != 0.f)
					frameFixJson["p2"] = p2Json;

				replayJson["frameFixes"].push_back(frameFixJson);
			}

			if (exportJson) {
				std::string replayString = replayJson.dump();
				return std::vector<uint8_t>(replayString.begin(), replayString.end());
			}
			else {
				return json::to_msgpack(replayJson);
			}
		}
	};

}