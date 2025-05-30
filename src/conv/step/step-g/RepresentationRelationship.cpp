/*                 RepresentationRelationship.cpp
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
/** @file step/RepresentationRelationship.cpp
 *
 * Routines to convert STEP "RepresentationRelationship" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "Representation.h"
#include "RepresentationRelationship.h"

#define CLASSNAME "RepresentationRelationship"
#define ENTITYNAME "Representation_Relationship"
string RepresentationRelationship::entityname = Factory::RegisterClass(ENTITYNAME, (FactoryMethod) RepresentationRelationship::Create);

RepresentationRelationship::RepresentationRelationship()
{
    step = NULL;
    id = 0;
    name = "";
    description = "";
    rep_1 = NULL;
    rep_2 = NULL;
}

RepresentationRelationship::RepresentationRelationship(STEPWrapper *sw, int step_id)
{
    step = sw;
    id = step_id;
    name = "";
    description = "";
    rep_1 = NULL;
    rep_2 = NULL;
}

RepresentationRelationship::~RepresentationRelationship()
{
    // created through factory will be deleted there.
    rep_1 = NULL;
    rep_2 = NULL;
}

string RepresentationRelationship::ClassName()
{
    return entityname;
}

Representation *
RepresentationRelationship::GetRepresentationRelationshipRep_1()
{
    return rep_1;
}

Representation *
RepresentationRelationship::GetRepresentationRelationshipRep_2()
{
    return rep_2;
}

bool RepresentationRelationship::Load(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    step = sw;
    id = sse->STEPfile_id;

    // need to do this for local attributes to makes sure we have
    // the actual entity and not a complex/supertype parent
    sse = step->getEntity(sse, ENTITYNAME);

    name = step->getStringAttribute(sse, "name");
    description = step->getStringAttribute(sse, "description");

    if (rep_1 == NULL) {
	SDAI_Application_instance *entity = step->getEntityAttribute(sse, "rep_1");
	if (entity) { //this attribute is optional
	    rep_1 = dynamic_cast<Representation *>(Factory::CreateObject(sw, entity));
	} else {
	    std::cout << CLASSNAME << ":Error loading attribute 'rep_1'." << std::endl;
	    sw->entity_status[id] = STEP_LOAD_ERROR;
	    return false;
	}
    }

    if (rep_2 == NULL) {
	SDAI_Application_instance *entity = step->getEntityAttribute(sse, "rep_2");
	if (entity) { //this attribute is optional
	    rep_2 = dynamic_cast<Representation *>(Factory::CreateObject(sw, entity));
	} else {
	    std::cout << CLASSNAME << ":Error loading attribute 'rep_2'." << std::endl;
	    sw->entity_status[id] = STEP_LOAD_ERROR;
	    return false;
	}
    }

    sw->entity_status[id] = STEP_LOADED;
    return true;
}

void RepresentationRelationship::Print(int level)
{
    TAB(level);
    std::cout << CLASSNAME << ":" << "(";
    std::cout << "ID:" << STEPid() << ")" << std::endl;

    TAB(level);
    std::cout << "Attributes:" << std::endl;
    TAB(level + 1);
    std::cout << "name:" << name << std::endl;
    TAB(level + 1);
    std::cout << "description:" << description << std::endl;
    TAB(level + 1);
    std::cout << "rep_1:" << std::endl;
    rep_1->Print(level + 2);
    TAB(level + 1);
    std::cout << "rep_2:" << std::endl;
    rep_2->Print(level + 2);
}

STEPEntity *
RepresentationRelationship::GetInstance(STEPWrapper *sw, int id)
{
    return new RepresentationRelationship(sw, id);
}

STEPEntity *
RepresentationRelationship::Create(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    return STEPEntity::CreateEntity(sw, sse, GetInstance, CLASSNAME);
}

bool RepresentationRelationship::LoadONBrep(ON_Brep *brep)
{
    std::cerr << "Error: ::LoadONBrep(ON_Brep *brep<" << std::hex << brep << std::dec << ">) not implemented for " << entityname << std::endl;
    return false;
}

double RepresentationRelationship::GetLengthConversionFactor()
{
    double d1 = rep_1->GetLengthConversionFactor();
    return d1;
}

double RepresentationRelationship::GetPlaneAngleConversionFactor()
{
    double d1 = rep_1->GetPlaneAngleConversionFactor();
    return d1;
}

double RepresentationRelationship::GetSolidAngleConversionFactor()
{
    double d1 = rep_1->GetSolidAngleConversionFactor();
    return d1;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
