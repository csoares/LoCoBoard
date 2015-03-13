/*
 * File:   main.cpp
 * Author: Christophe Soares
 *
 * Created on 6 de Outubro de 2009, 19:46
 * Copyright 2009 . All rights reserved.
 */

#include "Algorithms.h"

using namespace std;

///////////////////////////////////////////////////////////////////////
// Main Function of the LoCoBoard                                    //
///////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	// Constructor, if you insert a value in the parameter you change the threshold value
	Algorithms * alg = new Algorithms();

	// this will change the value of the capture FPS, Width, Height
	//alg->setCapture(10, 320, 240);

	// this will set the step value for the spiral pre-process (lookup-table) for the Algorithms A4
	alg->jumpValue = 1;
	// this will set the deeper of the spiral for the Algorithms A4
	alg->deep = 8;
	// this will set the step for the Algorithms A2 and A3
	alg->step = 2;

	//printf("\n%d DEEP\n", alg->deep);


	// this method is required to initialize the aplication
	alg->initialize(argc, argv);

	// set on the vertical flip of capture frame
	//alg->verticalFlipON();

	// set on the blur on the image
	// alg->blurON();


	//puts("Insert please the Value of your resolution");
	// capture from the console line the value of widht and height
	//puts("Width (ex: 640) and press enter ... \n");
	//scanf("%d", &alg->screenWidth);
	//puts("Height (ex:480) and press enter ... \n");
	//scanf("%d", &alg->screenHeight);

	//alg->screenWidth=1280;
	//alg->screenHeight=800;
	alg->screenWidth=1152;
	alg->screenHeight=864;
	//alg->screenWidth=800;
	//alg->screenHeight=600;


	// this method will calibrate the aplication
	if (PROJECTION) alg->startCalibrate();

	// 1 - simglePointProcess (use for)
	// 2 - simglePointProcess (use for with step)
	// 3 - simglePointProcessPTR	(use pointer with step)
	// 4 - smartSimglePointProcess (use spiral)
	// 5 - Multipoint

	// choose the initial algorithm, the algorithm could be change in real-time by pressing the space key
	alg->setAlgorithm(1);

	// start the main process of the aplication, aquisition of frame and processing them with an algorithm (this is the main loop)
	alg->startProcessingMain();

	// after escape have been pressed the main loop finish and we start the destructor
	alg->~Algorithms();

	return -1;
}
