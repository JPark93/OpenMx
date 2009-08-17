/*
 *  Copyright 2007-2009 The OpenMx Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <R.h> 
#include <Rinternals.h> 
#include <Rdefines.h>
#include <R_ext/Rdynload.h> 
#include <R_ext/BLAS.h>
#include <R_ext/Lapack.h>
#include "omxAlgebraFunctions.h"

#ifndef _OMX_R_OBJECTIVE_
#define _OMX_R_OBJECTIVE_ TRUE

typedef struct {

	SEXP objfun;
	SEXP model;
	PROTECT_INDEX modelIndex;
	SEXP flatModel;
	SEXP parameters;
	SEXP state;
	PROTECT_INDEX stateIndex;

} omxRObjective;

void omxDestroyRObjective(omxObjective *oo) {
	
	UNPROTECT(5); 			// objfun, model, flatModel, parameters, and state
}

void omxCallRObjective(omxObjective *oo) {

	omxRObjective* rObjective = (omxRObjective*)oo->argStruct; 
	SEXP theCall, theReturn;
	PROTECT(theCall = allocVector(LANGSXP, 3));
	SETCAR(theCall, rObjective->objfun);
	SETCADR(theCall, rObjective->model);
	SETCADDR(theCall, rObjective->state);
	PROTECT(theReturn = eval(theCall, R_GlobalEnv));
	oo->matrix->data[0] = REAL(AS_NUMERIC(theReturn))[0];
	if (LENGTH(theReturn) > 1) {
		REPROTECT(rObjective->state = CADR(theReturn), rObjective->stateIndex);
	}

	UNPROTECT(2); // theCall and theReturn


}

// I have no idea what I'm supposed to do here...
unsigned short int omxNeedsUpdateRObjective(omxObjective* oo) {
	return(TRUE);
}

void omxRepopulateRObjective(omxObjective* oo, double* x, int n) {
	omxRObjective* rObjective = (omxRObjective*)oo->argStruct; 

	SEXP theCall, theReturn, estimate;

	PROTECT(estimate = allocVector(REALSXP, n));
	double *est = REAL(estimate);
	for(int i = 0; i < n ; i++) {
		est[i] = x[i];
	}

	PROTECT(theCall = allocVector(LANGSXP, 5));
	
	SETCAR(theCall, install("updateModelValues"));
	SETCADR(theCall, rObjective->model);
	SETCADDR(theCall, rObjective->flatModel);
	SETCADDDR(theCall, rObjective->parameters);
	SETCAD4R(theCall, estimate);

	REPROTECT(rObjective->model = eval(theCall, R_GlobalEnv), rObjective->modelIndex);

	UNPROTECT(2); // theCall, estimate
}

void omxInitRObjective(omxObjective* oo, SEXP rObj, SEXP dataList) {
	if(OMX_DEBUG) { Rprintf("Initializing R objective function.\n"); }
	omxRObjective *newObj = (omxRObjective*) R_alloc(1, sizeof(omxRObjective));
	PROTECT(newObj->objfun = GET_SLOT(rObj, install("objfun")));
	PROTECT_WITH_INDEX(newObj->model = GET_SLOT(rObj, install("model")), &(newObj->modelIndex));
	PROTECT(newObj->flatModel = GET_SLOT(rObj, install("flatModel")));
	PROTECT(newObj->parameters = GET_SLOT(rObj, install("parameters")));
	PROTECT_WITH_INDEX(newObj->state = NEW_NUMERIC(1), &(newObj->stateIndex));
	REAL(newObj->state)[0] = NA_REAL;
	
	oo->objectiveFun = omxCallRObjective;
	oo->needsUpdateFun = omxNeedsUpdateRObjective;
	oo->setFinalReturns = NULL;
	oo->destructFun = omxDestroyRObjective;
	oo->repopulateFun = omxRepopulateRObjective;
	oo->argStruct = (void*) newObj;
}


#endif /* _OMX_R_OBJECTIVE_ */
