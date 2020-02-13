//---------------------------------------------------------------------------//
// Copyright (c) 2020 Eleftherios Avramidis <ea461@cam.ac.uk>
// Research Computing Services, University of Cambridge, UK
//
// Distributed under The MIT License (MIT)
// See accompanying file LICENSE
//---------------------------------------------------------------------------//

#include <iostream>
#include <algorithm>
#include <numeric>

#include "ModelParameters.hpp"
#include "Model.hpp"
#include "Agent.hpp"
#include "HelpFunctions.hpp"
#include "H5Cpp.h"

namespace station_sim {

	Model::~Model() = default;

	Model::Model(int unique_id, const std::shared_ptr<ModelParameters> model_parameters)
	{
		this->model_parameters = model_parameters;
		initialize_model(unique_id);
		print_per_steps = 100;

		history_state = std::vector<std::vector<Point2D>>(model_parameters->get_step_limit(),
				std::vector<Point2D>(model_parameters->get_population_total()));
	}

	void Model::initialize_model(int unique_id)
	{
		generator = new std::mt19937(model_parameters->get_random_seed());

		model_id = unique_id;
		status = ModelStatus::active;
		step_id = 0;
		pop_active = 0;
		pop_finished = 0;
		history_collisions_number = 0;
		wiggle_collisions_number = 0;

		speed_step =
				(model_parameters->get_speed_mean()-model_parameters->get_speed_min())
						/model_parameters->get_speed_steps();

		set_boundaries();
		set_gates_locations();

		generate_agents();
	}

	void Model::set_boundaries()
	{
		boundaries[0].x = 0; // x1
		boundaries[0].y = 0; // y1
		boundaries[1].x = model_parameters->get_space_width();  // x2
		boundaries[1].y = model_parameters->get_space_height(); // y2
	}

	void Model::set_gates_locations()
	{
		gates_in_locations.resize(model_parameters->get_gates_in());
		gates_out_locations.resize(model_parameters->get_gates_out());

		create_gates(gates_in_locations, 0, model_parameters->get_space_height(), model_parameters->get_gates_in()+2);
		create_gates(gates_out_locations, model_parameters->get_space_width(), model_parameters->get_space_height(),
				model_parameters->get_gates_out()+2);
	}

	void Model::create_gates(std::vector<Point2D>& gates, float x, float y, int gates_number)
	{
		std::vector<float> result = HelpFunctions::linear_spaced_vector(0, y, gates_number);
		result.erase(result.begin());
		result.pop_back();

		for (unsigned long i = 0; i<result.size(); i++) {
			gates[i].x = x;
			gates[i].y = result[i];
		}
	}

	void Model::generate_agents()
	{
		for (int i = 0; i<model_parameters->get_population_total(); i++) {
			agents.emplace_back(Agent(i, *this, *model_parameters));
		}
	}

	const std::vector<Point2D>& Model::get_gates_in_locations() const
	{
		return gates_in_locations;
	}

	const std::vector<Point2D>& Model::get_gates_out_locations() const
	{
		return gates_out_locations;
	}

	void Model::step()
	{
		if (pop_finished<model_parameters->get_population_total() && step_id<model_parameters->get_step_limit()
				&& status==ModelStatus::active) {
			if (model_parameters->is_do_print() && step_id%print_per_steps==0) {
				std::cout << "\tIteration: " << step_id << "/" << model_parameters->get_step_limit() << std::endl;
			}

			// get agents and move them
			move_agents();

			if (model_parameters->is_do_history()) {
				history_state[step_id] = get_agents_location();
			}

			step_id += 1;
		}
		else {
			if (pop_finished<model_parameters->get_population_total()) {
				status = ModelStatus::finished;

				if (model_parameters->is_do_print() && status==ModelStatus::active) {
					std::cout << "StationSim " << model_id << " - Everyone made it!" << std::endl;
				}
			}
		}
	}

	int Model::get_history_collisions_number() const
	{
		return history_collisions_number;
	}

	void Model::set_history_collisions_number(int history_collisions_number)
	{
		this->history_collisions_number = history_collisions_number;
	}

	void Model::increase_history_collisions_number_by_value(int value_increase)
	{
		history_collisions_number += value_increase;
	}

	void Model::add_to_history_collision_locations(Point2D new_location)
	{
		history_collision_locations.push_back(new_location);
	}

	void Model::increase_wiggle_collisions_number_by_value(int value_increase)
	{
		wiggle_collisions_number += value_increase;
	}

	void Model::add_to_history_wiggle_locations(Point2D new_location)
	{
		history_wiggle_locations.push_back(new_location);
	}

	void Model::move_agents()
	{
		for (auto& agent:agents) {
			agent.step(*this, *model_parameters);
		}
	}

	int Model::get_unique_id() const
	{
		return model_id;
	}

	float Model::get_speed_step() const
	{
		return speed_step;
	}

	std::mt19937* Model::get_generator() const
	{
		return generator;
	}

	std::vector<Point2D> Model::get_agents_location()
	{
		std::vector<Point2D> agents_locations;

		for (const Agent& agent:agents) {
			agents_locations.push_back(agent.get_agent_location());
		}

		return std::vector<Point2D>();
	}

	void Model::calculate_print_model_run_analytics()
	{
		std::cout << "Finish step number: " << step_id << std::endl;
		std::cout << "Population size reached target: " << pop_finished << std::endl;
		std::cout << "Population size active: " << pop_active << std::endl;
		std::cout << "Mean steps taken per agent: "
				  << std::reduce(steps_taken.begin(), steps_taken.end(), 0.0)/agents.size() << std::endl;
		std::cout << "Expected steps per agent: "
				  << std::reduce(steps_expected.begin(), steps_expected.end(), 0.0)/agents.size() << std::endl;
		std::cout << "Mean time delay per agent: "
				  << std::reduce(steps_delay.begin(), steps_delay.end(), 0.0)/agents.size() << std::endl;
		std::cout << "Mean number of collisions per agent: "
				  << std::accumulate(agents.begin(), agents.end(), 0.0, [&](int total, const Agent& agent) {
					return total += agent.get_history_collisions();
				  })/agents.size() << std::endl;
		std::cout << "Mean number of wiggles per agent: "
				  << std::accumulate(agents.begin(), agents.end(), 0.0, [&](int total, const Agent& agent) {
					return total += agent.get_history_wiggles();
				  })/agents.size() << std::endl;

	}

	const std::shared_ptr<ModelParameters>& Model::get_model_parameters() const
	{
		return model_parameters;
	}

	bool Model::model_simulation_finished()
	{
		return pop_finished==model_parameters->get_population_total();
	}

	void Model::write_model_output_to_hdf5()
	{
		int steps_done_by_model = step_id;
		int RANK = 3;

		// Create a file
		H5::H5File file("model_run.h5", H5F_ACC_TRUNC);

		// Create the data space for the dataset.
		hsize_t dims[3];
		dims[0] = steps_done_by_model;
		dims[1] = this->agents.size();
		dims[2] = 2;
		H5::DataSpace dataspace(RANK, dims);

		// Create the dataset
		H5::DataSet dataset = file.createDataSet("agents_locations", H5::PredType::NATIVE_FLOAT, dataspace);

		float agents_locations[steps_done_by_model][this->agents.size()][2];

		for (int i = 0; i<agents.size(); i++) {

			std::vector<float> agent_location_history_x;
			std::vector<float> agent_location_history_y;

			for (int j = 0; j<steps_done_by_model; j++) {
				if (agents[i].get_history_locations().size()<=steps_done_by_model) {
					agents_locations[j][i][0] = agents[i].get_history_locations()[j].x;
					agents_locations[j][i][1] = agents[i].get_history_locations()[j].y;
				}
			}
		}

		// Write the data to the dataset using default memory space, file
		// space, and transfer properties.
		dataset.write(agents_locations, H5::PredType::IEEE_F32LE);

		dataspace.close();
		dataset.close();
		file.close();
	}
}