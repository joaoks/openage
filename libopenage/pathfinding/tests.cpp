// Copyright 2015-2024 the openage authors. See copying.md for legal info.

#include <deque>
#include <random>
#include <vector>

#include "log/log.h"
#include "testing/testing.h"

#include "coord/chunk.h"
#include "coord/tile.h"
#include "pathfinding/cost_field.h"
#include "pathfinding/definitions.h"
#include "pathfinding/flow_field.h"
#include "pathfinding/grid.h"
#include "pathfinding/integration_field.h"
#include "pathfinding/integrator.h"
#include "pathfinding/path.h"
#include "pathfinding/pathfinder.h"
#include "pathfinding/sector.h"
#include "pathfinding/types.h"
#include "time/time.h"


namespace openage {
namespace path {
namespace tests {

void flow_field() {
	// Create initial cost grid
	auto cost_field = std::make_shared<CostField>(3);

	// | 1 | 1 | 1 |
	// | 1 | X | 1 |
	// | 1 | 1 | 1 |
	const time::time_t time = time::TIME_ZERO;
	cost_field->set_costs({1, 1, 1, 1, 255, 1, 1, 1, 1}, time);

	// Test the different field types
	{
		auto integration_field = std::make_shared<IntegrationField>(3);
		integration_field->integrate_cost(cost_field, coord::tile_delta{2, 2});
		auto &int_cells = integration_field->get_cells();

		// The integration field should look like:
		// | 4 | 3 | 2 |
		// | 3 | X | 1 |
		// | 2 | 1 | 0 |
		auto int_expected = std::vector<integrated_cost_t>{
			4,
			3,
			2,
			3,
			65535,
			1,
			2,
			1,
			0,
		};

		// The flow field for targeting (2, 2) hould look like this:
		// | E  | SE | S |
		// | SE | X  | S |
		// | E  | E  | N |
		auto ff_expected = std::vector<flow_t>{
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH_EAST),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH_EAST),
			0,
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_TARGET_MASK | FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::NORTH),
		};

		// Compare the integration field cells with the expected values
		for (size_t i = 0; i < int_cells.size(); i++) {
			TESTEQUALS(int_cells[i].cost, int_expected[i]);
		}

		// Build the flow field
		auto flow_field = std::make_shared<FlowField>(3);
		flow_field->build(integration_field);
		auto ff_cells = flow_field->get_cells();

		// Compare the flow field cells with the expected values
		for (size_t i = 0; i < ff_cells.size(); i++) {
			TESTEQUALS(ff_cells[i], ff_expected[i]);
		}
	}

	// Integrator test
	{
		// Integrator for managing the flow field
		auto integrator = std::make_shared<Integrator>();

		// Build the flow field
		auto flow_field = integrator->get(cost_field, coord::tile_delta{2, 2}).second;
		auto &ff_cells = flow_field->get_cells();

		// The flow field for targeting (2, 2) hould look like this:
		// | E  | SE | S |
		// | SE | X  | S |
		// | E  | E  | N |
		auto ff_expected = std::vector<flow_t>{
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH_EAST),
			FLOW_LOS_MASK | FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH),
			FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH_EAST),
			0,
			FLOW_LOS_MASK | FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::SOUTH),
			FLOW_LOS_MASK | FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_LOS_MASK | FLOW_PATHABLE_MASK | static_cast<uint8_t>(flow_dir_t::EAST),
			FLOW_LOS_MASK | FLOW_PATHABLE_MASK | FLOW_TARGET_MASK,
		};

		// Compare the flow field cells with the expected values
		for (size_t i = 0; i < ff_cells.size(); i++) {
			TESTEQUALS(ff_cells[i], ff_expected[i]);
		}
	}
}

void grid_from_sectors() {
	std::vector<std::shared_ptr<Sector>> sectors;
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < 3; x++) {
			sectors.push_back(std::make_shared<Sector>(sectors.size(), coord::chunk(x, y), 5));
		}
	}

	Grid grid{0, util::Vector2s{3, 2}, std::move(sectors)};
	TESTEQUALS(grid.get_sector_size(), 5);
	TESTEQUALS(grid.get_sectors().size(), 6);
}


void pathfinder_random_maps() {
	// fixed seeds keep the test deterministic; together they cover every
	// combination of sector size, grid size and wall density
	for (unsigned seed = 0; seed < 225; ++seed) {
		std::mt19937 rng{seed};

		const size_t sector_size = std::vector<size_t>{4, 5, 8}.at(seed % 3);
		const util::Vector2s grid_size{2 + (seed / 3) % 5, 2 + (seed / 15) % 5};
		const double wall_chance = std::vector<double>{0.1, 0.25, 0.4}.at((seed / 75) % 3);
		const auto width = static_cast<coord::tile_t>(grid_size[0] * sector_size);
		const auto height = static_cast<coord::tile_t>(grid_size[1] * sector_size);

		std::bernoulli_distribution is_wall{wall_chance};
		std::vector<bool> passable(width * height);
		for (auto &&cell : passable) {
			cell = not is_wall(rng);
		}
		auto is_passable = [&](coord::tile_t x, coord::tile_t y) {
			return x >= 0 and y >= 0 and x < width and y < height and passable[x + y * width];
		};

		auto grid = std::make_shared<Grid>(0, grid_size, sector_size);
		for (const auto &sector : grid->get_sectors()) {
			auto origin = sector->get_position().to_tile(sector_size);
			std::vector<cost_t> costs;
			for (coord::tile_t y = 0; y < static_cast<coord::tile_t>(sector_size); ++y) {
				for (coord::tile_t x = 0; x < static_cast<coord::tile_t>(sector_size); ++x) {
					costs.push_back(is_passable(origin.ne + x, origin.se + y) ? COST_MIN : COST_IMPASSABLE);
				}
			}
			sector->get_cost_field()->set_costs(std::move(costs), time::TIME_ZERO);
		}
		grid->init_portals();
		grid->init_portal_nodes();

		Pathfinder pathfinder{};
		pathfinder.add_grid(grid);

		// reference solution: label the connected regions of passable cells
		std::vector<int> region(width * height, -1);
		int region_count = 0;
		for (coord::tile_t i = 0; i < width * height; ++i) {
			if (region[i] != -1 or not passable[i]) {
				continue;
			}
			std::deque<coord::tile_t> open_list{i};
			region[i] = region_count;
			while (not open_list.empty()) {
				auto current = open_list.front();
				open_list.pop_front();
				coord::tile_t x = current % width;
				coord::tile_t y = current / width;
				for (auto [nx, ny] : {std::pair{x, y - 1}, {x - 1, y}, {x, y + 1}, {x + 1, y}}) {
					if (is_passable(nx, ny) and region[nx + ny * width] == -1) {
						region[nx + ny * width] = region_count;
						open_list.push_back(nx + ny * width);
					}
				}
			}
			++region_count;
		}

		std::uniform_int_distribution<coord::tile_t> random_x{0, width - 1};
		std::uniform_int_distribution<coord::tile_t> random_y{0, height - 1};
		for (int request = 0; request < 150; ++request) {
			coord::tile start{random_x(rng), random_y(rng)};
			coord::tile target{random_x(rng), random_y(rng)};
			if (not is_passable(start.ne, start.se)) {
				continue;
			}

			auto path = pathfinder.get_path(PathRequest{0, start, target, time::TIME_ZERO});
			auto start_region = region[start.ne + start.se * width];
			bool reachable = is_passable(target.ne, target.se)
			                 and region[target.ne + target.se * width] == start_region;

			if (not reachable) {
				if (path.status != PathResult::NOT_FOUND) {
					TESTFAILMSG("seed " << seed << ": path from " << start << " to unreachable "
					                    << target << " was found");
				}
				continue;
			}

			if (path.status != PathResult::FOUND or path.waypoints.empty()) {
				TESTFAILMSG("seed " << seed << ": path from " << start << " to " << target
				                    << " was not found");
			}
			if (path.waypoints.front() != start or path.waypoints.back() != target) {
				TESTFAILMSG("seed " << seed << ": path from " << start << " to " << target
				                    << " does not begin at the start and end at the target");
			}
			for (const auto &waypoint : path.waypoints) {
				if (not is_passable(waypoint.ne, waypoint.se)
				    or region[waypoint.ne + waypoint.se * width] != start_region) {
					TESTFAILMSG("seed " << seed << ": path from " << start << " to " << target
					                    << " has invalid waypoint " << waypoint);
				}
			}
		}
	}
}

} // namespace tests
} // namespace path
} // namespace openage
