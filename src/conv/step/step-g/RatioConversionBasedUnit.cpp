/*                 RatioConversionBasedUnit.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2025 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file step/RatioConversionBasedUnit.cpp
 *
 * Routines to convert STEP "RatioConversionBasedUnit" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "MeasureValue.h"
#include "Unit.h"
#include "MeasureWithUnit.h"
#include "RatioConversionBasedUnit.h"

#define CLASSNAME "RatioConversionBasedUnit"
#define ENTITYNAME "Ratio_Conversion_Based_Unit"
string RatioConversionBasedUnit::entityname = Factory::RegisterClass(ENTITYNAME, (FactoryMethod)RatioConversionBasedUnit::Create);

RatioConversionBasedUnit::RatioConversionBasedUnit()
{
    step = NULL;
    id = 0;
}

RatioConversionBasedUnit::RatioConversionBasedUnit(STEPWrapper *sw, int step_id)
{
    step = sw;
    id = step_id;
}

RatioConversionBasedUnit::~RatioConversionBasedUnit()
{
}

bool
RatioConversionBasedUnit::Load(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    step = sw;
    id = sse->STEPfile_id;


    // load base class attributes
    if (!RatioUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }
    if (!ConversionBasedUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }
    sw->entity_status[id] = STEP_LOADED;
    return true;
}

void
RatioConversionBasedUnit::Print(int level)
{
    TAB(level);
    std::cout << CLASSNAME << ":" << "(";
    std::cout << "ID:" << STEPid() << ")" << std::endl;

    TAB(level);
    std::cout << "Inherited Attributes:" << std::endl;
    RatioUnit::Print(level + 1);
    ConversionBasedUnit::Print(level + 1);

}

STEPEntity *
RatioConversionBasedUnit::GetInstance(STEPWrapper *sw, int id)
{
    return new RatioConversionBasedUnit(sw, id);
}

STEPEntity *
RatioConversionBasedUnit::Create(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    return STEPEntity::CreateEntity(sw, sse, GetInstance, CLASSNAME);
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
