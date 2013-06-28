/*
 * This is throw-away proof-of-concept code that will likely be
 * replaced by something else.
 *
 * JNP 2012Dec10
 */

#include "omxFitFunction.h"
#include "omxExpectationBA81.h"

static const char *NAME = "FitFunctionBA81";

typedef struct {

	omxData *data;

} omxBA81State;


static void ba81Destroy(omxFitFunction *oo) {
	if(OMX_DEBUG) {
		Rprintf("Freeing %s function.\n", NAME);
	}
	//omxBA81State *mml = (omxBA81State *) oo->argStruct;
	// nothing to do yet
}

static omxRListElement* ba81SetFinalReturns(omxFitFunction *off, int *numReturns) {

	omxRListElement* retVal;

	*numReturns = 1;

	retVal = (omxRListElement*) R_alloc(1, sizeof(omxRListElement));

	retVal[0].numValues = 1;
	retVal[0].values = (double*) R_alloc(1, sizeof(double));
	strcpy(retVal[0].label, "Minus2LogLikelihood");
	retVal[0].values[0] = omxMatrixElement(off->matrix, 0, 0);

	return retVal;
}

// TODO: Don't trample the Expectation/FitFunction separation.

static void ba81GradientHook(omxFitFunction* oo, double *out)
{
	ba81Gradient(oo->expectation, out);
}

static void ba81Compute(omxFitFunction *oo) {
	if(OMX_DEBUG_MML) {Rprintf("Beginning %s Computation.\n", NAME);}

	omxExpectation* expectation = oo->expectation;
  
	oo->matrix->data[0] = ba81ComputeFit(expectation);
}

void omxInitFitFunctionBA81(omxFitFunction* oo, SEXP rObj) {
	//omxExpectation* expectation = oo->expectation;

	//omxState* currentState = oo->matrix->currentState;
	
	if(OMX_DEBUG) {
	  Rprintf("Initializing %s.\n", NAME);
	}
	
	//omxBA81State *newObj = (omxBA81State*) R_alloc(1, sizeof(omxBA81State));
	
	//newObj->data = oo->expectation->data;

	omxExpectationCompute(oo->expectation);

	oo->computeFun = ba81Compute;
	oo->setFinalReturns = ba81SetFinalReturns;
	oo->destructFun = ba81Destroy;

	if (ba81ExpectationHasGradients(oo->expectation)) {
		oo->gradientFun = ba81GradientHook;
	}
}
