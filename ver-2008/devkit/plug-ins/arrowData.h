//+
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors, which is protected by U.S. and Canadian federal copyright 
// law and by international treaties.
//
// The Data is provided for use exclusively by You. You have the right 
// to use, modify, and incorporate this Data into other products for 
// purposes authorized by the Autodesk software license agreement, 
// without fee.
//
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the 
// following disclaimer, must be included in all copies of the 
// Software, in whole or in part, and all derivative works of 
// the Software, unless such copies or derivative works are solely 
// in the form of machine-executable object code generated by a 
// source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
// PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
// TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
// BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
// DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
// AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
// OR PROBABILITY OF SUCH DAMAGES.
//
// ==========================================================================
//-

// 
// Description: 
//  The draw data for curvedArrowsLocator node. The vertex and face
//  lists here are used to draw the locator in curvedArrowsNode.cpp.
//  If you intend on using this file for your own locator code, you
//  should statically define the data in a cpp file and forward
//  declare the definitions.
//

#ifndef __arrowData_h 
#define __arrowData_h 

#include <maya/MTypes.h> 

double3 fsVertexList[] = { 
	{2.008225, 1.079209, 1.099684},
	{1.372727, 4.319467, 1.062919},
	{1.280773, 4.292139, 2.119867},
	{0.539631, 6.385876, 0.001970},
	{1.280571, 4.293406, -2.116594},
	{1.372622, 4.320125, -1.059360},
	{2.008054, 1.080287, -1.098606},
	{1.371678, 1.043522, -4.338864},
	{1.279918, 2.100470, -4.311536},
	{0.538348, -0.017427, -6.405273},
	{1.279295, -2.135990, -4.312802},
	{1.371354, -1.078757, -4.339522},
	{2.007523, -1.118003, -1.099684},
	{1.370871, -4.358261, -1.062919},
	{1.278961, -4.330933, -2.119867},
	{0.537693, -6.424670, -0.001970},
	{1.279162, -4.332200, 2.116594},
	{1.370975, -4.358919, 1.059360},
	{2.007695, -1.119081, 1.098606},
	{1.371921, -1.082316, 4.338864},
	{1.279816, -2.139264, 4.311536},
	{0.538976, -0.021367, 6.405273},
	{1.280439, 2.097197, 4.312802},
	{1.372244, 1.039963, 4.339522},
	{0.689836, 6.083689, -0.304354},
	{0.687872, -6.122423, -0.308345},
	{0.688532, -0.323764, -6.103074},
	{2.085568, -0.350431, -0.331356},
	{1.499430, 1.051045, -3.919533},
	{1.529212, -0.339001, -3.927586},
	{1.060962, 1.230698, -5.173299},
	{1.094450, -0.331015, -5.187352},
	{1.358106, 1.257148, -4.334800},
	{1.062640, 1.235920, -5.168137},
	{1.500284, 3.900855, -1.067099},
	{1.530372, 3.908165, -0.319592},
	{1.063729, 5.149192, -1.251617},
	{1.358988, 4.316187, -1.273126},
	{1.062019, 5.154456, -1.246286},
	{1.095755, 5.167962, -0.311606},
	{1.805411, 1.068872, -2.630027},
	{1.840273, -0.344989, -2.635986},
	{1.805912, 2.611508, -1.085823},
	{1.841150, 2.616522, -0.325579},
	{1.989196, 1.079267, -1.319875},
	{2.027111, -0.348901, -1.323331},
	{1.989278, 1.301442, -1.097376},
	{2.027605, 1.303832, -0.329491},
	{1.499065, -1.086496, -3.920275},
	{1.060692, -1.265690, -5.173846},
	{1.062416, -1.271064, -5.168539},
	{1.357708, -1.292573, -4.335583},
	{1.804689, -2.649458, -1.088268},
	{1.839898, -2.655130, -0.328084},
	{1.804946, -1.105219, -2.630971},
	{1.988671, -1.116772, -1.320941},
	{1.988629, -1.339324, -1.098664},
	{2.026941, -1.342456, -0.330810},
	{1.498628, -3.938942, -1.070441},
	{1.528676, -3.946769, -0.323016},
	{1.357257, -4.354195, -1.276643},
	{1.061859, -5.187435, -1.255416},
	{1.060152, -5.192692, -1.250098},
	{1.093819, -5.206605, -0.315548},
	{0.689167, -0.327754, 6.103014},
	{1.840678, -0.347494, 2.635666},
	{1.805806, 1.066425, 2.630939},
	{1.500402, 3.900112, 1.070442},
	{1.359117, 4.315402, 1.276595},
	{1.063801, 5.148689, 1.255368},
	{1.062106, 5.153909, 1.250088},
	{1.806063, 2.610563, 1.088270},
	{2.027326, -0.350221, 1.322957},
	{1.989405, 1.077978, 1.320891},
	{1.989447, 1.300376, 1.098664},
	{1.529760, -0.342425, 3.927349},
	{1.499966, 1.047702, 3.920264},
	{1.095076, -0.334957, 5.187215},
	{1.061565, 1.226892, 5.173850},
	{1.063244, 1.232120, 5.168687},
	{1.358666, 1.253631, 4.335587},
	{1.805341, -1.107666, 2.629994},
	{1.804840, -2.650403, 1.085822},
	{1.988881, -1.118061, 1.319826},
	{1.988799, -1.340390, 1.097375},
	{1.498746, -3.939685, 1.067098},
	{1.061931, -5.188034, 1.251568},
	{1.357386, -4.354982, 1.273078},
	{1.060238, -5.193239, 1.246297},
	{1.499600, -1.089839, 3.919521},
	{1.061296, -1.269488, 5.173302},
	{1.358268, -1.296090, 4.334797},
	{1.063021, -1.274864, 5.167988}
};
static unsigned int fsVertexListSize = sizeof(fsVertexList)/sizeof(fsVertexList[0]);

double3 fsNormalList[] = { 
	{-0.931122, 0.038131, -0.362710},
	{-0.912231, -0.005503, -0.409640},
	{-0.933578, 0.006943, -0.358306},
	{-0.931095, 0.362789, 0.038019},
	{-0.912227, 0.409648, -0.005557},
	{-0.933550, 0.358383, 0.006858},
	{-0.997459, -0.042270, 0.057351},
	{-0.993341, -0.026643, 0.112084},
	{-0.994834, 0.005967, 0.101338},
	{-0.944191, -0.031638, 0.327875},
	{-0.925551, -0.028775, 0.377527},
	{-0.933560, 0.006761, 0.358358},
	{-0.907808, -0.032590, 0.418117},
	{-0.912226, -0.005666, 0.409648},
	{-0.938752, -0.081124, 0.334909},
	{-0.937371, -0.096144, 0.334802},
	{-0.939100, -0.066084, 0.337231},
	{-0.944104, -0.327156, 0.040453},
	{-0.966679, -0.252334, 0.043109},
	{-0.960969, -0.276637, 0.003159},
	{-0.939124, -0.337153, 0.066138},
	{-0.937387, -0.334742, 0.096190},
	{-0.938771, -0.334845, 0.081163},
	{-0.912227, -0.409644, -0.005734},
	{-0.931111, -0.362760, 0.037919},
	{-0.933585, -0.358294, 0.006690},
	{-0.967351, -0.023114, 0.252386},
	{-0.960826, 0.013512, 0.276822},
	{-0.982288, 0.011914, 0.187001},
	{-0.984228, -0.170339, 0.047757},
	{-0.982359, -0.186756, 0.009630},
	{-0.985027, -0.025548, 0.170498},
	{-0.992471, -0.111860, 0.049884},
	{-0.995415, -0.094359, 0.015656},
	{-0.939102, 0.066141, 0.337213},
	{-0.937369, 0.096190, 0.334792},
	{-0.938752, 0.081181, 0.334895},
	{-0.931093, 0.037925, 0.362805},
	{-0.944074, 0.040491, 0.327237},
	{-0.964270, 0.041449, 0.261658},
	{-0.991342, 0.121848, 0.048936},
	{-0.978396, 0.201767, 0.045070},
	{-0.982265, 0.187120, 0.011932},
	{-0.978414, 0.045051, 0.201683},
	{-0.991353, 0.048984, 0.121739},
	{-0.996495, 0.059201, 0.059092},
	{-0.994819, 0.101489, 0.005905},
	{-0.964254, 0.261709, 0.041505},
	{-0.960805, 0.276893, 0.013589},
	{-0.938752, 0.334874, 0.081267},
	{-0.937370, 0.334766, 0.096273},
	{-0.939103, 0.337190, 0.066241},
	{-0.944069, 0.327242, 0.040582},
	{-0.907792, -0.032472, -0.418163},
	{-0.925549, -0.028624, -0.377545},
	{-0.995398, 0.015903, -0.094495},
	{-0.982336, 0.009926, -0.186865},
	{-0.992245, -0.025986, -0.121548},
	{-0.938785, -0.334775, -0.081291},
	{-0.937401, -0.334667, -0.096321},
	{-0.939136, -0.337099, -0.066236},
	{-0.944233, -0.327743, -0.031761},
	{-0.925574, -0.377464, -0.028876},
	{-0.907810, -0.418108, -0.032678},
	{-0.979223, -0.201333, -0.024215},
	{-0.998284, -0.041303, -0.041510},
	{-0.992261, -0.121362, -0.026260},
	{-0.960946, 0.003462, -0.276715},
	{-0.979198, -0.023910, -0.201495},
	{-0.944194, -0.031458, -0.327883},
	{-0.939094, -0.065903, -0.337282},
	{-0.937364, -0.095970, -0.334872},
	{-0.938744, -0.080943, -0.334974},
	{-0.939071, 0.337339, -0.065948},
	{-0.937345, 0.334923, -0.095971},
	{-0.938725, 0.335024, -0.080962},
	{-0.992448, 0.050148, -0.111948},
	{-0.984202, 0.048049, -0.170403},
	{-0.985000, 0.170655, -0.025537},
	{-0.993321, 0.112257, -0.026691},
	{-0.997443, 0.057550, -0.042372},
	{-0.967321, 0.252508, -0.023037},
	{-0.907802, 0.418140, -0.032480},
	{-0.925535, 0.377575, -0.028671},
	{-0.944163, 0.327965, -0.031530},
	{-0.966661, 0.043405, -0.252353},
	{-0.938785, 0.081433, -0.334740},
	{-0.937400, 0.096451, -0.334631},
	{-0.939136, 0.066388, -0.337069},
	{-0.944108, 0.040709, -0.327112},
	{-0.999889, 0.010615, 0.010460},
	{-0.964989, -0.261340, -0.022320},
	{-0.964949, -0.022014, -0.261513}
};
// static unsigned int fsNormalListSize = sizeof(fsNormalList)/sizeof(fsNormalList[0]);

typedef unsigned int uint3[3]; 

uint3 fsFaceList[] = {
	{91, 65, 78},
	{63, 26, 64},
	{7, 45, 46},
	{8, 31, 32},
	{31, 10, 27},
	{33, 9, 34},
	{6, 35, 36},
	{37, 5, 38},
	{25, 39, 40},
	{29, 30, 42},
	{35, 43, 44},
	{41, 42, 46},
	{43, 47, 48},
	{51, 11, 52},
	{27, 50, 32},
	{12, 49, 30},
	{57, 53, 54},
	{49, 55, 30},
	{55, 56, 42},
	{13, 57, 58},
	{53, 59, 60},
	{61, 15, 62},
	{14, 63, 64},
	{65, 22, 79},
	{73, 66, 74},
	{69, 3, 70},
	{2, 71, 40},
	{71, 4, 25},
	{36, 44, 72},
	{1, 75, 48},
	{44, 48, 75},
	{66, 76, 67},
	{78, 79, 24},
	{80, 23, 81},
	{87, 17, 88},
	{84, 82, 73},
	{58, 54, 83},
	{58, 85, 19},
	{54, 60, 86},
	{26, 16, 89},
	{64, 89, 18},
	{82, 90, 66},
	{92, 21, 93},
	{20, 91, 78},
	{48, 47, 7},
	{46, 28, 7},
	{7, 28, 48},
	{8, 33, 34},
	{30, 29, 8},
	{8, 32, 30},
	{34, 31, 8},
	{27, 32, 31},
	{37, 38, 6},
	{40, 39, 6},
	{36, 40, 6},
	{6, 39, 37},
	{42, 41, 29},
	{44, 36, 35},
	{46, 45, 41},
	{48, 44, 43},
	{51, 52, 12},
	{32, 50, 12},
	{30, 32, 12},
	{12, 50, 51},
	{54, 58, 57},
	{55, 42, 30},
	{56, 46, 42},
	{46, 56, 13},
	{58, 28, 13},
	{13, 28, 46},
	{60, 54, 53},
	{14, 61, 62},
	{60, 59, 14},
	{14, 64, 60},
	{62, 63, 14},
	{79, 78, 65},
	{66, 67, 74},
	{2, 69, 70},
	{36, 68, 2},
	{2, 40, 36},
	{70, 71, 2},
	{25, 40, 71},
	{72, 68, 36},
	{73, 74, 1},
	{48, 28, 1},
	{1, 28, 73},
	{75, 72, 44},
	{76, 77, 67},
	{80, 81, 24},
	{24, 77, 76},
	{76, 78, 24},
	{24, 79, 80},
	{82, 66, 73},
	{83, 85, 58},
	{19, 84, 73},
	{19, 28, 58},
	{73, 28, 19},
	{86, 83, 54},
	{89, 64, 26},
	{87, 88, 18},
	{18, 86, 60},
	{60, 64, 18},
	{18, 89, 87},
	{90, 76, 66},
	{20, 92, 93},
	{76, 90, 20},
	{20, 78, 76},
	{93, 91, 20}
};

static unsigned int fsFaceListSize = sizeof(fsFaceList)/sizeof(fsFaceList[0]);

uint3 fsFaceVertexNormalList[] = {
	{1, 2, 3},
	{4, 5, 6},
	{7, 8, 9},
	{10, 11, 12},
	{11, 13, 14},
	{15, 16, 17},
	{18, 19, 20},
	{21, 22, 23},
	{24, 25, 26},
	{27, 28, 29},
	{19, 30, 31},
	{32, 29, 9},
	{30, 33, 34},
	{35, 36, 37},
	{14, 38, 12},
	{39, 40, 28},
	{41, 42, 43},
	{40, 44, 28},
	{44, 45, 29},
	{46, 41, 47},
	{42, 48, 49},
	{50, 51, 52},
	{53, 4, 6},
	{2, 54, 55},
	{56, 57, 58},
	{59, 60, 61},
	{62, 63, 26},
	{63, 64, 24},
	{20, 31, 65},
	{66, 67, 34},
	{31, 34, 67},
	{57, 68, 69},
	{3, 55, 70},
	{71, 72, 73},
	{74, 75, 76},
	{77, 78, 56},
	{47, 43, 79},
	{47, 80, 81},
	{43, 49, 82},
	{5, 83, 84},
	{6, 84, 85},
	{78, 86, 57},
	{87, 88, 89},
	{90, 1, 3},
	{34, 33, 7},
	{9, 91, 7},
	{7, 91, 34},
	{10, 15, 17},
	{28, 27, 10},
	{10, 12, 28},
	{17, 11, 10},
	{14, 12, 11},
	{21, 23, 18},
	{26, 25, 18},
	{20, 26, 18},
	{18, 25, 21},
	{29, 32, 27},
	{31, 20, 19},
	{9, 8, 32},
	{34, 31, 30},
	{35, 37, 39},
	{12, 38, 39},
	{28, 12, 39},
	{39, 38, 35},
	{43, 47, 41},
	{44, 29, 28},
	{45, 9, 29},
	{9, 45, 46},
	{47, 91, 46},
	{46, 91, 9},
	{49, 43, 42},
	{53, 50, 52},
	{49, 48, 53},
	{53, 6, 49},
	{52, 4, 53},
	{55, 3, 2},
	{57, 69, 58},
	{62, 59, 61},
	{20, 92, 62},
	{62, 26, 20},
	{61, 63, 62},
	{24, 26, 63},
	{65, 92, 20},
	{56, 58, 66},
	{34, 91, 66},
	{66, 91, 56},
	{67, 65, 31},
	{68, 93, 69},
	{71, 73, 70},
	{70, 93, 68},
	{68, 3, 70},
	{70, 55, 71},
	{78, 57, 56},
	{79, 80, 47},
	{81, 77, 56},
	{81, 91, 47},
	{56, 91, 81},
	{82, 79, 43},
	{84, 6, 5},
	{74, 76, 85},
	{85, 82, 49},
	{49, 6, 85},
	{85, 84, 74},
	{86, 68, 57},
	{90, 87, 89},
	{68, 86, 90},
	{90, 3, 68},
	{89, 1, 90}
};

// static unsigned int fsFaceVertexNormalListSize = sizeof(fsFaceVertexNormalList)/sizeof(fsFaceVertexNormalList[0]);

unsigned int fsEdgeLoop[] = { 0, 74, 71, 67, 1, 
							  68, 2, 69, 70, 3,
							  24, 38, 36, 4, 37, 
							  5, 34, 42, 46, 6, 
							  44, 40, 28, 7, 
							  32, 8, 33, 30, 9,
							  26, 49, 50, 10, 51, 11, 
							  48, 54, 55, 12, 56, 52, 
							  58, 13, 60, 14, 61, 62,
							  25, 15, 88, 86, 16, 87, 17, 
							  85, 82, 84, 18, 83, 81, 89, 19, 
							  91, 20, 92, 90, 64, 21, 78, 79, 
							  22, 80, 23, 76, 66, 73 };

static unsigned int fsEdgeLoopSize = sizeof(fsEdgeLoop)/sizeof(fsEdgeLoop[0]);

#endif 
