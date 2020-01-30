//---------------------------------------------------------------------------//
// Copyright (c) 2020 Eleftherios Avramidis <ea461@cam.ac.uk>
// Research Computing Services, University of Cambridge, UK
//
// Distributed under The MIT License (MIT)
// See accompanying file LICENSE
//---------------------------------------------------------------------------//

#include "MultipleModelsRun.hpp"

namespace station_sim {

	MultipleModelsRun::MultipleModelsRun() = default;
	MultipleModelsRun::~MultipleModelsRun() = default;

	void MultipleModelsRun::run_all_models()
	{
#pragma omp parallel for schedule(static, 1) default(none)
		for (int i = 0; i<models.size(); i++) {
			run_model(i);
		}
	}

	void MultipleModelsRun::run_model(int model_index)
	{
		for (int i = 0; i<model_parameters[model_index].get_step_limit(); i++) {
			models[model_index].step(models[model_index], model_parameters[model_index]);
		}
	}

}

