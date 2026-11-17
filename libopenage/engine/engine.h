// Copyright 2023-2024 the openage authors. See copying.md for legal info.

#pragma once

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "renderer/window.h"
#include "util/path.h"


namespace openage {

namespace cvar {
class CVarManager;
} // namespace cvar

namespace gamestate {
class GameSimulation;
} // namespace gamestate


namespace time {
class TimeLoop;
} // namespace time


namespace engine {

/**
 * Encapsulates all subcomponents needed for a run.
 */
class Engine {
public:
	enum class mode {
		LEGACY,
		HEADLESS,
		FULL,
	};

	/**
	 * Create the engine instance for this run.
	 *
	 * @param mode The run mode to use.
	 * @param root_dir openage root directory.
	 * @param mods The mods to load.
	 * @param window_settings The settings to customize the display window (e.g. size, display mode, vsync).
	 */
	Engine(mode mode,
	       const util::Path &root_dir,
	       const std::vector<std::string> &mods,
	       const renderer::window_settings &window_settings = {});

	// engine should not be copied or moved
	Engine(const Engine &) = delete;
	Engine &operator=(const Engine &) = delete;
	Engine(Engine &&) = delete;
	Engine &operator=(Engine &&) = delete;
	~Engine() = default;


	/**
	 * Run the main loop.
	 */
	void loop();

	/**
	 * current simulation state variable.
	 * to be set to false to stop the simulation loop.
	 */
	bool running;

private:
	/**
	 * The run mode to use.
	 */
	mode run_mode;

	/**
	 * openage root directory.
	 */
	util::Path root_dir;

	/**
	 * Environment variables.
	 */
	std::shared_ptr<cvar::CVarManager> cvar_manager;

	/**
	 * Controls and update the clock for time-based measurements.
	 */
	std::shared_ptr<time::TimeLoop> time_loop;

	/**
	 * Gameplay simulation.
	 */
	std::shared_ptr<gamestate::GameSimulation> simulation;

	/**
	 * The threads used by the engine.
	 *
	 * Declared last so that they are stopped and joined before the other members
	 * are destroyed, which the threads may still use.
	 */
	std::vector<std::jthread> threads;
};

} // namespace engine
} // namespace openage
