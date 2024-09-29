#pragma once

#include <iostream>
#include <vector>
#include <optional>

#include "json.hpp"
#include <Geode/Geode.hpp>

cocos2d::CCPoint dataFromString(std::string dataString);

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

			if (json::accept(data) == 0) {
				replayJson = json::from_msgpack(data);
			}
			else {
				replayJson = json::parse(data);
			}

			// try {
			// 	replayJson = json::from_msgpack(data);
			// } catch(std::exception& e) {
			// 	replayJson = json::parse(data);
			// }

			replay.gameVersion = replayJson["gameVersion"];
			replay.description = replayJson["description"];
			replay.version = replayJson["version"];
			replay.duration = replayJson["duration"];
			replay.botInfo.name = replayJson["bot"]["name"];
			replay.botInfo.version = replayJson["bot"]["version"];
			replay.levelInfo.id = replayJson["level"]["id"];
			replay.levelInfo.name = replayJson["level"]["name"];
			replay.author = replayJson["author"];
			replay.seed = replayJson["seed"];
			replay.coins = replayJson["coins"];
			replay.ldm = replayJson["ldm"];

			if (replayJson.contains("framerate"))
				replay.framerate = replayJson["framerate"];
			replay.parseExtension(replayJson.get<json::object_t>());

			if (!importInputs)
				return replay;

			for (json const& inputJson : replayJson["inputs"]) {
				InputType input;
				input.frame = inputJson["frame"];
				input.button = inputJson["btn"];
				input.player2 = inputJson["2p"];
				input.down = inputJson["down"];
				input.parseExtension(inputJson.get<json::object_t>());

				replay.inputs.push_back(input);
			}

			for (json const& frameFixJson : replayJson["frameFixes"]) {
				FrameFix frameFix;

				if (!frameFixJson.contains("frame")) continue;

				frameFix.frame = frameFixJson["frame"];

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
					if (frameFixJson["p1"].contains("x"))
						frameFix.p1.pos.x = frameFixJson["p1"]["x"];

					if (frameFixJson["p1"].contains("y"))
						frameFix.p1.pos.y = frameFixJson["p1"]["y"];

					if (frameFixJson["p1"].contains("r"))
						frameFix.p1.rotation = frameFixJson["p1"]["r"];

					if (frameFixJson.contains("p2")) {
						if (frameFixJson["p2"].contains("x"))
							frameFix.p2.pos.x = frameFixJson["p2"]["x"];

						if (frameFixJson["p2"].contains("y"))
							frameFix.p2.pos.y = frameFixJson["p2"]["y"];

						if (frameFixJson["p2"].contains("r"))
							frameFix.p2.rotation= frameFixJson["p2"]["r"];
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