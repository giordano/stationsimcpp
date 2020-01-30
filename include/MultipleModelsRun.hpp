//---------------------------------------------------------------------------//
// Copyright (c) 2020 Eleftherios Avramidis <ea461@cam.ac.uk>
// Research Computing Services, University of Cambridge, UK
//
// Distributed under The MIT License (MIT)
// See accompanying file LICENSE
//---------------------------------------------------------------------------//

#ifndef STATIONSIM_MULTIPLEMODELSRUN_HPP
#define STATIONSIM_MULTIPLEMODELSRUN_HPP

#include "stationsim_export.h"
#include <vector>
#include "ModelParameters.hpp"
#include "Agent.hpp"
#include "Model.hpp"

namespace station_sim {

	class STATIONSIM_EXPORT MultipleModelsRun {
	private:
		std::vector<Model> models;
		std::vector<ModelParameters> models_parameters;

	public:
		MultipleModelsRun();
		~MultipleModelsRun();

		virtual void run_all_models();
		void run_model(int model_index);

		void add_model_and_model_parameters(Model model, ModelParameters model_parameters);

		Model get_model(int index);

		ModelParameters get_model_parameters(int index);

		[[nodiscard]] int get_number_of_models();
	};
}

#endif