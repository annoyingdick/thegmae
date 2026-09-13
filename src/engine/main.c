#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "PathHandler.h"
#include "InstancesHandler.h"
#include "PhysicsSimulationHandler.h"
#include "NavigationHandler.h"
#include "CharactersHandler.h"
#include "TaskManager.h"
#include "game/GameMain.h"

//initalization order:

//0: WindowHandler, PathHandler, InstancesHandler, PhysicsSimulationHandler, NavigationHandler and TaskManager are independent
//1: Render depends on WH -> WH creates a graphics api context (OpenGL) that is used by Render
//2: WindowHandler (somewhat) and CharactersHandler depend on Render -> WH inits DebugGuiHandler which depends on Render
//3: Game code should be initialized the last because i said so

//FixedLoop functions are called every 64th of a second or whatever tickrate is. They are used for deterministic stuff. 
//Fixed loop order:

//0: GM_PSH_PreFixedLoop should run before PHS
//1: PhysicsSimulationHandler
//2: Game fixed loop runs after everything

//FixedLoopOnce functions are called just like f.l. functions, but f.l. functions can be called as many times as possible,
//and f.l.o. only once.

//Note: Loop function of Render must run after FixedLoopOnce functions, because they usually update things that are then
//rendered in R_Loop

int main(const int argc, const char* const argv[const]) {
    if (argc) {
	//init systems
	WH_Init();
	PH_Init(argv[0]);
	IH_Init();
	PSH_Init();
	NH_Init();
	TM_Init();

	R_Init();

	CH_Init();
	WH_R_PostInit();

	GM_Init();

	//LH_Load("level0.json");

	while (true) {
	    WHLoopResult result;

	    result = WH_Loop();

	    if (result.code == WH_LOOP_RESULT_QUIT) {
		WH_Quit();

		return EXIT_SUCCESS;
	    }

	    for (; result.code >= WH_LOOP_RESULT_DO_FIXED_LOOP; result.code--) {
		GM_PSH_PreFixedLoop();

		PSH_FixedLoop();

		GM_FixedLoop();
	    }
	    if (result.code == WH_LOOP_RESULT_DO_FIXED_LOOP - 1) {
		GM_R_PreFixedLoopOnce();
		R_FixedLoopOnce();
	    }

	    CH_Loop();

	    R_Loop_UpdatePVMat();
	    GM_Loop(result.interp);

	    R_Loop(result.interp);

	    GM_R_PostLoop();

	    WH_R_PostLoop();
	}
    }
    
    printf("Entry error : argc is 0\n");

    return EXIT_FAILURE;
}
