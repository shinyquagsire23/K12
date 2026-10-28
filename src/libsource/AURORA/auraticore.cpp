#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurrendergl.h"
#include "Vector.h"
#include "aurglcore.h"
#include "aurtexman.h"


char cHoloSkinVPARB[] = "!!ARBvp1.0\nPARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nATTRIB vTex0 = vertex.texcoord[0];\nATTRIB vWght = vertex.attrib[1];\nPARAM boneArray[51] = {program.env[18..68] };\nPARAM fogParams = state.fog.params;\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nMUL   vReg0, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg5.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg5.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg5.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg6.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg6.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg6.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg7.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg7.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg7.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg8.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg8.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg8.z, vertex.normal, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nMAD   vReg9, vReg2, vWght.y, vReg9;\nMAD   vReg9, vReg3, vWght.z, vReg9;\nMAD   vReg9, vReg4, vWght.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nMUL   vReg10, vWght.x, vReg5;\nMAD   vReg10, vReg6, vWght.y, vReg10;\nMAD   vReg10, vReg7, vWght.z, vReg10;\nMAD   vReg10, vReg8, vWght.w, vReg10;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nADD\tvReg1.x, -fogParams.y, vReg0.x;\nMUL\tresult.color.secondary.x, vReg1.x, state.fog.params.w;\nMOV\tresult.fogcoord.x, vReg0.x;\nDP4\tvReg0.x, vReg10, program.env[ 92 ];\nDP4\tvReg0.y, vReg10, program.env[ 91 ];\nDP4\tvReg0.z, vReg10, program.env[ 90 ];\nDP4\tvReg0.w, vReg10, program.env[ 89 ];\nMUL\tvReg10, vReg0, program.env[ 16 ].wxwx;\nDP3\tvReg10.w, vReg10, vReg10;\nRSQ\tvReg10.w, vReg10.w;\nRCP\tvReg10.w, vReg10.w;\nMOV\tresult.texcoord[ 1 ].x, vReg10.w;\nMOV\tresult.texcoord[ 1 ].y, vReg0.z;\nMOV   result.texcoord[0], vTex0;\n";
char cHoloVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP4\tvReg0.x, vReg10, program.env[ 92 ];\nDP4\tvReg0.y, vReg10, program.env[ 91 ];\nDP4\tvReg0.z, vReg10, program.env[ 90 ];\nDP4\tvReg0.w, vReg10, program.env[ 89 ];\nMUL\tvReg10, vReg0, program.env[ 16 ].wxwx;\nDP3\tvReg10.w, vReg10, vReg10;\nRSQ\tvReg10.w, vReg10.w;\nRCP\tvReg10.w, vReg10.w;\nMOV\tresult.texcoord[ 1 ].x, vReg10.w;\nMOV\tresult.texcoord[ 1 ].y, vReg0.z;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[2], vertex.texcoord[2];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
#if !KOTOR1
char szcStandardDirtyVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10, vReg11;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP4\tvReg11.x, vReg10, program.env[92];\nDP4\tvReg11.y, vReg10, program.env[91];\nDP4\tvReg11.z, vReg10, program.env[90];\nDP4\tvReg11.w, vReg10, program.env[89];\nMAD\tvReg11, vReg11, program.env[28].x, vReg11;\nMAD\tvReg11, vReg11, program.env[28].y, vReg11.yzxw;\nMAD\tvReg11, vReg11, program.env[28].z, vReg11.zxyw;\nMAD\tresult.texcoord[1], vReg11, program.env[28].w, vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[2], vertex.texcoord[2];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyLMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10, vReg11;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP4\tvReg11.x, vReg10, program.env[92];\nDP4\tvReg11.y, vReg10, program.env[91];\nDP4\tvReg11.z, vReg10, program.env[90];\nDP4\tvReg11.w, vReg10, program.env[89];\nMAD\tvReg11, vReg11, program.env[28].x, vReg11;\nMAD\tvReg11, vReg11, program.env[28].y, vReg11.yzxw;\nMAD\tvReg11, vReg11, program.env[28].z, vReg11.zxyw;\nMAD\tresult.texcoord[2], vReg11, program.env[28].w, vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyEMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nTEMP vReg11;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP4\tvReg11.x, vReg10, program.env[92];\nDP4\tvReg11.y, vReg10, program.env[91];\nDP4\tvReg11.z, vReg10, program.env[90];\nDP4\tvReg11.w, vReg10, program.env[89];\nMAD\tvReg11, vReg11, program.env[28].x, vReg11;\nMAD\tvReg11, vReg11, program.env[28].y, vReg11.yzxw;\nMAD\tvReg11, vReg11, program.env[28].z, vReg11.zxyw;\nMAD\tresult.texcoord[2], vReg11, program.env[28].w, vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vReg4;\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyEMLMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nTEMP vReg11;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP4\tvReg11.x, vReg10, program.env[92];\nDP4\tvReg11.y, vReg10, program.env[91];\nDP4\tvReg11.z, vReg10, program.env[90];\nDP4\tvReg11.w, vReg10, program.env[89];\nMAD\tvReg11, vReg11, program.env[28].x, vReg11;\nMAD\tvReg11, vReg11, program.env[28].y, vReg11.yzxw;\nMAD\tvReg11, vReg11, program.env[28].z, vReg11.zxyw;\nMAD\tresult.texcoord[3], vReg11, program.env[28].w, vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[2], vReg4;\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyLSVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10, vReg11;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nMOV\tvReg11, vertex.position;\nADD\tvReg11, vReg11, vertex.texcoord[0];\nMOV\tvReg11.w, program.env[16].w;\nMOV\tresult.texcoord[1], vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[2], vertex.texcoord[2];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyLSLMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10, vReg11;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nMOV\tvReg11, vertex.position;\nADD\tvReg11, vReg11, vertex.texcoord[0];\nMOV\tvReg11.w, program.env[16].w;\nMOV\tresult.texcoord[2], vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szcStandardDirtyLSEMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nTEMP vReg11;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nMOV\tvReg11, vertex.position;\nADD\tvReg11, vReg11, vertex.texcoord[0];\nMOV\tvReg11.w, program.env[16].w;\nMOV\tresult.texcoord[2], vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vReg4;\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szszcStandardDirtyLSEMLMVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nTEMP vReg11;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nMOV\tvReg11, vertex.position;\nADD\tvReg11, vReg11, vertex.texcoord[0];\nMOV\tvReg11.w, program.env[16].w;\nMOV\tresult.texcoord[3], vReg11;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[2], vReg4;\nMOV result.color, program.env[15].wwwz;\n";
#endif

char szBumpmapVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vReg10, vLength, vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nALIAS vNormalWorld = vLength;\nALIAS vFinalColour = vTemp;\nPARAM fogParams = state.fog.params;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nSUB  vReg10, program.env[12], vertex.position;\nDP3   vReg10.w, vReg10, vReg10;\nRSQ   vLength.w, vReg10.w;\nDST vTemp, vReg10.wwww, vLength.wwww;\nDP3 vTemp.w, vTemp, program.env[14];\nRCP vTemp.w, vTemp.w;\nMOV   result.color.secondary, vTemp.wwww;\nMUL   vReg10, vReg10, vLength.w;\nDP3   result.texcoord[1].x, vertex.attrib[12], vReg10;\nDP3   result.texcoord[1].y, vertex.attrib[13], vReg10;\nDP3   result.texcoord[1].z, vertex.attrib[14], vReg10;\nMOV   result.texcoord[1].w, 1;\nSUB vTemp, program.env[13], vertex.position;\nDP3   vTemp.w, vTemp, vTemp;\nRSQ   vTemp.w, vTemp.w;\nMUL   vTemp, vTemp, vTemp.w;\nADD vTemp, vTemp,vReg10;\nDP3   result.texcoord[2].x, vertex.attrib[12], vTemp;\nDP3   result.texcoord[2].y, vertex.attrib[13], vTemp;\nDP3   result.texcoord[2].z, vertex.attrib[14], vTemp;\nMOV   result.texcoord[2].w, 1;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[3], vertex.texcoord[0];\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord.x, vReg0.x;\n";
char szNormalmapVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vReg10, vLength, vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nALIAS vNormalWorld = vLength;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nSUB  vReg10, program.env[12], vertex.position;\nDP3   vReg10.w, vReg10, vReg10;\nRSQ   vLength.w, vReg10.w;\nDST vTemp, vReg10.wwww, vLength.wwww;\nDP3 vTemp.w, vTemp, program.env[14];\nRCP vTemp.w, vTemp.w;\nMOV   result.color.secondary, vTemp.wwww;\nMUL   vReg10, vReg10, vLength.w;\nMOV   result.texcoord[2], vReg10;\nMOV   result.texcoord[2].w, 1;\nSUB vTemp, program.env[13], vertex.position;\nDP3   vTemp.w, vTemp, vTemp;\nRSQ   vTemp.w, vTemp.w;\nMUL   vTemp, vTemp, vTemp.w;\nADD vTemp, vTemp,vReg10;\nMOV   result.texcoord[3], vTemp;\nMOV   result.texcoord[3].w, 1;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvTemp.x, vReg10, vReg10;\nRSQ\tvTemp.x, vTemp.x;\nRCP\tvTemp.x, vTemp.x;\nMOV result.fogcoord, vTemp.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\n";
char szBumpyShinySetupVPARB[] = {
    33, 33, 65, 82, 66, 118, 112, 49, 46, 48, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 48, 32,
    61, 32, 118, 101, 114, 116, 101, 120, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 48, 93, 59,
    10, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 52, 32, 61, 32, 118, 101, 114, 116, 101, 120,
    46, 97, 116, 116, 114, 105, 98, 91, 49, 50, 93, 59, 10, 65, 84, 84, 82, 73, 66, 32, 118, 84,
    101, 120, 53, 32, 61, 32, 118, 101, 114, 116, 101, 120, 46, 97, 116, 116, 114, 105, 98, 91, 49,
    51, 93, 59, 10, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 54, 32, 61, 32, 118, 101, 114,
    116, 101, 120, 46, 97, 116, 116, 114, 105, 98, 91, 49, 52, 93, 59, 10, 80, 65, 82, 65, 77, 32,
    109, 118, 77, 97, 116, 114, 105, 120, 91, 52, 93, 32, 61, 32, 123, 32, 115, 116, 97, 116, 101,
    46, 109, 97, 116, 114, 105, 120, 46, 109, 111, 100, 101, 108, 118, 105, 101, 119, 32, 125, 59,
    10, 80, 65, 82, 65, 77, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 52, 93, 32, 61, 32,
    123, 32, 115, 116, 97, 116, 101, 46, 109, 97, 116, 114, 105, 120, 46, 109, 118, 112, 32, 125,
    59, 10, 84, 69, 77, 80, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 51, 44, 32, 118,
    82, 101, 103, 49, 48, 59, 10, 84, 69, 77, 80, 32, 118, 82, 101, 103, 48, 59, 10, 84, 69, 77, 80,
    32, 118, 70, 105, 110, 97, 108, 67, 111, 108, 111, 117, 114, 59, 10, 68, 80, 52, 32, 32, 32,
    114, 101, 115, 117, 108, 116, 46, 112, 111, 115, 105, 116, 105, 111, 110, 46, 120, 44, 32, 109,
    118, 112, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112,
    111, 115, 105, 116, 105, 111, 110, 59, 10, 68, 80, 52, 32, 32, 32, 114, 101, 115, 117, 108, 116,
    46, 112, 111, 115, 105, 116, 105, 111, 110, 46, 121, 44, 32, 109, 118, 112, 77, 97, 116, 114,
    105, 120, 91, 49, 93, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105,
    111, 110, 59, 10, 68, 80, 52, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 112, 111, 115, 105,
    116, 105, 111, 110, 46, 122, 44, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 50, 93, 44,
    32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 59, 10, 68, 80,
    52, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 112, 111, 115, 105, 116, 105, 111, 110, 46,
    119, 44, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 51, 93, 44, 32, 118, 101, 114, 116,
    101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82,
    101, 103, 49, 46, 120, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44, 32, 118,
    84, 101, 120, 52, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 120, 44, 32,
    109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44, 32, 118, 84, 101, 120, 53, 59, 32, 68, 80,
    51, 32, 32, 32, 118, 82, 101, 103, 51, 46, 120, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120,
    91, 48, 93, 44, 32, 118, 84, 101, 120, 54, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103,
    49, 46, 121, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 49, 93, 44, 32, 118, 84, 101,
    120, 52, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 121, 44, 32, 109, 118,
    77, 97, 116, 114, 105, 120, 91, 49, 93, 44, 32, 118, 84, 101, 120, 53, 59, 32, 68, 80, 51, 32,
    32, 32, 118, 82, 101, 103, 51, 46, 121, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 49,
    93, 44, 32, 118, 84, 101, 120, 54, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46,
    122, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 50, 93, 44, 32, 118, 84, 101, 120, 52,
    59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 122, 44, 32, 109, 118, 77, 97,
    116, 114, 105, 120, 91, 50, 93, 44, 32, 118, 84, 101, 120, 53, 59, 32, 68, 80, 51, 32, 32, 32,
    118, 82, 101, 103, 51, 46, 122, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 50, 93, 44,
    32, 118, 84, 101, 120, 54, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46,
    116, 101, 120, 99, 111, 111, 114, 100, 91, 49, 93, 46, 120, 44, 32, 112, 114, 111, 103, 114, 97,
    109, 46, 101, 110, 118, 91, 57, 50, 93, 44, 32, 118, 82, 101, 103, 49, 59, 32, 68, 80, 51, 32,
    32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 49, 93, 46,
    121, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 50, 93, 44, 32, 118,
    82, 101, 103, 49, 48, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116,
    101, 120, 99, 111, 111, 114, 100, 91, 49, 93, 46, 122, 44, 32, 112, 114, 111, 103, 114, 97, 109,
    46, 101, 110, 118, 91, 57, 50, 93, 44, 32, 118, 82, 101, 103, 51, 59, 32, 68, 80, 51, 32, 32,
    32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50, 93, 46,
    120, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 49, 93, 44, 32, 118,
    82, 101, 103, 49, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101,
    120, 99, 111, 111, 114, 100, 91, 50, 93, 46, 121, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46,
    101, 110, 118, 91, 57, 49, 93, 44, 32, 118, 82, 101, 103, 49, 48, 59, 32, 68, 80, 51, 32, 32,
    32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50, 93, 46,
    122, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 49, 93, 44, 32, 118,
    82, 101, 103, 51, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101,
    120, 99, 111, 111, 114, 100, 91, 51, 93, 46, 120, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46,
    101, 110, 118, 91, 57, 48, 93, 44, 32, 118, 82, 101, 103, 49, 59, 32, 68, 80, 51, 32, 32, 32,
    114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 51, 93, 46, 121,
    44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 48, 93, 44, 32, 118, 82,
    101, 103, 49, 48, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101,
    120, 99, 111, 111, 114, 100, 91, 51, 93, 46, 122, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46,
    101, 110, 118, 91, 57, 48, 93, 44, 32, 118, 82, 101, 103, 51, 59, 32, 68, 80, 52, 32, 32, 32,
    118, 82, 101, 103, 49, 48, 46, 120, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115,
    105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 32, 59, 10,
    68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 121, 44, 32, 118, 101, 114, 116, 101,
    120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120,
    91, 49, 93, 32, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 122, 44, 32, 118,
    101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97,
    116, 114, 105, 120, 91, 50, 93, 32, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48,
    46, 119, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44,
    32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 51, 93, 32, 59, 10, 68, 80, 51, 9, 118, 82, 101,
    103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 118, 82, 101, 103, 49, 48, 59, 10,
    82, 83, 81, 9, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 49, 46, 120, 59, 10,
    82, 67, 80, 9, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 49, 46, 120, 59, 10,
    77, 79, 86, 32, 114, 101, 115, 117, 108, 116, 46, 102, 111, 103, 99, 111, 111, 114, 100, 44, 32,
    118, 82, 101, 103, 49, 46, 120, 59, 10, 82, 67, 80, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46,
    119, 44, 32, 118, 82, 101, 103, 49, 48, 46, 119, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 59,
    10, 77, 85, 76, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 120, 121, 122, 44, 32, 118, 82, 101,
    103, 49, 48, 44, 32, 45, 118, 82, 101, 103, 49, 48, 46, 119, 32, 32, 32, 32, 59, 10, 77, 79, 86,
    32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 48, 93,
    44, 32, 118, 84, 101, 120, 48, 59, 10, 77, 79, 86, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46,
    99, 111, 108, 111, 114, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 49, 53,
    93, 46, 120, 59, 10, 0, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120,
    99, 111, 111, 114, 100, 91, 49, 93, 46, 119, 44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 112,
    114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 50, 93, 59, 10, 68, 80, 51, 32, 32, 32,
    114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50, 93, 46, 119,
    44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118,
    91, 57, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120,
    99, 111, 111, 114, 100, 91, 51, 93, 46, 119, 44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 112,
    114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 48, 93, 59, 10
};
char szBumpyShinyVPARB_NV[] = "DP3   result.texcoord[1].w, vReg10, program.env[92];\nDP3   result.texcoord[2].w, vReg10, program.env[91];\nDP3   result.texcoord[3].w, vReg10, program.env[90];\n";
char szBumpyShinyVPARB_ATI[] = "DP3   result.texcoord[4].x, vReg10, program.env[92];\nDP3   result.texcoord[4].y, vReg10, program.env[91];\nDP3   result.texcoord[4].z, vReg10, program.env[90];\n";
char szStandardVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[2], vertex.texcoord[2];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szStandardCubeVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vReg4;\nMOV   result.texcoord[2], vertex.texcoord[2];\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szStandardLightmapCubeVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3, vReg4;\nTEMP vReg10;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord, vReg0.x;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV   result.texcoord[2], vReg4;\nMOV   result.texcoord[3], vertex.texcoord[3];\nMOV result.color, program.env[15].wwwz;\n";
char szStandardFogVPARB[] = "!!ARBvp1.0PARAM const0 = 0.0;\nPARAM const1 = 1.0;\nPARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vTemp;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vReg10;\nTEMP vNormalWorld;\nALIAS vFinalColour = vTemp;\nDP4   result.position.x, mvpMatrix[0], vertex.position;\nDP4   result.position.y, mvpMatrix[1], vertex.position;\nDP4   result.position.z, mvpMatrix[2], vertex.position;\nDP4   result.position.w, mvpMatrix[3], vertex.position;\nDP4   vReg10.x, mvMatrix[0], vertex.position;\nDP4   vReg10.y, mvMatrix[1], vertex.position;\nDP4   vReg10.z, mvMatrix[2], vertex.position;\nDP4   vReg10.w, mvMatrix[3], vertex.position;\nDP3   vNormalWorld.x, mvMatrix[0], vertex.normal;\nDP3   vNormalWorld.y, mvMatrix[1], vertex.normal;\nDP3   vNormalWorld.z, mvMatrix[2], vertex.normal;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nADD\tvReg0.x, -state.fog.params.y, vReg0;\nMOV    result.color.xyz, state.fog.color;\nMUL\tresult.color.w, vReg0.x, state.fog.params.w;\nMOV result.fogcoord, const1;\n";
char szBumpyShinySkinSetupVPARB[] = {
    33, 33, 65, 82, 66, 118, 112, 49, 46, 48, 80, 65, 82, 65, 77, 32, 109, 118, 77, 97, 116, 114,
    105, 120, 91, 52, 93, 32, 61, 32, 123, 32, 115, 116, 97, 116, 101, 46, 109, 97, 116, 114, 105,
    120, 46, 109, 111, 100, 101, 108, 118, 105, 101, 119, 91, 48, 93, 32, 125, 59, 10, 80, 65, 82,
    65, 77, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 52, 93, 32, 61, 32, 123, 32, 115,
    116, 97, 116, 101, 46, 109, 97, 116, 114, 105, 120, 46, 109, 118, 112, 32, 125, 59, 10, 65, 84,
    84, 82, 73, 66, 32, 118, 87, 103, 104, 116, 32, 61, 32, 118, 101, 114, 116, 101, 120, 46, 97,
    116, 116, 114, 105, 98, 91, 49, 93, 59, 10, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 48,
    32, 61, 32, 118, 101, 114, 116, 101, 120, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 48, 93,
    59, 10, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 52, 32, 61, 32, 118, 101, 114, 116, 101,
    120, 46, 97, 116, 116, 114, 105, 98, 91, 49, 50, 93, 59, 10, 65, 84, 84, 82, 73, 66, 32, 118,
    84, 101, 120, 53, 32, 61, 32, 118, 101, 114, 116, 101, 120, 46, 97, 116, 116, 114, 105, 98, 91,
    49, 51, 93, 59, 10, 65, 84, 84, 82, 73, 66, 32, 118, 84, 101, 120, 54, 32, 61, 32, 118, 101,
    114, 116, 101, 120, 46, 97, 116, 116, 114, 105, 98, 91, 49, 52, 93, 59, 10, 80, 65, 82, 65, 77,
    32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 53, 49, 93, 32, 61, 32, 123, 112, 114, 111,
    103, 114, 97, 109, 46, 101, 110, 118, 91, 49, 56, 46, 46, 54, 56, 93, 32, 125, 59, 10, 65, 68,
    68, 82, 69, 83, 83, 32, 65, 48, 59, 10, 84, 69, 77, 80, 32, 118, 82, 101, 103, 48, 44, 32, 118,
    82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 50, 44, 32, 32, 118, 82, 101, 103, 51, 44, 32, 118,
    82, 101, 103, 52, 44, 32, 118, 82, 101, 103, 53, 44, 32, 118, 82, 101, 103, 54, 44, 32, 118, 82,
    101, 103, 57, 44, 32, 118, 82, 101, 103, 49, 48, 59, 10, 65, 76, 73, 65, 83, 32, 118, 70, 105,
    110, 97, 108, 67, 111, 108, 111, 117, 114, 32, 61, 32, 118, 82, 101, 103, 57, 59, 10, 77, 85,
    76, 32, 32, 32, 118, 82, 101, 103, 48, 44, 32, 118, 101, 114, 116, 101, 120, 46, 97, 116, 116,
    114, 105, 98, 91, 52, 93, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 49,
    54, 93, 46, 122, 122, 122, 122, 59, 10, 65, 82, 76, 32, 32, 32, 65, 48, 46, 120, 44, 32, 118,
    82, 101, 103, 48, 46, 120, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44,
    32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111,
    110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 52,
    32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111,
    115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46,
    120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32,
    118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110,
    101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 85, 76, 32, 32,
    32, 118, 82, 101, 103, 57, 44, 32, 118, 87, 103, 104, 116, 46, 120, 44, 32, 118, 82, 101, 103,
    49, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120,
    52, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93,
    59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 52,
    44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59,
    10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 52, 44,
    32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10,
    77, 85, 76, 32, 32, 32, 118, 82, 101, 103, 52, 44, 32, 118, 87, 103, 104, 116, 46, 120, 44, 32,
    118, 82, 101, 103, 49, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32,
    118, 84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120,
    32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118,
    84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43,
    32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84,
    101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32,
    50, 93, 59, 10, 77, 85, 76, 32, 32, 32, 118, 82, 101, 103, 53, 44, 32, 118, 87, 103, 104, 116,
    46, 120, 44, 32, 118, 82, 101, 103, 49, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49,
    46, 120, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91,
    65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46,
    121, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65,
    48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122,
    44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46,
    120, 32, 43, 32, 50, 93, 59, 10, 77, 85, 76, 32, 32, 32, 118, 82, 101, 103, 54, 44, 32, 118, 87,
    103, 104, 116, 46, 120, 44, 32, 118, 82, 101, 103, 49, 59, 65, 82, 76, 32, 32, 32, 65, 48, 46,
    120, 44, 32, 118, 82, 101, 103, 48, 46, 121, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103,
    49, 46, 120, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110,
    44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59,
    10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 101, 114, 116, 101,
    120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97,
    121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103,
    49, 46, 122, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110,
    44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59,
    10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 57, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118,
    87, 103, 104, 116, 46, 121, 44, 32, 118, 82, 101, 103, 57, 59, 10, 68, 80, 51, 32, 32, 32, 118,
    82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114,
    114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82,
    101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114, 114,
    97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101,
    103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97,
    121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103,
    52, 44, 32, 118, 87, 103, 104, 116, 46, 121, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82,
    101, 103, 52, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84,
    101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32,
    48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101,
    120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49,
    93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120,
    53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93,
    59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 53, 44, 32, 118, 87, 103, 104, 116, 46, 121,
    44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 53, 59, 10, 68, 80, 51, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118,
    82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 54, 44, 32, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118,
    82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114,
    114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82,
    101, 103, 54, 44, 32, 118, 87, 103, 104, 116, 46, 121, 44, 32, 118, 82, 101, 103, 49, 44, 32,
    118, 82, 101, 103, 54, 59, 65, 82, 76, 32, 32, 32, 65, 48, 46, 120, 44, 32, 118, 82, 101, 103,
    48, 46, 122, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 101,
    114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 52, 32, 32, 32, 118,
    82, 101, 103, 49, 46, 121, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116,
    105, 111, 110, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43,
    32, 49, 93, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 101,
    114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118,
    82, 101, 103, 57, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 87, 103, 104, 116, 46, 122, 44,
    32, 118, 82, 101, 103, 57, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44,
    32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46,
    120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32,
    118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120,
    32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118,
    84, 101, 120, 52, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43,
    32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 52, 44, 32, 118, 87, 103, 104,
    116, 46, 122, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 52, 59, 10, 68, 80, 51,
    32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120, 53, 44, 32, 98, 111, 110,
    101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32,
    32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 53, 44, 32, 98, 111, 110, 101,
    65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118,
    82, 101, 103, 53, 44, 32, 118, 87, 103, 104, 116, 46, 122, 44, 32, 118, 82, 101, 103, 49, 44,
    32, 118, 82, 101, 103, 53, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44,
    32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46,
    120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32,
    118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120,
    32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118,
    84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43,
    32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 54, 44, 32, 118, 87, 103, 104,
    116, 46, 122, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 54, 59, 65, 82, 76, 32,
    32, 32, 65, 48, 46, 120, 44, 32, 118, 82, 101, 103, 48, 46, 119, 59, 10, 68, 80, 52, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105,
    116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32,
    43, 32, 48, 93, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118,
    101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 98, 111, 110, 101,
    65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 52, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105,
    116, 105, 111, 110, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32,
    43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 57, 44, 32, 118, 82, 101,
    103, 49, 44, 32, 118, 87, 103, 104, 116, 46, 119, 44, 32, 118, 82, 101, 103, 57, 59, 10, 68, 80,
    51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111,
    110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51,
    32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110,
    101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32,
    32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 52, 44, 32, 98, 111, 110, 101,
    65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32,
    118, 82, 101, 103, 52, 44, 32, 118, 87, 103, 104, 116, 46, 119, 44, 32, 118, 82, 101, 103, 49,
    44, 32, 118, 82, 101, 103, 52, 59, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 120, 44,
    32, 118, 84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46,
    120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 121, 44, 32,
    118, 84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120,
    32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46, 122, 44, 32, 118,
    84, 101, 120, 53, 44, 32, 98, 111, 110, 101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43,
    32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118, 82, 101, 103, 53, 44, 32, 118, 87, 103, 104,
    116, 46, 119, 44, 32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 53, 59, 68, 80, 51, 32,
    32, 32, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110,
    101, 65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 48, 93, 59, 10, 68, 80, 51, 32, 32,
    32, 118, 82, 101, 103, 49, 46, 121, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101,
    65, 114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 49, 93, 59, 10, 68, 80, 51, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 122, 44, 32, 118, 84, 101, 120, 54, 44, 32, 98, 111, 110, 101, 65,
    114, 114, 97, 121, 91, 65, 48, 46, 120, 32, 43, 32, 50, 93, 59, 10, 77, 65, 68, 32, 32, 32, 118,
    82, 101, 103, 54, 44, 32, 118, 87, 103, 104, 116, 46, 119, 44, 32, 118, 82, 101, 103, 49, 44,
    32, 118, 82, 101, 103, 54, 59, 77, 79, 86, 32, 32, 32, 118, 82, 101, 103, 57, 46, 119, 44, 32,
    112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 49, 54, 93, 46, 119, 59, 10, 68, 80,
    52, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 112, 111, 115, 105, 116, 105, 111, 110, 46,
    120, 44, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44, 32, 118, 82, 101, 103,
    57, 59, 10, 68, 80, 52, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 112, 111, 115, 105, 116,
    105, 111, 110, 46, 121, 44, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120, 91, 49, 93, 44, 32,
    118, 82, 101, 103, 57, 59, 10, 68, 80, 52, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 112,
    111, 115, 105, 116, 105, 111, 110, 46, 122, 44, 32, 109, 118, 112, 77, 97, 116, 114, 105, 120,
    91, 50, 93, 44, 32, 118, 82, 101, 103, 57, 59, 10, 68, 80, 52, 32, 32, 32, 114, 101, 115, 117,
    108, 116, 46, 112, 111, 115, 105, 116, 105, 111, 110, 46, 119, 44, 32, 109, 118, 112, 77, 97,
    116, 114, 105, 120, 91, 51, 93, 44, 32, 118, 82, 101, 103, 57, 59, 10, 68, 80, 51, 32, 32, 32,
    118, 82, 101, 103, 49, 46, 120, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44,
    32, 118, 82, 101, 103, 52, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 50, 46, 120, 44,
    32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 44, 32, 118, 82, 101, 103, 53, 59, 32, 68,
    80, 51, 32, 32, 32, 118, 82, 101, 103, 51, 46, 120, 44, 32, 109, 118, 77, 97, 116, 114, 105,
    120, 91, 48, 93, 44, 32, 118, 82, 101, 103, 54, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101,
    103, 49, 46, 121, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 49, 93, 44, 32, 118, 82,
    101, 103, 52, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 50, 46, 121, 44, 32, 109, 118,
    77, 97, 116, 114, 105, 120, 91, 49, 93, 44, 32, 118, 82, 101, 103, 53, 59, 32, 68, 80, 51, 32,
    32, 32, 118, 82, 101, 103, 51, 46, 121, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 49,
    93, 44, 32, 118, 82, 101, 103, 54, 59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 49, 46,
    122, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 50, 93, 44, 32, 118, 82, 101, 103, 52,
    59, 32, 68, 80, 51, 32, 32, 32, 118, 82, 101, 103, 50, 46, 122, 44, 32, 109, 118, 77, 97, 116,
    114, 105, 120, 91, 50, 93, 44, 32, 118, 82, 101, 103, 53, 59, 32, 68, 80, 51, 32, 32, 32, 118,
    82, 101, 103, 51, 46, 122, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 50, 93, 44, 32,
    118, 82, 101, 103, 54, 59, 32, 68, 80, 51, 32, 118, 82, 101, 103, 49, 46, 119, 44, 32, 118, 82,
    101, 103, 49, 44, 32, 118, 82, 101, 103, 49, 59, 10, 82, 83, 81, 32, 118, 82, 101, 103, 49, 46,
    119, 44, 32, 118, 82, 101, 103, 49, 46, 119, 59, 10, 77, 85, 76, 32, 118, 82, 101, 103, 49, 44,
    32, 118, 82, 101, 103, 49, 44, 32, 118, 82, 101, 103, 49, 46, 119, 59, 10, 68, 80, 51, 32, 118,
    82, 101, 103, 50, 46, 119, 44, 32, 118, 82, 101, 103, 50, 44, 32, 118, 82, 101, 103, 50, 59, 10,
    82, 83, 81, 32, 118, 82, 101, 103, 50, 46, 119, 44, 32, 118, 82, 101, 103, 50, 46, 119, 59, 10,
    77, 85, 76, 32, 118, 82, 101, 103, 50, 44, 32, 118, 82, 101, 103, 50, 44, 32, 118, 82, 101, 103,
    50, 46, 119, 59, 10, 68, 80, 51, 32, 118, 82, 101, 103, 51, 46, 119, 44, 32, 118, 82, 101, 103,
    51, 44, 32, 118, 82, 101, 103, 51, 59, 10, 82, 83, 81, 32, 118, 82, 101, 103, 51, 46, 119, 44,
    32, 118, 82, 101, 103, 51, 46, 119, 59, 10, 77, 85, 76, 32, 118, 82, 101, 103, 51, 44, 32, 118,
    82, 101, 103, 51, 44, 32, 118, 82, 101, 103, 51, 46, 119, 59, 10, 68, 80, 51, 32, 32, 32, 114,
    101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 49, 93, 46, 120, 44, 32,
    112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 50, 93, 44, 32, 118, 82, 101, 103,
    49, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111,
    111, 114, 100, 91, 49, 93, 46, 121, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118,
    91, 57, 50, 93, 44, 32, 118, 82, 101, 103, 50, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115,
    117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 49, 93, 46, 122, 44, 32, 112, 114,
    111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 50, 93, 44, 32, 118, 82, 101, 103, 51, 59,
    32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114,
    100, 91, 50, 93, 46, 120, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57,
    49, 93, 44, 32, 118, 82, 101, 103, 49, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108,
    116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50, 93, 46, 121, 44, 32, 112, 114, 111, 103,
    114, 97, 109, 46, 101, 110, 118, 91, 57, 49, 93, 44, 32, 118, 82, 101, 103, 50, 59, 32, 68, 80,
    51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50,
    93, 46, 122, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 49, 93, 44,
    32, 118, 82, 101, 103, 51, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46,
    116, 101, 120, 99, 111, 111, 114, 100, 91, 51, 93, 46, 120, 44, 32, 112, 114, 111, 103, 114, 97,
    109, 46, 101, 110, 118, 91, 57, 48, 93, 44, 32, 118, 82, 101, 103, 49, 59, 32, 68, 80, 51, 32,
    32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 51, 93, 46,
    121, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 48, 93, 44, 32, 118,
    82, 101, 103, 50, 59, 32, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101,
    120, 99, 111, 111, 114, 100, 91, 51, 93, 46, 122, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46,
    101, 110, 118, 91, 57, 48, 93, 44, 32, 118, 82, 101, 103, 51, 59, 32, 68, 80, 52, 32, 32, 32,
    118, 82, 101, 103, 49, 48, 46, 120, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115,
    105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 48, 93, 32, 59, 10,
    68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 121, 44, 32, 118, 101, 114, 116, 101,
    120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97, 116, 114, 105, 120,
    91, 49, 93, 32, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 122, 44, 32, 118,
    101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44, 32, 109, 118, 77, 97,
    116, 114, 105, 120, 91, 50, 93, 32, 59, 10, 68, 80, 52, 32, 32, 32, 118, 82, 101, 103, 49, 48,
    46, 119, 44, 32, 118, 101, 114, 116, 101, 120, 46, 112, 111, 115, 105, 116, 105, 111, 110, 44,
    32, 109, 118, 77, 97, 116, 114, 105, 120, 91, 51, 93, 32, 59, 10, 68, 80, 51, 9, 118, 82, 101,
    103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 52, 44, 32, 118, 82, 101, 103, 52, 59, 10, 82, 83,
    81, 9, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 49, 46, 120, 59, 10, 82, 67,
    80, 9, 118, 82, 101, 103, 49, 46, 120, 44, 32, 118, 82, 101, 103, 49, 46, 120, 59, 10, 77, 79,
    86, 32, 114, 101, 115, 117, 108, 116, 46, 102, 111, 103, 99, 111, 111, 114, 100, 46, 120, 44,
    32, 118, 82, 101, 103, 49, 46, 120, 59, 10, 82, 67, 80, 32, 32, 32, 118, 82, 101, 103, 49, 48,
    46, 119, 44, 32, 118, 82, 101, 103, 49, 48, 46, 119, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    59, 10, 77, 85, 76, 32, 32, 32, 118, 82, 101, 103, 49, 48, 46, 120, 121, 122, 44, 32, 118, 82,
    101, 103, 49, 48, 44, 32, 45, 118, 82, 101, 103, 49, 48, 46, 119, 32, 32, 32, 32, 59, 10, 77,
    79, 86, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91,
    48, 93, 44, 32, 118, 101, 114, 116, 101, 120, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 48,
    93, 59, 10, 77, 79, 86, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 99, 111, 108, 111, 114,
    44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 49, 53, 93, 46, 120, 59, 10, 0,
    68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100,
    91, 49, 93, 46, 119, 44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 112, 114, 111, 103, 114, 97,
    109, 46, 101, 110, 118, 91, 57, 50, 93, 59, 10, 68, 80, 51, 32, 32, 32, 114, 101, 115, 117, 108,
    116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91, 50, 93, 46, 119, 44, 32, 118, 82, 101, 103,
    49, 48, 44, 32, 112, 114, 111, 103, 114, 97, 109, 46, 101, 110, 118, 91, 57, 49, 93, 59, 10, 68,
    80, 51, 32, 32, 32, 114, 101, 115, 117, 108, 116, 46, 116, 101, 120, 99, 111, 111, 114, 100, 91,
    51, 93, 46, 119, 44, 32, 118, 82, 101, 103, 49, 48, 44, 32, 112, 114, 111, 103, 114, 97, 109,
    46, 101, 110, 118, 91, 57, 48, 93, 59, 10
};
char szSkinVPARB[] = "!!ARBvp1.0\nPARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nATTRIB vTex0 = vertex.texcoord[0];\nATTRIB vWght = vertex.attrib[1];\nPARAM boneArray[51] = {program.env[18..68] };\nPARAM fogParams = state.fog.params;\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nMUL   vReg0, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg5.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg5.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg5.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg6.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg6.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg6.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg7.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg7.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg7.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg8.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg8.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg8.z, vertex.normal, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nMAD   vReg9, vReg2, vWght.y, vReg9;\nMAD   vReg9, vReg3, vWght.z, vReg9;\nMAD   vReg9, vReg4, vWght.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nMUL   vReg10, vWght.x, vReg5;\nMAD   vReg10, vReg6, vWght.y, vReg10;\nMAD   vReg10, vReg7, vWght.z, vReg10;\nMAD   vReg10, vReg8, vWght.w, vReg10;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nADD\tvReg1.x, -fogParams.y, vReg0.x;\nMUL\tresult.color.secondary.x, vReg1.x, state.fog.params.w;\nMOV\tresult.fogcoord.x, vReg0.x;\nMOV   result.texcoord[0], vTex0;\n";
char szSkinFogVPARB[] = "!!ARBvp1.0\nPARAM const0 = 0.0;\nPARAM const1 = 1.0;\nPARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nATTRIB vWght = vertex.attrib[1];\nPARAM boneArray[51] = {program.env[18..68] };\nPARAM fogParams = state.fog.params;\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nMUL   vReg0, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nMAD   vReg9, vReg2, vWght.y, vReg9;\nMAD   vReg9, vReg3, vWght.z, vReg9;\nMAD   vReg9, vReg4, vWght.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nADD\tvReg0.x, -fogParams.y, vReg0.x;\nMOV result.fogcoord.x, const1;\nMOV    result.color.xyz, state.fog.color;\nMUL\tresult.color.w, vReg0.x, state.fog.params.w;\n";
char szSkinCubeMapVPARB[] = "!!ARBvp1.0\nPARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nPARAM boneArray[51] = {program.env[18..68] };\nPARAM fogParams = state.fog.params;\nATTRIB vTex0 = vertex.texcoord[0];\nATTRIB vWeight = vertex.attrib[1];\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nMUL   vReg0, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg5.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg5.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg5.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg6.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg6.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg6.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg7.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg7.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg7.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg8.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg8.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg8.z, vertex.normal, boneArray[A0.x + 2];\nMUL   vReg9, vWeight.x, vReg1;\nMAD   vReg9, vReg2, vWeight.y, vReg9;\nMAD   vReg9, vReg3, vWeight.z, vReg9;\nMAD   vReg9, vReg4, vWeight.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nMUL   vReg10, vWeight.x, vReg5;\nMAD   vReg10, vReg6, vWeight.y, vReg10;\nMAD   vReg10, vReg7, vWeight.z, vReg10;\nMAD   vReg10, vReg8, vWeight.w, vReg10;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nMOV   vNormalWorld.w, mvMatrix[3].w;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV\tresult.fogcoord, vReg0.x;\nADD\tvReg1.x, -fogParams.y, vReg0.x;\nMUL\tresult.color.secondary.x, vReg1.x, fogParams.w;\nMOV result.texcoord[0], vTex0;\nDP3   vReg0.x, vNormalWorld, -vReg10;\nMUL   vReg1, vReg0.x, vNormalWorld;\nMUL   vReg2, vReg1, program.env[16].y;\nADD   vReg3, vReg10, vReg2;\nDP3   vReg3.w, vReg3, vReg3;\nRSQ   vReg3.w, vReg3.w;\nMUL   vReg3, vReg3, vReg3.w;\nDP3   vReg4.x, program.env[92], vReg3;\nDP3   vReg4.y, program.env[91], vReg3;\nDP3   vReg4.z, program.env[90], vReg3;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[1], vReg4;\n";
char szSkinBumpedOutVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nPARAM boneArray[51] = {program.env[18..68] };\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nATTRIB vWght = vertex.attrib[1];\nMUL   vReg0, vertex.attrib[4], program.env[16].z;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg5.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg5.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg5.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg6.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg6.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg6.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg7.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg7.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg7.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg8.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg8.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg8.z, vertex.normal, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nMAD   vReg9, vReg2, vWght.y, vReg9;\nMAD   vReg9, vReg3, vWght.z, vReg9;\nMAD   vReg9, vReg4, vWght.w, vReg9;\nMUL   vReg10, vWght.x, vReg5;\nMAD   vReg10, vReg6, vWght.y, vReg10;\nMAD   vReg10, vReg7, vWght.z, vReg10;\nMAD   vReg10, vReg8, vWght.w, vReg10;\nMOV   vReg10.w, program.env[12].w;\nMAD vReg9, vReg10, program.env[12].x, vReg9;\nMOV   vReg9.w, program.env[16].w;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nMOV   vNormalWorld.w, mvMatrix[3].w;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV    result.fogcoord.x, vReg0.x;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.color, program.env[12].w;\n";
char szSkinBumpyVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nPARAM boneArray[51] = {program.env[18..68] };\nATTRIB vWght = vertex.attrib[1];\nATTRIB vTex4 = vertex.attrib[12];\nATTRIB vTex5 = vertex.attrib[13];\nATTRIB vTex6 = vertex.attrib[14];\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nMUL   vReg0, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg5.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg5.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg5.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.y;\nDP4   vReg2.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg2.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg2.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg6.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg6.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg6.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.z;\nDP4   vReg3.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg3.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg3.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg7.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg7.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg7.z, vertex.normal, boneArray[A0.x + 2];\nARL   A0.x, vReg0.w;\nDP4   vReg4.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg4.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg4.z, vertex.position, boneArray[A0.x + 2];\nDP3   vReg8.x, vertex.normal, boneArray[A0.x + 0];\nDP3   vReg8.y, vertex.normal, boneArray[A0.x + 1];\nDP3   vReg8.z, vertex.normal, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nMAD   vReg9, vReg2, vWght.y, vReg9;\nMAD   vReg9, vReg3, vWght.z, vReg9;\nMAD   vReg9, vReg4, vWght.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nMUL   vReg10, vWght.x, vReg5;\nMAD   vReg10, vReg6, vWght.y, vReg10;\nMAD   vReg10, vReg7, vWght.z, vReg10;\nMAD   vReg10, vReg8, vWght.w, vReg10;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nMOV   vNormalWorld.w, program.env[16].w;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nDP3\tvReg0.x, vReg10, vReg10;\nRSQ\tvReg0.x, vReg0.x;\nRCP\tvReg0.x, vReg0.x;\nMOV result.fogcoord.x, vReg0.x;\nADD  vReg0, program.env[12], -vReg9;\nDP3   vReg0.w, vReg0, vReg0;\nRSQ   vReg1.w, vReg0.w;\nDST vReg2, vReg0.w, vReg1.w;\nDP3 vReg2.w, vReg2, program.env[14];\nRCP vReg2.w, vReg2.w;\nMOV   result.color.secondary, vReg2.w;\nMUL   vReg0, vReg0, vReg1.w;\nMOV   vReg0.w, program.env[16].w;\nADD vReg2, program.env[13],-vReg9;\nDP3   vReg2.w, vReg2, vReg2;\nRSQ   vReg2.w, vReg2.w;\nMUL   vReg2, vReg2, vReg2.w;\nMOV   vReg2.w, program.env[16].w;\nADD vReg2, vReg2,vReg0;\nMOV   vReg2.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[3], vertex.texcoord[0];\nDP3   vReg1.x, vTex4, vReg0;\nDP3   vReg1.y, vTex5, vReg0;\nDP3   vReg1.z, vTex6, vReg0;\nMOV   vReg1.w, program.env[16].w;\nMOV   result.texcoord[1], vReg1;\nDP3   vReg1.x, vTex4, vReg2;\nDP3   vReg1.y, vTex5, vReg2;\nDP3   vReg1.z, vTex6, vReg2;\nMOV   vReg1.w, program.env[16].w;\nMOV   result.texcoord[2], vReg1;\n";
char szNewSkinBumpyVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nPARAM boneArray[51] = {program.env[18..68] };\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vFinalColour = vReg9;\nATTRIB vWght = vertex.attrib[1];\nMUL   vReg0, vertex.attrib[4], program.env[16].z;\nARL   A0.x, vReg0.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nDP3   vReg1.x, vertex.attrib[12], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[12], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[12], boneArray[A0.x + 2];\nMUL   vReg4, vWght.x, vReg1;\nDP3   vReg1.x, vertex.attrib[13], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[13], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[13], boneArray[A0.x + 2];\nMUL   vReg5, vWght.x, vReg1;\nDP3   vReg1.x, vertex.attrib[14], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[14], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[14], boneArray[A0.x + 2];\nMUL   vReg6, vWght.x, vReg1;\nARL   A0.x, vReg0.y;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1, vWght.y, vReg9;\nDP3   vReg1.x, vertex.attrib[12], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[12], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[12], boneArray[A0.x + 2];\nMAD   vReg4, vWght.y, vReg1, vReg4;\nDP3   vReg1.x, vertex.attrib[13], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[13], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[13], boneArray[A0.x + 2];\nMAD   vReg5, vWght.y, vReg1, vReg5;\nDP3   vReg1.x, vertex.attrib[14], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[14], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[14], boneArray[A0.x + 2];\nMAD   vReg6, vWght.y, vReg1, vReg6;\nARL   A0.x, vReg0.z;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1, vWght.z, vReg9;\nDP3   vReg1.x, vertex.attrib[12], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[12], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[12], boneArray[A0.x + 2];\nMAD   vReg4, vWght.z, vReg1, vReg4;\nDP3   vReg1.x, vertex.attrib[13], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[13], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[13], boneArray[A0.x + 2];\nMAD   vReg5, vWght.z, vReg1, vReg5;\nDP3   vReg1.x, vertex.attrib[14], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[14], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[14], boneArray[A0.x + 2];\nMAD   vReg6, vWght.z, vReg1, vReg6;\nARL   A0.x, vReg0.w;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1,vWght.w, vReg9;\nDP3   vReg1.x, vertex.attrib[12], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[12], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[12], boneArray[A0.x + 2];\nMAD   vReg4, vWght.w, vReg1, vReg4\n;DP3   vReg1.x, vertex.attrib[13], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[13], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[13], boneArray[A0.x + 2];\nMAD   vReg5, vWght.w, vReg1, vReg5\n;DP3   vReg1.x, vertex.attrib[14], boneArray[A0.x + 0];\nDP3   vReg1.y, vertex.attrib[14], boneArray[A0.x + 1];\nDP3   vReg1.z, vertex.attrib[14], boneArray[A0.x + 2];\nMAD   vReg10,vWght.w, vReg1, vReg6\n;MOV   vReg9.w, program.env[16].w;\nMOV   vReg10.w, program.env[16].w;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP3   vNormalWorld.x, mvMatrix[0], vReg10;\nDP3   vNormalWorld.y, mvMatrix[1], vReg10;\nDP3   vNormalWorld.z, mvMatrix[2], vReg10;\nMOV   vNormalWorld.w, program.env[16].w;\nDP3   vNormalWorld.w, vNormalWorld, vNormalWorld;\nRSQ   vNormalWorld.w, vNormalWorld.w;\nMUL   vNormalWorld, vNormalWorld, vNormalWorld.w;\nDP4   vReg10.x, mvMatrix[0], vReg9;\nDP4   vReg10.y, mvMatrix[1], vReg9;\nDP4   vReg10.z, mvMatrix[2], vReg9;\nMOV   vReg10.w, mvMatrix[3].w;\nADD  vReg0, program.env[12], -vReg9;\nDP3   vReg0.w, vReg0, vReg0;\nRSQ   vReg1.w, vReg0.w;\nDST vReg2, vReg0.w, vReg1.w;\nDP3 vReg2.w, vReg2, program.env[14];\nRCP vReg2.w, vReg2.w;\nMOV   result.color.secondary, vReg2.wwww;\nMUL   vReg0, vReg0, vReg1.w;\nMOV   vReg0.w, program.env[16].w;\nADD vReg2, program.env[13],-vReg9;\nDP3   vReg2.w, vReg2, vReg2;\nRSQ   vReg2.w, vReg2.w;\nMUL   vReg2, vReg2, vReg2.w;\nMOV   vReg2.w, program.env[16].w;\nADD vReg2, vReg2,vReg0;\nMOV   vReg2.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[3], vertex.texcoord[0];\nDP3   vReg1.x, vReg4, vReg0;\nDP3   vReg1.y, vReg5, vReg0;\nDP3   vReg1.z, vReg6, vReg0;\nMOV   vReg1.w, program.env[16].w;\nMOV   result.texcoord[1], vReg1;\nDP3   vReg1.x, vReg4, vReg2;\nDP3   vReg1.y, vReg5, vReg2;\nDP3   vReg1.z, vReg6, vReg2;\nMOV   vReg1.w, program.env[16].w;\nMOV   result.texcoord[2], vReg1;\n";
char szBumpedOutVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nTEMP vReg1, vReg2, vReg9, vReg10, vReg11;\nMOV\tvReg1, vertex.position;\nMOV\tvReg2, vertex.normal;\nMAD\tvReg1, vReg2, program.env[12].x, vReg1;\nDP4   result.position.x, mvpMatrix[0], vReg1;\nDP4   result.position.y, mvpMatrix[1], vReg1;\nDP4   result.position.z, mvpMatrix[2], vReg1;\nDP4   result.position.w, mvpMatrix[3], vReg1;\nDP4   vReg10.x, mvMatrix[0], vReg1;\nDP4   vReg10.y, mvMatrix[1], vReg1;\nDP4   vReg10.z, mvMatrix[2], vReg1;\nMOV   vReg10.w, program.env[12].w;\nDP3   vReg1.w, vReg10, vReg10;\nRSQ   vReg1.w, vReg1.w;\nRCP   vReg1.w, vReg1.w;\nMOV   result.fogcoord, vReg1.w;\nDP3   vReg11.x, mvMatrix[0], vertex.normal;\nDP3   vReg11.y, mvMatrix[1], vertex.normal;\nDP3   vReg11.z, mvMatrix[2], vertex.normal;\nDP3   vReg11.w, vReg11, vReg11;\nRSQ   vReg11.w, vReg11.w;\nMUL   vReg11, vReg11, vReg11.w;\nMOV   vReg9, vertex.color;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[1], vertex.texcoord[1];\nMOV result.color, program.env[12].w;\n";
char szSkinBumpyWSVPARB[] = "!!ARBvp1.0PARAM mvMatrix[4] = { state.matrix.modelview[0] };\nPARAM mvpMatrix[4] = { state.matrix.mvp };\nPARAM boneArray[51] = {program.env[18..68] };\nADDRESS A0;\nTEMP vReg4, vReg5, vReg6, vReg7, vReg8, vReg9, vReg10;\nTEMP vReg0, vReg1, vReg2, vReg3;\nTEMP vNormalWorld;\nALIAS vLightVectorWS = vReg4;\nALIAS vFinalColour = vReg9;\nALIAS vBoneIndices = vReg8;\nATTRIB vWght = vertex.attrib[1];\nMUL   vBoneIndices, vertex.attrib[4], program.env[16].zzzz;\nARL   A0.x, vBoneIndices.x;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMUL   vReg9, vWght.x, vReg1;\nARL   A0.x, vBoneIndices.y;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1, vWght.y, vReg9;\nARL   A0.x, vBoneIndices.z;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1, vWght.z, vReg9;\nARL   A0.x, vBoneIndices.w;\nDP4   vReg1.x, vertex.position, boneArray[A0.x + 0];\nDP4   vReg1.y, vertex.position, boneArray[A0.x + 1];\nDP4   vReg1.z, vertex.position, boneArray[A0.x + 2];\nMAD   vReg9, vReg1,vWght.w, vReg9;\nMOV   vReg9.w, program.env[16].w;\nDP4   result.position.x, mvpMatrix[0], vReg9;\nDP4   result.position.y, mvpMatrix[1], vReg9;\nDP4   result.position.z, mvpMatrix[2], vReg9;\nDP4   result.position.w, mvpMatrix[3], vReg9;\nDP4   vReg10.x, program.env[8], vReg9;\nDP4   vReg10.y, program.env[9], vReg9;\nDP4   vReg10.z, program.env[10], vReg9;\nMOV   vReg10.w, program.env[16].w;\nDP3\tvReg1.x, vReg10, vReg10;\nRSQ\tvReg1.x, vReg1.x;\nRCP\tvReg1.x, vReg1.x;\nMOV result.fogcoord, vReg1.x;\nADD  vReg0, program.env[12], -vReg9;\nDP3   vReg0.w, vReg0, vReg0;\nRSQ   vReg1.w, vReg0.w;\nDST vReg2, vReg0.w, vReg1.w;\nDP3 vReg2.w, vReg2, program.env[14];\nRCP vReg2.w, vReg2.w;\nMOV   result.color.secondary, vReg2.wwww;\nMUL   vReg0, vReg0, vReg1.w;\nMOV   vReg0.w, program.env[16].w;\nADD vReg2, program.env[13],-vReg9;\nDP3   vReg2.w, vReg2, vReg2;\nRSQ   vReg2.w, vReg2.w;\nMUL   vReg2, vReg2, vReg2.w;\nMOV   vReg2.w, program.env[16].w;\nADD vReg2, vReg2,vReg0;\nMOV   vReg2.w, program.env[16].w;\nMOV   result.texcoord[0], vertex.texcoord[0];\nMOV   result.texcoord[3], vertex.texcoord[0];\nMUL\tvReg9, boneArray[A0.x + 0], vReg0.x;\nMAD\tvReg9, boneArray[A0.x + 1], vReg0.y, vReg9;\nMAD\tvReg9, boneArray[A0.x + 2], vReg0.z, vReg9;\nMOV\tvReg5.x, boneArray[A0.x+0].w;\nMOV\tvReg5.y, boneArray[A0.x+1].w;\nMOV\tvReg5.z, boneArray[A0.x+2].w;\nSUB\tvReg9, vReg9, vReg5;\nMUL\tvReg4, vReg9, vWght.w;\nMUL\tvReg9, vReg2.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg2.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg2.z, boneArray[A0.x + 2], vReg9;\nSUB\tvReg9, vReg9, vReg5;\nMUL\tvReg6, vReg9, vWght.w;\nARL   A0.x, vBoneIndices.z;\nMUL\tvReg9, vReg0.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg0.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg0.z, boneArray[A0.x + 2], vReg9;\nMOV\tvReg5.x, boneArray[A0.x+0].w;\nMOV\tvReg5.y, boneArray[A0.x+1].w;\nMOV\tvReg5.z, boneArray[A0.x+2].w;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg4, vReg9, vWght.z, vReg4;\nMUL\tvReg9, vReg2.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg2.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg2.z, boneArray[A0.x + 2], vReg9;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg6, vReg9, vWght.z, vReg6;\nARL   A0.x, vBoneIndices.y;\nMUL\tvReg9, vReg0.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg0.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg0.z, boneArray[A0.x + 2], vReg9;\nMOV\tvReg5.x, boneArray[A0.x+0].w;\nMOV\tvReg5.y, boneArray[A0.x+1].w;\nMOV\tvReg5.z, boneArray[A0.x+2].w;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg4, vReg9, vWght.y, vReg4;\nMUL\tvReg9, vReg2.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg2.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg2.z, boneArray[A0.x + 2], vReg9;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg6, vReg9, vWght.y, vReg6;\nARL   A0.x, vBoneIndices.x;\nMUL\tvReg9, vReg0.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg0.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg0.z, boneArray[A0.x + 2], vReg9;\nMOV\tvReg5.x, boneArray[A0.x+0].w;\nMOV\tvReg5.y, boneArray[A0.x+1].w;\nMOV\tvReg5.z, boneArray[A0.x+2].w;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg4, vReg9, vWght.x, vReg4;\nMOV   vReg4.w, program.env[16].w;\nMOV   result.texcoord[2], vReg4;\nMUL\tvReg9, vReg2.x, boneArray[A0.x + 0];\nMAD\tvReg9, vReg2.y, boneArray[A0.x + 1], vReg9;\nMAD\tvReg9, vReg2.z, boneArray[A0.x + 2], vReg9;\nSUB\tvReg9, vReg9, vReg5;\nMAD\tvReg6, vReg9, vWght.z, vReg6;\nMOV   vReg6.w, program.env[16].w;\nMOV   result.texcoord[3], vReg6;\n";
char* szBumpyShinyVPARB;
char* szBumpyShinySkinVPARB;
unsigned int* szNormalmapVPARBDeclaration;

unsigned int lightmapFragmentShaderID;
unsigned int envmapFragmentShaderID;
unsigned int envmapFogFragmentShaderID;
unsigned int bumpmapFragmentShaderID;
unsigned int normalmapFragmentShaderID;
unsigned int envmapLightmapFragmentShaderID;
unsigned int bumpyshinyFragmentShaderID;
unsigned int fogFragmentShaderID;
unsigned int standardFogFragmentShaderID;
unsigned int envmapCubeFragmentShaderID;
unsigned int envmapFogCubeFragmentShaderID;
unsigned int envmapLightmapCubeFragmentShaderID;
unsigned int NoiseSubtractFragmentShaderID;
unsigned int NoiseLMSubtractFragmentShaderID;
unsigned int NoiseLMAddFragmentShaderID;
unsigned int NoiseEMLMSubtractFragmentShaderID;
unsigned int NoiseEMLMAddFragmentShaderID;
unsigned int FGlowFragmentShaderID;
unsigned int FGlowEMFragmentShaderID;
unsigned int FGlowBumpyShinyFragmentShaderID;
unsigned int DesatFragmentShaderID;
unsigned int DesatLMFragmentShaderID;
unsigned int DesatEMFragmentShaderID;
unsigned int DesatEMLMFragmentShaderID;
unsigned int DesatNoiseSubLMFragmentShaderID;
unsigned int DesatNoiseSubEMLMFragmentShaderID;
unsigned int HoloFragmentShaderID;
unsigned int HoloDesatFragmentShaderID;
unsigned int IllumFragmentShaderID;
unsigned int IllumEnvFragmentShaderID;
unsigned int IllumDesatFragmentShaderID;
#if !KOTOR1
unsigned int FGlowLMFragmentShaderID;
unsigned int FGlowEMLMFragmentShaderID;
unsigned int FGlowBumpmapFragmentShaderID;
unsigned int DesatBumpyShinyFragmentShaderID;
unsigned int DesatBumpmapFragmentShaderID;
unsigned int DesatNoiseSubFragmentShaderID;
unsigned int DesatNoiseSubEMFragmentShaderID;
unsigned int DesatNoiseAddFragmentShaderID;
unsigned int DesatNoiseAddEMFragmentShaderID;
unsigned int DesatNoiseAddLMFragmentShaderID;
unsigned int DesatNoiseAddEMLMFragmentShaderID;
unsigned int DesatNoiseMultFragmentShaderID;
unsigned int DesatNoiseMultEMFragmentShaderID;
unsigned int DesatNoiseMultLMFragmentShaderID;
unsigned int DesatNoiseMultEMLMFragmentShaderID;
unsigned int IllumNoiseSubtractFragmentShaderID;
unsigned int IllumNoiseAddFragmentShaderID;
unsigned int IllumNoiseMultiplyShaderID;
unsigned int IllumNoiseEMSubtractFragmentShaderID;
unsigned int IllumNoiseEMAddFragmentShaderID;
unsigned int IllumNoiseEMMultiplyFragmentShaderID;
unsigned int IllumDesatEMFragmentShaderID;
unsigned int IllumDesatBumpyShinyFragmentShaderID;
unsigned int IllumDesatBumpmapFragmentShaderID;
unsigned int IllumDesatNoiseSubFragmentShaderID;
unsigned int IllumDesatNoiseSubEMFragmentShaderID;
unsigned int IllumDesatNoiseAddFragmentShaderID;
unsigned int IllumDesatNoiseAddEMFragmentShaderID;
unsigned int IllumDesatNoiseMultFragmentShaderID;
unsigned int IllumDesatNoiseMultEMFragmentShaderID;
#endif

extern "C" const char* gluErrorString(unsigned int error);
float* Homogenize(const Vector& v, const float& w);
bool AuroraUseARBShaders();
int AurMultiTextureAvailable();
CAurTexture* AurTextureGetNormalizationCubeMap();
int AurTextureGetTID(CAurTexture* pTexture);

extern int envmapon;
extern int tex1on;

extern bool g_illumShader;
extern GLRender::PerInstanceColor g_illumShaderValues;
extern float g_DesatureFactor;
extern float g_DesatureFactor;

#include "port/libcstring.h"

static const char cLightmapFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "TEMP r0, r1, r2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "ADD r2.rgb, v0, r1;\n"
    "MUL r0.a, v0, r0;\n"
    "MUL r0.rgb, r0, r2;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapFogFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB v1 = fragment.color.secondary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "TEMP r0, r1;\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "MUL r0.rgb, r0, v0;\n"
    "LRP r0.rgb, r0.a, r0, r1;\n"
    "LRP r0.rgb, v1.r, c[1], r0;\n"
    "MOV r0.a, c[0];\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapFogCubeFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB v1 = fragment.color.secondary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "TEMP r0, r1;\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "MUL r0.rgb, r0, v0;\n"
    "LRP r0.rgb, r0.a, r0, r1;\n"
    "LRP r0.rgb, v1.r, c[1], r0;\n"
    "MOV r0.a, c[0];\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c[1] = { program.env[0] };\n"
    "TEMP r0, r1, r2, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "MUL r0.rgb, r0, v0;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r0.rgb, rTemp0.a, r1, r0;\n"
    "MOV r0.a, c[0];\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapCubeFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c[1] = { program.env[0] };\n"
    "TEMP r0, r1, r2, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "MUL r0.rgb, r0, v0;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r0.rgb, rTemp0.a, r1, r0;\n"
    "MOV r0.a, c[0];\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cBumpmapFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB v1 = fragment.color.secondary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEMP r0, r1, r2, r3, r4, r5;\n"
    "TEMP rTemp0, rTemp1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "TEX r2, T2, texture[2], CUBE;\n"
    "TEX r3, T3, texture[3], 2D;\n"
    "MAD  rTemp0,  r0, 2.0, -1.0;\n"
    "MAD  rTemp1,  r2, 2.0, -1.0;\n"
    "DP3 r5.rgb, rTemp0, rTemp1;\n"
    "MAD  rTemp1,  r1, 2.0, -1.0;\n"
    "DP3 r4.rgb, rTemp0, rTemp1;\n"
    "MUL r0.rgb, r5, r5;\n"
    "MOV r5.rgb, r5.b;\n"
    "MUL r0.rgb, r4, c[1];\n"
    "MUL r5.a, r5.r, r5.r;\n"
    "MUL r0.rgb, r4, v1;\n"
    "MUL r1.rgb, v1, c[0];\n"
    "MUL r1.a, r5.a, r5.a;\n"
    "ADD_SAT r0.rgb, v0, r0;\n"
    "MUL r1.a, r1.a, r1.a;\n"
    "MUL r0.rgb, r0, r3;\n"
    "MUL r1.rgb, r1.a, r1;\n"
    "SUB rTemp1, 1.0, r3;\n"
    "MUL r1.rgb, r1, rTemp1.a;\n"
    "ADD_SAT r0.rgb, r1, r0;\n"
    "MOV r0.a, v0.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNormalmapFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB v1 = fragment.color.secondary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "TEMP r0, r1, r2, r3, r4, r5;\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEMP rTemp1, rTemp2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], CUBE;\n"
    "TEX r3, T3, texture[3], CUBE;\n"
    "MAD  rTemp1,  r1, 2.0, -1.0;\n"
    "MAD  rTemp2,  r3, 2.0, -1.0;\n"
    "DP3 r5.xyz, rTemp1, rTemp2;\n"
    "MAD  rTemp2,  r2, 2.0, -1.0;\n"
    "DP3 r4.xyz, rTemp1, rTemp2;\n"
    "MUL r1.xyz, r5, r5;\n"
    "MOV r5.xyz, r5.z;\n"
    "MUL r1.xyz, r4, c[1];\n"
    "MUL r5.w, r5.x, r5.x;\n"
    "MUL r1.xyz, r4, v1;\n"
    "MUL r2.xyz, v1, c[0];\n"
    "MUL r2.w, r5.w, r5.w;\n"
    "ADD_SAT r1.xyz, v0, r1;\n"
    "MUL r2.w, r2.w, r2.w;\n"
    "MUL r1.xyz, r1, r0;\n"
    "MUL r2.xyz, r2.w, r2;\n"
    "SUB rTemp1, 1, r0;\n"
    "MUL r2.xyz, r2, rTemp1.w;\n"
    "ADD_SAT r0.xyz, r2, r1;\n"
    "MOV r0.w, v0.w;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapLightmapFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "PARAM c[1] = { program.env[0] };\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "TEMP rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "MUL r3.rgb, v0, r0;\n"
    "MAD r3.rgb, r1, r0, r3;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MUL r4.rgb, r2, rTemp0.a;\n"
    "ADD r0.rgb, r3, r4;\n"
    "MOV r0.a, c[0].a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cEnvmapLightmapCubeFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "PARAM c[1] = { program.env[0] };\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "TEMP rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], CUBE;\n"
    "MUL r3.rgb, v0, r0;\n"
    "MAD r3.rgb, r1, r0, r3;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MUL r4.rgb, r2, rTemp0.a;\n"
    "ADD r0.rgb, r3, r4;\n"
    "MOV r0.a, c[0].a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cBumpyshinyFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "ATTRIB T4 = fragment.texcoord[4];\n"
    "PARAM c[1] = { program.env[0] };\n"
    "TEMP r0, r1, r2, r3, r4, r5;\n"
    "TEMP rTemp0, rTemp1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "TEX r2, T2, texture[2], CUBE;\n"
    "TEX r3, T3, texture[3], CUBE;\n"
    "TEX r4, T4, texture[4], CUBE;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "MAD  rTemp0,  r1, 2.0, -1.0;\n"
    "DP3 r5.r, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r2, 2.0, -1.0;\n"
    "DP3 r5.g, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r3, 2.0, -1.0;\n"
    "DP3 r5.b, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r4, 2.0, -1.0;\n"
    "DP3 r3.rgb, r5, rTemp0;\n"
    "MUL r3.rgb, r3, 2.0;\n"
    "MUL r3.rgb, r5, r3;\n"
    "DP3 r2.rgb, r5, r5;\n"
    "MAD rTemp1, -r4, 2.0, 1.0;MAD r3.xyz, rTemp1, r2, r3;TEX r5, r3, texture[5], CUBE;\n"
    "MOV r0.rgb, r5;\n"
    "MOV r0.a, r5;\n"
    "MUL r0.a, r0, c[0];\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cFogFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0;\n"
    "MOV r0.rgb, v0;\n"
    "MOV r0.a, v0.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cStandardFogFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB v1 = fragment.color.secondary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEMP r0, r1, r2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MUL r0.rgb, v0, r0;\n"
    "LRP r0.rgb, v1.r, c[1], r0;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cFGlowFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "PARAM c1 = program.env[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "DP3 r1.rgb, r0, c7;\n"
    "MOV r1.a, r0.a;\n"
    "MUL r1.rgb, r1, c1;\n"
    "MAD r0.rgb, c1, c1.a, r1;\n"
    "MUL r0.a, r1.a, c7.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cFGlowEMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c1 = program.env[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1, r2, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r2.rgb, r1, rTemp0.a, r0;\n"
    "MOV r2.a, 1;\n"
    "DP3 r2.rgb, r2, c7;\n"
    "MUL r2.rgb, r2, c1;\n"
    "MAD r0.rgb, c1, c1.a, r2;\n"
    "MUL r0.a, r2.a, c7.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cFGlowBumpyShinyFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "ATTRIB T4 = fragment.texcoord[4];\n"
    "PARAM c1 = program.env[0];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1, r2, r3, r4, r5;\n"
    "TEMP rTemp0, rTemp1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "TEX r2, T2, texture[2], CUBE;\n"
    "TEX r3, T3, texture[3], CUBE;\n"
    "TEX r4, T4, texture[4], CUBE;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "MAD  rTemp0,  r1, 2.0, -1.0;\n"
    "DP3 r5.r, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r2, 2.0, -1.0;\n"
    "DP3 r5.g, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r3, 2.0, -1.0;\n"
    "DP3 r5.b, rTemp0, rTemp1;\n"
    "MAD  rTemp0,  r4, 2.0, -1.0;\n"
    "DP3 r3.rgb, r5, rTemp0;\n"
    "MUL r3.rgb, r3, 2.0;\n"
    "MUL r3.rgb, r5, r3;\n"
    "DP3 r2.rgb, r5, r5;\n"
    "MAD rTemp1, -r4, 2.0, 1.0;MAD r3.xyz, rTemp1, r2, r3;TEX r5, r3, texture[5], CUBE;\n"
    "DP3 r3.rgb, r5, c7;\n"
    "MUL r2.rgb, r3, c1;\n"
    "MAD r0.rgb, c1, c1.a, r2;MUL r0.a, r3.a, c7.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "PARAM c1 = program.env[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MUL r1.rgb, r0, v0;\n"
    "MUL r1.a, r0.a, v0.a;\n"
    "DP3 r1.rgb, r1, c7;\n"
    "MUL r0.rgb, r1, c7.a;\n"
    "MUL r0.a, r1.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatLMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1, r2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "MUL r2.rgb, r0, r1;\n"
    "MAD r2.rgb, r0, v0, r2;\n"
    "DP3 r2.rgb, r2, c7;\n"
    "MUL r0.rgb, r2, c7.a;\n"
    "MUL r2.a, r0.a, v0.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatEMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c7 = program.env[7];\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP r0, r1, r2, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "MUL r2.rgb, r0, v0;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r2.rgb, r1, rTemp0.a, r2;\n"
    "DP3 r2.rgb, r2, c7;\n"
    "MUL r0.rgb, r2, c7.a;\n"
    "MOV r0.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatEMLMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "PARAM c7 = program.env[7];\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP r0, r1, r2, r3, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "MUL r3.rgb, r0, r1;\n"
    "MAD r3.rgb, r0, v0, r3;\n"
    "MOV r3.a, 1;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r3.rgb, r2, rTemp0.a, r3;\n"
    "DP3 r3.rgb, r3, c7;\n"
    "MUL r0.rgb, r3, c7.a;\n"
    "MOV r0.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatNoiseSubLMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "PARAM c7 = program.env[7];\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "SUB r3.rgb, r0, r2;\n"
    "MUL r4.rgb, r3, r1;\n"
    "MOV r0.a, 1;\n"
    "MAD r0.rgb, r3, v0, r4;\n"
    "DP3 r3.rgb, r0, c7;\n"
    "MUL r0.rgb, r3, c7.a;\n"
    "MUL r0.a, r0.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cDesatNoiseSubEMLMFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "PARAM c7 = program.env[7];\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP r0, r1, r2, r3, r4, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 3D;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r4.rgb, r2, rTemp0.a, r0;\n"
    "SUB r4.rgb, r4, r3;\n"
    "MUL r3.rgb, r4, r1;\n"
    "MAD r0.rgb, r4, v0, r3;\n"
    "MOV r0.a, 1;\n"
    "DP3 r3.rgb, r0, c7;\n"
    "MUL r0.rgb, r3, c7.a;\n"
    "MUL r0.a, r0.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cHoloFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1, r2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "DP3 r2.rgb, r0, c7;\n"
    "MUL r0.rgb, r2, r1;\n"
    "MOV r0.a, r1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cHoloDesatFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP r0, r1, r2, r3;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "DP3 r2.rgb, r0, c7;\n"
    "MUL r3.rgb, r2, r1;\n"
    "DP3 r0.rgb, r3, c7;\n"
    "MUL r0.a, r1.a, c7.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cIllumFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP temp1, r0, r1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MAD temp1, c1, 2, -1.0;\n"
    "ADD r1.rgb, v0, temp1;\n"
    "MUL r0.rgb, r0, r1;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cIllumEnvFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "PARAM c1 = program.env[1];\n"
    "TEMP temp1, r0, r1, r2;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], CUBE;\n"
    "MAD temp1, c1, 2, -1.0;\n"
    "ADD r2.rgb, v0, temp1;\n"
    "MUL r0.rgb, r0, r2;\n"
    "LRP r0.rgb, r0.a, r0, r1;\n"
    "MOV r0.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNoiseSubtractFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0, r1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 3D;\n"
    "SUB r0.rgb, r0, r1;\n"
    "MUL r0.rgb, r0, v0;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNoiseLMSubtractFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 3D;\n"
    "SUB r3.rgb, r0, r2;\n"
    "MUL r4.rgb, r3, r1;\n"
    "MAD r0.rgb, r3, v0, r4;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNoiseLMAddFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 3D;\n"
    "ADD r3.rgb, r0, r2;\n"
    "MUL r4.rgb, r3, r1;\n"
    "MAD r0.rgb, r3, v0, r4;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNoiseEMLMSubtractFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0, r1, r2, r3, r4, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 3D;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r4.rgb, r2, rTemp0.a, r0;\n"
    "SUB r4.rgb, r4, r3;\n"
    "MUL r3.rgb, r4, r1;\n"
    "MAD r0.rgb, r4, v0, r3;\n"
    "MOV r0.a, 1;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cNoiseEMLMAddFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "TEMP r0, r1, r2, r3, r4, rTemp0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 3D;\n"
    "SUB rTemp0, 1.0, r0;\n"
    "MAD r4.rgb, r2, rTemp0.a, r0;\n"
    "ADD r4.rgb, r4, r3;\n"
    "MUL r3.rgb, r4, r1;\n"
    "MAD r0.rgb, r4, v0, r3;\n"
    "MOV r0.a, 1;\n"
    "MOV result.color, r0;\n"
    "END\n";

static const char cIllumDesatFragmentShader[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "PARAM c1 = program.env[1];\n"
    "PARAM c7 = program.env[7];\n"
    "TEMP temp1, r0, r1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MAD temp1, c1, 2, -1.0;\n"
    "ADD r1.rgb, v0, temp1;\n"
    "MUL r1.rgb, r0, r1;\n"
    "MUL r1.a, r0.a, v0.a;\n"
    "DP3 r1.rgb, r1, c7;\n"
    "MUL r0.rgb, r1, c7.a;\n"
    "MUL r0.a, r1.a, c1.a;\n"
    "MOV result.color, r0;\n"
    "END\n";

void sInitializeARBFragmentShaders()
{
    int nErrorPos;
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glGenProgramsARB(1, &lightmapFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, lightmapFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cLightmapFragmentShader, sizeof(cLightmapFragmentShader))),
                              cLightmapFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &envmapFogFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapFogFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapFogFragmentShader, sizeof(cEnvmapFogFragmentShader))),
                              cEnvmapFogFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &envmapFogCubeFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapFogCubeFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapFogCubeFragmentShader, sizeof(cEnvmapFogCubeFragmentShader))),
                              cEnvmapFogCubeFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &envmapFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapFragmentShader, sizeof(cEnvmapFragmentShader))),
                              cEnvmapFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &envmapCubeFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapCubeFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapCubeFragmentShader, sizeof(cEnvmapCubeFragmentShader))),
                              cEnvmapCubeFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &bumpmapFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, bumpmapFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cBumpmapFragmentShader, sizeof(cBumpmapFragmentShader))),
                              cBumpmapFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &normalmapFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, normalmapFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNormalmapFragmentShader, sizeof(cNormalmapFragmentShader))),
                              cNormalmapFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &envmapLightmapFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapLightmapFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapLightmapFragmentShader, sizeof(cEnvmapLightmapFragmentShader))),
                              cEnvmapLightmapFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &envmapLightmapCubeFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, envmapLightmapCubeFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cEnvmapLightmapCubeFragmentShader, sizeof(cEnvmapLightmapCubeFragmentShader))),
                              cEnvmapLightmapCubeFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &bumpyshinyFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, bumpyshinyFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cBumpyshinyFragmentShader, sizeof(cBumpyshinyFragmentShader))),
                              cBumpyshinyFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &fogFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fogFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cFogFragmentShader, sizeof(cFogFragmentShader))),
                              cFogFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &standardFogFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, standardFogFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cStandardFogFragmentShader, sizeof(cStandardFogFragmentShader))),
                              cStandardFogFragmentShader);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &FGlowFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cFGlowFragmentShader, sizeof(cFGlowFragmentShader))),
                              cFGlowFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &FGlowEMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowEMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cFGlowEMFragmentShader, sizeof(cFGlowEMFragmentShader))),
                              cFGlowEMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &FGlowBumpyShinyFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowBumpyShinyFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cFGlowBumpyShinyFragmentShader, sizeof(cFGlowBumpyShinyFragmentShader))),
                              cFGlowBumpyShinyFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatFragmentShader, sizeof(cDesatFragmentShader))),
                              cDesatFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatLMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatLMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatLMFragmentShader, sizeof(cDesatLMFragmentShader))),
                              cDesatLMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatEMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatEMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatEMFragmentShader, sizeof(cDesatEMFragmentShader))),
                              cDesatEMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatEMLMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatEMLMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatEMLMFragmentShader, sizeof(cDesatEMLMFragmentShader))),
                              cDesatEMLMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatNoiseSubLMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatNoiseSubLMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatNoiseSubLMFragmentShader, sizeof(cDesatNoiseSubLMFragmentShader))),
                              cDesatNoiseSubLMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &DesatNoiseSubEMLMFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatNoiseSubEMLMFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cDesatNoiseSubEMLMFragmentShader, sizeof(cDesatNoiseSubEMLMFragmentShader))),
                              cDesatNoiseSubEMLMFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &HoloFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, HoloFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cHoloFragmentShader, sizeof(cHoloFragmentShader))),
                              cHoloFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &HoloDesatFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, HoloDesatFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cHoloDesatFragmentShader, sizeof(cHoloDesatFragmentShader))),
                              cHoloDesatFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &IllumFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cIllumFragmentShader, sizeof(cIllumFragmentShader))),
                              cIllumFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &IllumEnvFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumEnvFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cIllumEnvFragmentShader, sizeof(cIllumEnvFragmentShader))),
                              cIllumEnvFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &NoiseSubtractFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseSubtractFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNoiseSubtractFragmentShader, sizeof(cNoiseSubtractFragmentShader))),
                              cNoiseSubtractFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &NoiseLMSubtractFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseLMSubtractFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNoiseLMSubtractFragmentShader, sizeof(cNoiseLMSubtractFragmentShader))),
                              cNoiseLMSubtractFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &NoiseLMAddFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseLMAddFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNoiseLMAddFragmentShader, sizeof(cNoiseLMAddFragmentShader))),
                              cNoiseLMAddFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &NoiseEMLMSubtractFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseEMLMSubtractFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNoiseEMLMSubtractFragmentShader, sizeof(cNoiseEMLMSubtractFragmentShader))),
                              cNoiseEMLMSubtractFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &NoiseEMLMAddFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseEMLMAddFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cNoiseEMLMAddFragmentShader, sizeof(cNoiseEMLMAddFragmentShader))),
                              cNoiseEMLMAddFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glGenProgramsARB(1, &IllumDesatFragmentShaderID);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumDesatFragmentShaderID);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                              static_cast<int>(__strlen_chk(cIllumDesatFragmentShader, sizeof(cIllumDesatFragmentShader))),
                              cIllumDesatFragmentShader);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
}


void GetGLError()
{
    unsigned int nError = ASLgl::glGetError();
    if (nError != 0)
        gluErrorString(nError);
}

void LoadShader(int nLen, const char* szSource)
{
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, nLen, szSource);
}

// pszProgram is unused; the local is a 40-byte scratch buffer for the error-position probe.
void AurGetVertexProgramARBError(char* pszProgram)
{
    K2_UNUSED(pszProgram);
#if KOTOR1  // K1 has no error-position probe to make
}
#else
    int anErrorPos[10];
    if (ASLgl::glGetString(GL_PROGRAM_ERROR_STRING_ARB)[0] != 0)
        ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, anErrorPos);
}
#endif

void GLRender::SetFragmentShaderConstant0(Vector& v, float f)
{
    if (AuroraUseARBShaders())
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, Homogenize(v, f));
}

void GLRender::SetFragmentShaderConstant1(Vector& v, float f)
{
    if (AuroraUseARBShaders())
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, Homogenize(v, f));
}

// Remaps the shader ids to the ATI driver's fragment-shader indices; the non-ATI path leaves the
// table alone. The environment-map id is the one large value; every other entry is a small index.
void GLRender::InitializeATIFragmentShaders()
{
#if KOTOR1  // K1 has no ATI remap table
}
#else
    if (!AuroraUseARBShaders())
        return;
    standardFogFragmentShaderID = 3;
    bumpmapFragmentShaderID = 6;
    normalmapFragmentShaderID = 7;
    envmapCubeFragmentShaderID = 19;
    envmapFragmentShaderID = 117;
    envmapFogFragmentShaderID = 20;
    envmapFogCubeFragmentShaderID = 20;
    lightmapFragmentShaderID = 26;
    envmapLightmapFragmentShaderID = 22;
    envmapLightmapCubeFragmentShaderID = 22;
    bumpyshinyFragmentShaderID = 24;
    DesatLMFragmentShaderID = 57;
    DesatNoiseSubLMFragmentShaderID = 57;
    DesatNoiseAddLMFragmentShaderID = 57;
    DesatNoiseMultLMFragmentShaderID = 57;
    DesatEMLMFragmentShaderID = 59;
    DesatNoiseSubEMLMFragmentShaderID = 59;
    DesatNoiseAddEMLMFragmentShaderID = 59;
    DesatNoiseMultEMLMFragmentShaderID = 59;
    FGlowFragmentShaderID = 50;
    FGlowLMFragmentShaderID = 51;
    FGlowEMFragmentShaderID = 52;
    FGlowEMLMFragmentShaderID = 53;
    FGlowBumpyShinyFragmentShaderID = 55;
    FGlowBumpmapFragmentShaderID = 54;
    IllumFragmentShaderID = 78;
    IllumNoiseSubtractFragmentShaderID = 78;
    IllumNoiseAddFragmentShaderID = 78;
    IllumNoiseMultiplyShaderID = 78;
    IllumEnvFragmentShaderID = 79;
    IllumNoiseEMSubtractFragmentShaderID = 79;
    IllumNoiseEMAddFragmentShaderID = 79;
    IllumNoiseEMMultiplyFragmentShaderID = 79;
    DesatBumpyShinyFragmentShaderID = 61;
    IllumDesatBumpyShinyFragmentShaderID = 61;
    DesatBumpmapFragmentShaderID = 60;
    IllumDesatBumpmapFragmentShaderID = 60;
    DesatFragmentShaderID = 56;
    DesatNoiseSubFragmentShaderID = 56;
    DesatNoiseAddFragmentShaderID = 56;
    DesatNoiseMultFragmentShaderID = 56;
    IllumDesatFragmentShaderID = 56;
    IllumDesatNoiseSubFragmentShaderID = 56;
    IllumDesatNoiseAddFragmentShaderID = 56;
    IllumDesatNoiseMultFragmentShaderID = 56;
    DesatEMFragmentShaderID = 58;
    DesatNoiseSubEMFragmentShaderID = 58;
    DesatNoiseAddEMFragmentShaderID = 58;
    DesatNoiseMultEMFragmentShaderID = 58;
    IllumDesatEMFragmentShaderID = 58;
    IllumDesatNoiseSubEMFragmentShaderID = 58;
    IllumDesatNoiseAddEMFragmentShaderID = 58;
    IllumDesatNoiseMultEMFragmentShaderID = 58;
    HoloFragmentShaderID = 75;
    HoloDesatFragmentShaderID = 76;
}
#endif


// EnableFragmentShadersARB is K2's alone (K1 has no ARB installer, though its build compiles this
// one), so its modes past kPSIllumDesatNoiseEMLMMultiply carry K2's numbers in both builds.
enum {
    kK2PSDefault = 110,
    kK2PSFixedFunction = 116,
    kK2PSEnvMap = 117,
};

// ARB fragment-program entry point. One switch on the shader mode installs the whole
// multi-texture state and a fragment program: which unit carries which texture, the env-map
// texcoord generators, the cube-map wrap/filter parameters, the program environment constants the
// ARB programs read, and the program bind. The prologue is the same for every mode:
// GL_FRAGMENT_PROGRAM_ARB on, the shared default program bound, and the per-instance illumination
// flag cleared. The teardown modes take the whole ARB state back down again.
void GLRender::EnableFragmentShadersARB(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                                        CAurTexture* pTex2, CAurTexture* pTex3)
{
    g_illumShader = false;
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kK2PSDefault);

    switch (nShaderID)
    {
    // Reset to fixed function: bind program 0, drop the ARB state.
    case kK2PSFixedFunction:
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glEnable(GL_LIGHTING);
        break;

    // Unbind every unit, then clear the per-stage TID table.
    case kPSNone:
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kK2PSDefault);
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glEnable(GL_LIGHTING);
        SetActiveTexture(0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        SetActiveTexture(1);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        SetActiveTexture(2);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        SetActiveTexture(3);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        SetActiveTexture(0);
        SetCurrentStageTID(0, 0);
        SetCurrentStageTID(1, 0);
        SetCurrentStageTID(2, 0);
        SetCurrentStageTID(3, 0);
        break;

    case kPSFog:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fogFragmentShaderID);
        break;

    // One or two plain units, then a colour constant and a program.
    case kPSStandardFog:
        SetActiveTexture(0);
        pTex0->Bind();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, standardFogFragmentShaderID);
        break;

    // Four-unit bump/normal-map family, with FOG off, units 1 and 2 as cube maps and the fourth
    // texture argument on unit 3.
    case kPSBumpmap:
        tex1on = 1;
        ASLgl::glDisable(GL_LIGHTING);
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex1->Bind();
        SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex2->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex3->Bind();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, bumpmapFragmentShaderID);
        break;

    case kPSNormalmap:
        tex1on = 1;
        ASLgl::glDisable(GL_LIGHTING);
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex2->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex3->Bind();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, normalmapFragmentShaderID);
        break;

    // TID-binding family. These do not use CAurTexture::Bind, they read the texture's TID and hand
    // it to glBindTexture with the target following IsCubeMap(). glGetError() is called after each
    // unit, and the family also selects its program off the second texture's cube-ness.
    case kPSEnvMapCube:
    case kPSEnvMapFog:
    {
        int nTID = AurTextureGetTID(pTex0);
        ASLgl::glActiveTexture(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(pTex0->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, nTID);
        ASLgl::glGetError();
        nTID = AurTextureGetTID(pTex1);
        ASLgl::glActiveTexture(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(pTex1->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, nTID);
        ASLgl::glGetError();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pTex1->IsCubeMap() ? kPSEnvMapCube : kK2PSEnvMap);
        break;
    }

    case kPSBumpyShinyTextures:
    {
        int nTID = AurTextureGetTID(pTex0);
        ASLgl::glActiveTexture(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(pTex0->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, nTID);
        ASLgl::glGetError();
        nTID = AurTextureGetTID(pTex3);
        ASLgl::glActiveTexture(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(pTex3->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, nTID);
        ASLgl::glGetError();
        break;
    }

    // Multi-unit teardown modes, with no program at all, just unwound state. The first two clear
    // the TIDs of the stages they leave; the other two additionally drop the env-map/cube state and
    // clear envmapon/tex1on.
    case kPSBumpmapOff:
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        SetActiveTexture(1);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        SetActiveTexture(2);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        SetActiveTexture(3);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        ASLgl::glEnable(GL_LIGHTING);
        SetActiveTexture(0);
        break;

    case kPSEnvMapLightmapOff:
        SetActiveTexture(1);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        SetActiveTexture(2);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        ASLgl::glDisable(GL_TEXTURE_GEN_S);
        ASLgl::glDisable(GL_TEXTURE_GEN_T);
        ASLgl::glDisable(GL_TEXTURE_GEN_R);
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        SetActiveTexture(0);
        break;

    case kPSMultiTextureOff:
        SetActiveTexture(3);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        SetActiveTexture(2);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        if (envmapon)
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        SetActiveTexture(1);
        if (envmapon)
        {
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
            ASLgl::glDisable(GL_TEXTURE_GEN_S);
            ASLgl::glDisable(GL_TEXTURE_GEN_T);
            ASLgl::glDisable(GL_TEXTURE_GEN_R);
            cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
            envmapon = 0;
        }
        ASLgl::glDisable(GL_TEXTURE_2D);
        tex1on = 0;
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        ASLgl::glDisable(GL_TEXTURE_3D);
        SetActiveTexture(0);
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        break;

    case kPSDesatOff:
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glEnable(GL_LIGHTING);
        SetActiveTexture(1);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        if (envmapon)
        {
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
            ASLgl::glDisable(GL_TEXTURE_GEN_S);
            ASLgl::glDisable(GL_TEXTURE_GEN_T);
            ASLgl::glDisable(GL_TEXTURE_GEN_R);
        }
        SetActiveTexture(2);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        if (envmapon)
        {
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
            ASLgl::glDisable(GL_TEXTURE_GEN_S);
            ASLgl::glDisable(GL_TEXTURE_GEN_T);
            ASLgl::glDisable(GL_TEXTURE_GEN_R);
            envmapon = 0;
        }
        SetActiveTexture(3);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        SetActiveTexture(0);
        SetCurrentStageTID(0, 0);
        SetCurrentStageTID(1, 0);
        SetCurrentStageTID(2, 0);
        SetCurrentStageTID(3, 0);
        tex1on = 0;
        break;

    case kPSHoloOff:
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        SetActiveTexture(1);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        SetActiveTexture(0);
        SetCurrentStageTID(0, 0);
        SetCurrentStageTID(1, 0);
        break;

    // The two 2D-unit family.
    case kPSLightmap:
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        tex1on = 1;
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, lightmapFragmentShaderID);
        break;

    // The unit-1 texture is the cube map (no texgen); a null env map is a no-op.
    case kPSNoiseSubtract:
        if (pTex1 == 0)
            return;
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex1->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseSubtractFragmentShaderID);
        break;

    // Two 2D units plus the cube map on unit 2 (the only family that flags unit 1 in use).
    case kPSNoiseLMSubtract:
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(2);
        tex1on = 1;
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseLMSubtractFragmentShaderID);
        break;

    case kPSNoiseLMAdd:
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(2);
        tex1on = 1;
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseLMAddFragmentShaderID);
        break;

    // The same battery as the group above, but finished with the desaturation colour constant and
    // the desat-noise program instead of the noise one.
    case kPSDesatNoiseLMSubtract:
    {
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(2);
        tex1on = 1;
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatNoiseSubLMFragmentShaderID);
        break;
    }

    // In the env-map families unit 0 carries the base texture and one unit takes the environment
    // map. The cube arm also sets the three GL_TEXTURE_GEN_* selectors; the 2D arm enables 2D
    // texturing and asks for GL_SPHERE_MAP instead. The two arms then converge on the same program
    // bind, which is why their bodies share their tails.
    case kPSEnvMapLightmapNV1x:
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        break;

    case kPSFGlowEM:
    {
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        PerInstanceColor vGlow = { 0.3f, 0.59f, 0.11f, 0.8f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vGlow));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowEMFragmentShaderID);
        break;
    }

    case kPSDesatEM:
    {
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatEMFragmentShaderID);
        break;
    }

    case kPSIllumEM:
    {
        g_illumShader = true;
        g_illumShaderValues = m_PerInstanceIllumination;
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumEnvFragmentShaderID);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_PerInstanceIllumination));
        break;
    }

    // Base on unit 0, env map on unit 2, then the second material texture on unit 1.
    case kPSEnvMapLightmap:
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pTex1->IsCubeMap() ? envmapLightmapCubeFragmentShaderID
                                                          : envmapLightmapFragmentShaderID);
        break;

    case kPSDesatEMLM:
    {
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatEMLMFragmentShaderID);
        break;
    }

    // Four-unit family, with the base on unit 0, the env map on unit 2, the second material texture
    // on unit 1 and the cube map on unit 3.
    case kPSNoiseEMLMSubtract:
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseEMLMSubtractFragmentShaderID);
        break;

    case kPSNoiseEMLMAdd:
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, NoiseEMLMAddFragmentShaderID);
        break;

    case kPSDesatNoiseEMLMSubtract:
    {
        envmapon = 1;
        tex1on = 1;
        SetActiveTexture(0);
        pTex0->Bind();
        SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatNoiseSubEMLMFragmentShaderID);
        break;
    }

    // The single-program modes.
    case kPSBumpyShiny:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSBumpyShiny);
        break;

    case kPSFGlow:
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        PerInstanceColor vGlow = { 0.3f, 0.59f, 0.11f, 0.8f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vGlow));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowFragmentShaderID);
        break;
    }

    case kPSFGlowBumpyShiny:
    {
        MakeLookUpTexture(5.0f);
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        SetActiveTexture(4);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        SetActiveTexture(5);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex3->Bind();
        SetActiveTexture(0);
        PerInstanceColor vGlow = { 0.3f, 0.59f, 0.11f, 0.8f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vGlow));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FGlowBumpyShinyFragmentShaderID);
        break;
    }

    case kPSDesat:
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatFragmentShaderID);
        break;
    }

    case kPSDesatLM:
    {
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        tex1on = 1;
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_ForceSightColor));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, DesatLMFragmentShaderID);
        break;
    }

    // Hologram pass, with the engine's dedicated filter texture on unit 1 and a different colour
    // constant (0.75 alpha rather than 0.8).
    case kPSHolo:
    {
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        m_pHologramFilterTexture->Bind();
        PerInstanceColor vHolo = { 0.3f, 0.59f, 0.11f, 0.75f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vHolo));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, HoloFragmentShaderID);
        break;
    }

    case kPSHoloDesat:
    {
        SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        m_pHologramFilterTexture->Bind();
        PerInstanceColor vHolo = { 0.3f, 0.59f, 0.11f, 0.75f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vHolo));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, HoloDesatFragmentShaderID);
        break;
    }

    // Per-instance illumination mode, the only body that reads the illumination parameters out and
    // pushes them back as program constants, and it latches the shared illumination flag and colour
    // the vertex programs consume later in the frame.
    case kPSIllum:
        g_illumShader = true;
        g_illumShaderValues = m_PerInstanceIllumination;
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumFragmentShaderID);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_PerInstanceIllumination));
        break;

    // Illumination plus desaturation in one program, with the desaturation colour carried in the
    // illumination slot.
    case kPSIllumDesat:
    case kPSIllumDesatLM:
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        PerInstanceColor vDesat = { 0.3f, 0.59f, 0.11f, g_DesatureFactor };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, reinterpret_cast<const float*>(&vDesat));
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, reinterpret_cast<const float*>(&m_PerInstanceIllumination));
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, IllumDesatFragmentShaderID);
        break;
    }

    // For the mode the prologue already installed, keep the default program and just bind the base
    // map when there is one (the null case is tolerated).
    case kK2PSDefault:
        SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        if (pTex0 != 0)
            pTex0->Bind();
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kK2PSDefault);
        break;
    }
}

// Fragment-program entry point. With ARB shaders available it hands the whole job to
// EnableFragmentShadersARB and only re-selects unit 0 (the ARB installers leave the current stage
// wherever they stopped). Otherwise it is one flat switch over the shader mode that installs the
// fixed-function multi-texture state: which unit carries which texture and, for the env-map
// families, the texcoord generator. Every case body is the same four steps: latch the stage,
// re-select that unit when multi-texturing is available, bind the unit's texture, and (env-map
// families only) configure the texcoord generator off the env map's cube-ness. envmapon and tex1on
// are the flags the fixed-function passes read back later in the frame.
#if KOTOR1
extern bool s_useAlphaTestShader;

static int s_anFragmentStageTID[4];

// Binds pTex on nUnit unless that unit already carries its TID.
static inline void BindFragmentStage(unsigned int nUnit, CAurTexture* pTex)
{
    int nTID = AurTextureGetTID(pTex);
    if (s_anFragmentStageTID[nUnit] != nTID) {
        ASLgl::glActiveTexture(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(pTex->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, nTID);
        ASLgl::glGetError();
        s_anFragmentStageTID[nUnit] = nTID;
    }
}
#endif

void GLRender::EnableFragmentShaders(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                                     CAurTexture* pTex2, CAurTexture* pTex3)
{
#if KOTOR1  // K1 installs ARB fragment programs only
    switch (nShaderID)
    {
    case kPSStandardFog:
        BindFragmentStage(0, pTex0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSStandardFog);
        break;
    case kPSBumpmap:
        BindFragmentStage(0, pTex3);
        BindFragmentStage(1, pTex0);
        BindFragmentStage(2, pTex1);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSBumpmap);
        break;
    case kPSNormalmap:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(1, pTex1);
        BindFragmentStage(2, pTex2);
        BindFragmentStage(3, pTex3);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSNormalmap);
        break;
    case kPSEnvMapCube:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(2, pTex1);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pTex1->IsCubeMap() ? kPSEnvMapCube : kPSEnvMap);
        break;
    case kPSEnvMapFog:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(2, pTex1);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
#ifdef K2_FIX_ENGINE_BUGS
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pTex1->IsCubeMap() ? kPSEnvMapFog : kPSEnvMap);
#else
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSEnvMapFog);
#endif
        break;
    case kPSEnvMapLightmap:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(2, pTex1);
        BindFragmentStage(1, pTex2);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSEnvMapLightmap);
        break;
    case kPSBumpyShiny:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSBumpyShiny);
        break;
    case kPSLightmap:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(1, pTex1);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSLightmap);
        break;
    case kPSBumpyShinyTextures:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(2, pTex3);
        break;
    case kPSDefault:
        BindFragmentStage(0, pTex0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSDefault);
        break;
    case kPSBaseLightmap:
        BindFragmentStage(0, pTex0);
        BindFragmentStage(1, pTex2);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPSLightmap);
        break;
    case kPSFixedFunction:
        s_anFragmentStageTID[0] = 0;
        s_anFragmentStageTID[1] = 0;
        s_anFragmentStageTID[2] = 0;
        s_anFragmentStageTID[3] = 0;
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glEnable(GL_LIGHTING);
        s_useAlphaTestShader = false;
        break;
    }
    ASLgl::glActiveTexture(GL_TEXTURE0);
#else
    if (AuroraUseARBShaders())
    {
        EnableFragmentShadersARB(nShaderID, pTex0, pTex1, pTex2, pTex3);
        GLRender::SetActiveTexture(0);
        return;
    }

    switch (nShaderID)
    {
    // Base texture on unit 0, the env map on unit 1 (cube map -> normal-map texgen on S/T/R, 2D ->
    // sphere-map texgen on S/T), and the shared normalization cube map on unit 2.
    case kPSNoiseEMSubtract: case kPSNoiseEMAdd: case kPSNoiseEMMultiply:
    case kPSDesatNoiseEMSubtract: case kPSDesatNoiseEMAdd: case kPSDesatNoiseEMMultiply:
    case kPSIllumNoiseEMSubtract: case kPSIllumNoiseEMAdd: case kPSIllumNoiseEMMultiply:
    case kPSIllumDesatNoiseEMSubtract: case kPSIllumDesatNoiseEMAdd: case kPSIllumDesatNoiseEMMultiply:
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Like the group above, but the unit-1 texture is the cube map (no texgen setup) and a null env
    // map is a silent no-op.
    case kPSNoiseSubtract: case kPSNoiseAdd: case kPSNoiseMultiply:
    case kPSDesatNoiseSubtract: case kPSDesatNoiseAdd: case kPSDesatNoiseMultiply:
    case kPSIllumNoiseSubtract: case kPSIllumNoiseAdd: case kPSIllumNoiseMultiply:
    case kPSIllumDesatNoiseSubtract: case kPSIllumDesatNoiseAdd: case kPSIllumDesatNoiseMultiply:
        if (pTex1 == 0)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex1->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Two 2D textures plus the cube map on unit 2. The second unit is flagged here (the only family
    // that sets tex1on without envmapon).
    case kPSNoiseLMSubtract: case kPSNoiseLMAdd: case kPSNoiseLMMultiply:
    case kPSDesatNoiseLMSubtract: case kPSDesatNoiseLMAdd: case kPSDesatNoiseLMMultiply:
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        tex1on = 1;
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Base texture on unit 0, the env map on unit 2 (texgen), then the second material texture on
    // unit 1 and the cube map on unit 3: the four-unit family.
    case kPSNoiseEMLMSubtract: case kPSNoiseEMLMAdd: case kPSNoiseEMLMMultiply:
    case kPSDesatNoiseEMLMSubtract: case kPSDesatNoiseEMLMAdd: case kPSDesatNoiseEMLMMultiply:
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Base texture on unit 0, the env map on unit 1 (texgen), cube map on unit 2. Same layout as the
    // four-unit family minus the second material texture.
    case kPSIllumNoiseEMLMSubtract: case kPSIllumNoiseEMLMAdd: case kPSIllumNoiseEMLMMultiply:
    case kPSIllumDesatNoiseEMLMSubtract: case kPSIllumDesatNoiseEMLMAdd: case kPSIllumDesatNoiseEMLMMultiply:
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Base texture on unit 0 and the cube map on unit 1, no texgen: the env map is dropped straight
    // onto the cube unit. Silent no-op without a unit-2 texture.
    case kPSIllumNoiseLMSubtract: case kPSIllumNoiseLMAdd: case kPSIllumNoiseLMMultiply:
    case kPSIllumDesatNoiseLMSubtract: case kPSIllumDesatNoiseLMAdd: case kPSIllumDesatNoiseLMMultiply:
        if (pTex2 == 0)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    // Two-texture env-map family, with the base on unit 0, the env map on unit 1 with texgen, and
    // nothing else.
    case kPSFGlowEM: case kPSDesatEM: case kPSIllumEM: case kPSIllumDesatEM: case kPSIllumDesatEMLM:
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        if (pTex1->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        return;

    // In the normalization-cube-map family the lookup table is baked first, then the base texture
    // goes on unit 0 and units 1..4 each take a fresh reference to the shared normalization cube
    // map (stage 5 takes the fourth texture argument).
    case kPSBumpyShinyTextures: case kPSFGlowBumpyShiny: case kPSDesatBumpyShiny: case kPSIllumDesatBumpyShiny:
        MakeLookUpTexture(5.0f);
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        GLRender::SetActiveTexture(4);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        AurTextureGetNormalizationCubeMap()->Bind();
        GLRender::SetActiveTexture(5);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex3->Bind();
        GLRender::SetActiveTexture(0);
        break;

    // For the plain 2D pass, select unit 0 and enable 2D texturing, with no stage bookkeeping at
    // all (the only case that skips cm_uiCurrentStage entirely).
    case kPSFGlow: case kPSDesat: case kPSIllumDesat: case kPSIllumDesatLM:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        return;

    // Four-unit family with FOG disabled and no texgen, with the base on unit 0, units 1 and 2 as
    // cube maps and the fourth texture on unit 3.
    case kPSFGlowBumpmap: case kPSDesatBumpmap: case kPSIllumDesatBumpmap:
        tex1on = 1;
        ASLgl::glDisable(GL_LIGHTING);
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex1->Bind();
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex2->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex3->Bind();
        return;

    // Two plain 2D textures, the second unit flagged in use.
    case kPSFGlowLM: case kPSDesatLM:
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        tex1on = 1;
        return;

    // Base on unit 0, env map on unit 2 (texgen), second material texture on unit 1.
    case kPSFGlowEMLM: case kPSDesatEMLM:
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(2);
        pTex2->Bind();
        if (pTex2->IsCubeMap())
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            ASLgl::glEnable(GL_TEXTURE_GEN_R);
        }
        else
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
        }
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        return;

    // Hologram pass, using the engine's dedicated filter texture, not a material one. The stage
    // starts at 1 (there is no unit-0 arm), and the pointer is a bare data member.
    case kPSHolo: case kPSHoloDesat:
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        m_pHologramFilterTexture->Bind();
        return;
    }
#endif
}
